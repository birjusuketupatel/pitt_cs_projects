package assignment4;

public class MergeSort<T extends Comparable<? super T>> implements Sorter<T> {

     private int MIN_SIZE; // min size to recurse, use InsertionSort, for smaller sizes to complete sort
     
     public MergeSort() {

          MIN_SIZE = 3;
         
     }//MergeSort

     public void setMin(int min) {
     	
     	if(min < 0)
     		throw new IllegalArgumentException();
     	
     	MIN_SIZE = min;
     	
     }//setMin
     
     //user facing function
     //throws error if user enters illegal length
     //else runs mergesort
     public void sort(T a[], int len) {
    	 
    	 if(len > 0 && len <= a.length)
    		 mergeSort(a, 0, len-1);
    	 else if(len < 0 || len > a.length)
    		 throw new IllegalArgumentException();
    	 
     }//sort
     
     //merges two sub arrays into one
     @SuppressWarnings("unchecked")
     private void merge(T a[], int low, int med, int high) {
    	
    	 int i, j, k;
    	 int n1 = med - low + 1;
    	 int n2 = high - med;
    	 
		T[] left = (T[]) new Comparable[n1];
		T[] right = (T[]) new Comparable[n2];
    	
    	//separates input array into left and right side
    	for(i = 0; i < n1; i++)
    		left[i] = a[low+i];
    	for(j = 0; j < n2; j++)
    		right[j] = a[med+j+1];
    	
    	i = 0;
    	j = 0;
    	k = low;
    	
    	//inserts items back into input array in order
    	while(i < n1 && j < n2) {
    		
    		if(left[i].compareTo(right[j]) <= 0) {
    			a[k] = left[i];
    			i++;
    			k++;
    		}
    		else {
    			a[k] = right[j];
    			j++;
    			k++;
    		}
    		
    	}
    	
    	//if there are items left in left sub array, inserts them into main array
    	while(i < n1) {
    		a[k] = left[i];
    		i++;
    		k++;
    	}
    	
    	//if there are items left in right sub array, inserts them into main array
    	while(j < n2) {
    		a[k] = right[j];
    		j++;
    		k++;
    	}
    	
     }//merge
     
     //implementation of insertion sort
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
     
     //implementation of merge sort
     private void mergeSort(T a[], int low, int high) {
    	 
    	 int med = (low + high) / 2;
    	 
    	 if(high - low + 1 <= MIN_SIZE) {
    		 
    		 insertionSort(a, low, high); //if length of sub array is less than user entered minimum size, reverts to insertion sort
    		 merge(a, low, med, high);
    		 
    	 }
    	 else if(low < high) {
    		 
    		 mergeSort(a, low, med);
    		 mergeSort(a, med+1, high);
    		 merge(a, low, med, high);
    		 
    	 }
    	 
     }//mergeSort
     
}//MergeSort
