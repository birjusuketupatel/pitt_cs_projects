package cs1501_p2;

import java.util.*;
import java.io.*;

public class AutoCompleter implements AutoComplete_Inter {
	
	private static final int SUGGESTIONS = 5;
	private UserHistory user_hist; //stores current user history
	private DLB dictionary; //stores all words
	
	/*
	 * constructor if user history is given
	 */
	public AutoCompleter(String dict_fname, String hist_fname) {
		user_hist = new UserHistory();
		dictionary = new DLB();
		
		//scans all words from dictionary file to dictionary
		try (Scanner s = new Scanner(new File(dict_fname))) {
			while (s.hasNext()) {
				dictionary.add(s.nextLine());
			}
		}
		catch (IOException e) {
			e.printStackTrace();
		}
		
		//scans user history from history file to user_hist
		String word = new String();
		Integer count = 0;
		try (Scanner scan = new Scanner(new File(hist_fname))) {
			while (scan.hasNext()) {
				String line = scan.nextLine();
				
				//line format: "word,count"
				word = line.substring(0, line.indexOf(","));
				count = Integer.parseInt(line.substring(line.indexOf(",") + 1, line.length()));
				
				//adds word to user_hist count times
				for(int i = 0; i < count; i++) {
					user_hist.add(word);
				}
			}
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}//AutoCompleter
	
	
	/*
	 * constructor if no user history is given
	 */
	public AutoCompleter(String dict_fname) {
		user_hist = new UserHistory();
		dictionary = new DLB();
		
		//scans all english words from dictionary file to dictionary
		try (Scanner s = new Scanner(new File(dict_fname))) {
			while (s.hasNext()) {
				dictionary.add(s.nextLine());
			}
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}//AutoCompleter
	
	
	/*
	 * returns total of 5 suggestions from user history and dictionary
	 */
	public ArrayList<String> nextChar(char next) {
		ArrayList<String> suggestions = new ArrayList<String>();
		
		user_hist.searchByChar(next);
		dictionary.searchByChar(next);
		
		//adds suggestions from history and dictionary to return 5 total suggestions
		//suggestions from history have priority
		ArrayList<String> hist_sugg = user_hist.suggest();
		ArrayList<String> dict_sugg = dictionary.suggest();
		
		suggestions = hist_sugg;
		
		//fills rest of suggestions with words from dictionary
		int count = 0;
		while(suggestions.size() < SUGGESTIONS && count < dict_sugg.size()) {
			if(suggestions.contains(dict_sugg.get(count)) == false) {
				suggestions.add(dict_sugg.get(count));
			}
			
			count = count + 1;
		}
		
		return suggestions;
	}//nextChar
	
	
	/*
	 * updates user history with input
	 * input must be within the dictionary
	 */
	public void finishWord(String cur) {
		//if cur contains illegal characters or is shorter than 1, does nothing
		if(dictionary.valid_word(cur) == false || cur.length() < 1) {
			return;
		}
		
		dictionary.resetByChar();
		user_hist.resetByChar();
		user_hist.add(cur);
		return;
	}//finishWord
	
	
	public void saveUserHistory(String fname) {
		File hist = new File(fname);
		
		try {
			FileWriter fw = new FileWriter(hist);
			fw.write(user_hist.toString());
			fw.close();
		}
		catch (IOException e) {
			e.printStackTrace();
		}
		
		return;
	}//saveUserHistory
}//AutoCompleter
