// CS 0445 Spring 2020

// Simple class to represent the teller.  It simply stores the current
// customer and a busy flag.  You must use this class as is in Assignment 1.

package assignment1;

public class Teller
{
	private boolean busy;  // is the Teller busy or not?
	private int id;		   // what is the Teller's id?
	private Customer currentCust;   // Customer being served by the Teller

	public Teller(int val)  // Constructor -- id is passed as a parameter
	{
		busy = false;
		id = val;
		currentCust = null;
	}

	public int getID()  // return ID of the teller
	{
		return id;
	}

	public boolean isBusy()
	{
		return busy;
	}

	// Add a Customer to the teller and mark the teller as busy
	public void addCust(Customer c)
	{
		currentCust = c;
		busy = true;
	}

	// Remove and return the Customer and mark the teller as not busy
	public Customer removeCust()
	{
		Customer t = currentCust;
		currentCust = null;
		busy = false;
		return t;
	}
}