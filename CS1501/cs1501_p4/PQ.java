/*
 * implements a min heap of generic objects using an array
 */

package cs1501_p4;

@SuppressWarnings("unchecked")

public class PQ<T extends Comparable<T>> {
	private Object[] heap;
	private int heap_length;
	private int arr_length;
	private final static int INIT_LEN = 10;
	
	public PQ() {
		heap = new Object[INIT_LEN];
		heap_length = 0;
		arr_length = INIT_LEN;
	}//PQ
	
	/*
	 * gets size of queue
	 */
	public int size() {
		return heap_length;
	}//size
	
	/*
	 * replaces value at target index with key
	 * modifies heap to follow heap conditions
	 */
	public void update(int target, T key) {
		//if invalid target, do nothing
		if(target < 0 || target > heap_length) {
			return;
		}
		
		heap[target] = key;
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
	 * gets item at index
	 */
	public T get(int index) {
		if(index >= 0 && index < heap_length) {
			return (T) heap[index];
		}
		else {
			return null;
		}
	}//get
	
	/*
	 * adds key to heap
	 */
	public void add(T key) {
		if(heap_length == arr_length) {
			resize();
		}
		//adds key to end of heap
		heap[heap_length] = key;
		
		//pulls key up heap as far as possible
		int child_index = heap_length;
		int parent_index = parent(heap_length);
		
		pull_up(child_index, parent_index);
				
		heap_length++;
		return;
	}//add
	
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
	 * removes key at given index from heap
	 */
	public void remove(int target) {
		//does nothing if target is out of bounds
		if(target < 0 || target >= heap_length) {
			return;
		}
		
		//swaps target with last value in heap
		//removes last value from heap
		swap(target, heap_length - 1);
		heap_length--;
		
		//pushes down curr as far as possible down the heap
		int curr = target;
		int right = right_child(curr);
		int left = left_child(curr);
		
		push_down(curr, right, left);

		return;
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
	 * if returns < 0, val(x) < val(y)
	 * if returns 0, val(x) = val(y)
	 * if returns > 0, val(x) > val(y)
	 */
	private int compare(int x, int y) {
		return ((T) heap[x]).compareTo((T) heap[y]);
	}//compare
	
	/*
	 * doubles the size of the array
	 */
	private void resize() {
		arr_length = 2 * arr_length;
		Object[] new_heap = new Object[arr_length];
		
		for(int i = 0; i < heap_length; i++) {
			new_heap[i] = (T) heap[i];
		}
		
		heap = new_heap;
		return;
	}//resize
	
	/*
	 * swaps values of 2 nodes
	 */
	private void swap(int x, int y) {
		T temp = (T) heap[x];
		heap[x] = heap[y];
		heap[y] = temp;
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
				out = out + ((T) heap[i]).toString() + ", ";
			}
			out = out.substring(0, out.length() - 2);
		}
		
		out = out + "]";
		
		return out;
	}//toString
}//PQ
