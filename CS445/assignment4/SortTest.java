package assignment4;

//CS 0445 Spring 2020
//Assignment 4
//Your QuickSort and MergeSort classes should work correctly with this program
//without any changes.  Your output should exactly match that shown in the
//file 

import java.util.*;
public class SortTest
{
	public static Random R = new Random();
	
	// Data will be an ArrayList of Sorter<T> objects.
	private ArrayList<Sorter<Integer>> sorts;
	private Integer [] A;	
	private int size;
	
	// Fill array with random data
	@SuppressWarnings("deprecation")
	public void fillArray()
	{
		for (int i = 0; i < A.length; i++)
		{
			// Values will be 0 <= X < 1 billion
			A[i] = new Integer(R.nextInt(1000000000));
		}
	}

	public void showArray()
	{
		for (int i = 0; i < A.length; i++)
		{
			System.out.print(A[i] + " ");
		}
		System.out.println("\n");
	}

	public SortTest(String sz)
	{
		size = Integer.parseInt(sz);
		
		// Put the sorting objects into the ArrayList
		sorts = new ArrayList<Sorter<Integer>>();
		sorts.add(new QuickSort<Integer>(new SimplePivot<Integer>()));
		sorts.add(new QuickSort<Integer>(new MedOfThree<Integer>()));
		sorts.add(new QuickSort<Integer>(new RandomPivot<Integer>()));
		sorts.add(new MergeSort<Integer>());
		
		A = new Integer[size];
		
		// Iterate through all of the sorts and test each one
		for (int i = 0; i < sorts.size(); i++)
		{
			R.setSeed(123456);  // This will enable all sorts to use the same data.  If
					// you have multiple runs with the same algorithm you should only
					// set this one time for each algorithm so that the different runs
					// will have different data.
			fillArray();
			System.out.print("Initial data: ");
			showArray();
			// Get the current Sorter<T> object, set the min and sort the data
			sorts.get(i).setMin(3);
			sorts.get(i).sort(A, A.length);
			System.out.print("Sorted data: ");
			showArray();
			System.out.println();
		}
	}
					
	public static void main(String [] args)
	{
		new SortTest(args[0]);
	}
}
