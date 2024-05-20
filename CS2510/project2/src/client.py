import os, ipaddress, rpyc, sys
from structs import Group, Message
from threading import Condition, Lock, Thread

class DevNull:
    def write(self, msg):
        pass

sys.stderr = DevNull() # mutes EOFError which we end up catching; uncomment this in production

def clear():
    if os.name == 'nt':
        os.system('cls') # windows
    else:
        os.system('clear') # linux and mac

def help():
    val = ""
    val += "   c [hostname]:[port] | connect to a chat server hosted at [hostname]:[port]\n"
    val += "   u [username]        | log into the server as [username]\n"
    val += "   j [group]           | join existing [group] on server, create [group] if it does not exist\n"
    val += "   a                   | opens a new prompt for you to type a message\n"
    val += "   l [message_id]      | like the message with the id [message_id]\n"
    val += "   r [message_id]      | remove your like from the message with the id [message_id]\n"
    val += "   p                   | print all messages in the group's message log\n"
    val += "   v                   | print the ids of all servers in this servers view\n"
    val += "   q                   | quit the program\n"

    return val

def show_group(group_data):
    val = ""
    val += "Group: " + group_data["name"] + "\n"

    val += "Participants: "
    for u in group_data["users"]:
        val += u + ", "
    val = val[:-2]
    val += "\n"

    for m in group_data["messages"]:
        val += render_message(m) + "\n"

    return val

def render_message(m):
    val = str(m["id"]) + ". " + m["sender"] + ": " + m["content"]
    if len(m["likes"]) > 0:
        val += ", " + "Likes: " + str(len(m["likes"]))
    return val

def validate_ip(ip_str):
    try:
        ipaddress.ip_address(ip_str)
        return True
    except:
        return ip_str == "localhost"

def validate_port(port_str):
    try:
        port = int(port_str)
        return (port >= 1 and port <= 65535)
    except:
        return False

# maintains the current state of the group
group_obj = None

# manages access when printing to console
print_lock = Condition()
monitor_active = False

# any messages that need to be displayed to the user
content = ""

def update_group_obj(new_group):
    global group_obj, content, print_lock
    group_obj = new_group
    print("running callback function")
    print_lock.acquire()
    print_lock.notify_all()
    print_lock.release()

def monitor_group_data():
    global monitor_active, group_obj, content, print_lock

    while monitor_active:
        print_lock.acquire()
        print_lock.wait()
        if not monitor_active:
            break
        print("printing...")
        clear()
        print("\n" + "-" * 100)
        print("CHAT CLIENT")
        print(content)
        if group_obj != None:
            print(show_group(group_obj))
        print("-" * 100)
        print_lock.release()

def main():
    global group_obj, content, print_lock, monitor_active
    connected = False
    username = ""
    group_name = ""
    active = True
    user = None

    # thread to monitor and coordinate printing to console
    mon = Thread(target = monitor_group_data)
    monitor_active = True
    mon.start()

    while active:
        print_lock.acquire()
        clear()
        print("-" * 100)
        print("CHAT CLIENT")
        print(content)
        if group_obj != None:
            print(show_group(group_obj))
        print("-" * 100)
        print("(enter \"?\" for help)")
        print_lock.release()

        content = ""
        command = input().split(" ")

        def reset():
            global content, connected, username, group_name, active, user, group_obj

            content += "> connection to the server has closed, please re-connect to a new server\n"
            connected = False
            username = ""
            group_name = ""
            active = True
            user = None
            group_obj = None

        match command[0]:
            case 'c':
                # connection command: c [hostname]:[port]
                # connects the client to the server they specify
                # must be completed before the user can execute any other command

                # validate input format, parse ip and port
                if len(command) != 2:
                    content += "> invalid usage of \"c\"\n"
                    content += "> c [hostname]:[port]\n"
                    continue

                addr = command[1].split(":")
                if len(addr) != 2:
                    content += "> invalid usage of \"c\"\n"
                    content += "> c [hostname]:[port]\n"
                    continue

                if not (validate_ip(addr[0]) and validate_port(addr[1])):
                    content += "> \"" + command[1] + "\" is an invalid ip address and port number\n"
                    content += "> c [hostname]:[port]\n"
                    continue

                # try to connect to server
                # on success, continue
                # on failure, inform user
                ip = addr[0]
                port = int(addr[1])
                content += "> connecting to " + ip + ":" + str(port) + "\n"

                try:
                    conn = rpyc.connect(ip, port)
                    bgsrv = rpyc.BgServingThread(conn)
                    content += "> successfully connected\n"
                    connected = True
                except:
                    content += "> connection failed\n"

            case 'u':
                # username command: u [username]
                # log in on server, reset username
                # removes user from their current group and deletes group data

                # must be connected to the server to run
                if not connected:
                    content += "> must be connected to a server to log in\n"
                    continue

                # validate input format
                if len(command) != 2 or command[1] == "":
                    content += "> invalid usage of \"u\"\n"
                    content += "> u [username]\n"
                    continue

                # try to call set_username on the server
                try:
                    conn.root.set_username(command[1])
                    username = command[1]
                    content += "> succcessfully logged in as \"" + username + "\"\n"

                    # reset group data
                    group_name = ""
                    group_obj = None
                except EOFError:
                    reset()
                except Exception as e:
                    print(e)
                    content += "> log in failed\n"

            case 'j':
                # join command: j [group]
                # joins group on server
                # removes user from current group if in one, deletes group data

                # must have username set to join a group
                if username == "":
                    content += "> must have username set to join a group\n"
                    continue

                # validate input format
                if len(command) != 2 or command[1] == "":
                    content += "> invalid usage of \"j\"\n"
                    content += "> j [group]\n"

                # try to call join_group on the server
                try:
                    group_obj = conn.root.join_group(command[1], update_group_obj)
                    group_name = command[1]
                    content += "> successfully joined group \"" + group_name + "\"\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to join the group\n"

            case 'a':
                # append command: a
                # starts a new prompt
                # appends the message you enter to the group's message log

                # must be in a group to append message
                if group_name == "":
                    content += "> must be in a group to append a message\n"
                    continue

                # validate input format
                if len(command) != 1:
                    content += "> invalid usage of \"a\"\n"
                    content += "> a\n"
                    continue

                # get message from user
                next_msg = input("> ")

                # try to call write_message on the server
                try:
                    group_obj = conn.root.write_message(next_msg)
                    content += "> successfully sent message\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to send message\n"

            case 'l':
                # like command: l [message_id]
                # adds a like to the given message

                # must be in a group to like message
                if group_name == "":
                    content += "> must be in a group to like a message\n"
                    continue

                # validate input format
                if len(command) != 3:
                    content += "> invalid usage of \"l\"\n"
                    content += "> l [message_id_0] [message_id_1] \n"
                    continue

                # parse message_id
                try:
                    message_sender = int(command[1])
                    message_timestamp = int(command[2])
                except:
                    content += "> message_id must be two integers\n"
                    content += "> l [message_id_0] [message_id_1] \n"
                    continue

                # try to call add_like on the server
                try:
                    group_obj = conn.root.add_like(message_sender, message_timestamp)
                    content += "> successfully added like\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to add like\n"
            case 'r':
                # remove like command: r [message_id]
                # removes a like from the given message

                # must be in a group to remove like
                if group_name == "":
                    content += "> must be in a group to remove a like\n"
                    continue

                # validate input format
                if len(command) != 3:
                    content += "invalid usage of \"r\"\n"
                    content += "> r [message_id_0] [message_id_1] \n"
                    continue

                # parse message_id
                try:
                    message_sender = int(command[1])
                    message_timestamp = int(command[2])
                except:
                    content += "> message_id must be two integers\n"
                    content += "> l [message_id_0] [message_id_1] \n"
                    continue

                # try to call remove_like on the server
                try:
                    group_obj = conn.root.remove_like(message_sender, message_timestamp)
                    content += "> successfully removed like\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to remove like\n"
            case 'p':
                # print command: p
                # gets full message log of group

                # must be in a group to request log
                if group_name == "":
                    content += "> must be in a group to remove a like\n"

                # validate input
                if len(command) != 1:
                    content += "invalid usage of \"p\"\n"
                    content += "> p\n"
                    continue

                # try to call message_history on the server
                try:
                    history = conn.root.message_history()
                    content += "> received message history\n"
                    for m in history:
                        content += "> " + render_message(m) + "\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to retrieve message history\n"
            case 'v':
            # view command: p
            # gets list of servers that are in the connected server's view

                # must be connected to the server to run
                if not connected:
                    content += "> must be connected to a server to log in\n"
                    continue

                # validate input
                if len(command) != 1:
                    content += "invalid usage of \"v\"\n"
                    content += "> v\n"
                    continue

                # try to call view on the server
                try:
                    id_list = conn.root.view()
                    content += "> received server's view\n"
                    content += "> " + str(id_list) + "\n"
                except EOFError:
                    reset()
                except:
                    content += "> failed to retrieve view\n"
            case 'q':
                print("closing...")
                try:
                    bgsrv.stop()
                    conn.close()
                except:
                    print("error in closing rpc connection")

                # tell monitor thread to close
                monitor_active = False
                print_lock.acquire()
                print_lock.notify_all()
                print_lock.release()

                # exit control loop
                active = False
            case '?':
                content += "commands\n"
                content += help()
            case other:
                content += "> invalid command\n\n"
                content += "commands\n"
                content += help()


if __name__ == "__main__":
    main()
