// CS 0445 Spring 2020
// This will allow exponentially distributed values to be
// generated.  To see how it is used also see handout RandTest.java

package assignment1;
import java.util.Random;
import java.lang.Math;

public class RandDist
{
	// Use a Random object to generate the underlying pseudo-random
	// values.  We will then transform these into the distribution that
	// we want (i.e. exponential)
	private Random R;
	
	public RandDist(long seed)
	{
		// The seed value is what initializes the pseudo-random sequence.
		// If two Random objects are initilialized with the same seed, and
		// if the same methods are called for both, they will produce
		// identical pseudo-random sequences.
		R = new Random(seed);
	}
	// Transform U(0,1) (i.e. the uniform distribution from 0 to 1, which
	// is basically nextDouble()) into the exponential distribution using the
	// inverse transform technique.  You may look this up if you wish but
	// for our purposes we can abstract this implementation out of our
	// view.  The argument to this method is the arrival rate, or expected
	// number of occurrences per unit time.
	public double exponential(double lambda)
	{
		double rVal = R.nextDouble();
		double mean = 1/lambda;
		double nextran = -mean * Math.log(rVal);
		return nextran;
	}
}