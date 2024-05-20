package assignment4;

public interface Partitionable <T extends Comparable<? super T>> {
	
	public int partition(T[] a, int first, int last);
	
}//Partitionable