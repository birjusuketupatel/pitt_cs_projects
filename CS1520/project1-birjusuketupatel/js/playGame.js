"use strict";

var colNames = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J"];

if(!sessionStorage.getItem("turn")){
  sessionStorage.setItem("turn", 0);
}

if(!sessionStorage.getItem("p1Hits")){
  let p1Hits = {};
  p1Hits["A"] = 5;
  p1Hits["B"] = 4;
  p1Hits["S"] = 3;

  sessionStorage.setItem("p1Hits", JSON.stringify(p1Hits));
}

if(!sessionStorage.getItem("p2Hits")){
  let p2Hits = {};
  p2Hits["A"] = 5;
  p2Hits["B"] = 4;
  p2Hits["S"] = 3;

  sessionStorage.setItem("p2Hits", JSON.stringify(p2Hits));
}

/*
 * creates a grid with rows labeled 1-10
 * and columns labeled from A-J
 * type 0 grid: grid of player's own ships
 * type 1 grid: grid of player's hits and misses on other player
 */
function buildGrid(player, type, rows, cols){
  let game = load("game");
  let grid = document.createElement("table");
  grid.className = "grid";

  for(let i = 0; i < rows + 1; i++){
    let tr = document.createElement("tr");
    for(let j = 0; j < cols + 1; j++){
      let cell = document.createElement("td");

      if(i === 0 && j !== 0){
        //adds letters for each column in first row
        cell.innerHTML = colNames[j-1];
      }
      else if(j === 0 && i !== 0){
        //adds numbers for each row in first column
        cell.innerHTML = i;
      }
      else if(i !== 0 && j !== 0){
        //marks square with particular color and text based on its data
        let x = j - 1;
        let y = i - 1;
        let squareData = game.getSquareProperties(player, type, x, y); //gets data for particular square

        cell.className = squareData[0];

        if(type === 0 && squareData[1] !== "W"){
          //marks ship squares with corresponding letter
          cell.innerHTML = squareData[1];
        }

        if(type === 1){
          //adds class clickable to cell
          //for styling purposes
          cell.className += " clickable";

          //adds click handler to each cell in the clickable grid
          cell.addEventListener("click", function(){
            let result = game.clickOnGrid(player, x, y);

            //updates count of the number of hits player has left on opponent's ship
            if(result == ""){
              return;
            }
            else if(result !== "W"){
              let hits;
              let other;
              if(player == 0){
                hits = JSON.parse(sessionStorage.getItem("p1Hits"));
                other = 1;
              }
              else{
                hits = JSON.parse(sessionStorage.getItem("p2Hits"));
                other = 0;
              }

              //if ship sunk, send message and remove item
              //else decrement count
              let count = hits[result];
              let shipNames = {"A": "Aircraft Carrier",
                               "B": "Battleship",
                               "S": "Submarine"};
              if(count === 1){
                let message = game.playerNames[player] + " sunk " +
                              game.playerNames[other] + "'s " + shipNames[result];
                sessionStorage.setItem("message", message);
                delete hits[result];
              }
              else{
                hits[result] = count - 1;
              }

              //save update to hits
              if(player == 0){
                sessionStorage.setItem("p1Hits", JSON.stringify(hits));
              }
              else{
                sessionStorage.setItem("p2Hits", JSON.stringify(hits));
              }
            }

            //stores message to be displayed on waiting screen
            let lastClickInfo = game.playerNames[player] + " selected " + colNames[x] + (y + 1) + "<br>";
            if(result !== "W"){
              lastClickInfo += "It was a hit<br>";
            }
            else{
              lastClickInfo += "It was a miss<br>";
            }
            sessionStorage.setItem("lastClickInfo", lastClickInfo);

            //switches turn
            if(player == 1){
              sessionStorage.setItem("turn", 0);
            }
            else{
              sessionStorage.setItem("turn", 1);
            }

            save("game", game);
            window.location.href = "wait.html";
          });
        }
      }

      tr.appendChild(cell);
    }
    grid.appendChild(tr);
  }

  return grid;
}//buildGrid

function setup(){
  let game = load("game");
  let turn = parseInt(sessionStorage.getItem("turn"));

  //if game has not started, redirect to home page
  if(!game.started()){
    window.location.href = "index.html";
  }

  //if players have not been initialized, redirect to input page
  if(game.playerNames.length < 2){
    window.location.href = "initGame.html";
  }

  //display flash message
  if(sessionStorage.getItem("message")){
    console.log(sessionStorage.getItem("message"));
    sessionStorage.removeItem("message");
  }

  //displays player name
  let name = document.createElement("h2");
  name.innerHTML += game.playerNames[turn];
  document.querySelector("header").appendChild(name);

  //build grid and display on screen
  let grid0 = buildGrid(turn, 0, game.height, game.width);
  let grid1 = buildGrid(turn, 1, game.height, game.width);

  document.getElementById("playerGrid").appendChild(grid0);
  document.getElementById("targetGrid").appendChild(grid1);

  save("game", game);
}//setup

window.addEventListener("DOMContentLoaded", setup);
