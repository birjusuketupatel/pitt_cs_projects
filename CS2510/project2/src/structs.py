import json
from dataclasses import asdict, dataclass, is_dataclass
from typing import List, Dict

@dataclass
class Message:
    id: List[int] # id of the message - the tuple contains the server id of the sender the message originated from and its lamport timestamp
    content: str     # message content
    sender: str      # username of the sender
    likes: List[str] # usernames of users who have liked this message

@dataclass
class Group:
    name: str               # name of the group
    users: List[str]        # list of usernames of users currently in this group
    messages: List[Message] # list of all messages in this

class EnhancedJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if is_dataclass(o):
            return asdict(o)
        return super().default(o)

class EnhancedJSONDecoder(json.JSONDecoder):
    def __init__(self, *args, **kwargs):
        json.JSONDecoder.__init__(self, object_hook=self.object_hook, *args, **kwargs)

    def object_hook(self, dct):
        if 'sender_id' in dct:
            return BroadcastMsg(**dct)
        elif 'content' in dct:
            return Message(**dct)
        elif 'name' in dct:
            return Group(**dct)
        return dct
    
@dataclass
class IPV4:
    addr: str
    port: int

def getIPV4(line):
    return IPV4(line.split(':')[0], int(line.split(':')[1]))

@dataclass
class BroadcastMsg:
    sender_id: int
    ack: bool
    nack: bool
    executed: bool
    ts: int
    seq: int
    content: str

    def __init__(self, sender_id: int, ts: int, seq: int, content: str, ack: bool = False, nack: bool = False, executed: bool = False):
        self.sender_id = sender_id
        self.ack = ack
        self.nack = nack
        self.executed = executed
        self.ts = ts
        self.seq = seq
        self.content = content