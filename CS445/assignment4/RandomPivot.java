package assignment4;

import java.util.*;

public class RandomPivot <T extends Comparable <? super T>> implements Partitionable<T> {
	
	private SimplePivot<T> pivot; //uses partition function defined in SimplePivot
	
	public RandomPivot() {
		
		pivot = new SimplePivot<T>();
		
	}//RandomPivot
	
	//swaps two values in an array given indexes
	private boolean swap(T a[], int i, int j) {
		
			T temp = a[i];
			a[i] = a[j];
			a[j] = temp;
			return true;
		
	}//swap
	
	//selects a random index in the range
	//uses this value as the pivot
	//calls partition function from SimplePivot
	public int partition(T a[], int low, int high) {
		
		if(high < low || low < 0 || high < 0)
			throw new IllegalArgumentException();
		
		Random r = new Random();
		int piv = r.nextInt((high - low) + 1) + low;
		swap(a, piv, high);
		
		return pivot.partition(a, low, high);
		
	}//partition
	
}//RandomPivot
