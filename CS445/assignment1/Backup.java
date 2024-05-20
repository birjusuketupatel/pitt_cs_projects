/*
 * Birju Patel
 * CS 445
 * Dr. John Ramirez
 * 02/04/2020
 */

package assignment1;
import java.util.*;

public class Backup{
	
	private PriorityQueue<SimEvent> event_list; //stores future events waiting to be processed
	private ArrayList<LinkedList <Customer>> bank_queues; //stores Customer objects waiting to be processed
	private ArrayList <Teller> tell_list;
	private ArrayList <Customer> cust_list; //stores Customer objects after they leave the simulation
	private double sim_time, avg_arrival, avg_service;
	private int tot_capacity;
	private boolean single_queue; //if true, simulation runs with single queue, if false, it runs with one queue for each teller
	private long rand_seed;
	private double avg_wait_all, avg_wait_waited, max_wait, stddev_wait, avg_serve_time, avg_sys_time; //summary statistics of run
	private int num_served, num_left, num_custs, num_waited; //summary statistics of run
	
	
	public Backup(int num_tells, boolean one_queue, double hrs, double cust_per_hour, double min_per_transaction, int max_capacity, long seed) {
		
		if(num_tells < 1 || hrs < 0 || cust_per_hour < 0 || min_per_transaction < 0 || max_capacity < 0) {
			throw new IllegalArgumentException("illegal input into constructor");
		}
		
		tell_list = new ArrayList<Teller>();
		
		for(int i = 0; i < num_tells; i++) {
			tell_list.add(new Teller(i));
		}
		
		sim_time = hrs * 60.0;
		tot_capacity = max_capacity;
		avg_arrival = cust_per_hour;
		avg_service = min_per_transaction;
		rand_seed = seed;
		single_queue = one_queue;
		event_list = new PriorityQueue<SimEvent>();
		cust_list = new ArrayList<Customer>();
		
		bank_queues = new ArrayList<LinkedList <Customer>>();
		
		if(one_queue == false) {
			for(int i = 0; i < num_tells; i ++) {
				bank_queues.add(new LinkedList<Customer>()); //adds one queue for each teller
			}
		}
		else {
			bank_queues.add(new LinkedList<Customer>()); //adds one queue total
		}
	}//SimBank
	
	
	//runs simulation based on user set parameters
	//logs simulation data in cust_list
	public void runSimulation() {
		
		RandDist R = new RandDist(rand_seed);
		double curr_time = 0.0;
		int curr_capacity = 0;
		SimEvent curr_event;
		int cust_id = 0;
		
		//generates first ArrivalEvent
		double arr_rate_min = avg_arrival / 60;
		double next_arr_time = R.exponential(arr_rate_min);
		ArrivalEvent arrival = new ArrivalEvent(next_arr_time);
		event_list.offer(arrival); //adds ArrivalEvent to future events list
		
		while(event_list.size() > 0) {
			curr_event = event_list.poll(); //removes next event from future event queue
			curr_time = curr_event.get_e_time(); //updates time to time of the next event
			
			//handles ArrivalEvent and CompletionLocEvent as they are pulled from event_list
			if(curr_event instanceof ArrivalEvent && curr_time < sim_time) {
				int shortest = this.shortQueue(); //finds shortest queue in bank
				Customer new_cust = new Customer(cust_id, curr_time); //creates new Customer
				cust_id ++;
				
				//generates next ArrivalEvent and adds it to the future event list
				next_arr_time = curr_time + R.exponential(arr_rate_min);
				ArrivalEvent next_arrival = new ArrivalEvent(next_arr_time);
				event_list.offer(next_arrival);
				
				if(curr_capacity < tot_capacity) {
					
					//sets queue customer will be added to
					//determines if the customer must wait
					new_cust.setQueue(shortest); //sets customer queue ID to ID shortest queue
					//if queue is empty and teller is not busy, resets queue ID to -1
					if(bank_queues.get(shortest).size() == 0) {
						if(!single_queue && !tell_list.get(shortest).isBusy()) {
							new_cust.setQueue(-1);
						}
						
						if(single_queue) {
							
							for(int i = 0; i < tell_list.size(); i ++) {
								if(!tell_list.get(i).isBusy()) {
									new_cust.setQueue(-1);
								}
								
							}
							
						}
					}
					
					bank_queues.get(shortest).offer(new_cust); //if bank is under capacity, places new Customer in shortest queue
					curr_capacity ++;
				}
				else {
					new_cust.setArrivalT(0.0); //all customers turned away have all time values set to 0, Queue Id = -1
					cust_list.add(new_cust); //if bank is over capacity, new Customer is added directly to cust_list
				}
				
			}
			
			if(curr_event instanceof CompletionLocEvent) {
				CompletionLocEvent completion = (CompletionLocEvent) curr_event;
				int location = completion.getLoc(); //finds location of finished transaction
				Customer finished_cust = tell_list.get(location).removeCust(); //removes Customer from Teller at that location
				
				finished_cust.setEndT(curr_time); //sets ending time for customer
				finished_cust.setServiceT(finished_cust.getEndT() - finished_cust.getStartT());
				cust_list.add(finished_cust); //sends Customer to cust_list
				curr_capacity --;
			}
			
			//pulls customers at front of queue to free tellers
			//creates CompletionLocEvent objects as customers are served
			LinkedList <Customer> curr_queue = bank_queues.get(0);
			for(int i = 0; i < tell_list.size(); i ++) {
				Teller curr_teller = tell_list.get(i);
				
				if(single_queue == false) {
					curr_queue = bank_queues.get(i); //changes value of curr_queue if there is more than one queue
				}
				
				//checks if teller is free and his queue is not empty
				if(curr_teller.isBusy() == false && curr_queue.peek() != null) {
					Customer next_cust = curr_queue.poll(); //takes next customer from queue
					
					//generates CompletionLocEvent with time and location of future transaction
					double serve_rate = 1 / avg_service;
					double serve_time = R.exponential(serve_rate);
					double finish_time = curr_time + serve_time;
					int tell_id = curr_teller.getID();
					CompletionLocEvent next_completion = new CompletionLocEvent(finish_time, tell_id);
					
					next_cust.setStartT(curr_time); //logs time next_cust was served
					next_cust.setTeller(tell_id);
					event_list.offer(next_completion); //adds CompletionLocEvent to future event list
					curr_teller.addCust(next_cust); //assigns customer to free teller
				}
			}
		}
		
		calcSumStats(); //calculates summary statistics of simulation
		
	}//runSimulation
	
	
	//prints data for all Customer objects processed by simulation
	//shows summary statistics for one run of Simulation
	public void showResults() {
		
		ArrayList<Customer> served = new ArrayList<Customer>();
		ArrayList<Customer> not_served = new ArrayList<Customer>();

		this.splitCustList(served, not_served);
		
		//sorts served and unserved customers by Customer Id
		CompareCustomer comparator = new CompareCustomer();
		Collections.sort(served, comparator);
		Collections.sort(not_served, comparator);
		
		System.out.println("\nCUSTOMERS SERVED:\n");
		System.out.println("ID   ARRIVAL   SERVICE   QUEUE   TELLER   TIME SERVE   TIME CUST   TIME SERVE   TIME SPENT");
		System.out.println("     TIME      TIME      LOC     LOC      BEGIN        WAITS       ENDS         IN SYS");
		System.out.println("------------------------------------------------------------------------------------------\n");
		for(int i = 0; i < served.size(); i ++) {
			printCustomer(served.get(i));
		}	
	
		System.out.println("\nCUSTOMERS NOT SERVED:\n");
		System.out.println("ID   ARRIVAL   SERVICE   QUEUE   TELLER   TIME SERVE   TIME CUST   TIME SERVE   TIME SPENT");
		System.out.println("     TIME      TIME      LOC     LOC      BEGIN        WAITS       ENDS         IN SYS");
		System.out.println("------------------------------------------------------------------------------------------\n");
		for(int i = 0; i < not_served.size(); i ++) {
			printCustomer(not_served.get(i));
		}
		
		System.out.println("SYSTEM PARAMETERS:\n");
		System.out.println("number of tellers: " + tell_list.size());
		System.out.println("number of queues: " + bank_queues.size());
		System.out.printf("customer arrival rate (per hour): %.2f\n", avg_arrival);
		System.out.printf("average length of service (mins): %.2f\n", avg_service);
		
		System.out.println("\nSUMMARY STATISTICS:\n");
		System.out.printf("average wait (all): %.2f\n", avg_wait_all);
		System.out.printf("average wait (customers who had to wait): %.2f\n", avg_wait_waited);
		System.out.printf("average time in system: %.2f\n", avg_sys_time);
		System.out.printf("average service time: %.2f\n", avg_serve_time);
		System.out.printf("longest wait: %.2f\n", max_wait);
		System.out.printf("total customers served: %d\n", num_served);
		System.out.printf("total customers turned away: %d\n", num_left);
		System.out.printf("total customers that had to wait: %d\n", num_waited);
		System.out.printf("total customers processed: %d\n", num_custs);
		
	}//showResults
	
	
	//allows user access to summary statistics from a run
	public double getAvgWait() {
		return avg_wait_all;
	}//getAvgWait
	
	public double getStdDevWait() {
		return stddev_wait;
	}//getStdDevWait
	
	public double getAvgWaitWaited() {
		return avg_wait_waited;
	}//getAvgWaitWaited
	
	public double getAvgSysT() {
		return avg_sys_time;
	}//getAvgSysT
	
	public double getAvgServeT() {
		return avg_serve_time;
	}
	
	public double getMaxWait() {
		return max_wait;
	}//getMaxWait
	
	public int getServed() {
		return num_served;
	}//getServed
	
	public int getNotServed() {
		return num_left;
	}//getNotServed
	
	public int getNumCusts() {
		return num_custs;
	}//getNumCusts
	
	public int getNumWaited() {
		return num_waited;
	}//getNumWaited
	
	
	//calculates and sets all summary statistics
	//num_custs = number of customers processed by simulation
	//num_served = number of customers served by teller
	//num_left = number of customers turned away because bank was over capacity
	//avg_wait_all = average wait time for all customers
	//avg_wait_waited = average wait time for customers that had to wait in a line
	//avg_sys_time = average time each customer spent in the bank
	//avg_serve_time = average time customer spent with teller
	//num_waited = number of customers that had to wait in a line
	//stddev_wait = standard deviation in wait times for all customers
	private void calcSumStats() {
		
		num_custs = cust_list.size();
		
		ArrayList<Customer> served = new ArrayList<Customer>();
		ArrayList<Customer> not_served = new ArrayList<Customer>();
		this.splitCustList(served, not_served);
		num_served = served.size();
		num_left = not_served.size();
		
		max_wait = 0.0;
		
		for(int i = 0; i < served.size(); i ++) {
			Customer check_cust = served.get(i);
			if(check_cust.getWaitT() > max_wait) {
				max_wait = check_cust.getWaitT();
			}
		}
		
		avg_wait_all = 0.0;
		avg_wait_waited = 0.0;
		avg_sys_time = 0.0;
		avg_serve_time = 0.0;
		num_waited = 0;
		
		for(int i = 0; i < served.size(); i ++) {
			Customer curr_cust = served.get(i);
			if(curr_cust.getQueue() != -1) {
				avg_wait_waited = avg_wait_waited + curr_cust.getWaitT();
				num_waited ++;
			}
			avg_wait_all = avg_wait_all + curr_cust.getWaitT();
			avg_sys_time = avg_sys_time + curr_cust.getInSystem();
			avg_serve_time = avg_serve_time + curr_cust.getServiceT();
		}
		
		avg_wait_all = avg_wait_all / num_served;
		avg_wait_waited = avg_wait_waited / num_waited;
		avg_sys_time = avg_sys_time / num_served;
		avg_serve_time = avg_serve_time / num_served;
		
		stddev_wait = 0;
		
		for(int i = 0; i < served.size(); i ++) {
			Customer temp_cust = served.get(i);
			stddev_wait = stddev_wait + ((temp_cust.getWaitT() - avg_wait_all) * (temp_cust.getWaitT() - avg_wait_all));
		}
		
		stddev_wait = stddev_wait / num_served;
		stddev_wait = Math.sqrt(stddev_wait);
	
	}//calcSumStats
	
	
	//prints all data for one Customer
	//formatted to fit table
	private void printCustomer(Customer cust) {
		
		System.out.printf("%-4d %-10.2f %-8.2f %-7d %-8d %-12.2f %-11.2f %-12.2f %-10.2f\n", 
				cust.getId(), cust.getArrivalT(), cust.getServiceT(), cust.getQueue(), cust.getTeller(), 
				cust.getStartT(), cust.getWaitT(), cust.getEndT(), cust.getInSystem());
		
	}//custOutput
	
	
	//finds the queue in the bank with the shortest length
	//returns index of that queue
	private int shortQueue() {
		
		LinkedList <Customer> test_queue = bank_queues.get(0);
		int short_index = 0;
		int shortest_length = test_queue.size();
		
		for(int i = 1; i < bank_queues.size(); i++) {
			test_queue = bank_queues.get(i);
			if(test_queue.size() < shortest_length) {
				shortest_length = test_queue.size();
				short_index = i;
			}
		}
		
		return short_index;
		
	}//shortQueue
	
	
	//adds customers from cust_list into two ArrayLists
	//depending on if the customer was served
	private void splitCustList(ArrayList <Customer> custs_served, ArrayList <Customer> custs_not_served) {
			
		for(int i = 0; i < cust_list.size(); i ++) {
			Customer curr_cust = cust_list.get(i);
			if (this.wasServed(curr_cust)) {
				custs_served.add(curr_cust);
			}
			else {
				custs_not_served.add(curr_cust);
			}
		}
			
	}//splitCustList
	
	
	//returns true if Customer was served by teller
	//returns false if Customer was turned away due to over capacity
	//the only way for all Customer time stamps to equal 0 is if the customer was never served
	private boolean wasServed(Customer cust) {	
		return !(cust.getArrivalT() == 0 && cust.getServiceT() == 0 && cust.getEndT() == 0);
	}//wasServed
	
}//SimBank