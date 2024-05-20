// CS 0445 Spring 2020
// Simple demonstration of using a class to generate
// exponentially distributed arrivals.  Also demonstrates
// seeding a generator to reproduce a consistent set of
// pseudo-random values.

// You will use this distribution to generate arrivals
// for Assignment 1.

package assignment1;
import java.util.*;

public class RandTest
{
	public static void main(String [] args)
	{
		Scanner inScan = new Scanner(System.in);
		System.out.print("Enter rate in arrivals per hour: ");
		double lambda = inScan.nextDouble();
		// The above value for lambda is the arrival rate, which will
		// be passed as an argument to the exponential() function.  In this
		// case we are asking for arrivals per hour, however the unit of time
		// can be arbitrary.
		
		System.out.print("Enter number of trials: ");
		int num = inScan.nextInt();
		
		// Run this program using using the same value for both seeds and
		// different values for the seeds.  You should see that if the same
		// seed is used the identical results will occur for both calls. On
		// the other hand, if different seeds are used the results should be
		// similar but not identical.
		System.out.print("Enter a seed: ");
		long seed1 = inScan.nextLong();
		System.out.print("Enter another seed: ");
		long seed2 = inScan.nextLong();
	
		testDist(num, seed1, lambda);
		testDist(num, seed2, lambda);
	}
	
	public static void testDist(int reps, long seed, double lambda)
	{
		// The average time between arrivals will be 1/lambda.  The relationship
		// between the arrival rate (lamba) and the average time between arrivals
		// (1/lambda) is important and intuitive.  For example, if an arrival rate
		// lambda = 15 arrivals / hr, then the average time between arrivals is
		// 1 / (15 arrivals / hr) = (1/15) hr / arrival.  (1/15) hour is 4 minutes.
		// Intuitively, if we get 15 arrivals in an hour then on average there will
		// be 4 minutes between arrivals.  However, with an exponential distribution
		// there is a lot of variance in the actual arrival gaps.  Some differences
		// will be much more than the average and some will be much less.
		double goalAve = 1/lambda;  // Average time between in hours
		double goalMin = goalAve * 60;  // Average time between in minutes
		double total = 0, calcAve, calcMin;
		RandDist R = new RandDist(seed);
		for (int i = 0; i < reps; i++)
		{
			// Get next time until next arrival using exponential dist.
			// See RandDist.java
			double val = R.exponential(lambda);
			double minval = val*60;
			// Show actual values if reps <= 100
			if (reps <= 100)
			{
				System.out.printf("Next arrival after:  %10.7f Hrs  or  %10.7f Min\n", val, minval);
				//if ((i+1) % 5 == 0)
				//	System.out.println();
			}
			// Add to total so that we can empirically calculate the ave.
			total += val;
		}
		System.out.println();
		// Show the formulaic results for average time between arrivals and
		// compare with empirical results.  The answers should be more closely
		// aligned as the number of arrivals increases (to a large value).
		calcAve = total / reps;
		calcMin = calcAve * 60;
		System.out.println("Seed: " + seed);
		System.out.println("Goal Ave: " + goalAve + " hr/arrival");
		System.out.println("Goal Ave: " + goalMin + " min/arrival");
		System.out.println("Goal Lambda: " + lambda + " arrivals/hr");
		System.out.println("N: " + reps + " arrivals");
		System.out.println("Calc Ave: " + calcAve + " hr/arrival");
		System.out.println("Calc Ave: " + calcMin + " min/arrival");
		System.out.println("Calc Lambda: " + (1/calcAve) + " arrivals/hr");
		System.out.println();
	}
}