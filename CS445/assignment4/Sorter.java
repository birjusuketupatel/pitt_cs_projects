package assignment4;

public interface Sorter <T extends Comparable<? super T>> {

	public void sort(T[] a, int size);
    public void setMin(int minSize);
	
}//Sorter