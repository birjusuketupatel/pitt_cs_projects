package assignment4;

public class MedOfThree <T extends Comparable <? super T>> implements Partitionable<T> {

	private SimplePivot<T> pivot; //uses partition function defined in SimplePivot
	
	public MedOfThree() {
		
		pivot = new SimplePivot<T>();
		
	}//MedOfThree
	
	//swaps two values given two indexes in an array
	private boolean swap(T a[], int i, int j) {
		
		T temp = a[i];
		a[i] = a[j];
		a[j] = temp;
		return true;
		
	}//swap
	
	//finds the index of the median value between three values in an array
	private int findMid(T a[], int low, int med, int high) {
		
		if((a[low].compareTo(a[med]) >= 0 && a[low].compareTo(a[high]) <= 0) || (a[low].compareTo(a[med]) <= 0 && a[low].compareTo(a[high]) >= 0))
			return low;
		else if((a[med].compareTo(a[low]) >= 0 && a[med].compareTo(a[high]) <= 0) || (a[med].compareTo(a[low]) <= 0 && a[med].compareTo(a[high]) >= 0))
			return med;
		else
			return high;
		
	}//findMid
	
	//selects 3 indexes inside of array range
	//finds the median value of the three indexes
	//uses that value as a pivot
	//calls partition function from SimplePivot
	public int partition(T a[], int low, int high) {
		
		if(high < low || low < 0 || high < 0)
			throw new IllegalArgumentException();
		
		int med = (high + low) / 2;
		int piv = findMid(a, low, med, high);
		
		swap(a, piv, high);
		return pivot.partition(a, low, high);
		
	}//partition
	
}//MedOfThree
