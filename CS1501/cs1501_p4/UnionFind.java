package cs1501_p4;

import java.util.Arrays;

public class UnionFind {
	private int[] parentOf;
	private int[] rank;
	private int nodeCount;
	
	/*
	 * implements union find algorithm
	 * optimizes by minimizing subtree rank and with path compression
	 * takes input of number of nodes in graph
	 */
	public UnionFind(int numNodes) {
		nodeCount = numNodes;
		parentOf = new int[nodeCount];
		rank = new int[nodeCount];
		
		//initializes parallel arrays
		//all nodes are initially their own parents and have rank of 1
		for(int i = 0; i < nodeCount; i++) {
			parentOf[i] = i;
			rank[i] = 1;
		}
	}//UnionFind
	
	/*
	 * returns the number of connected components in the graph
	 */
	public int numComponents() {
		int components = 0; //number of components seen
		boolean[] isParent = new boolean[nodeCount]; //if a node is a parent, is true
		
		//initializes all nodes as not parents
		for(int i = 0; i < nodeCount; i++) {
			isParent[i] = false;
		}
		
		//finds parent of all nodes in the graph
		int currParent;
		for(int i = 0; i < nodeCount; i++) {
			currParent = findParent(i);
			
			//if parent of current node has not been seen yet, add to list and increment component
			if(!isParent[currParent]) {
				isParent[currParent] = true;
				components++;
			}
		}
		
		return components;
	}//numComponents
	
	/*
	 * returns true if, given all edges added, the graph is completely connected
	 */
	public boolean connectedGraph() {
		//graph is connected if all nodes in graph have the same parent
		boolean connected = true;
		int currParent = findParent(0);
		for(int i = 0; i < nodeCount; i++) {
			if(currParent != findParent(i)) {
				connected = false;
			}
		}
		
		return connected;
	}//connectedGraph
	
	/*
	 * returns true if node1 and node2 are connected
	 * returns false if node1 and node2 are separated
	 */
	public boolean connected(int node1, int node2) {
		//if nodes have the same parents, they are connected
		int parent1 = findParent(node1);
		int parent2 = findParent(node2);
		return parent1 == parent2;
	}//connected
	
	/*
	 * updates on a new edge
	 * optimizes tree for lowest rank
	 */
	public void union(Edge newEdge) {
		int source = newEdge.getSource();
		int destination = newEdge.getDestination();

		int sourceParent = findParent(source);
		int destinationParent = findParent(destination);
		
		//if source and destination are not in the same subtree, set new parent
		//else do nothing
		if(sourceParent != destinationParent) {
			//sets new parent as node with the highest rank
			//if both have equal rank, sets sourceParent as parent of destinationParent, increments rank
			if(rank[sourceParent] > rank[destinationParent]) {
				parentOf[destinationParent] = sourceParent;
			}
			else if(rank[destinationParent] > rank[sourceParent]) {
				parentOf[sourceParent] = destinationParent;
			}
			else if(rank[destinationParent] == rank[sourceParent]) {
				parentOf[destinationParent] = sourceParent;
				rank[sourceParent]++;
			}
		}
	}//union
	
	/*
	 * finds parent of the subtree that given node is in
	 * makes node the direct child of its parent
	 */
	private int findParent(int node) {
		int currNode = node;
		while(!isParent(currNode)) {
			currNode = parentOf[currNode];
		}
		
		//currNode is now the parent of node, sets direct parent of node to currNode
		//compresses path from node to parent
		parentOf[node] = currNode;
		return currNode;
	}//findParent
	
	/*
	 * returns true if node is a parent of a subtree
	 * if node is its own parent, node is a parent of a subtree
	 */
	private boolean isParent(int node) {
		if(parentOf[node] == node) {
			return true;
		}
		else {
			return false;
		}
	}//isParent
	
	public String toString() {
		return "Subtrees: " + Arrays.toString(parentOf) + "\nRanks: " + Arrays.toString(rank);
	}//toString
}//UnionFind
