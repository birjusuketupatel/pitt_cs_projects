package assignment4;

public class SimplePivot <T extends Comparable <? super T>> implements Partitionable<T> {
	
	public SimplePivot() {
		
	}//SimplePivot
	
	//swaps two values in an array given two indexes
	private boolean swap(T a[], int i, int j) {
		
		T temp = a[i];
		a[i] = a[j];
		a[j] = temp;
		return true;
		
	}//swap
	
	//partitions array based on value
	//constrains to portion of array between low and high indexes
	//returns index of pivot after partition is completed
	public int partition(T a[], int low, int high) {
		
		if(high < low || low < 0 || high < 0)
			throw new IllegalArgumentException();
		
		T pivot = a[high]; //pivot is the last item in the range of the array
		int i = low;
		
		for(int j = low; j <= high; j++) {

			//moves all values smaller than pivot to the left
			//moves all values larger than pivot to right
			if(a[j].compareTo(pivot) < 0) {
				swap(a, i, j);
				i++;
			}
			
		}
		
		swap(a, i, high); //moves pivot to center
		
		return i;
		
	}//partition
	
}//SimplePivot
