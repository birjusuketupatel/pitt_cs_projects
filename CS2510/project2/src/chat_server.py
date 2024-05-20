import argparse, copy, json, os, random, rpyc, select, sys, time
from constants import RECV_BUFFER_SIZE, DEBUG_BROADCAST, LOSS, PROB, ACK_TIME, NACK_TIMEOUT, VIEW_TIMEOUT, N_SERVERS, DEBUG_SERVER, PORT_NUM
from dataclasses import asdict
from rpyc.utils.server import ThreadedServer
from socket import AF_INET, SOCK_DGRAM, socket
from structs import BroadcastMsg, EnhancedJSONDecoder, EnhancedJSONEncoder, getIPV4, Group, Message
from threading import Condition, Thread, Lock

# BROADCAST PROTOCOL
# Utilizes lamport timestamps to provide a totally ordered broadcast service.

addrs = []                      # IPV4 addresses of all replicas in group
id = -1                         # my server_id (inputted id - 1)
backup_file = ""                # name of file that stores backup information

lock = Lock()                   # lock protects access to shared data below
timestamp = 0                   # my current lamport timestamp
log = []                        # ordered messages in group
seq = 0                         # my current sequence number
buf = []                        # buffer of sent messages
highest_seq_rcvd = [0] * N_SERVERS      # highest sequence number received from each node
highest_ts_rcvd = [0] * N_SERVERS       # highest lamport timestamp receiveed from each node
pkts_until_ack = ACK_TIME       # time until next ack is sent
seq_matrix = [[0] * N_SERVERS] * N_SERVERS      # contains highest_seq_rcvd vectors of every node
ts_matrix = [[0] * N_SERVERS] * N_SERVERS       # contains highest_ts_rcvd vectors of every node
in_view = [True] * N_SERVERS            # is a particular server is in our view of the system
last_heard = [time.time()] * N_SERVERS   # last time we heard from a particular server
confirmed_order = []            # messages who's order is definitely consistent across the system

# returns the current state of the system as a string
def get_state():
    global timestamp, seq
    global buf, log, confirmed_order
    global highest_seq_rcvd, highest_ts_rcvd
    global seq_matrix, ts_matrix, pkts_until_ack
    global group_dict, cv_dict, active_users
    global in_view, last_heard

    state = {
        "timestamp": timestamp,
        "seq": seq,
        "buf": buf,
        "log": log,
        "confirmed_order": confirmed_order,
        "highest_seq_rcvd": highest_seq_rcvd,
        "highest_ts_rcvd": highest_ts_rcvd,
        "seq_matrix": seq_matrix,
        "ts_matrix": ts_matrix,
        "pkts_until_ack": pkts_until_ack,
        "group_dict": group_dict,
        "groups": list(cv_dict.keys()),
        "active_users": active_users,
        "in_view": in_view,
        "last_heard": last_heard
    }

    return json.dumps(state, cls=EnhancedJSONEncoder)

# loads state of the system given a string
def load_state(backup):
    global timestamp, seq
    global buf, log, confirmed_order
    global highest_seq_rcvd, highest_ts_rcvd
    global seq_matrix, ts_matrix, pkts_until_ack
    global group_dict, cv_dict, active_users
    global in_view, last_heard

    state = json.loads(backup, cls=EnhancedJSONDecoder)

    print(state)

    timestamp = state["timestamp"]
    seq = state["seq"]
    buf = state["buf"]
    log = state["log"]
    confirmed_order = state["confirmed_order"]
    highest_seq_rcvd = state["highest_seq_rcvd"]
    highest_ts_rcvd = state["highest_ts_rcvd"]
    ts_matrix = state["ts_matrix"]
    pkts_until_ack = state["pkts_until_ack"]
    group_dict = state["group_dict"]
    cv_dict = { group : Condition() for group in state["groups"] }
    active_users = state["active_users"]
    in_view = state["in_view"]
    last_heard = state["last_heard"]

def backup():
    # write new state to file
    global backup_file
    new_state = get_state()
    with open(backup_file + "_next", "w+") as f:
        f.write(new_state)
        f.close()

    # replace old backup with new backup if write is successful
    if os.path.exists(backup_file):
        os.remove(backup_file)
    if os.path.exists(backup_file + "_next"):
        os.rename(backup_file + "_next", backup_file)

# compares two BroadcastMsgs according to total ordering condition
# msg1 > msg2 -> 1
# msg1 == msg2 -> 0
# msg1 < msg2 -> -1
def compare(msg1: BroadcastMsg, msg2: BroadcastMsg):
    if msg1.ts > msg2.ts or (msg1.ts == msg2.ts and msg1.sender_id > msg2.sender_id):
        return 1
    elif msg1.ts == msg2.ts and msg1.sender_id == msg2.sender_id:
        return 0
    else:
        return -1

# compares two messages
# msg1 > msg2 -> 1
# msg1 == msg2 -> 0
# msg1 < msg2 -> -1
def compare_msg(msg1: Message, msg2: Message):
    if msg1.id[1] > msg2.id[1] or (msg1.id[1] == msg2.id[1] and msg1.id[0] > msg2.id[0]):
        return 1
    elif msg1.id[1] == msg2.id[1] and msg1.id[0] == msg2.id[0]:
        return 0
    else:
        return -1

# places a new BroadcastMsg in the correct position in a list of BroadcastMsgs
def put_in_order(ls, msg: BroadcastMsg):
    i = len(ls) - 1

    while i >= 0:
        if compare(msg, ls[i]) == 1:
            # msg comes after ls[i]
            ls.insert(i + 1, msg)
            return ls
        elif compare(msg, ls[i]) == 0:
            # msg already in ls
            return ls
        else:
            i -= 1

    ls.insert(0, msg)
    return ls

# gets server id and name of config file from command line
def get_args(argv):
    parser = argparse.ArgumentParser(description="Broadcast")
    parser.add_argument("-file", required=True, type=str)
    parser.add_argument("-id", required=True, type=int)
    return parser.parse_args()

# listens for broadcast messages from others in the group
def listen():
    global addrs, id, highest_seq_rcvd, highest_ts_rcvd, lock, timestamp, log, pkts_until_ack, confirmed_order, backup_file

    # setup socket to listen at port specified in config file
    addr = addrs[id]
    listen_sock = socket(AF_INET, SOCK_DGRAM)
    listen_sock.bind(("", addr.port))

    # sets NACK_TIMEOUT second nack timeout
    nack_time = time.time() + NACK_TIMEOUT

    while True:
        # set nack timeout
        # must be non-negative
        timeout = nack_time - time.time()
        if timeout < 0:
            timeout = 0

        readable, writable, exceptional = select.select([listen_sock], [], [], timeout)

        if readable:
            # new message received at listen_sock
            message, a = listen_sock.recvfrom(RECV_BUFFER_SIZE)

            # convert message into BroadcastMsg
            message = json.loads(message.decode())
            pkt = BroadcastMsg(**message)

            if DEBUG_BROADCAST:
                print("received pkt: " + str(pkt))

            lock.acquire()
            # note time we have received the latest packet from pkt.sender_id
            last_heard[pkt.sender_id] = time.time()
            in_view[pkt.sender_id] = True

            # send ack after sending or receiving ACK_TIME messages since sending last ack
            pkts_until_ack -= 1
            if pkts_until_ack <= 0:
                pkts_until_ack = ACK_TIME
                send_acks(listen_sock)

            if pkt.nack:
                # retransmission request received
                # sends all messages in buf with seq >= received sequence number to target
                handle_nack(listen_sock, pkt.seq, pkt.sender_id)
            elif pkt.ack:
                # packet is ack
                handle_ack(pkt)
            elif pkt.seq == highest_seq_rcvd[pkt.sender_id] + 1:
                # packet contains sequence number we are expecting
                handle_packet(pkt)
            else:
                # packet contains an unexpected sequence number
                # send nack to sender
                send_nack(listen_sock, pkt.sender_id)

            if DEBUG_BROADCAST:
                print("confirmed: " + str(confirmed_order))
                print("msg log: " + str(log))

            # write new state to file
            backup()
            lock.release()
        else:
            # timeout registered
            # send nacks to all nodes
            for i in range(0,N_SERVERS):
                if i != id:
                    send_nack(listen_sock, i)

            # reset timeout
            nack_time = time.time() + NACK_TIMEOUT

        # checks if any server has dropped out of the view
        lock.acquire()
        for i in range(0,N_SERVERS):
            if ((time.time() - last_heard[i]) >= VIEW_TIMEOUT) and i != id:
                in_view[i] = False
        backup()
        lock.release()

# handles ack by placing received vectors in matricies
# purges log and buf of unnecessary packets
def handle_ack(pkt):
    global seq_matrix, ts_matrix, buf, log, id, confirmed_order, highest_ts_rcvd

    vectors = json.loads(pkt.content)

    # swap with vector in matrix if each element is strictly greater
    curr_seq = seq_matrix[pkt.sender_id]
    curr_ts = ts_matrix[pkt.sender_id]
    new_seq = vectors["seq_vector"]
    new_ts = vectors["ts_vector"]

    replace_seq = True
    replace_ts = True
    for i in range(0,N_SERVERS):
        if new_seq[i] < curr_seq[i]:
            replace_seq = False
        if new_ts[i] < curr_ts[i]:
            replace_ts = False

    # swap vectors
    if replace_seq:
        seq_matrix[pkt.sender_id] = new_seq
    if replace_ts:
        ts_matrix[pkt.sender_id] = new_ts

    # update this node's view of the sender node's timestamp
    # if we have received every packet up to the ack from this node
    if highest_seq_rcvd[pkt.sender_id] == pkt.seq:
        highest_ts_rcvd[pkt.sender_id] = max(highest_ts_rcvd[pkt.sender_id], pkt.ts)
        ts_matrix[id] = highest_ts_rcvd

    if DEBUG_BROADCAST:
        print("ts_matrix: " + str(ts_matrix))
        print("seq_matrix: " + str(seq_matrix))

    # find the sequence number that all nodes have received up to
    min_seq = sys.maxsize
    for i in range(0,N_SERVERS):
        min_seq = min(min_seq, seq_matrix[i][id])

    # find the minimum timestamp everyone knows about
    min_ts = sys.maxsize
    for v in ts_matrix:
        min_ts = min(min(v), min_ts)

    # purge buf and log of unnecessary packets
    # removes all packets with smaller or equal seq to min_seq
    to_remove = 0
    while to_remove < len(buf):
        if buf[to_remove].seq > min_seq:
            break
        to_remove += 1
    buf = buf[to_remove:]

    # remove all packets with lower timestamp than min_ts from log
    # place packets in confirmed_order list
    to_remove = 0
    while to_remove < len(log):
        if log[to_remove].ts >= min_ts:
            break
        to_remove += 1
    log = log[to_remove:]

# sends acks to all nodes
def send_acks(sock):
    global id, timestamp, seq, highest_seq_rcvd, highest_ts_rcvd, seq_matrix, ts_matrix

    # writes sequence and timestamp vectors on packet
    vectors = {}
    vectors["seq_vector"] = highest_seq_rcvd
    vectors["ts_vector"] = highest_ts_rcvd
    ack_pkt = BroadcastMsg(sender_id=id, ts=timestamp,
                           seq=seq, ack=True, content=json.dumps(vectors))
    ack_as_str = json.dumps(asdict(ack_pkt))

    # update matricies
    seq_matrix[id] = highest_seq_rcvd
    ts_matrix[id] = highest_ts_rcvd

    # sends ack to every other node in the system
    for i in range(0,N_SERVERS):
        if i != id:
            addr = addrs[i]
            if LOSS:
                # send with simulated loss
                sendto_dbg(sock, ack_as_str.encode(), (addr.addr, addr.port))
            else:
                sock.sendto(ack_as_str.encode(), (addr.addr, addr.port))

# handles a retransmission request
def handle_nack(sock, req_seq, target_id):
    global addrs, seq, buf

    # sends all messages in buf with seq >= received sequence number to target
    target_addr = addrs[target_id]

    if req_seq > seq:
        # nack requested for packet node has not yet sent
        return

    # search buffer for requested packet
    i = len(buf) - 1
    found = False
    while i >= 0:
        if buf[i].seq == req_seq:
            found = True
            break
        i -= 1

    # if packet not found, nack is spurious, do nothing
    if found:
        # resend requested packet and all subsequent packets
        to_send = buf[i:]
        for pkt in to_send:
            pkt_as_str = json.dumps(asdict(pkt))
            if LOSS:
                # send with simulated network loss
                sendto_dbg(sock, pkt_as_str.encode(), (target_addr.addr, target_addr.port))
            else:
                sock.sendto(pkt_as_str.encode(), (target_addr.addr, target_addr.port))

# handles receiving expected packet
def handle_packet(pkt):
    global id, highest_ts_rcvd, highest_seq_rcvd, timestamp, log, seq_matrix, ts_matrix, group_dict

    timestamp = max(timestamp, pkt.ts) + 1 # update lamport timestamp
    highest_ts_rcvd[id] = timestamp
    highest_seq_rcvd[pkt.sender_id] = pkt.seq # record seq received from sender
    highest_ts_rcvd[pkt.sender_id] = pkt.ts # record ts received from sender

    # update matricies
    seq_matrix[id] = highest_seq_rcvd
    ts_matrix[id] = highest_ts_rcvd

    put_in_order(log, pkt) # write to log

    # attempt to run all unexecuted commands in the log from left to right
    for i in range(0, len(log)):
        m = log[i]
        if not m.executed:
            op = json.loads(m.content)
            command = op["operation_name"]
            val = ""

            match command:
                case "create_group":
                    val = create_group(op["arg_1"], op["arg_2"])
                case "join_group":
                    val = join_group(op["arg_1"], op["arg_2"])
                case "leave_group":
                    val = leave_group(op["arg_1"], op["arg_2"])
                case "write_message":
                    val = write_message(op["arg_1"], op["arg_2"], op["arg_3"], op["arg_4"], op["arg_5"])
                case "add_like":
                    val = add_like(op["arg_1"], op["arg_2"], op["arg_3"], op["arg_4"])
                case "remove_like":
                    val = remove_like(op["arg_1"], op["arg_2"], op["arg_3"], op["arg_4"])
                case _:
                    if DEBUG_BROADCAST:
                        print("invalid operation")

            # if command was successful, mark it as having been executed
            # and put it back in the log
            print(group_dict)
            if val != "":
                m.executed = True
                log[i] = m
                cv_dict[op["arg_1"]].acquire()
                cv_dict[op["arg_1"]].notify_all()
                cv_dict[op["arg_1"]].release()


# sends nack to the node with the given id
def send_nack(sock, target_id):
    global id, timestamp, highest_seq_rcvd, addrs

    nack_pkt = BroadcastMsg(sender_id=id, nack=True, ts=timestamp,
                            seq=highest_seq_rcvd[target_id] + 1, content="")
    nack_as_str = json.dumps(asdict(nack_pkt))
    target_addr = addrs[target_id]

    if LOSS:
        # send with simulated network loss
        sendto_dbg(sock, nack_as_str.encode(), (target_addr.addr, target_addr.port))
    else:
        sock.sendto(nack_as_str.encode(), (target_addr.addr, target_addr.port))


# broadcasts message to group
def broadcast(msg):
    global timestamp, seq, id, log, addrs, highest_seq_rcvd, backup_file
    global highest_ts_rcvd, pkts_until_ack, seq_matrix, ts_matrix

    # create a socket to send on
    sock = socket(AF_INET, SOCK_DGRAM)


    # create next packet
    # packet has been executed on this node
    timestamp += 1
    seq += 1

    # this operation has been executed on this node, but not on any other node
    exec_pkt = BroadcastMsg(sender_id=id, ts=timestamp, seq=seq, content=msg, executed=True)
    broadcast_pkt = BroadcastMsg(sender_id=id, ts=timestamp, seq=seq, content=msg)

    # update highest received values
    highest_ts_rcvd[id] = timestamp
    highest_seq_rcvd[id] = seq

    # update matricies
    seq_matrix[id] = highest_seq_rcvd
    ts_matrix[id] = highest_ts_rcvd

    # places executed packet in total order
    log = put_in_order(log, exec_pkt)

    buf.append(broadcast_pkt) # buffers unexecuted packet to serve potential retransmission requests

    # send ack after sending or receiving ACK_TIME messages since sending last ack
    pkts_until_ack -= 1
    if pkts_until_ack <= 0:
        pkts_until_ack = ACK_TIME
        send_acks(sock)

    # write new state to file
    backup()

    # convert BroadcastMsg to JSON
    pkt_as_str = json.dumps(asdict(broadcast_pkt))

    # broadcast pkt to all replicas
    for i in range(0,N_SERVERS):
        if i != id:
            addr = addrs[i]
            if LOSS:
                # send with simulated loss
                sendto_dbg(sock, pkt_as_str.encode(), (addr.addr, addr.port))
            else:
                sock.sendto(pkt_as_str.encode(), (addr.addr, addr.port))
    sock.close()

# has a 50% chance of sending a packet, 50% chance of doing nothing
def sendto_dbg(sock, content, addr):
    r = random.randrange(0, PROB)
    if r < PROB - 1:
        if DEBUG_BROADCAST:
            print("sent packet " + content.decode() + ", " + str(addr))
        sock.sendto(content, addr)

# SERVER
# Serves clients concurrent and handles updates from other servers.

# dictionary mapping group name to group object
# contains data for all groups
group_dict = {}

cv_dict = {}

# list of all users connected to this server and currently in a group
# contains dictionaries that contain username and group_name
active_users = []

# given a group name, creates a new group
# returns operation message if successful, "" if not
def create_group(group_name, username):
    if DEBUG_BROADCAST:
        print("running create_group")

    if group_name in group_dict:
        # treat an attempt to create a duplicate group
        # as a request to join it
        group = group_dict[group_name]
        if username not in group.users:
            group.users.append(username)
        else:
            return ""
    else:
        # create a new group
        group_dict[group_name] = Group(group_name, [username], [])
        cv_dict[group_name] = Condition()

    msg = {
        "operation_name": "create_group",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": "",
        "arg_4": "",
        "arg_5": ""
    }
    return json.dumps(msg)

# adds a user to a given group
# returns operation message if successful, "" if not
def join_group(group_name, username):
    if DEBUG_BROADCAST:
        print("running join_group")

    if group_name not in group_dict:
        return ""
    group_dict[group_name].users.append(username)
    msg = {
        "operation_name": "join_group",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": "",
        "arg_4": "",
        "arg_5": ""
    }
    return json.dumps(msg)

# remove a user from a given group
# returns operation message if successful, "" if not
def leave_group(group_name, username):
    if DEBUG_BROADCAST:
        print("running leave_group")

    if group_name not in group_dict:
        return ""
    old_group = group_dict[group_name]
    if username not in old_group.users:
        return ""

    old_group.users.remove(username)
    group_dict[group_name] = old_group

    msg = {
        "operation_name": "leave_group",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": "",
        "arg_4": "",
        "arg_5": ""
    }
    return json.dumps(msg)

# writes a message in the message log in order
# returns operation message if successful, "" if not
def write_message(group_name, username, content, ts, server_id):
    if DEBUG_BROADCAST:
        print("running write_message")

    if group_name not in group_dict:
        return ""
    group = group_dict[group_name]
    if username not in group.users:
        return ""

    # put new message in order
    m = Message([server_id, ts], content, username, [])
    msg_log = group.messages

    if DEBUG_SERVER:
        print(msg_log)

    i = len(msg_log) - 1
    found = False
    while i >= 0:
        if compare_msg(m, msg_log[i]) == 1:
            msg_log.insert(i + 1, m)
            found = True
            break
        elif compare_msg(m, msg_log[i]) == 0:
            return ""
        else:
            i -= 1

    if not found:
        msg_log.insert(0, m)

    group.messages = msg_log
    group_dict[group_name] = group

    msg = {
        "operation_name": "write_message",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": content,
        "arg_4": ts,
        "arg_5": server_id
    }
    return json.dumps(msg)

# adds a like to the identified message
# returns operation message if successful, "" if not
def add_like(group_name, username, ts, server_id):
    if DEBUG_BROADCAST:
        print("running add_like")

    if group_name not in group_dict:
        return ""
    group = group_dict[group_name]
    if username not in group.users:
        return ""

    # search message log for message with matching server_id and timestamp
    target = -1
    for i in range(0, len(group.messages)):
        m = group.messages[i]
        if m.id[0] == server_id and m.id[1] == ts:
            target = i
            break
    if target == -1:
        return ""

    # add like if not already present
    to_like = group.messages[target]
    if username in to_like.likes:
        return ""

    to_like.likes.append(username)
    group.messages[target] = to_like
    group_dict[group_name] = group

    msg = {
        "operation_name": "add_like",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": ts,
        "arg_4": server_id,
        "arg_5": ""
    }
    return json.dumps(msg)

# removes a like from the identified message
# returns operation message if successful, "" if not
def remove_like(group_name, username, ts, server_id):
    if DEBUG_BROADCAST:
        print("running remove_like")

    if group_name not in group_dict:
        return ""
    group = group_dict[group_name]
    if username not in group.users:
        return ""

    # search message log for message with matching server_id and timestamp
    target = -1
    for i in range(0, len(group.messages)):
        m = group.messages[i]
        if m.id[0] == server_id and m.id[1] == ts:
            target = i
            break
    if target == -1:
        return ""

    # remove like if one is present
    target_msg = group.messages[target]
    if username not in target_msg.likes:
        return ""

    target_msg.likes.remove(username)
    group.messages[target] = target_msg
    group_dict[group_name] = group

    msg = {
        "operation_name": "remove_like",
        "arg_1": group_name,
        "arg_2": username,
        "arg_3": ts,
        "arg_4": server_id,
        "arg_5": ""
    }
    return json.dumps(msg)

class ChatServer(rpyc.Service):
    # handle new user connection
    def __init__(self):
        self.username = ""
        self.group_name = ""

        self.thread = None

        if DEBUG_SERVER:
            print("new connection established\n")

    def on_disconnect(self, conn):
        lock.acquire()
        if self.username != "":
            if (self.group_name != "") and (self.group_name in group_dict):
                # remove user from current group
                op = leave_group(self.group_name, self.username)

                # broadcast and backup
                backup()
                if op != "":
                    broadcast(op)

                # user is no longer active, remove from active_users
                for u in active_users:
                    if u["username"] == self.username and u["group_name"] == self.group_name:
                        active_users.remove(u)
                        break
        lock.release()

        if DEBUG_SERVER:
            print("closed connection\n")

    # converts the given group object to a dictionary
    # return last 10 messages
    def get_group_as_dict(self, group: Group):
        to_return = vars(group)

        msgs = []
        if (len(group.messages)) >= 10:
            i = len(group.messages) - 10
        else:
            i = 0

        while i < len(group.messages):
            msgs.append(vars(group.messages[i]))
            i += 1

        to_return["messages"] = msgs
        return to_return

    # reset username
    def exposed_set_username(self, username: str):
        # if user already has a username set
        # then remove them from their current group and active users list
        if self.username != "":
            lock.acquire()
            if self.group_name != "" and (self.group_name in group_dict):
                old_group = self.group_name
                self.group_name = ""

                op = leave_group(old_group, self.username)
                
                cv_dict[old_group].acquire()
                cv_dict[old_group].notify_all()
                cv_dict[old_group].release()

                # broadcast and backup
                backup()
                if op != "":
                    broadcast(op)

                # user is no longer active, remove from active_users
                for u in active_users:
                    if u["username"] == self.username and u["group_name"] == self.group_name:
                        active_users.remove(u)
                        break

                # print new server state
                if DEBUG_SERVER:
                    print(group_dict)

            self.group_name = ""
            lock.release()

        self.username = username # set new username
        if DEBUG_SERVER:
            print(self.username + " logged on\n")

    # join existing group given group name
    # create new group if one does not exist
    # leave current group if in one
    def exposed_join_group(self, new_group: str, callback_function: callable):
        # may not call if username is not set
        if self.username == "":
            raise Exception("must set username before joining group")

        # if user is already in an existing group
        # then remove them from that group
        lock.acquire()
        if self.group_name in group_dict:
            old_group = self.group_name
            self.group_name = ""

            op = leave_group(old_group, self.username)
            
            cv_dict[old_group].acquire()
            cv_dict[old_group].notify_all()
            cv_dict[old_group].release()

            # broadcast and backup
            backup()
            if op != "":
                broadcast(op)

            # user is no longer active, remove from active_users
            for u in active_users:
                if u["username"] == self.username and u["group_name"] == self.group_name:
                    active_users.remove(u)
                    break
        lock.release()

        # if the given group exists, join it
        # else create a new group with that name
        lock.acquire()
        self.group_name = new_group

        op = ""
        if self.group_name in group_dict:
            op = join_group(self.group_name, self.username)
        else:
            op = create_group(self.group_name, self.username)
        cv_dict[self.group_name].acquire()
        cv_dict[self.group_name].notify_all()
        cv_dict[self.group_name].release()

        # add new active user
        active_users.append({
            "username": self.username,
            "group_name": self.group_name
        })

        # broadcast and backup
        backup()
        if op != "":
            broadcast(op)
        lock.release()

        # establish callback function and thread
        self.callback = rpyc.async_(callback_function)
        self.thread = Thread(target=self.update_client_status)
        self.thread.start()

        if DEBUG_SERVER:
            print(group_dict)

        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # append a message to the group message log
    def exposed_write_message(self, message: str):
        global timestamp, id

        # user must be in a group to append a message
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to append a message")

        lock.acquire()
        group = group_dict[self.group_name]
        if self.username not in group.users:
            lock.release()
            raise Exception("user must be a member of a group to append a message")

        # write new message
        op = write_message(self.group_name, self.username, message, timestamp + 1, id)
        cv_dict[self.group_name].acquire()
        cv_dict[self.group_name].notify_all()
        cv_dict[self.group_name].release()

        # broadcast and backup
        backup()
        if op != "":
            broadcast(op)

        if DEBUG_SERVER:
            print(group_dict)
        lock.release()

        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # add like to a message given a message id
    def exposed_add_like(self, server_id: int, ts: int):
        # user have a username and group to like a message
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to like a message")

        lock.acquire()
        group = group_dict[self.group_name]
        # user must be in this group
        if self.username not in group.users:
            lock.release()
            raise Exception("user must be a member of a group to like a message")

        # search group for message
        target = -1
        for i in range(0, len(group.messages)):
            m = group.messages[i]
            if m.id[0] == server_id and m.id[1] == ts:
                target = i
                break

        if target == -1:
            lock.release()
            raise Exception("no message with id (" + server_id + "," + ts + ")")

        if self.username in group.messages[target].likes:
            lock.release()
            raise Exception("cannot like the same message twice")

        op = add_like(self.group_name, self.username, ts, server_id)
        cv_dict[self.group_name].acquire()
        cv_dict[self.group_name].notify_all()
        cv_dict[self.group_name].release()

        # broadcast and backup
        backup()
        if op != "":
            broadcast(op)

        if DEBUG_SERVER:
            print(group_dict)
        lock.release()

        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # remove like from a message
    def exposed_remove_like(self, server_id: int, ts: int):
        # user must be in a group to remove a like
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to remove a like")

        lock.acquire()
        group = group_dict[self.group_name]
        if self.username not in group.users:
            lock.release()
            raise Exception("user must be a member of a group to remove a like")

        # search group for message
        target = -1
        for i in range(0, len(group.messages)):
            m = group.messages[i]
            if m.id[0] == server_id and m.id[1] == ts:
                target = i
                break

        if target == -1:
            lock.release()
            raise Exception("no message with id (" + server_id + "," + ts + ")")

        if self.username not in group.messages[target].likes:
            lock.release()
            raise Exception("user has not yet liked this message")

        op = remove_like(self.group_name, self.username, ts, server_id)
        cv_dict[self.group_name].acquire()
        cv_dict[self.group_name].notify_all()
        cv_dict[self.group_name].release()

        # broadcast and backup
        backup()
        if op != "":
            broadcast(op)
        lock.release()

        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # return full message history
    def exposed_message_history(self):
        # user must be in a group to get message history
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to get message history")
        my_group = group_dict[self.group_name]
        if self.username not in my_group.users:
            raise Exception("user must be a member of a group to get message history")

        # return full message history
        msg_ls = copy.deepcopy(my_group.messages)
        return_ls = []
        for m in msg_ls:
            return_ls.append(vars(m))
        return return_ls

    # returns a list of which server_ids are in the view
    def exposed_view(self):
        view_ids = []

        lock.acquire()
        for i in range(0, N_SERVERS):
            if in_view[i]:
                view_ids.append(i + 1)
        lock.release()

        return view_ids

    def update_client_status(self):
        while self.group_name != "":
            curr_group_name = self.group_name

            if DEBUG_SERVER:
                print("Update client status method working...")

            cv_dict[curr_group_name].acquire()
            print(self.username + " acquired lock")

            self.callback(self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name])))
            print(self.username + " sent callback")
            print(self.username + " sent " + str(self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))))

            cv_dict[curr_group_name].wait()
            print(self.username + " woken up")

            if DEBUG_SERVER:
                print(group_dict[curr_group_name])

            cv_dict[curr_group_name].release()

def main(argv):
    global id, addrs, backup_file, active_users, group_dict

    args = get_args(argv)

    # set server id
    id = args.id - 1

    # read ip addresses and ports of all nodes in system
    f = open(args.file, "r")
    content = f.readlines()
    for i in range(0,N_SERVERS):
        next_addr = getIPV4(content[i][:-1])
        addrs.append(next_addr)

    # create a backup file titled log{id} if one does not exist
    # if it does exist, recover state from it
    backup_file = "log" + str(id)
    if os.path.isfile(backup_file):
        # load state from file
        lock.acquire()
        f = open(backup_file, "r")
        load_state(f.read())
        f.close()

        # We keep a list of users that are connected to this node and in a group
        # as active_users. On crash recovery, all of these users are kicked out
        # of the group_dict and leave_group messages for these users are broadcast.
        for u in active_users:
            if u["group_name"] in group_dict:
                group = group_dict[u["group_name"]]
                if u["username"] in group.users:
                    op = leave_group(u["group_name"], u["username"])

                    # broadcast and backup
                    backup()
                    if op != "":
                        broadcast(op)
        active_users = []
        backup()
        lock.release()

        if DEBUG_BROADCAST:
            print("loading state from file...")
            print(get_state())
    else:
        # create new file
        if DEBUG_BROADCAST:
            print("creating backup file...")
        lock.acquire()
        backup()
        lock.release()

    # create a new thread to listen for messages from others in the group
    t = Thread(target=listen)
    t.daemon = True
    t.start()

    t = ThreadedServer(ChatServer, port=PORT_NUM)
    t.start()

if __name__ == "__main__":
    main(sys.argv[1:])
