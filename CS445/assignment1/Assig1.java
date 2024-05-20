// CS 0445 Spring 2020
// Assignment 1 Main Program
// Your SimBank class must run with this program as is, with no alterations.
// For more information on the particulars of the SimBank requirements, see
// comments below and also see the Assignment 1 specifications.

package assignment1;
import java.util.*;

public class Assig1
{
	public static void main (String [] args)
	{
		Scanner inScan = new Scanner(System.in);
		System.out.println("Welcome to the Bank Simulation!");
		System.out.println("We need some information to initialize your simulation...");
		System.out.print("How many tellers will you have? ");
		int ntell = Integer.parseInt(inScan.nextLine());
		System.out.print("How many hours will your bank be open? ");
		double hrs = Double.parseDouble(inScan.nextLine());
		System.out.print("How many customers do you expect per hour? ");
		double arr_rate = Double.parseDouble(inScan.nextLine());
		System.out.print("What is the average transaction time (in minutes) of a customer? ");
		double t_min = Double.parseDouble(inScan.nextLine());
		System.out.print("How many people can wait in your bank? ");
		int maxq = Integer.parseInt(inScan.nextLine());
		System.out.print("Enter a seed: ");
		long seed = Long.parseLong(inScan.nextLine());
		
		// Create bank objects passing following parameters to constructor (in order):
		//    number of tellers (int)
		//    whether bank uses a single queue (true) or multiple queues (false)
		//    number of hours to run simulation (double)
		//    arrival rate of customers (double, in arrivals per hour)
		//    ave transaction length (double, in minutes)
		//    max customers allowed to wait (int, not counting those being served)
		//    seed for initilializing RandDist object (see RandDist.java)
		SimBank sqbank = new SimBank(ntell, true, hrs, arr_rate, t_min, maxq, seed);
		SimBank mqbank = new SimBank(ntell, false, hrs, arr_rate, t_min, maxq, seed);
		
		// The runSimulation() method will execute the simulation based on the
		// parameters above, storing the results in instance variables.  The
		// showResults() method will then display the results in a nicely formatted
		// way.  For a sample execution see a1-out2.txt.
		
		sqbank.runSimulation();
		sqbank.showResults();
		System.out.println();
		mqbank.runSimulation();
		mqbank.showResults();
		
	}
}