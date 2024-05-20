from chat import db
from datetime import datetime

class User(db.Model):
    __tablename__ = "User"
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(30), unique=True, nullable=False)
    password = db.Column(db.String(30), nullable=False)

    def __init__(self, username, password):
        self.username = username
        self.password = password

    def __repr__(self):
        return "<User id={}, user={}, pass={}>".\
        format(self.id, self.username, self.password)

class Room(db.Model):
    __tablename__ = "Room"
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(30), unique=True)
    owner_id = db.Column(db.Integer, db.ForeignKey("User.id"), nullable=False)

    def __init__(self, name, owner):
        self.name = name
        self.owner_id = owner

    def __repr__(self):
        return "<Room id={}, name={}, owner={}>".\
        format(self.id, self.name, self.owner_id)

class Message(db.Model):
    __tablename__ = "Message"
    id = db.Column(db.Integer, primary_key=True)
    content = db.Column(db.String(140), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.now)
    author_id = db.Column(db.Integer, db.ForeignKey("User.id", ondelete="CASCADE"), nullable=False)
    room_id = db.Column(db.Integer, db.ForeignKey("Room.id", ondelete="CASCADE"), nullable=False)

    def __init__(self, content, author, room):
        self.content = content
        self.author_id = author
        self.room_id = room

    def __repr__(self):
        return "<Message id={}, content={}, timestamp={}, author={}, room={}>".\
        format(self.id, self.content, self.timestamp, self.author_id, self.room_id)
