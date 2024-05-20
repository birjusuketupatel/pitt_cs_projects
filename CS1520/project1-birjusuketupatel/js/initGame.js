"use strict";

if(!sessionStorage.getItem("curr")){
  sessionStorage.setItem("curr", 0);
}

/*
 * Validates data from the new player form.
 * If invalid, returns -1
 * If valid, adds player to game and returns 0
 * If player limit reached, adds final player to game and returns 1
 */
function addNewPlayer(){
  let playerForm = document.getElementById("newPlayer").elements;
  let curr = sessionStorage.getItem("curr");
  let nextPlayer = {};

  for(let i = 0; i < playerForm.length; i++){
    let item = playerForm.item(i);
    if(item.name != "submit"){
      nextPlayer[item.name] = item.value;
    }
  }

  //validates and stores user input
  let game = load("game");
  if(!game.addPlayer(nextPlayer)){
    sessionStorage.setItem("error", "<li>Ship position input incorrect.</li>");
    return;
  }
  save("game", game);

  //move to next player
  curr++;
  sessionStorage.setItem("curr", curr);
}//addNewPlayer

function setup(){
  let game = load("game");

  //if game has not started, redirect to home page
  if(!game.started()){
    window.location.href = "index.html";
  }

  //if info for all players has been entered, redirect to battleship page
  let curr = parseInt(sessionStorage.getItem("curr"));
  if(curr >= game.maxPlayers){
    sessionStorage.setItem("lastClickInfo", "Start " + game.playerNames[0] + "'s turn<br>");
    window.location.href = "wait.html";
  }

  //loads error if exists, remove error after it is flashed
  if(sessionStorage.getItem("error")){
    let errorList = document.getElementById("errors");
    errorList.innerHTML += sessionStorage.getItem("error");
    sessionStorage.removeItem("error");
  }

  //hide query string
  history.replaceState({}, null, 'initGame.html');

  let addPlayer = document.getElementById("newPlayer");
  addPlayer.addEventListener("submit", addNewPlayer);

  //update player number label
  curr = parseInt(sessionStorage.getItem("curr"));
  let label = document.getElementById("nextPlayer");
  label.innerHTML = "Player " + (curr + 1) + ":";
}//setup

window.addEventListener("DOMContentLoaded", setup);
