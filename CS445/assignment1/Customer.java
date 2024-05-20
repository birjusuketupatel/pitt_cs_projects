// CS 0445 Spring 2020

// Simple class to represent a customer.  Its main purpose is to store all
// of the data associated with each customer during the simulation.  Note the
// accessors and mutators for the various fields of the class.

// You must use this class as is to represent a Customer in Assignment 1

package assignment1;

public class Customer
{
	private int id;
	private int teller, queue;
	private double arrivalT, serviceT, startServiceT, waitT, endServiceT, inSystemT;

	public Customer(int newid, double arr)
	{
		id = newid;
		arrivalT = arr;
		queue = -1;
	}
	
	public int getId()
	{
		return id;
	}
	
	public void setArrivalT(double arrT)
	{
		arrivalT = arrT;
	}
	
	public double getArrivalT()
	{
		return arrivalT;
	}
	
	public void setTeller(int tell)
	{
		teller = tell;
	}
		
	public int getTeller()
	{
		return teller;
	}
	
	public void setQueue(int q)
	{
		queue = q;
	}
	
	// Note that queue is initialized to -1.  Thus, a Customer who does not have to
	// wait will still have the -1 value for getQueue() since the Customer is not
	// assigned to any queue.
	public int getQueue()
	{
		return queue;
	}
	
	public void setServiceT(double s_time)
	{
		serviceT = s_time;
	}
	
	public double getServiceT()
	{
		return serviceT;
	}
	
	public void setStartT(double start_time)
	{
		startServiceT = start_time;
	}
	
	public double getStartT()
	{
		return startServiceT;
	}
	
	public void setEndT(double end_time)
	{
		endServiceT = end_time;
	}
	
	public double getEndT()
	{
		return endServiceT;
	}
	
	public double getWaitT()
	{
		double wait = startServiceT - arrivalT;
		if (wait < 0)
			throw new ArithmeticException("Illegal Wait Time");
		return wait;
	}
	
	public double getInSystem()
	{
		double inSys = endServiceT - arrivalT;
		if (inSys < 0)
			throw new ArithmeticException("Illegal In System Time");
		return inSys;
	}
		
}