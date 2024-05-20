package assignment4;

import java.util.*;

public class Assig4{
	
	public static Random r;
	
	public static void main(String [] args) {
		
		//takes user input on test configuration
		System.out.println("Sort Tester:\n");	
		Scanner scan = new Scanner(System.in);
		System.out.print("Enter the size of the array to be sorted: ");
		int arrSize = scan.nextInt();
		System.out.print("Enter the number of runs per algorithm: ");
		int runs = scan.nextInt();
		System.out.print("Enter if the data should be presorted (true) or random (false): ");
		String input = scan.next();
		boolean presorted = input.equals("true");
		scan.close();
		
		//creates list containing each version of sorting algorithm
		ArrayList<Sorter<Integer>> sorts = new ArrayList<Sorter<Integer>>();
		sorts.add(new QuickSort<Integer>(new SimplePivot<Integer>()));
		sorts.add(new QuickSort<Integer>(new RandomPivot<Integer>()));
		sorts.add(new QuickSort<Integer>(new MedOfThree<Integer>()));
		sorts.add(new MergeSort<Integer>());
		
		r = new Random(); //random number
		ArrayList<ArrayList<Double>> sortTimes = new ArrayList<ArrayList<Double>>(); //list of lists, outer list corresponds to algorithm, inner list corresponds to MinRecurse value
		
		//tests each sort
		for(int i = 0; i < sorts.size(); i++) {
			
			ArrayList<Double> times = new ArrayList<Double>(); //stores times for current sort
			r.setSeed(123456); //resets seed after between algorithm
			
			//tests current sort for each MinRecurse value
			for(int j = 0; j <= 14; j++) {
				MultiTester test = new MultiTester(sorts.get(i), j*5+3, arrSize, runs, presorted, r); //runs sort in specific configuration n times based on user input
				times.add(test.findAvgTime()); //calculates average sort time for specific configuration, adds to list
			}
			
			sortTimes.add(times); //adds list of times to list of lists
			
		}
		
		//finds best and worst configuration for each sort algorithm
		ArrayList<Integer> maxIndex = new ArrayList<Integer>();
		ArrayList<Integer> minIndex = new ArrayList<Integer>();
		ArrayList<Double> maxTime = new ArrayList<Double>();
		ArrayList<Double> minTime = new ArrayList<Double>();
		
		for(int i = 0; i < sortTimes.size(); i++) {
			
			double maxVal = Collections.max(sortTimes.get(i));
			double minVal = Collections.min(sortTimes.get(i));
			
			maxTime.add(maxVal);
			minTime.add(minVal);
			
			maxIndex.add(sortTimes.get(i).indexOf(maxVal));
			minIndex.add(sortTimes.get(i).indexOf(minVal));
			
		}
		
		//gets overall best and worst sort configuration
		double worstTime = Collections.max(maxTime);
		int worstIndex = maxTime.indexOf(worstTime);
		int worstConfig = maxIndex.get(worstIndex);
		double bestTime = Collections.min(minTime);
		int bestIndex = minTime.indexOf(bestTime);
		int bestConfig = minIndex.get(bestIndex);
		
		//prints information from the run
		System.out.println("\nSystem Parameters:");
		System.out.println("	Array Size: " + arrSize);
		System.out.println("	Runs per Configuration: " + runs);
		System.out.println("	Initial Data: " + dataConfig(presorted));
		
		System.out.println("\nBest Configuration: ");
		System.out.println("	Algorithm: " + indexToString(bestIndex));
		System.out.println("	Initial Data: " + dataConfig(presorted));
		System.out.println("	Min Recurse: " + (bestConfig*5+3));
		System.out.println("	Average Time: " + bestTime + " sec");
		
		System.out.println("\nWorst Configuration: ");
		System.out.println("	Algorithm: " + indexToString(worstIndex));
		System.out.println("	Initial Data: " + dataConfig(presorted));
		System.out.println("	Min Recurse: " + (worstConfig*5+3));
		System.out.println("	Average Time: " + worstTime + " sec");
		
		System.out.println("\nResults by Algorithm");
		for(int i = 0; i < sortTimes.size(); i++) {
			
			System.out.println("Algorithm: " + indexToString(i));
			System.out.println("	Best:");
			System.out.println("		Time: " + minTime.get(i) + " sec");
			System.out.println("		Min Recurse: " + (minIndex.get(i)*5+3));
			
			System.out.println("	Worst:");
			System.out.println("		Time: " + maxTime.get(i) + " sec");
			System.out.println("		Min Recurse: " + (maxIndex.get(i)*5+3));
			
		}
		
	}//main
	
	//allows program to print report systematically
	private static String dataConfig(boolean ordered) {
		
		if(ordered)
			return "Ordered";
		else 
			return "Random";
		
	}//dataConfig
	
	//allows program to print report systematically
	private static String indexToString(int index) {
		
		switch(index) {
		
			case 0:
				return "QuickSort Simple Pivot";
			case 1:
				return "QuickSort Random Pivot";
			case 2:
				return "QuickSort Median of Three";
			case 3:
				return "MergeSort";
			default:
				return "";
		
		}
		
	}//indexToString
	
	//creates multiple instances of SortTester objects
	//runs specific configuration of sort n times, depending on user input
	//returns average sort time for given configuration
	private static class MultiTester {
		
		double avgTime;
		int minRecurse, size, numTests;
		boolean presorted;
		Sorter<Integer> sort;
		Random rand;
		
		public MultiTester(Sorter<Integer> type, int minRec, int len, int runs, boolean ordered, Random r) {
			
			rand = r;
			minRecurse = minRec;
			size = len;
			numTests = runs;
			presorted = ordered;
			avgTime = 0.0;
			sort = type;
			
		}//MultiTester
		
		//runs n tests of sort
		//calculates average time for given sort configuration
		public double findAvgTime() {
			
			for(int i = 0; i < numTests; i++) {
				SortTester test = new SortTester(sort, minRecurse, size, presorted, rand); //runs one test
				avgTime = avgTime + test.getTime(); //adds time from that test to avgTime
			}
			
			avgTime = avgTime / numTests;
			return avgTime;
			
		}//findAvgTime
		
	}//MultiTester
	
	//runs one test of sort algorithm based on user input
	//stores time of the test
	private static class SortTester {
		
		double time;
		int minRecurse, size;
		boolean presorted;
		Sorter<Integer> sort;
		Random rand;
		
		public SortTester(Sorter<Integer> type, int minRec, int len, boolean ordered, Random r) {
			
			sort = type;
			size = len;
			presorted = ordered;
			minRecurse = minRec;
			rand = r;
			
			sort.setMin(minRecurse);
			
			Integer[] a = new Integer[size];
			fillArray(a);
			
			//runs sort of particular algorithm
			//times the sort and stores the time
			long startTime = System.nanoTime();
			sort.sort(a, a.length);
			long endTime = System.nanoTime();
			
			time = (endTime - startTime) / 1000000000.0;
			
		}//SortTester
		
		//fills array with random data if presorted = false
		//fills array with ordered data if presorted = true
		private void fillArray(Integer[] a) {
			
			if(presorted) {
				for(int i = 0; i < size; i++)
					a[i] = i;
			}
			else {
				for(int i = 0; i < size; i++)
					a[i] = rand.nextInt(1000000000);
			}
			
		}//fillArray
		
		//returns sort time
		public double getTime() {
			return time;
		}//getTime
		
	}//SortTester
	
}//Assig4