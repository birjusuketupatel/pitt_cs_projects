// CS 0445 Spring 2020
// Another subclass of SimEvent

package assignment1;

public class CompletionEvent extends SimEvent
{
	public CompletionEvent(double new_time)
	{
		super(new_time);
	}
	
	public String toString()
	{
		return "CompletionEvent: " + e_time;
	}
}