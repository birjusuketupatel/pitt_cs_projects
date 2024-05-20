package assignment3;

import java.util.*;
import java.io.*;

public class FindPhrase {
	
	private Stack <String> currPhrase; //tracks how much of the phrase the algorithm has found
	private Stack <Line> solution; //tracks location of words found on the grid
	
	public static void main(String[] args) {
		
		new FindPhrase();
	
	}//main
	
	public FindPhrase() {
		
		@SuppressWarnings("resource")
		Scanner inScan = new Scanner(System.in);
		Scanner fReader;
		File fName;
        String fString = "", temp_phrase = "";
       
       	// Make sure the file name is valid
        while (true) {
        	
           try {
               System.out.println("Please enter grid filename:");
               fString = inScan.nextLine();
               fName = new File(fString);
               fReader = new Scanner(fName);
              
               break;
           }
           catch (IOException e) {
               System.out.println("Problem " + e);
           }
           
        }
        System.out.println();
        
		// Parse input file to create 2-d grid of characters
		String [] dims = (fReader.nextLine()).split(" ");
		int rows = Integer.parseInt(dims[0]);
		int cols = Integer.parseInt(dims[1]);
		
		char [][] theBoard = new char[rows][cols];

		for (int i = 0; i < rows; i ++) {
			String rowString = fReader.nextLine();
			for (int j = 0; j < rowString.length(); j++) {
				theBoard[i][j] = Character.toLowerCase(rowString.charAt(j));
			}
		}

		// Show user the grid
		for (int i = 0; i < rows; i ++) {
			for (int j = 0; j < cols; j ++){
				System.out.print(theBoard[i][j] + " ");
			}
			System.out.println();
		}
		
		//takes in phrase from user and stores in ArrayList
		System.out.println("Please enter the phrase you would like to search for, enter -1 to quit:");
		temp_phrase = inScan.nextLine();
		temp_phrase.toLowerCase();
		ArrayList <String> phrase = new ArrayList<String>(Arrays.asList(temp_phrase.split(" ")));
		
		//loop executes until user enters a phrase with the character "-1"
		while(!phrase.contains("-1")) {
			
			//shows user the phrase
			System.out.print("\nlooking for: ");
			for(int i = 0; i < phrase.size(); i++)
				System.out.print(phrase.get(i) + " ");
			System.out.println();
			System.out.println("containing " + phrase.size() + " words");
			
			boolean found = false;
			
			//parses each character in grid
			for(int r = 0; (r < rows && !found); r++) {
				for(int c = 0; (c < cols && !found); c++) {	
					
					currPhrase = new Stack <String> (); //contains words found by matchPhrase method so far
					solution = new Stack<Line> (); //contains list of coordinates where words are located
					found = matchPhrase(r, c, phrase, theBoard); //determines if phrase is present at particular coordinate on grid
					
				}
			}
			
			//tells user whether or not phrase was found
			//if found, tells user location of phrase
			if(found) {
				
				System.out.print("the phrase: ");
				for(int i = 0; i < phrase.size(); i++)
					System.out.print(phrase.get(i) + " ");
				System.out.println();
				System.out.println("was found at...");
				
				for(int i = 0; i < phrase.size(); i++)
					System.out.println(phrase.get(i) + ": " + solution.get(i));
				System.out.println();		
			
			}
			else {
				
				System.out.print("the phrase: ");
				for(int i = 0; i < phrase.size(); i++)
					System.out.print(phrase.get(i) + " ");
				System.out.println();
				System.out.println("was not found");
				System.out.println();
				
			}
			
			//displays board with found phrase in upper case if phrase was found
			//resets board to lower case regardless
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++){
					if(found)
						System.out.print(theBoard[i][j] + " ");
					theBoard[i][j] = Character.toLowerCase(theBoard[i][j]);
				}
				if(found)
					System.out.println();
			}
			
			phrase.clear(); //clears the phrase
			
			//takes in new phrase from user, stores in ArrayList
			System.out.println("Please enter the phrase you would like to search for, enter -1 to quit:");
			temp_phrase = inScan.nextLine();
			temp_phrase.toLowerCase();
			phrase = new ArrayList<String>(Arrays.asList(temp_phrase.split(" ")));
		
		}
		
	}//FindPhrase

	/*
	 * Description: determines if the phrase is located at a given coordinate
	 * Input: r (y coordinate), c (x coordinate), phrase (phrase that is being searched for), board (search area)
	 * Output: whether or not the phrase is at this coordinate
	 */
	private boolean matchPhrase(int r, int c, ArrayList<String> phrase, char [][] board) {
		
		boolean found = false;
		
		//searches for word in all 4 directions
		for(int i = 1; i < 5; i++) {
			
			found = matchWord(i, r, c, phrase, board, 0); 
			if(found)
				break;
			
		}
		
		return found;
		
	}//matchPhrase
	
	/*
	 * Description: determines if there is a word at the given coordinate, if so,
	 * checks to see if next word is at adjacent coordinate, terminates when all words have been found
	 * Input: dir (direction being searched), r (y coordinate), c (x coordinate), phrase (list of words to be found)
	 * bo (search area), currWord (index of word in phrase is being searched for)
	 * Output: whether or not all words have been found
	 */
	private boolean matchWord(int dir, int r, int c, ArrayList<String> phrase, char [][] bo, int currWord) {
		
		if(r >= bo.length || r < 0 || c >= bo[0].length || c < 0)
			return false; //returns false if coordinate is out of bounds
		
		boolean found = false;	
		
		if(currWord == phrase.size())
			found =  true; //end condition, true if all words in phrase have been matched
		else if(checkDir(dir, r, c, phrase.get(currWord), bo, 0)) {
			
			Line coordinates = new Line();
			coordinates.changeInitial(c, r);
			
			//changes r and c to be coordinates where word ends in the grid
			switch(dir) {
			case 1:
				c = c + (phrase.get(currWord).length() - 1);
				break;
			case 2:
				r = r + (phrase.get(currWord).length() - 1);
				break;
			case 3:
				c = c - (phrase.get(currWord).length() - 1);
				break;
			case 4:
				r = r - (phrase.get(currWord).length() - 1);
				break;
			}
			
			//adds coordinates and word found to stacks
			coordinates.changeFinal(c, r);
			solution.push(coordinates);
			currPhrase.push(phrase.get(currWord));
			
			//searches adjacent coordinates for next word in phrase
			found = matchWord(1, r, c+1, phrase, bo, currWord+1);
			if(!found)
				found = matchWord(2, r+1, c, phrase, bo, currWord+1);
			if(!found)
				found = matchWord(3, r, c-1, phrase, bo, currWord+1);
			if(!found)
				found = matchWord(4, r-1, c, phrase, bo, currWord+1);
			
			//if the next word is not found, removes current coordinates and word from stack
			//changes last word found to lower case
			if(!found && currPhrase.size() >= 0) {
				
				Line makeLower = solution.pop();
				currPhrase.pop();
				lowerLine(makeLower, bo);
				
			}
			
		}
		
		return found;
		
	}//matchWord
	
	/*
	 * Description: makes all letters in a line on the grid lower case
	 * Input: board (character grid), lower (Line object of coordinates to be made lower case)
	 * Output: modifies the board, no output
	 */
	private void lowerLine(Line lower, char[][] board) {
		
		int delta_y = lower.yFinal() - lower.yInitial();
		int delta_x = lower.xFinal() - lower.xInitial();
		
		if(delta_y == 0) {
			
			int y = lower.yFinal();
			for(int i = lower.xInitial(); i <= lower.xFinal(); i += (delta_x / Math.abs(delta_x)))
				board[y][i] = Character.toLowerCase(board[y][i]);
			
		}
		else if(delta_x == 0) {
			
			int x = lower.xFinal();
			for(int i = lower.yInitial(); i <= lower.yFinal(); i += (delta_y / Math.abs(delta_y)))
				board[i][x] = Character.toLowerCase(board[i][x]);
			
		}
		
	}//lowerLine
	
	/* 
	 * Description: recursively checks if word is present in a given direction on the grid at a given coordinate
	 * Input: dir (direction, 1=right, 2=down, 3=left, 4=up), r (initial x coordinate), c (initial y coordinate)
	 * word (target word), bo (character grid), count (tracks which letter is being checked)
	 * Output: returns true if word is found, false if not, capitalized found word in grid
	 */
	private boolean checkDir(int dir, int r, int c, String word, char [][] bo, int count) {

		if(r >= bo.length || r < 0 || c >= bo[0].length || c < 0)
			return false; //returns false if coordinate is out of bounds
		
		boolean result = false;
		
		if(word.charAt(count) == bo[r][c] && count < word.length() - 1) {
			
			bo[r][c] = Character.toUpperCase(bo[r][c]); //changes found letters to upper case
			
			//dir variable controls which point on the board is checked next
			//checks next letter in cooresponding direction
			switch(dir) {
				case 1:
					result = checkDir(dir, r, c + 1, word, bo, count + 1); //right
					break;
				case 2:
					result = checkDir(dir, r + 1, c, word, bo, count + 1); //down
					break;
				case 3:
					result = checkDir(dir, r, c - 1, word, bo, count + 1); //left
					break;
				case 4:
					result = checkDir(dir, r - 1, c, word, bo, count + 1); //up
					break;
			}
			
			if(result == false)
				bo[r][c] = Character.toLowerCase(bo[r][c]); //if word was not found, resets letters to lower case
			
		}
		else if(word.charAt(count) == bo[r][c] && count == word.length() - 1) {
			bo[r][c] = Character.toUpperCase(bo[r][c]); //sets final letter to upper case
			result = true; //if last letter matches, word is found	
		}
		else
			result = false; //if letter does not match, sets result to false

		return result;
		
	}//checkDir
	
}//FindPhrase