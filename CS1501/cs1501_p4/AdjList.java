package cs1501_p4;

public class AdjList {
	private Edge[] list;
	private int size;
	private static final int INIT_LEN = 3;
	
	public AdjList() {
		list = new Edge[INIT_LEN];
		size = 0;
	}//AdjList
	
	public int length() {
		return size;
	}//length
	
	public void add(Edge newEdge) {
		if(size >= list.length) {
			resize();
		}
		
		list[size] = newEdge;
		size++;
	}//add
	
	public Edge get(int index) throws IllegalArgumentException {
		if(index >= size || index < 0) {
			throw new IllegalArgumentException("index out of bounds");
		}
		else {
			return list[index];
		}
	}//get
	
	public void modify(int index, Edge newEdge) throws IllegalArgumentException {
		if(index >= size || index < 0) {
			throw new IllegalArgumentException("index out of bounds");
		}
		else {
			list[index] = newEdge;
		}
	}//modify
	
	public Edge remove(int index) throws IllegalArgumentException {
		if(index >= size || index < 0) {
			throw new IllegalArgumentException("index out of bounds");
		}
		
		Edge target = list[index];
		for(int i = index; i < size - 1; i++) {
			list[i] = list[i+1];
		}
		
		size--;
		return target;
	}
	
	/*
	 * resizes array to double its current size
	 */
	private void resize() {
		Edge[] newList = new Edge[list.length * 2];
		
		for(int i = 0; i < list.length; i++) {
			newList[i] = list[i];
		}
		
		list = newList;
	}//add
	
	public String toString() {
		String output = "[";
		for(int i = 0; i < size; i++) {
			output = output + list[i] + ", ";
		}
		
		if(size > 0) {
			output = output.substring(0, output.length() - 2);
		}
		output = output + "]";
		return output;
	}
}//AList
