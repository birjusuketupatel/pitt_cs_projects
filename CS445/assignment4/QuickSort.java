package assignment4;

public class QuickSort <T extends Comparable<? super T>> implements Sorter<T> {

	private Partitionable<T> partAlgo;
    private int MIN_SIZE;  // min size to recurse, use InsertionSort, for smaller sizes to complete sort
    
    public QuickSort(Partitionable<T> part) {

          partAlgo = part;
          MIN_SIZE = 3;

    }//QuickSort
    
    public void setMin(int min) {
    	
    	if(min < 0)
    		throw new IllegalArgumentException();
    	
    	MIN_SIZE = min;
    	
    }//setMin
    
    //user facing function
    //throws error if user enters illegal length
    //else runs quick sort
    public void sort(T a[], int len) {
    	
    	if(len > 0 && len <= a.length)
    		quicksort(a, 0, len-1);
    	else if(len < 0 || len > a.length)
    		throw new IllegalArgumentException();
    	
    }//sort
	
    //implementation of quick sort
    private void quicksort(T a[], int low, int high) {
    	
    	if(high - low + 1 <= MIN_SIZE)
    		insertionSort(a, low, high); //if length of sub array is less than user entered minimum size, reverts to insertion sort
    	else if(low < high) {
    		
    		int div = partAlgo.partition(a, low, high);
    		quicksort(a, low, div-1);
    		quicksort(a, div+1, high);
    		
    	}
    	
    }//quicksort
    
    //implementation of insertion sort
    //sorts portion of array between index low and high
    private void insertionSort(T a[], int low, int high) {
    	
    	for(int i = low+1; i <= high; i++) {
    		
    		T curr = a[i];
    		int j = i - 1;
    		
    		while(j >= low && a[j].compareTo(curr) > 0) {
    			a[j+1] = a[j];
    			j = j - 1;
    		}
    		a[j+1] = curr;
    		
    	}
    	
    }//insertionSort
    
}//QuickSort
