from catering import db
from sqlalchemy.orm import relationship

class User(db.Model):
    __tablename__ = "User"
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(30), unique=True, nullable=False)
    password = db.Column(db.String(30), nullable=False)
    rank = db.Column(db.Integer, nullable=False, default=3) #owner=1, staff=2, customer=3

    def __init__(self, username, password, rank):
        self.username = username
        self.password = password
        self.rank = rank

    def __repr__(self):
        return "<User id={} user={} pass={} rank={}>".format(self.id, self.username, self.password, self.rank)

class Event(db.Model):
    __tablename__ = "Event"
    id = db.Column(db.Integer, primary_key=True)
    cust_id = db.Column(db.Integer, db.ForeignKey("User.id"), nullable=False)
    date = db.Column(db.Date, nullable=False, unique=True)
    name = db.Column(db.String(100), nullable=False)

    def __init__(self, name, cust_id, date):
        self.name = name
        self.cust_id = cust_id
        self.date = date

    def __repr__(self):
        return "<Event id={} name={} customer={} date={}>".format(self.id, self.name, self.cust_id, self.date)

class Staffing(db.Model):
    __tablename__ = "Staffing"
    id = db.Column(db.Integer, primary_key=True)
    event_id = db.Column(db.Integer, db.ForeignKey("Event.id"), nullable=False)
    event = relationship("Event")
    staff_id = db.Column(db.Integer, db.ForeignKey("User.id"), nullable=False)
    staff = relationship("User")

    def __init__(self, event_id, staff_id):
        self.event_id = event_id
        self.staff_id = staff_id

    def __repr__(self):
        return "<Staffing id={} event_id={} staff_id={}>".format(self.id, self.event_id, self.staff_id)
