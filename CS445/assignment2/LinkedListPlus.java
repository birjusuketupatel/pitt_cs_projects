// CS 0445 Spring 2020
// LinkedListPlus<T> class partial implementation

// See the commented methods below.  You must complete this class by
// filling in the method bodies for the remaining methods.  Note that you
// may NOT add any new instance variables, but you may use method variables
// as needed.

package assignment2;

public class LinkedListPlus<T> extends A2LList<T>
{
	// Default constructor simply calls super()
	public LinkedListPlus()
	{
		super();
	}
	
	// Copy constructor.  This is a "deepish" copy so it will make new
	// Node objects for all of the nodes in the old list.  However, it
	// is not totally deep since it does NOT make copies of the objects
	// within the Nodes -- rather it just copies the references.  The
	// idea of this method is as follows:  If oldList has at least one
	// Node in it, create the first Node for the new list, then iterate
	// through the old list, appending a new Node in the new list for each
	// Node in the old List.  At the end, link the Nodes around to make sure
	// that the list is circular.
	public LinkedListPlus(LinkedListPlus<T> oldList)
	{
		super();
		if (oldList.getLength() > 0)
		{
			// Special case for first Node since we need to set the
			// firstNode instance variable.
			Node temp = oldList.firstNode;
			Node newNode = new Node(temp.data);
			firstNode = newNode;
			
			// Now we traverse the old list, appending a new Node with
			// the correct data to the end of the new list for each Node
			// in the old list.  Note how the loop is done and how the
			// Nodes are linked.
			Node currNode = firstNode;
			temp = temp.next;
			int count = 1;
			while (count < oldList.getLength())
			{
				newNode = new Node(temp.data);
				currNode.next = newNode;
				newNode.prev = currNode;
				temp = temp.next;
				currNode = currNode.next;
				count++;
			}
			currNode.next = firstNode;  // currNode is now at the end of the list.
			firstNode.prev = currNode;	// link to make the list circular
			numberOfEntries = oldList.numberOfEntries;
		}			
	}
	
	// Make a StringBuilder then traverse the nodes of the list, appending the
	// toString() of the data for each node to the end of the StringBuilder.
	// Finally, return the StringBuilder as a String.  Note that since the list
	// is circular, we cannot look for null.  Rather we must count the Nodes as
	// we progress down the list.
	public String toString()
	{
		StringBuilder b = new StringBuilder();
		Node curr = firstNode;
		int i = 0;
		while (i < this.getLength())
		{
			b.append(curr.data.toString());
			b.append(" ");
			curr = curr.next;
			i++;
		}
		return b.toString();
	}
	
	// Remove num items from the front of the list
	public void leftShift(int num)
	{
		//if shift is larger than length of list, empties the list
		//does nothing if shift is negative or zero
		if(num >= numberOfEntries) {
			this.clear();
		}
		else if(num <= 0) {
			
		}
		else {
			int count = 0;	
			Node newFirst = firstNode;
			Node last = firstNode.prev;
			
			//finds node that will be the first after the shift
			while(count < num) {
				newFirst = newFirst.next;
				count++;
			}
		
			//sets firstNode pointer to the newFirst node
			//connects newFirst to the end of the list
			last.next = newFirst;
			newFirst.prev = last;
			firstNode = newFirst;
			numberOfEntries = numberOfEntries - num;
		}
	}//leftShift
	
	// Remove num items from the end of the list
	public void rightShift(int num)
	{
		//if shift is larger than the length of list, empties the list
		//does nothing if shift is negative or zero
		if(num >= numberOfEntries) {
			this.clear();
		}
		else if(num <= 0) {
			
		}
		else {
			int count = 0;
			Node newLast = firstNode.prev;
			
			//finds node that will be the new end of list after shift
			while(count < num) {
				newLast = newLast.prev;
				count++;
			}
			
			//sets last node as found new last
			//connects newLast to beginning of list
			firstNode.prev = newLast;
			newLast.next = firstNode;
			numberOfEntries = numberOfEntries - num;
		}
	}//rightShift

	// Rotate to the left num locations in the list.  No Nodes
	// should be created or destroyed.
	public void leftRotate(int num)
	{
		num = Math.floorMod(num, numberOfEntries); //leftRotate(num mod length) is equivalent to leftRotate(num)
		int count = 0;
		Node curr = firstNode;
		
		while(count < num) {
			curr = curr.next;
			count++;
		}
		
		firstNode = curr; //shifting firstNode changes display of list but not values
	}//leftRotate

	// Rotate to the right num locations in the list.  No Nodes
	// should be created or destroyed.
	public void rightRotate(int num)
	{
		num = Math.floorMod(num, numberOfEntries); //rightRotate(num mod length) is equivalent to rightRotate(num)
		int count = 0;
		Node curr = firstNode;
		
		while(count < num) {
			curr = curr.prev;
			count++;
		}
		
		firstNode = curr; //shifting firstNode changes display of list but not values
	}//rightRotate
	
	// Reverse the nodes in the list.  No Nodes should be created
	// or destroyed.
	public void reverse()
	{
		int count = 0;
		Node curr = firstNode;
		Node temp = null;
		
		//switched the next and previous pointer for every node in the list
		while(count < numberOfEntries) {
			temp = curr.next;
			curr.next = curr.prev;
			curr.prev = temp;
			
			curr = curr.next;
			count++;
		}
		
		firstNode = curr.next; //sets firstNode to last element in original list
	}//reverse	
}