// CS 0445 Spring 2020
// This class sets up the structure and functionality of events.

// The primary attribute of an event is its time stamp.  This will
// be used to compare / order events in a system.

// By extending this class for different types of events, we enable 
// the different types of events to be compatible but still 
// distinguishable in an object-oriented way.  Since the time values are
// double, we do not directly compare for equality -- rather we test to see
// if the difference between the two is very small.

// Note that this class is abstract -- we will not create any SimEvent
// objects in our program. Rather, we will create objects of its subclasses.

package assignment1;

public abstract class SimEvent implements Comparable<SimEvent>
{
	protected double e_time;  // Time when event was generated

	public SimEvent(double new_time)
	{
		e_time = new_time;
	}
	
	public double get_e_time()
	{
		return e_time;
	}
	
	// This method will allow events to be ordered (ex: via a
	// PriorityQueue)
	public int compareTo(SimEvent right)
	{
		double diff = e_time - right.e_time;
		if (Math.abs(diff) < 0.00001)
			return 0;
		else if (diff < 0)
			return -1;
		else
			return 1;
	}
}