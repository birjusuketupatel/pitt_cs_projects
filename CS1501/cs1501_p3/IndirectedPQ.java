/*
 * implements indirected priority queue for general case
 * T = value to be stored in queue
 * E = key used to find values
 * always returns deep copy of value in heap
 * adds deep copy of value to heap
 */

package cs1501_p3;

@SuppressWarnings("unchecked")

public class IndirectedPQ<Key,Value extends KeyVal_Inter<Key,Value>> {
	private Object[] heap;
	private LinearProbingHashST<Key,Integer> table;
	private int heap_length;
	private int arr_length;
	private final static int INIT_LEN = 10;
	
	public IndirectedPQ() {
		heap = new Object[INIT_LEN];
		heap_length = 0;
		arr_length = INIT_LEN;
		table = new LinearProbingHashST<Key,Integer>();
	}//PQ
	
	/*
	 * returns true if heap contains key
	 * returns false if heap does not
	 */
	public boolean contains(Key key) {
		return table.contains(key);
	}//contains
	
	/*
	 * returns true if heap contains value
	 * returns false if heap does not
	 */
	public boolean contains(Value target) {
		Key key = target.getKey();
		return table.contains(key);
	}//contains
	
	/*
	 * gets value associated with key in heap
	 * returns null if key is not in heap
	 */
	public Value get(Key key) {
		Integer target = table.get(key);
		
		if(target == null || target < 0 || target >= heap_length) {
			return null;
		}
		
		//returns copy of value at target in heap
		Value copy = ((Value) heap[target]).deepCopy();
		return copy;
	}//get
	
	/*
	 * returns smallest value in heap
	 */
	public Value get_min() {
		//if no items in heap, return null
		//else return first item in list
		if(heap_length == 0) {
			return null;
		}
		else {
			Value copy = ((Value) heap[0]).deepCopy();
			return copy;
		}
	}//get_min
	
	/*
	 * removes smallest value in heap
	 */
	public Value remove_min() {
		//if no items in heap, return null
		//else remove first item in heap and table
		if(heap_length == 0) {
			return null;
		}
		else {
			Value min_val = remove(0);
			table.delete(min_val.getKey());
			Value copy = min_val.deepCopy();
			return copy;
		}
	}//remove_min
	
	/*
	 * replaces value in heap with given key with new value
	 */
	public void update(Key target, Value new_val) {
		Integer target_index = table.get(target);
		
		if(target_index == null) {
			return;
		}
		
		//if key of updated value is different than target, do nothing
		if(!new_val.getKey().equals(target)) {
			System.out.println(target);
			return;
		}
		
		//makes copy of new_val to put in heap
		Value copy = new_val.deepCopy();
		
		update(target_index, copy);
		return;
	}//update
	
	/*
	 * replaces value at target index with new value
	 * modifies heap to follow heap conditions
	 */
	private void update(int target, Value new_val) {
		//if invalid target, do nothing
		if(target < 0 || target > heap_length) {
			return;
		}
		
		Value copy = new_val.deepCopy();
		heap[target] = copy;
		int parent = parent(target);
		int right = right_child(target);
		int left = left_child(target);
		
		//if parent > target, pull key up the heap
		if(parent >= 0 && compare(target, parent) < 0) {
			pull_up(target, parent);
		}
		//if target < left or target < right, push key down heap
		if(left < heap_length && ((right < heap_length && compare(right, target) < 0) || compare(left, target) < 0)) {
			push_down(target, right, left);
		}
	}//update
	
	/*
	 * removes target value from heap given its key
	 * returns null if target is not in heap
	 * returns target if it is successfully removed
	 */
	public Value remove(Key key) {
		Integer target_index = table.get(key);
		
		if(target_index == null) {
			return null;
		}
		
		Value target_value = remove(target_index);
		table.delete(key);
		
		Value copy = target_value.deepCopy();
		return copy;
	}//remove
	
	/*
	 * removes target at given index from heap
	 * returns null if target is not in heap
	 * returns target if it is successfully removed
	 */
	private Value remove(int target) {
		if(target < 0 || target >= heap_length) {
			return null;
		}
		
		//swaps target with last value in heap
		//removes last value from heap
		Value target_value = (Value) heap[target];
		swap(target, heap_length - 1);
		heap_length--;
		
		//pushes down curr as far as possible down the heap
		int curr = target;
		int right = right_child(curr);
		int left = left_child(curr);
		
		push_down(curr, right, left);

		return target_value;
	}//remove
	
	/*
	 * pushes value in curr as far down as possible in the heap
	 */
	private void push_down(int curr, int right, int left) {
		int smallest = curr;
		
		//while left child is in bounds and one of curr's children is smaller than curr, continue
		while(left < heap_length && ((right < heap_length && compare(right, curr) < 0) || compare(left, curr) < 0)) {
			//swaps curr with its smallest child that is in bounds
			if(compare(left, curr) < 0) {
				smallest = left;
			}
			
			if(right < heap_length && compare(right, smallest) < 0) {
				smallest = right;
			}
			
			swap(curr, smallest);
			curr = smallest;
			right = right_child(curr);
			left = left_child(curr);
		}
		
		return;
	}//push_down
	
	/*
	 * adds deep copy of target to heap
	 */
	public void add(Value target) {
		Value copy = target.deepCopy();
		
		//if target has the same key as another object in heap, do not add
		Key key = copy.getKey();
		if(table.contains(key)) {
			return;
		}
		
		//resize heap if required
		if(heap_length == arr_length) {
			resize();
		}
		
		//adds key to end of heap, updates table to reflect addition
		heap[heap_length] = copy;
		table.put(key, heap_length);
		
		//pulls key up heap as far as possible
		int child_index = heap_length;
		int parent_index = parent(heap_length);
		
		pull_up(child_index, parent_index);
				
		heap_length++;
		return;
	}//add
	
	/*
	 * checks if the heap is valid
	 */
	public boolean is_heap() {
		//if all non-leaf nodes are less than their children, heap is valid
		//node is non-leaf if its child is in bounds
		int curr = 0;
		while(left_child(curr) < heap_length) {
			//if left or right child of curr is less than curr, heap is invalid
			if(compare(left_child(curr), curr) < 0) {
				return false;
			}
			if(right_child(curr) < heap_length && compare(right_child(curr), curr) < 0) {
				return false;
			}
			curr++;
		}
		
		return true;
	}//is_heap
	
	/*
	 * pulls value at child_index as high as possible in the heap
	 */
	private void pull_up(int child_index, int parent_index) {
		//while value(parent) > value(child) and not at root, swap
		while(parent_index >= 0 && compare(child_index, parent_index) < 0) {
			swap(parent_index, child_index);
			child_index = parent_index;
			parent_index = parent(child_index);
		}
	}//pull_up
	
	/*
	 * if returns < 0, val(x) < val(y)
	 * if returns 0, val(x) = val(y)
	 * if returns > 0, val(x) > val(y)
	 */
	private int compare(int x, int y) {
		return ((Value) heap[x]).compareTo((Value) heap[y]);
	}//compare
	
	/*
	 * doubles the size of the array
	 */
	private void resize() {
		arr_length = 2 * arr_length;
		Object[] new_heap = new Object[arr_length];
		
		for(int i = 0; i < heap_length; i++) {
			new_heap[i] = (Value) heap[i];
		}
		
		heap = new_heap;
		return;
	}//resize
	
	/*
	 * swaps values of 2 nodes
	 * update table at each swap call
	 */
	private void swap(int x, int y) {		
		Value x_val = (Value) heap[x];
		Value y_val = (Value) heap[y];
		
		Key x_key = x_val.getKey();
		Key y_key = y_val.getKey();
		
		//in hash map, swap indices associated with both keys
		table.put(x_key, y);
		table.put(y_key, x);
		
		//swaps values in the heap
		heap[x] = y_val;
		heap[y] = x_val;
		
		return;
	}//swap
	
	/*
	 * returns index of parent of node
	 * returns -1 if node is root
	 */
	private int parent(int i) {
		return (int) Math.floor((i - 1) / 2);
	}//parent
	
	/*
	 * returns index of left child
	 * index may be out of bounds
	 */
	private int left_child(int i) {
		return 2 * i + 1;
	}//left_child
	
	/*
	 * returns index of right child
	 * index may be out of bounds
	 */
	private int right_child(int i) {
		return 2 * i + 2;
	}//right_child
	
	/*
	 * creates string representation of heap
	 */
	public String toString() {
		String out = "[";
		
		if(heap_length >= 1) {
			for(int i = 0; i < heap_length; i++) {
				out = out + ((Value) heap[i]).toString() + ", ";
			}
			out = out.substring(0, out.length() - 2);
		}
		
		out = out + "]";
		
		return out;
	}//toString
}//IndirectedPQ
