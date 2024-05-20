"use strict";

/*
 * Tracks the state of the battleship game.
 * Stores a list of n players and keeps track of the turn.
 */
class Game{
  constructor(){
    this.gameStarted = false;
    this.maxPlayers = 2;
    this.height = 10;
    this.width = 10;
    this.playerNames = [];
    this.shipLocs = [];
    this.boards = [];
    this.scores = [24, 24];

    //array of 10x10 grid of booleans representing squares each player has clicked
    this.clicked = [];

    for(let i = 0; i < this.maxPlayers; i++){
      let nextBoard = [];
      for(let j = 0; j < this.height; j++){
        let nextRow = [];
        for(let k = 0; k < this.width; k++){
          nextRow.push(false);
        }
        nextBoard.push(nextRow);
      }
      this.clicked.push(nextBoard);
    }
  }//constructor

  /*
   * Given a json format string of data, loads the data into the game object.
   */
  deserialize(jsonStr){
    let obj = JSON.parse(jsonStr);
    this.gameStarted = obj["gameStarted"];
    this.maxPlayers = obj["maxPlayers"];
    this.playerNames = obj["playerNames"];
    this.shipLocs = obj["shipLocs"];
    this.boards = obj["boards"];
    this.clicked = obj["clicked"];
    this.scores = obj["scores"];
  }//deserialize

  /*
   * Returns if a particular game instance has started or not.
   */
  started(){
    return !(this.gameStarted === null || this.gameStarted === false);
  }//started

  /*
   * Given data for a new player, validates data format.
   * If format is incorrect or max players reached, returns false.
   * If format is correct, adds user data to game and returns true.
   */
   addPlayer(nextPlayer){
     if(this.playerNames.length >= this.maxPlayers){
       return false;
     }

     //validates and parses the ship positions given by the use
     let positions = nextPlayer["shipPos"].trim().split(";");
     if(positions.pop() !== ""){
       return false;
     }

     //must have 3 ships specified
     if(positions.length != 3){
       return false;
     }

     //checks syntax of each ship
     var pattern = /^([ABS])\(([A-J])([1-9]|10)-([A-J])([1-9]|10)\)$/;
     for(let i = 0; i < positions.length; i++){
       if(!pattern.test(positions[i])){
         return false;
       }
     }

     //parse data from input
     let shipData = [];
     for(let i = 0; i < positions.length; i++){
       shipData[i] = positions[i].match(pattern).slice(1, 6);
     }

     let nextBoard = this.buildBoard(shipData);

     if(nextBoard === null){
       return false;
     }

     this.playerNames.push(nextPlayer["name"]);
     this.boards.push(nextBoard);

     return true;
   }//addPlayer

   /*
    * gets properties of a particular square for a given player and grid
    * grid 0: grid containing own ship positions and hits from opponent
    * grid 1: grid containing player's click history
    * if grid 0, gets color and character
    * if grid 1, gets
    */
    getSquareProperties(player, gridNumber, x, y){
      let properties = [];

      let other = 0;
      if(player == 0){
        other = 1;
      }

      if(gridNumber === 0){
        //this grid displays history of clicks made by other
        //and position of player's ships

        //if clicked and hit, color is red
        //if clicked and missed, color is white
        //if not clicked, color is blue
        //if board has ship, character is ship name
        if(this.clicked[other][y][x] && this.boards[player][y][x] !== "W"){
          properties[0] = "red";
        }
        else if(this.clicked[other][y][x] && this.boards[player][y][x] === "W"){
          properties[0] = "white";
        }
        else if(this.boards[player][y][x] !== "W"){
          properties[0] = "gray";
        }
        else{
          properties[0] = "blue";
        }

        properties[1] = this.boards[player][y][x];
      }
      else{
        //this grid displays history of clicks made by player

        //if clicked and hit, color is red
        //if clicked and miss, color is white
        //if no click, color is blue
        if(this.clicked[player][y][x] && this.boards[other][y][x] !== "W"){
          properties[0] = "red";
        }
        else if(this.clicked[player][y][x] && this.boards[other][y][x] === "W"){
          properties[0] = "white";
        }
        else{
          properties[0] = "blue";
        }
      }

      return properties;
    }//getSquareProperties

   /*
    * clicks on a given coordinate on opponent's board
    * returns what was hit, returns empty string if error
    */
    clickOnGrid(player, x, y){
      if(!this.inBounds(x, y)){
        return "";
      }

      if(this.clicked[player][y][x]){
        return "";
      }

      this.clicked[player][y][x] = true;

      //determines if this click was a hit on the other player
      let other = 0;
      if(player === 0){
        other = 1;
      }

      //if it was a hit, reduce other player's score by 2
      let objectHit = this.boards[other][y][x];
      if(objectHit !== "W"){
        this.scores[other] -= 2;
      }

      return objectHit;
    }//clickOnGrid

   /*
    * builds a board to represent the state of the game
    * the board is a 10x10 grid of characters, each player has a board
    * "W" represents water tile, "A", "B", and "S" represent a ship tiles,
    * returns board if board successfully built
    * returns null if input has logical errors
    */
    buildBoard(shipData){
      //initializes board as 10x10 grid of water
      let nextBoard = [];
      for(let i = 0; i < this.height; i++){
        let nextRow = [];
        for(let j = 0; j < this.width; j++){
          nextRow.push("W");
        }
        nextBoard.push(nextRow);
      }

      let shipLengths = {"A":5, "B":4, "S":3};
      let letterToRow = {"A":0, "B":1, "C":2, "D":3, "E":4,
                         "F":5, "G":6, "H":7, "I":8, "J":9};

      for(let i = 0; i < shipData.length; i++){
        //extracts x and y pos of start and end of ship
        let currShip = shipData[i];
        let shipType = currShip[0];
        let shipLen = shipLengths[shipType];
        let x1 = letterToRow[currShip[1]];
        let y1 = parseInt(currShip[2]) - 1;
        let x2 = letterToRow[currShip[3]];
        let y2 = parseInt(currShip[4]) - 1;

        if(!this.inBounds(x1, y1) || !this.inBounds(x2, y2)){
          return null;
        }

        //input must contain 1 of each ship
        if(!(shipType in shipLengths)){
          return null;
        }
        else{
          delete shipLengths[shipType];
        }

        //calculates length of ship
        let xLen = Math.abs(x2 - x1);
        let yLen = Math.abs(y2 - y1);

        //to be valid, one length must be zero
        //one must be equal to length of ship specified
        if(xLen !== 0 && yLen === 0){
          if(xLen !== shipLen - 1){
            return null;
          }

          //sets x1 as smaller, x2 as larger
          if(x1 > x2){
            let temp = x1;
            x1 = x2;
            x2 = temp;
          }

          //draws line from x1 to x2 on board
          for(let x = x1; x <= x2; x++){
            //error, ship already placed here
            if(nextBoard[y1][x] !== "W"){
              return null;
            }

            nextBoard[y1][x] = shipType;
          }
        }
        else if(xLen === 0 && yLen !== 0){
          if(yLen !== shipLen - 1){
            return null;
          }

          //sets y1 as smaller, y2 as larger
          if(y1 > y2){
            let temp = y1;
            y1 = y2;
            y2 = temp;
          }

          //draws line from y1 to y2 on board
          for(let y = y1; y <= y2; y++){
            //error, ship already placed here
            if(nextBoard[y][x1] !== "W"){
              return null;
            }

            nextBoard[y][x1] = shipType;
          }
        }
        else{
          return null;
        }
      }

      return nextBoard;
    }//buildBoard

    /*
     * returns true if coordinate is in bounds, false if not
     */
    inBounds(x, y){
      return x >= 0 && x < this.width && y >= 0 && y < this.height;
    }//inBounds
}//Game

/*
 * Saves an instance of the game as a JSON in session storage
 * given a unique identifier for this game and a game object.
 */
 function save(key, game){
   sessionStorage.setItem(key, JSON.stringify(game));
 }//save

 /*
  * Given a unique identifier of a game, loads that game's data from session
  * storage and returns a game object with that data.
  * If no such game exists, returns a new game.
  */
  function load(key){
    var data = sessionStorage.getItem(key);

    let game = new Game();

    if(data === null){
      return game;
    }

    game.deserialize(data);

    return game;
  }//load
