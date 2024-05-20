package cs1501_p2;

import java.util.*;

public class UserHistory extends DLB implements Dict{
	
	private static final int SUGGESTIONS = 5;
	private Hashtable<String, Integer> word_count;
	
	/*
	 * dictionary stores all words used by the user
	 * word_count stores the words with the number of times they were used
	 */
	public UserHistory() {
		super();
		word_count = new Hashtable<String, Integer>();
	}//UserHistory
	
	
	/*
	 * if key already exists in table, increment count by 1
	 * else add key to table and dictionary
	 */
	public void add(String key) {
		
		//if input is invalid or empty, does nothing
		if(super.valid_word(key) == false || key.length() < 1) {
			return;
		}
		
		if(word_count.containsKey(key) == true) {
			Integer new_count = word_count.get(key) + 1;
			word_count.put(key, new_count);
		}
		else {
			super.add(key);
			word_count.put(key, 1);
		}
		return;
	}//add
	
	
	/*
	 * traverses dictionary to location of last character in search string
	 * calls recursive method to deliver the top 5 most frequent words with that prefix
	 */
	public ArrayList<String> suggest() {
		ArrayList<String> suggestions = new ArrayList<String>();
		DLBNode curr_node = super.root;
		String search_str = super.search;
		int curr_char = 0;
		
		//if search string cannot be a prefix and is not in tree, return empty list
		if(super.containsPrefix(search_str) == false && super.contains(search_str) == false) {
			return suggestions;
		}
		
		//brings curr_node to the place in tree representing char after the final letter of given prefix
		//search_str is guaranteed to be in the tree
		while(curr_char < search_str.length()) {
			while(curr_node != null && search_str.charAt(curr_char) != curr_node.getLet()) {
				curr_node = curr_node.getRight();
			}
				
			curr_char = curr_char + 1;
			if(curr_node != null) {
				curr_node = curr_node.getDown();
			}
		}
		
		suggestions = rec_suggest(search_str, curr_node, suggestions);
		suggestions = sort_suggestions(suggestions);
		
		return suggestions;
	}//suggest
	
	
	/*
	 * uses bubble sort to sort suggestions from most used to least used
	 */
	private ArrayList<String> sort_suggestions(ArrayList<String> suggestions) {
		boolean swapped = true;
		String x = new String();
		String y = new String();
		
		while(swapped == true) {
			swapped = false;
			
			for(int i = 1; i < suggestions.size(); i++) {
				x = suggestions.get(i - 1);
				y = suggestions.get(i);
				
				if(word_count.get(y) > word_count.get(x)) {
					swapped = true;
					suggestions.set(i - 1, y);
					suggestions.set(i, x);
				}
			}
		}
		
		return suggestions;
	}//sort_suggestions
	
	
	/*
	 * recursively traverses DLB, starting down and then going right
	 * fills suggestions with first 5 strings with search_str as their prefix
	 * if more are found, replaces lowest used strings with higher used ones
	 */
	private ArrayList<String> rec_suggest(String curr_word, DLBNode curr_node, ArrayList<String> suggestions) {
		//if currently at end of word (^), check curr_word to add
		//cannot add if word is empty string
		if(curr_node.getLet() == '^' && curr_word.equals("") == false) {
			if(suggestions.size() < SUGGESTIONS) {
				suggestions.add(curr_word);
			}
			else {
				//finds smallest string in suggestions
				String small_string = suggestions.get(0);
				Integer small_val = word_count.get(small_string);
				Integer small_index = 0;
				String check_string = new String();
				Integer check_val = 0;
				
				for(int i = 1; i < suggestions.size(); i++) {
					check_string = suggestions.get(i);
					check_val = word_count.get(check_string);
					
					//if new string is smaller than current smallest string, replace
					if(small_val > check_val) {
						small_string = check_string;
						small_val = check_val;
						small_index = i;
					}
				}
				
				//if word count of curr word > word count of smallest word in suggestions, replace
				Integer curr_word_count = word_count.get(curr_word);
				if(curr_word_count > small_val) {
					suggestions.set(small_index, curr_word);
				}
			}
		}
		
		//continue traversing
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
	 * store all words in dictionary and the number of times they have been added in a string
	 */
	public String toString() {
		String output = new String();
		
		String next_word = new String();
		String next_line = new String();
		Integer next_count = 0;
		
		Set<String> keys = word_count.keySet();
		Iterator<String> iterate = keys.iterator();
		
		while(iterate.hasNext() == true) {
			next_word = iterate.next();
			next_count = word_count.get(next_word);
			next_line = next_word + "," + next_count + "\n";
			output = output + next_line;
		}
		
		return output;
	}//show_counts
}//UserHistory
