import rpyc, json, copy
from rpyc.utils.server import ThreadedServer
from structs import Group, Message
from threading import Condition, Lock, Thread

# prints server state after each modification
debug_prints = True

# server port
port_num = 18861

# dictionary mapping group name to group object
# contains data for all groups
group_dict = {}

# each group has its own condition variable
cv_dict = {}

# protects access to global data structures
global_lock = Lock()

class ChatServer(rpyc.Service):
    # handle new user connection
    def __init__(self):
        self.username = ""
        self.group_name = ""
        self.last_msg_update = -1

        self.callback = None
        self.thread = None

        if debug_prints:
            print("new connection established\n")

    def on_disconnect(self, conn):
        global_lock.acquire()

        if (self.group_name != "") and (self.group_name in group_dict):
            # remove user from current group
            old_group = group_dict[self.group_name]
            old_cv = cv_dict[self.group_name]
            old_group.users.remove(self.username)
            self.group_name = ""

            # signal membership update to others
            old_cv.acquire()
            old_cv.notify_all()
            old_cv.release()

        global_lock.release()

        if debug_prints:
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

    # reset username of user
    def exposed_set_username(self, username: str):
        # if user already has a username set
        # then remove them from their current group
        if self.username != "":
            global_lock.acquire()
            if self.group_name != "" and (self.group_name in group_dict):
                old_group = group_dict[self.group_name]
                old_cv = cv_dict[self.group_name]
                old_group.users.remove(self.username)
                self.group_name = ""
                self.callback = None

                # notify all others that group membership has changed
                old_cv.acquire()
                old_cv.notify_all()
                old_cv.release()

            self.group_name = ""
            global_lock.release()

        # set new username
        self.username = username
        if debug_prints:
            print(self.username +" logged on\n")

    # join existing group given group name
    # create new group if one does not exist
    def exposed_join_group(self, new_group: str, callback_function: callable):
        # may not call if username is not set
        if self.username == "":
            raise Exception("must set username before joining group")

        # if user is already in an existing group
        # then remove them from that group
        global_lock.acquire()
        if self.group_name in group_dict:
            old_group = group_dict[self.group_name]
            old_cv = cv_dict[self.group_name]
            old_group.users.remove(self.username)

            # notify others of membership change
            old_cv.acquire()
            old_cv.notify_all()
            old_cv.release()
        global_lock.release()

        # if the given group exists, join it
        # else create a new group with that name
        global_lock.acquire()
        if new_group in group_dict:
            group_dict[new_group].users.append(self.username)
        else:
            group_dict[new_group] = Group(new_group, [self.username], [], 0)
            cv_dict[new_group] = Condition()

        # notify others of membership change
        cv_dict[new_group].acquire()
        cv_dict[new_group].notify_all()
        cv_dict[new_group].release()

        if debug_prints:
            print(group_dict)
            print("")
        global_lock.release()

        # return updated group
        self.group_name = new_group

        # establish callback function and thread
        self.callback = rpyc.async_(callback_function)
        self.thread = Thread(target=self.update_client_status)
        self.thread.start()

        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # append a message to the group message log
    def exposed_write_message(self, message: str):
        # user must be in a group to append a message
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to append a message")
        my_group = group_dict[self.group_name]
        my_cv = cv_dict[self.group_name]
        if self.username not in my_group.users:
            raise Exception("user must be a member of a group to append a message")

        # add the next message to log
        global_lock.acquire()
        m = Message(my_group.next_msg_id, message, self.username, [])
        my_group.messages.append(m)
        my_group.next_msg_id += 1

        if debug_prints:
            print(group_dict)
            print("")

        # notify others of new message
        my_cv.acquire()
        my_cv.notify_all()
        my_cv.release()
        global_lock.release()

        # return updated group data
        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # add like to a message given a message id
    def exposed_add_like(self, message_id: int):
        # user must be in a group to like a message
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to like a message")
        my_group = group_dict[self.group_name]
        my_cv = cv_dict[self.group_name]
        if self.username not in my_group.users:
            raise Exception("user must be a member of a group to like a message")

        # get message from message_id
        global_lock.acquire()
        target = None
        for m in my_group.messages:
            if m.id == message_id:
                target = m
                break

        if target is None:
            global_lock.release()
            raise Exception("no message with id " + message_id)

        # if this message has not already been liked by this user
        # then add the like to the message
        if self.username in target.likes:
            global_lock.release()
            raise Exception("cannot like the same message twice")
        else:
            target.likes.append(self.username)

        if debug_prints:
            print(group_dict)
            print("")

        # notify users of new like
        my_cv.acquire()
        my_cv.notify_all()
        my_cv.release()
        global_lock.release()

        # return updated group data
        return self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name]))

    # remove like from a message
    def exposed_remove_like(self, message_id: int):
        # user must be in a group to remove a like
        if self.username == "" or self.group_name == "" or (self.group_name not in group_dict):
            raise Exception("user must be a member of a group to remove a like")
        my_group = group_dict[self.group_name]
        my_cv = cv_dict[self.group_name]
        if self.username not in my_group.users:
            raise Exception("user must be a member of a group to remove a like")

        # get message from message_id
        global_lock.acquire()
        target = None
        for m in my_group.messages:
            if m.id == message_id:
                target = m
                break

        if target is None:
            global_lock.release()
            raise Exception("no message with id " + message_id)

        # if this message has been liked by this user
        # then remove that like
        if self.username not in target.likes:
            global_lock.release()
            raise Exception("user has not yet liked this message")
        else:
            target.likes.remove(self.username)

        if debug_prints:
            print(group_dict)
            print("")

        # notify users of removal of like
        my_cv.acquire()
        my_cv.notify_all()
        my_cv.release()
        global_lock.release()

        # return updated group data
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

    def update_client_status(self):
        while self.group_name != "":
            curr_group_name = self.group_name

            if debug_prints:
                print("Update client status method working...")
                print(self.callback)

            cv_dict[curr_group_name].acquire()
            self.callback(self.get_group_as_dict(copy.deepcopy(group_dict[self.group_name])))

            if debug_prints:
                print("A thread is going to sleep...")

            cv_dict[curr_group_name].wait()

            if debug_prints:
                print("A thread woke up!")

            cv_dict[curr_group_name].release()

if __name__ == "__main__":
    t = ThreadedServer(ChatServer, port=port_num)
    t.start()
