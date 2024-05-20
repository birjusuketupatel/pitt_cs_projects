import json
from datetime import datetime
from flask import Flask, request, render_template, url_for, session, redirect, flash
from flask_sqlalchemy import SQLAlchemy
from werkzeug.exceptions import abort

app = Flask(__name__)

app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///chat.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.secret_key = "key"

db = SQLAlchemy(app)
from models import User, Room, Message

@app.cli.command('initdb')
def initdb_command():
    db.drop_all()
    db.create_all()
    print("Created the database.")

@app.route("/")
def home():
    #login required
    if "username" not in session:
        return redirect(url_for("login"))

    #remove data associated with room user may have been in
    if "room_id" in session: session.pop("room_id")
    if "room_name" in session: session.pop("room_name")
    if "last_update" in session: session.pop("last_update")

    #serves skeleton page of rooms available to enter
    return render_template("home.html", title="Home", session=session)

@app.route("/chatroom/<room_id>")
def chatroom(room_id):
    #login required
    if "username" not in session:
        return redirect(url_for("login"))

    #current room must exist
    current_room = Room.query.get(room_id)
    if current_room is None:
        flash("This room no was deleted.")
        return redirect(url_for("home"))

    #resets user's current room
    if "room_id" in session: session.pop("room_id")
    if "room_name" in session: session.pop("room_name")
    if "last_update" in session: session.pop("last_update")

    session["room_id"] = room_id
    session["room_name"] = current_room.name

    return render_template("room.html", title=current_room.name, session=session)

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]

        user = User.query.filter_by(username=username).first()

        #if username and password is correct, redirect to home
        if user is not None and user.password == password:
            session["username"] = user.username
            session["user_id"] = user.id
            return redirect(url_for("home"))
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
                new_user = User(username=username, password=password)
            else:
                new_user = User(username=username, password=password)

            db.session.add(new_user)
            db.session.commit()

            return redirect(url_for("home"))
        else:
            flash("Account creation failed.")

    return render_template("register.html", title="Register", session=session)

@app.route("/rooms", methods=["GET", "POST", "DELETE"])
def room():
    #login required
    if "username" not in session:
        abort(401)

    if request.method == "GET":
        #return list of all rooms
        all_rooms = db.session.\
                    query(Room.id, Room.name, Room.owner_id).\
                    all()
        return json.dumps(all_rooms)
    elif request.method == "POST":
        #checks if new room has unique name, owner exists
        #if so, creates a new room
        try:
            name = request.form["name"]
            owner_id = request.form["owner_id"]
        except:
            abort(400)

        other_room = db.session.\
                     query(Room.id).\
                     filter_by(name=name).\
                     first()

        owner = User.query.get(owner_id)

        if other_room is None and owner is not None:
            new_room = Room(name=name, owner=owner_id)
            db.session.add(new_room)
            db.session.commit()

            data = {}
            data["room_id"] = new_room.id
            data["room_name"] = new_room.name
            data["owner_id"] = new_room.owner_id

            return json.dumps(data)
        else:
            abort(400)
    elif request.method == "DELETE":
        #checks room to be removed exists
        #checks that user owns this room
        try:
            id = request.form["room_id"]
            owner_id = request.form["owner_id"]
        except:
            abort(400)

        room_to_delete = Room.query.get(id)
        owner = User.query.get(owner_id)

        if (room_to_delete is not None and owner is not None
            and room_to_delete.owner_id == owner.id):
            data = {}
            data["room_id"] = room_to_delete.id
            data["room_name"] = room_to_delete.name
            data["owner_id"] = room_to_delete.owner_id

            Message.query.filter_by(room_id=id).delete()
            db.session.delete(room_to_delete)
            db.session.commit()

            return json.dumps(data)
        else:
            abort(400)

@app.route("/rooms/<room_id>", methods=["GET", "POST"])
def messages(room_id):
    #login required
    if "username" not in session:
        abort(401)

    #current room must exist
    current_room = Room.query.get(room_id)
    if current_room is None:
        flash("This room was deleted.")
        abort(404)

    if request.method == "GET":
        if "last_update" not in session:
            session["last_update"] = datetime.now().strftime("%Y-%m-%d-%H-%M-%S-%f")

            past_messages = db.session.\
                            query(Message.content, Message.timestamp, User.username).\
                            join(User).\
                            filter(Message.room_id==room_id).\
                            order_by(Message.timestamp).\
                            all()

            return json.dumps(past_messages, default=str)
        else:
            last_fetch = datetime.strptime(session["last_update"], "%Y-%m-%d-%H-%M-%S-%f")
            session["last_update"] = datetime.now().strftime("%Y-%m-%d-%H-%M-%S-%f")

            new_messages = db.session.\
                           query(Message.content, Message.timestamp, User.username).\
                           join(User).\
                           filter(Message.room_id==room_id, Message.timestamp > last_fetch).\
                           order_by(Message.timestamp).\
                           all()

            return json.dumps(new_messages, default=str)
    elif request.method == "POST":
        #checks if new message is not empty
        #if so, adds it to database
        try:
            content = request.form["content"]
        except:
            abort(400)

        if content != "":
            new_message = Message(content=content, author=session["user_id"], room=room_id)
            db.session.add(new_message)
            db.session.commit()

            data = {}
            data["author_id"] = session["user_id"]
            data["room_id"] = room_id
            data["content"] = content

            return json.dumps(data)
        else:
            abort(400)

    return render_template("room.html", title=room_id, session=session)
