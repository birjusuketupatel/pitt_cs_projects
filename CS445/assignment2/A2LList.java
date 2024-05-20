// CS 0445 Spring 2020
// This code is (mostly) the Carrano LList<T> class with updates by me to make
// it a doubly-linked list.  You must use this class as is for Assignment 2.
// Look over the code and comments carefully -- understanding this code will
// help you to write your LinkedListPlus and ReallyLongInt classes.

// Note that this list is circular and NOT null-terminated.  This causes us to
// re-think many of the methods -- especially those that iterate through the list.

/**
   A linked implementation of the ADT list.
   @author Frank M. Carrano
*/

package assignment2;

public class A2LList<T> 
{
	protected Node firstNode;     // reference to first node
	protected int numberOfEntries; 

	public A2LList()
	{
		clear();
	} // end default constructor
	
	public final void clear() // note the final method
	{
		firstNode = null;
		numberOfEntries = 0;
	} // end clear
	
	public int getLength()
	{
		return numberOfEntries;
	}
	
	public void add(T newEntry)
	{
   		Node newNode = new Node(newEntry);
		
		// Special case for empty list -- node points to itself
		// in both directions.
   		if (isEmpty())
   		{
      		firstNode = newNode;
      		firstNode.setPrevNode(firstNode);
      		firstNode.setNextNode(firstNode);
      	}
   		else	// add to end of non-empty list.  Note that because
				// it is doubly-linked and circular, we can get to
				// the end Node easily.
   		{
      		Node lastNode = firstNode.getPrevNode();
      		lastNode.setNextNode(newNode); 
      		newNode.setPrevNode(lastNode);
      		newNode.setNextNode(firstNode);
      		firstNode.setPrevNode(newNode);
   		} // end if	
   
   		numberOfEntries++;
	}  // end add
	
	// For this add() we must traverse to the correct location before
	// adding the node.  
	public boolean add(int newPosition, T newEntry)	                                                 
	{
   		boolean isSuccessful = true;

   		if ((newPosition >= 1) && (newPosition <= numberOfEntries + 1)) 
   		{	
      		Node newNode = new Node(newEntry);

			// Special case for empty list -- node points to itself
			// in both directions.
   			if (isEmpty())
   			{
      			firstNode = newNode;
      			firstNode.setPrevNode(firstNode);
      			firstNode.setNextNode(firstNode);
      		}
      		else if (newPosition == 1)   // Special case for adding at front
      		{
      			Node lastNode = firstNode.getPrevNode();
         		newNode.setNextNode(firstNode);
         		firstNode.setPrevNode(newNode);
         		newNode.setPrevNode(lastNode);
         		lastNode.setNextNode(newNode);				
         		firstNode = newNode;
      		}
      		else	// Adding in the middle							
      		{                                
         		Node nodeBefore = getNodeAt(newPosition - 1);
         		Node nodeAfter = nodeBefore.getNextNode();
         		newNode.setNextNode(nodeAfter);
         		nodeBefore.setNextNode(newNode);
         		nodeAfter.setPrevNode(newNode);
         		newNode.setPrevNode(nodeBefore);
      		} // end if	
   
      		numberOfEntries++;
   		}
   		else
      		isSuccessful = false;
      
   		return isSuccessful;
	} // end add
	
	public boolean isEmpty()
	{
   		boolean result;
      
   		if (numberOfEntries == 0) // or getLength() == 0
   		{
      		result = true;
   		}
   		else
   		{
      		result = false;
   		} // end if
      
   		return result;
	} // end isEmpty
  
	public T remove(int givenPosition)
	{
   		T result = null;                           // return value

   		if ((givenPosition >= 1) && (givenPosition <= numberOfEntries))
   		{
			if (numberOfEntries == 1)	// case 1: remove only entry -- this is
			{							// special because of the circular list.
				result = firstNode.getData();
				firstNode = null;
			}	
      		else if (givenPosition == 1)             // case 2: remove first entry
      		{
         		result = firstNode.getData();        // save entry to be removed 
         		Node lastNode = firstNode.getPrevNode();
         		firstNode = firstNode.getNextNode(); // move firstNode down
         		lastNode.setNextNode(firstNode);	//  reset neighbors
         		firstNode.setPrevNode(lastNode);
      		}
      		else                                    // case 3: not first entry
      		{  
         		Node nodeBefore = getNodeAt(givenPosition - 1);
         		Node nodeToRemove = nodeBefore.getNextNode();
         		Node nodeAfter = nodeToRemove.getNextNode();
         		nodeBefore.setNextNode(nodeAfter);
         		nodeAfter.setPrevNode(nodeBefore);  
         		result = nodeToRemove.getData();    // save entry to be removed
      		} // end if

      		numberOfEntries--;
   		} // end if

   		return result;                            // return removed entry, or 
                                             // null if operation fails
	} // end remove

	public boolean replace(int givenPosition, T newEntry)
	{
   		boolean isSuccessful = true;

   		if ((givenPosition >= 1) && (givenPosition <= numberOfEntries))
   		{   
      		assert !isEmpty();

      		Node desiredNode = getNodeAt(givenPosition);
      		desiredNode.setData(newEntry);
   		}    
   		else
      		isSuccessful = false;
      
   		return isSuccessful;
	} // end replace

// This method is intentionally commented out and you may not use it in your
// LinkedListPlus or ReallyLongInt classes.  However, you can uncomment it
// during development / for testing as long as the comments are back in your
// final submitted version.
/*
	public T getEntry(int givenPosition)
	{
   		T result = null;  // result to return
   
   		if ((givenPosition >= 1) && (givenPosition <= numberOfEntries))
   		{
      		result = getNodeAt(givenPosition).getData();
   		} // end if
   
   		return result;
	} // end getEntry
*/

	public boolean contains(T anEntry)
	{
   		boolean found = false;
   		Node currentNode = firstNode;
   		int count = 0;
   		while (!found && (count < this.getLength()))
   		{
      		if (anEntry.equals(currentNode.getData()))
         		found = true;
      		else
      		{
         		currentNode = currentNode.getNextNode();
         		count++;
         	}
   		} // end while
   		return found;
	} // end contains

   // Returns a reference to the node at a given position.
   // Precondition: List is not empty;
   //               1 <= givenPosition <= numberOfEntries	   
	private Node getNodeAt(int givenPosition)
	{
		assert !isEmpty() && (1 <= givenPosition) && (givenPosition <= numberOfEntries);
		Node currentNode = firstNode;
		
      // traverse the list to locate the desired node
		for (int counter = 1; counter < givenPosition; counter++)
			currentNode = currentNode.getNextNode();
		
		assert currentNode != null;
      
		return currentNode;
	} // end getNodeAt

	// Note that this class is protected so you can access it directly from
	// your LinkedListPlus and ReallyLongInt classes.  However, in case you
	// prefer using accessors and mutators, those are also provided here.
   protected class Node
   {
      protected T data; 	// entry in list
      protected Node next; 	// link to next node
      protected Node prev;  // link to prev node
      
      protected Node(T dataPortion)
      {
         this(dataPortion, null, null);
      } // end constructor
      
      protected Node(T dataPortion, Node nextNode, Node prevNode)
      {
         data = dataPortion;
         next = nextNode;
         prev = prevNode;
      } // end constructor
      
      protected T getData()
      {
         return data;
      } // end getData
      
      protected void setData(T newData)
      {
         data = newData;
      } // end setData
      
      protected Node getNextNode()
      {
         return next;
      } // end getNextNode
      
      protected void setNextNode(Node nextNode)
      {
         next = nextNode;
      } // end setNextNode
      
      protected Node getPrevNode()
      {
      	return prev;
      }
      
      protected void setPrevNode(Node prevNode)
      {
      	prev = prevNode;
      }
   } // end Node
} // end A2LList