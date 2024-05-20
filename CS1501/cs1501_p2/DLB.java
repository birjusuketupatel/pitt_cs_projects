package cs1501_p2;

import java.util.ArrayList;

public class DLB implements Dict{
	
	private static final int SUGGESTIONS = 5;
	protected DLBNode root;
	protected String search;
	private int word_count;
	
	/*
	 * constructs DLB
	 * '^' is used as termination character of a word
	 * DLB initially contains the empty string ("")
	 */
	public DLB() {
		/*
		 * empty string is special case
		 * contained in DLB by default, but .contains will return false if user enters ""
		 * .containsPrefix behaves normally
		 * .add will not add ""
		 * "" does not count towards total word count, is not displayed in suggest/traverse
		 */
		root = new DLBNode('^');
		search = new String();
		word_count = 0;
	}//DLB
	
	/*
	 * verifies key is a word with length > 1 
	 * adds word to DLB
	 */
	public void add(String key) {
		
		if(valid_word(key) == false || key.length() < 1) {
			return;
		}
		
		key = key + '^'; //key plus terminating char
		DLBNode curr_node = root;
		boolean found_on_level = true;
		int curr_char = 0;
		
		/*
		 * after loop, 2 possibilities
		 * case 1: string not found in DLB
		 * found_on_level = false, curr_node = rightmost node of deepest possible prefix
		 * case 2: string exists in DLB
		 * found_on_level = true, curr_node = terminating char of current word
		 */
		while(curr_char < key.length() && found_on_level == true) {
			
			/* after loop, 2 possibilities
			 * case a: char not found on current level
			 * found_on_level = false, curr_node = rightmost node on level
			 * case b: char found on current level
			 * found_on_level = true, curr_node = node with char
			 */
			while(key.charAt(curr_char) != curr_node.getLet() && found_on_level == true) {
				
				//if next right node is null, char not found at current level
				//else check next right node for char
				if(curr_node.getRight() == null) {
					found_on_level = false;
				}
				else {
					curr_node = curr_node.getRight();
				}
			}
			
			//if case b, move down one level and search for next char in string
			if(found_on_level == true) {
				if(curr_node.getDown() != null) {
					curr_node = curr_node.getDown();
				}
				curr_char = curr_char + 1;
			}
			//if case a, leave loop and add rest of the word to the right of curr_node
		}
		
		//if case 1, add rest of string to the right of curr_node
		//increase word count by 1
		if(found_on_level == false) {
			DLBNode new_node = new DLBNode(key.charAt(curr_char));
			curr_node.setRight(new_node);
			
			curr_node = curr_node.getRight();
			curr_char = curr_char + 1;
			
			for(int i = curr_char; i < key.length(); i++) {
				new_node = new DLBNode(key.charAt(i));
				curr_node.setDown(new_node);
				curr_node = curr_node.getDown();
			}
			
			word_count = word_count + 1;
		}
		//if case 2, string is already in DLB, do nothing
		
		return;
	}//add
	
	
	/*
	 * verifies that string only contains letters or apostrophes
	 */
	public boolean valid_word(String key) {
		char[] word = key.toCharArray();
		
		for(int i = 0; i < key.length(); i++) {
			char c = word[i];
			if(!Character.isLetter(c) && c != '\'') {
				return false;
			}
		}
		
		return true;
	}//valid_word
	
	
	/*
	 * determines if a string is in the DLB
	 */
	public boolean contains(String key) {
		
		if(valid_word(key) == false || key.length() < 1) {
			return false;
		}
		
		boolean found = true;
		key = key + '^';
		int curr_char = 0;
		DLBNode curr_node = root;
		
		//outer loop traverses the input
		//inner loop traverses the DLB to the right
		while(curr_char < key.length() && found == true) {
			
			while(curr_node != null && curr_node.getLet() != key.charAt(curr_char)) {
				curr_node = curr_node.getRight();
			}
			
			//if curr_node is null, curr_char not found
			//else, curr_char is in curr_node
			if(curr_node == null) {
				found = false;
			}
			else {
				if(curr_node.getDown() != null) {
					curr_node = curr_node.getDown();
				}
				curr_char = curr_char + 1;
			}
		}
		
		return found;
	}//contains
	
	
	/*
	 * determines if input can be a prefix to another word in the DLB
	 */
	public boolean containsPrefix(String pre) {
		
		if(valid_word(pre) == false) {
			return false;
		}
		
		boolean found = true;
		int curr_char = 0;
		DLBNode curr_node = root;

		while(curr_char < pre.length() && found == true) {
			
			while(curr_node != null && curr_node.getLet() != pre.charAt(curr_char)) {
				curr_node = curr_node.getRight();
			}
			
			if(curr_node == null) {
				found = false;
			}
			else {
				if(curr_node.getDown() != null) {
					curr_node = curr_node.getDown();
				}
				curr_char = curr_char + 1;
			}
		}
		
		/*
		 * 2 cases
		 * 1: valid prefix found, do nothing
		 * 2: complete word found (curr_node contains ^), must determine if complete word can serve as prefix to another word
		 */
		if(found == true && curr_node.getLet() == '^') {
			if(curr_node.getRight() == null) {
				//word does not serve as a prefix to another
				found = false;
			}
		}
		
		return found;
	}//containsPrefix
	
	
	/*
	 * checks if input char is valid
	 * adds input char to search word
	 * returns info about search word
	 */
	public int searchByChar(char next) {
		search = search + next; //add input to search string
		
		//if search word has illegal chars, return -1 for invalid string
		if(valid_word(search) == false) {
			return -1;
		}
		
		/*
		 * 4 cases,
		 * 1: invalid prefix and invalid word, -1
		 * 2: valid prefix and invalid word, 0
		 * 3: invalid prefix and valid word, 1
		 * 4: valid prefix and valid word, 2
		 */
		boolean contains_prefix = containsPrefix(search);
		boolean contains = contains(search);
		
		if(contains_prefix == false && contains == false) {
			return -1;
		}
		else if(contains_prefix == true && contains == false) {
			return 0;
		}
		else if(contains_prefix == false && contains == true) {
			return 1;
		}
		else {
			return 2;
		}
	}//searchByChar
	
	
	/*
	 * resets search string to empty string
	 */
	public void resetByChar() {
		search = "";
		return;
	}//resetByChar
	
	
	/*
	 * finds position of last letter in given prefix in tree
	 * calls recursive function to suggest up to 5 words
	 */
	public ArrayList<String> suggest(){
		ArrayList<String> suggestions = new ArrayList<String>();
		
		//if search words cannot be a prefix and is not in tree, return empty list
		if(containsPrefix(search) == false && contains(search) == false) {
			return suggestions;
		}
		
		//search word is now guaranteed to be in the tree
		DLBNode curr_node = root;
		int curr_char = 0;
		
		//brings curr_node to the place in tree representing char after the final letter of given prefix
		while(curr_char < search.length()) {
			while(curr_node != null && search.charAt(curr_char) != curr_node.getLet()) {
				curr_node = curr_node.getRight();
			}
			
			curr_char = curr_char + 1;
			if(curr_node != null) {
				curr_node = curr_node.getDown();
			}
		}
		
		suggestions = rec_suggest(search, curr_node, suggestions);
		
		return suggestions;
	}//suggest
	
	
	/*
	 * recursively traverses DLB starting at curr_node, going down first then right
	 * adds first 5 words encountered in traversal
	 */
	private ArrayList<String> rec_suggest(String curr_word, DLBNode curr_node, ArrayList<String> suggestions){
		//if more than 5 found, stop looking
		if(suggestions.size() >= SUGGESTIONS) {
			return suggestions;
		}
		
		//if currently at end of word (^), add curr_word to list
		//does not add if word is empty string
		if(curr_node.getLet() == '^' && curr_word.equals("") == false) {
				suggestions.add(curr_word);
		}
		
		curr_word = curr_word + curr_node.getLet(); //adds current letter to word
		
		if(curr_node.getDown() != null) {
			suggestions = rec_suggest(curr_word, curr_node.getDown(), suggestions);
		}
		
		//before going to the right, last char must be removed so it can be replaced
		if(curr_word.length() > 0) {
			curr_word = curr_word.substring(0, curr_word.length() - 1);
		}
		
		if(curr_node.getRight() != null) {
			suggestions = rec_suggest(curr_word, curr_node.getRight(), suggestions);
		}
		
		return suggestions;
	}//rec_suggest
	
	
	/*
	 * wrapper function, calls recursive function to traverse
	 */
	public ArrayList<String> traverse(){
		ArrayList<String> traversal = new ArrayList<String>();
		traversal = rec_traversal("", root, traversal);
		
		return traversal;
	}//traverse
	
	
	/*
	 * recursively traverses DLB, going down first then right
	 */
	private ArrayList<String> rec_traversal(String curr_word, DLBNode curr_node, ArrayList<String> traversal) {
		//if currently at end of word (^), add curr_word to list
		//does not add if curr_word is the empty string
		if(curr_node.getLet() == '^' && curr_word.equals("") == false) {
			traversal.add(curr_word);
		}
		
		curr_word = curr_word + curr_node.getLet(); //adds current letter to word
		
		if(curr_node.getDown() != null) {
			traversal = rec_traversal(curr_word, curr_node.getDown(), traversal);
		}
		
		//before going to the right, last char must be removed so it can be replaced
		if(curr_word.length() > 0) {
			curr_word = curr_word.substring(0, curr_word.length() - 1);
		}
		
		if(curr_node.getRight() != null) {
			traversal = rec_traversal(curr_word, curr_node.getRight(), traversal);
		}
		
		return traversal;
	}//rec_traversal
	
	
	public int count() {
		return word_count;
	}//count
}//DLB
