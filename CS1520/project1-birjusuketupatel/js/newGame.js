"use strict";

function start(){
  //starts the game
  let game = load("game");
  game.gameStarted = true;
  save("game", game);

  //redirects to input screen
  window.location.href = "initGame.html";
}//start

function setup(){
  //clears previous session when user enters first page
  sessionStorage.clear();

  var startButton = document.getElementById("start");
  startButton.addEventListener("click", start);

  //creates and saves a new game
  let game = new Game();
  save("game", game);
}//setup

window.addEventListener("DOMContentLoaded", setup);
