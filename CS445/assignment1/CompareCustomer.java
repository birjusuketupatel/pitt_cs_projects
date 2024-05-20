/*
 * Birju Patel
 * CS 445
 * Dr. John Ramirez
 * 02/04/2020
 */

package assignment1;
import java.util.*;

//implementation of abstract Comparator class
//compares Customer objects by time they left the simulation
public class CompareCustomer implements Comparator<Customer> {
	
	public int compare(Customer x, Customer y) {
		
		return (x.getId() - y.getId());
		
	}//compare
	
}//CompareCustomer