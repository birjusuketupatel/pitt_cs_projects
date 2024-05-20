let timeout = 1000;

function setup(){
  document.getElementById("submit").addEventListener("click", sendMessage);
  getMessages();
}//setup

function getMessages(){
  //gets new messages and appends them to message log
  fetch("http://localhost:5000/rooms/" + room_id)
  .then((response) => {
    return response.json();
  })
  .then(updateLog)
  .then(() => {
    window.setTimeout(getMessages, timeout);
  })
  .catch(() => {
    //if getting messages has failed, room has been delete
    window.location.replace("http://localhost:5000");
  });
}//getMessages

function updateLog(messages){
  if(messages.length == 0){
    return;
  }

  let content = 0;
  let timestamp = 1;
  let user = 2;

  messageList = document.getElementById("all_messages");
  for(let i = 0; i < messages.length; i++){
    item = document.createElement("li");
    item.appendChild(document.createTextNode(messages[i][user] + ": " + messages[i][content] + " " + messages[i][timestamp]));
    messageList.appendChild(item);
  }
}//updateLog

function sendMessage(){
  let new_message = document.getElementById("message").value;
  document.getElementById("message").value = "";

  if(new_message == ""){
    return;
  }

  //sends a new message
  //on success prints message, on failure redirects to home
  fetch("http://localhost:5000/rooms/" + room_id, {
      method: "post",
      headers: { "Content-type": "application/x-www-form-urlencoded; charset=UTF-8" },
      body: `content=${new_message}`
  })
  .then((response) => {
    return response.json();
  })
  .then((result) => {
    console.log(result);
  })
  .catch(() => {
    //if post fails, room has been deleted
    window.location.replace("http://localhost:5000");
  });
}//sendMessage

window.addEventListener("DOMContentLoaded", setup);
