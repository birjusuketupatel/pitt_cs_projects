from typing import List, Dict
from dataclasses import dataclass

@dataclass
class Message:
    id: int          # unique message id
    content: str     # message content
    sender: str      # username of the sender
    likes: List[str] # usernames of users who have liked this message

@dataclass
class Group:
    name: str               # name of the group
    users: List[str]        # list of usernames of users currently in this group
    messages: List[Message] # list of all messages in this group
    next_msg_id: int        # each message in group has unique message id