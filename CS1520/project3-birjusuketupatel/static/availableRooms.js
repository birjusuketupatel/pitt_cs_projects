let timeout = 2500;

function setup(){
  document.getElementById("submit").addEventListener("click", createNewRoom);
  getAllRooms();
}//setup

function createNewRoom(){
  let new_room_name = document.getElementById("room_name").value;
  document.getElementById("room_name").value = "";

  if(new_room_name == ""){
    return;
  }

  //creates a new room
  //on success prints new room, on failure prints ERROR
  fetch("http://localhost:5000/rooms", {
      method: "post",
			headers: { "Content-type": "application/x-www-form-urlencoded; charset=UTF-8" },
			body: `name=${new_room_name}&owner_id=${user_id}`
  })
  .then((response) => {
    return response.json();
  })
  .then((result) => {
    console.log(result);
  })
  .catch(() => {
    console.log("ERROR");
  });
}//createNewRoom

function deleteRoom(roomId, ownerId){
  //deletes a room owned by the user
  //on success prints deleted room, on failure prints ERROR
  fetch("http://localhost:5000/rooms", {
      method: "delete",
      headers: { "Content-type": "application/x-www-form-urlencoded; charset=UTF-8" },
      body: `room_id=${roomId}&owner_id=${ownerId}`
  })
  .then((response) => {
    return response.json();
  })
  .then((result) => {
    console.log(result);
  })
  .catch(() => {
    console.log("ERROR");
  });
}//deleteRoom

function joinRoom(roomId){
  window.location.replace("http://localhost:5000/chatroom/" + roomId);
}//joinRoom

function getAllRooms(){
  //gets all rooms available to join
  //updates to reflect additions and deletions of rooms
  fetch("http://localhost:5000/rooms")
  .then((response) => {
    return response.json();
  })
  .then(updateRooms)
  .then(() => {
    window.setTimeout(getAllRooms, timeout);
  })
  .catch(() => {
    console.log("ERROR");
  });
}//getAllRooms

function updateRooms(rooms){
  let allRooms = document.getElementById("all_rooms");

  let roomList = document.createElement("ul");

  //if no rooms available, display message to user
  if(rooms.length == 0){
    message = document.createElement("li");
    message.appendChild(document.createTextNode("No rooms right now."));
    roomList.appendChild(message);
    allRooms.innerHTML = "";
    allRooms.appendChild(roomList);
    return;
  }

  let id = 0;
  let name = 1;
  let ownerId = 2;

  //fill unordered list with list of rooms available to join
  for(let i = 0; i < rooms.length; i++){
    let item = document.createElement("li");

    joinButton = document.createElement("button");
    joinButton.textContent = rooms[i][name];
    joinButton.addEventListener("click", function(){
      joinRoom(rooms[i][id]);
    });
    item.appendChild(joinButton);

    //if user owns this room, give option to delete it
    if(user_id == rooms[i][ownerId]){
      cancelButton = document.createElement("button");
      cancelButton.textContent = "X";
      cancelButton.addEventListener("click", function(){
        deleteRoom(rooms[i][id], rooms[i][ownerId]);
      });
      item.appendChild(cancelButton);
    }

    roomList.appendChild(item);
  }

  allRooms.innerHTML = "";
  allRooms.appendChild(roomList);
}//updateRooms

window.addEventListener("DOMContentLoaded", setup)
