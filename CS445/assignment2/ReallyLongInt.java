// CS 0445 Spring 2020
// This is a partial implementation of the ReallyLongInt class.  You need to
// complete the implementations of the remaining methods.  Also, for this class
// to work, you must complete the implementation of the LinkedListPlus class.
// See additional comments below.

package assignment2;

import assignment2.A2LList.Node;

public class ReallyLongInt extends LinkedListPlus<Integer> implements Comparable<ReallyLongInt>
{
	private ReallyLongInt()
	{
		super();
	}

	// Data is stored with the LEAST significant digit first in the list.  This is
	// done by adding all digits at the front of the list, which reverses the order
	// of the original string.  Note that because the list is doubly-linked and 
	// circular, we could have just as easily put the most significant digit first.
	// You will find that for some operations you will want to access the number
	// from least significant to most significant, while in others you will want it
	// the other way around.  A doubly-linked list makes this access fairly
	// straightforward in either direction.
	
	public ReallyLongInt(String s)
	{
		super();
		char c;
		int digit = -1;
		// Iterate through the String, getting each character and converting it into
		// an int.  Then make an Integer and add at the front of the list.  Note that
		// the add() method (from A2LList) does not need to traverse the list since
		// it is adding in position 1.  Note also the the author's linked list
		// uses index 1 for the front of the list.
		for (int i = 0; i < s.length(); i++)
		{
			c = s.charAt(i);
			if (('0' <= c) && (c <= '9'))
			{
				digit = c - '0';
				// Do not add leading 0s
				if (!(digit == 0 && this.getLength() == 0)) 
					this.add(1, new Integer(digit));
			}
			else throw new NumberFormatException("Illegal digit " + c);
		}
		// If number is all 0s, add a single 0 to represent it
		if (digit == 0 && this.getLength() == 0)
			this.add(1, new Integer(digit));
	}

	public ReallyLongInt(ReallyLongInt rightOp)
	{
		super(rightOp);
	}
	
	// Method to put digits of number into a String.  Note that toString()
	// has already been written for LinkedListPlus, but you need to
	// override it to show the numbers in the way they should appear.
	public String toString()
	{	
		StringBuilder b = new StringBuilder();
		Node curr = this.firstNode.prev;
		int i = 0;
		
		while (i < numberOfEntries)
		{
			b.append(curr.data);
			curr = curr.prev;
			i++;
		}
		
		return b.toString();
	}//toString

	// See notes in the Assignment sheet for the methods below.  Be sure to
	// handle the (many) special cases.  Some of these are demonstrated in the
	// RLITest.java program.

	// Return new ReallyLongInt which is sum of current and argument
	public ReallyLongInt add(ReallyLongInt rightOp)
	{		
		ReallyLongInt ans = new ReallyLongInt();
		ReallyLongInt x, y;
		boolean carry = false;
		
		//sets x to be the longer number and y to be the smaller number
		int result = this.compareTo(rightOp);
		if(result == 1 || result == 0) {
			x = this;
			y = rightOp;
		}
		else {
			x = rightOp;
			y = this;
		}
		
		//adds each pair of nodes and adds the sum to ans list
		Node curr_x = x.firstNode, curr_y = y.firstNode;
		for(int i = 0; i < y.numberOfEntries; i++) {
			int sum = curr_x.data + curr_y.data; //calculates sum of these digits
			
			//checks if previous digit addition resulted in a carry
			//if so, adds 1 to this digit
			if(carry == true) {
				sum = sum + 1;
			}
			
			//if sum is larger than 9, removes 10s place
			//determines if next addition will have a carried term
			if(sum > 9) {
				sum = sum - 10;
				carry = true;
			}
			else {
				carry = false;
			}
			
			//adds next digit to answer
			//goes to next digit in x and y
			ans.add(sum);
			curr_x = curr_x.next;
			curr_y = curr_y.next;
		}
		
		//adds extra terms in x that were not modified by y
		//handles carry overs that were a result of earlier adds
		for(int i = y.numberOfEntries; i < x.numberOfEntries; i++) {
			int next_digit = curr_x.data;
			
			if(carry == true) {
				next_digit = next_digit + 1;
			}
			
			if(next_digit > 9) {
				next_digit = next_digit - 10;
				carry = true;
			}
			else {
				carry = false;
			}
			
			ans.add(next_digit);
			curr_x = curr_x.next;
		}
		
		//handles carryover from last addition
		if(carry == true) {
			ans.add(1);
		}
		
		return ans;
	}//add
	
	// Return new ReallyLongInt which is difference of current and argument
	public ReallyLongInt subtract(ReallyLongInt rightOp) throws Exception
	{
		ReallyLongInt ans = new ReallyLongInt();
		boolean borrow = false;
		Node curr_x = this.firstNode, curr_y = rightOp.firstNode;
		
		//throws exception if rightOp is greater than number
		int result = this.compareTo(rightOp);
		if(result == -1) {
			Exception e = new ArithmeticException("first term less than second term");
			throw(e);
		}
		
		//subtracts each pair of nodes and adds difference to ans list
		for(int i = 0; i < rightOp.numberOfEntries; i++) {
			int diff = curr_x.data - curr_y.data;
			
			//checks if the previous subtraction required borrowing
			if(borrow == true) {
				diff = diff - 1;
			}
			
			//checks if current subtraction requires borrowing
			if(diff < 0) {
				diff = diff + 10;
				borrow = true;
			}
			else {
				borrow = false;
			}
			
			//adds difference to list, shifts to next digits
			ans.add(diff);
			curr_x = curr_x.next;
			curr_y = curr_y.next;
		}
		
		for(int i = rightOp.numberOfEntries; i < this.numberOfEntries; i++) {
			int next_digit = curr_x.data;
			
			//checks if previous subtraction required borrowing
			if(borrow == true) {
				next_digit = next_digit - 1;
			}
			
			//checks if current subtraction requires borrowing
			if(next_digit < 0) {
				next_digit = next_digit + 10;
				borrow = true;
			}
			else {
				borrow = false;
			}
			
			//adds next digit to list, shifts to next digit
			ans.add(next_digit);
			curr_x = curr_x.next;
		}
		
		ans = removeZero(ans); //removes leading zeros
		
		return ans;
	}//subtract

	// Return -1 if current ReallyLongInt is less than rOp
	// Return 0 if current ReallyLongInt is equal to rOp
	// Return 1 if current ReallyLongInt is greater than rOp
	public int compareTo(ReallyLongInt rOp)
	{
		int result = 0;
		
		if(this.numberOfEntries > rOp.numberOfEntries) {
			result = 1;
		}
		else if(this.numberOfEntries < rOp.numberOfEntries) {
			result = -1;
		}
		else {
			Node curr_node = this.firstNode.prev;
			Node comp_node = rOp.firstNode.prev;
			
			for(int i = 0; i < numberOfEntries; i++) {
				if(curr_node.data > comp_node.data) {
					result = 1;
					break;
				}
				else if(curr_node.data < comp_node.data) {
					result = -1;
					break;
				}
				else {
					curr_node = curr_node.prev;
					comp_node = comp_node.prev;
				}
			}
		}
		
		return result;
	}//compareTo

	// Is current ReallyLongInt equal to rightOp?
	public boolean equals(Object rightOp)
	{
		return (this.compareTo((ReallyLongInt) rightOp) == 0);
	}//equals

	// Mult. current ReallyLongInt by 10^num
	public void multTenToThe(int num)
	{
		for(int i = 0; i < num; i++) {
			this.add(1, 0);
		}
		
		removeZero(this);
	}//multTenToThe

	// Divide current ReallyLongInt by 10^num
	public void divTenToThe(int num)
	{
		this.leftShift(num);
		if(this.firstNode == null) {
			this.add(0);
		}
	}//divTenToThe
	
	//helper method
	//removes leading zeros from answers
	private ReallyLongInt removeZero(ReallyLongInt num) {
		int shift = 0;
		Node newLast = num.firstNode.prev;
		
		//finds the number of leading zeros
		while(newLast.data == 0 && shift < numberOfEntries - 1) {
			shift++;
			newLast = newLast.prev;
		}
		
		//deletes those zeros with a right shift
		num.rightShift(shift);
		return num;
	}//removeZero

}//ReallyLongInt