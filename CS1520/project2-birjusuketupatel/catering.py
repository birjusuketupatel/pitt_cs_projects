from datetime import datetime
from flask import Flask, request, render_template, url_for, redirect, session, flash, abort
from flask_sqlalchemy import SQLAlchemy
from flask import render_template

app = Flask(__name__)

app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///catering.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
from models import User, Event, Staffing
db.drop_all()
db.create_all()
owner = User(username="owner", password="pass", rank=1)
db.session.add(owner)
db.session.commit()

@app.route("/", methods=["GET", "POST"])
def index():
    #login required
    if "username" not in session:
        return redirect(url_for("login"))

    user = User.query.filter_by(username=session["username"]).first()

    if user is None:
        return redirect(url_for("login"))

    #if customer (rank=3), display booked events, form to book another
    #if staff (rank=2), display staffed events, form to sign up for another
    #if owner (rank=1), display report of all events and workers
    if session["rank"] == 1:
        events = db.session.\
                 query(Event.name, Event.date, Event.id).\
                 all()

        staffings = []
        for event in events:
            event_no = event[2]
            workers = db.session.\
                      query(User.username).\
                      join(Staffing).\
                      join(Event).\
                      filter(Event.id==event_no).\
                      all()
            staffings.append(workers)

        return render_template("owner.html", title="Home", session=session, staffings=staffings, events=events)
    elif session["rank"] == 2:
        #gets details for all events current user is staffed on
        my_events = db.session.\
                    query(Event.name, Event.date).\
                    join(Staffing).\
                    filter_by(staff_id=user.id).\
                    all()

        #gets ids of all events current user is staffed on
        subquery = db.session.\
                   query(Staffing.event_id).\
                   filter(Staffing.staff_id==user.id).\
                   subquery()

        #user can sign up for an event if they are not already signed up
        #and if fewer than 3 other staff have signed up
        available_events = db.session.\
                           query(Event.name, Event.date, Event.id).\
                           outerjoin(Staffing).\
                           filter(Event.id.not_in(subquery)).\
                           group_by(Event.id).\
                           having(db.func.count(Staffing.staff_id)<3).\
                           all()

        return render_template("staff.html", title="Home", session=session,
                               my_events=my_events, available_events=available_events)
    else:
        #handle event submission
        if request.method == "POST":
            name = request.form["event_name"]
            date = datetime.strptime(request.form["event_date"], "%Y-%m-%d")

            #add next event
            try:
                new_event = Event(name=name, date=date, cust_id=user.id)
                db.session.add(new_event)
                db.session.commit()
            except:
                #if date is already booked, display message
                flash("We are already booked on that date.")
                db.session.rollback()

        #gets all events associated with this user
        events = Event.query.filter_by(cust_id=user.id).all()

        return render_template("customer.html", title="Home", session=session, events=events)

@app.route("/login", methods=["GET", "POST"])
def login():
    #handles login attempt
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]

        user = User.query.filter_by(username=username).first()

        #if username and password is correct, redirect to home
        if user is not None and user.password == password:
            session["username"] = user.username
            session["rank"] = user.rank
            session["id"] = user.id
            return redirect(url_for("index"))
        else:
            flash("Bad login attempt.")

    return render_template("login.html", title="Login", session=session)

@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))

@app.route("/register", methods=["GET", "POST"])
def register():
    #handles registration attempt
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]
        confirm = request.form["confirm_password"]
        user = User.query.filter_by(username=username).first()

        #new username must not be in database
        #password must be confirmed
        if user is None and password == confirm:
            if "staff_member" in request.form:
                new_user = User(username=username, password=password, rank=2)
            else:
                new_user = User(username=username, password=password, rank=3)

            db.session.add(new_user)
            db.session.commit()

            return redirect(url_for("index"))

    return render_template("register.html", title="Register", session=session)

@app.route("/cancel/<to_cancel>", methods=["GET", "POST"])
def cancel_event(to_cancel):
    #loads event to be cancelled
    event_to_cancel = Event.query.filter_by(id=to_cancel).first()

    #if no event of that id exists, display error
    #user can only cancel events associated with that user
    if event_to_cancel is None or event_to_cancel.cust_id != session["id"]:
        abort(404)

    #on user confirmation, delete event and redirect to home
    if request.method == "POST":
        #remove event and associations with staff
        staff_rels = Staffing.query.filter_by(event_id=event_to_cancel.id).delete()
        db.session.delete(event_to_cancel)
        db.session.commit()
        return redirect(url_for("index"))

    return render_template("cancel.html", event=event_to_cancel, title="Cancel")

@app.route("/signup/<to_signup>", methods=["GET", "POST"])
def signup(to_signup):
    subquery = db.session.\
               query(Staffing.event_id).\
               filter(Staffing.staff_id == session["id"]).\
               subquery()

    #gets event from list of available events this user can signup for
    event_to_signup = db.session.\
                       query(Event.name, Event.date, Event.id).\
                       outerjoin(Staffing).\
                       filter(Event.id.not_in(subquery), Event.id==to_signup).\
                       group_by(Event.id).\
                       having(db.func.count(Staffing.staff_id)<3).\
                       first()

    #abort if invalid event id,
    #user has already signed up for this event,
    #or 3 workers have already signed up
    if event_to_signup is None:
        abort(404)

    #on user confirmation, record that this user is staffing this event
    if request.method == "POST":
        #ensure no duplicate signup
        duplicate = Staffing.query.filter_by(event_id=to_signup, staff_id=session["id"]).first()
        if duplicate is None:
            is_staffing = Staffing(event_id=to_signup, staff_id=session["id"])
            db.session.add(is_staffing)
            db.session.commit()

        return redirect(url_for("index"))

    return render_template("signup.html", event=event_to_signup, title="Sign Up")

app.secret_key = "key"
