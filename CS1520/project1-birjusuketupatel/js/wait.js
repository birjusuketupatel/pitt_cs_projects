"use strict";

function setup(){
  let game = load("game");

  //if game has not started, redirect to home page
  if(!game.started()){
    window.location.href = "index.html";
  }

  //if players have not been initialized, redirect to input page
  if(game.playerNames.length < 2){
    window.location.href = "initGame.html";
  }

  //if no tile was clicked, redirect to battleship
  if(!sessionStorage.getItem("lastClickInfo")){
    window.location.href = "battleship.html";
  }

  //displays last hit
  //displays if ship was sunk
  let out = document.getElementById("turn-info");
  let msg = sessionStorage.getItem("lastClickInfo");
  if(sessionStorage.getItem("message")){
    msg += sessionStorage.getItem("message");
    msg += "<br>";
    sessionStorage.removeItem("message");
  }

  out.innerHTML += msg;

  //adds button to go back to game
  let continueOn = document.querySelector("button");
  continueOn.addEventListener("click", function(){
    window.location.href = "battleship.html";
  });

  //gets score
  let names = game.playerNames;
  let scores = game.scores;
  let endGame = "";

  //if game ends on this turn, display final score
  //change continue button to new game button
  if(scores[0] === 0 || scores[1] === 0){
    if(scores[0] === 0){
      endGame += names[1] + " wins!<br>";
    }
    else if(scores[1] === 0){
      endGame += names[0] + " wins!<br>";
    }

    endGame += "Final Scores:<br>";
    endGame += names[0] + ": " + scores[0] + "<br>";
    endGame += names[1] + ": " + scores[1] + "<br>";

    out.innerHTML += endGame;
    continueOn.innerHTML = "New Game";
    continueOn.addEventListener("click", function(){
      window.location.href = "index.html";
    });
  }
}//setup

window.addEventListener("DOMContentLoaded", setup);
