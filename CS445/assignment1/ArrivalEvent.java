// CS 0445 Spring 2020
// Simply a subclass with a constructor, and toString(), but no other
// methods.

// This subclass is simply to distinguish it from other event types.

package assignment1;

public class ArrivalEvent extends SimEvent
{
	public ArrivalEvent(double new_time)
	{
		super(new_time);
	}
	
	public String toString()
	{
		return "ArrivalEvent: " + e_time;
	}
}