package cs1501_p4;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Scanner;

public class NetAnalysis implements NetAnalysis_Inter {
	public AdjList[] graph;
	private int nodeCount;
	
	/*
	 * reads text file in standard format:
	 * 
	 * number of nodes
	 * edge 1
	 * edge 2
	 * ...
	 * 
	 * adds edge at index of source
	 * adds reversed edge at index of destination
	 * assumes no duplicate edges in file
	 */
	public NetAnalysis(String file) {
		try (Scanner s = new Scanner(new File(file))) {
			nodeCount = Integer.parseInt(s.nextLine()); //first line contains number of nodes in graph
			
			//graph is an array of adjacency lists
			graph = new AdjList[nodeCount];
			for(int i = 0; i < nodeCount; i++) {
				graph[i] = new AdjList();
			}
			
			//all other lines contain edges in graph
			String line;
			String[] values;
			int source, destination, band;
			boolean is_copper;
			double len;
			while(s.hasNext()) {
				line = s.nextLine();
				values = line.split(" ");
				
				source = Integer.parseInt(values[0]);
				destination = Integer.parseInt(values[1]);
				band = Integer.parseInt(values[3]);
				
				if(values[2].equals("copper")) {
					is_copper = true;
				}
				else {
					is_copper = false;
				}
				
				len = Double.parseDouble(values[4]);
				
				//adds edge from source -> destination and one from destination -> source
				Edge nextEdge = new Edge(source, destination, is_copper, band, len);
				Edge reverseEdge = nextEdge.getReverse();
				graph[source].add(nextEdge);
				graph[destination].add(reverseEdge);
			}
		}
		catch (IOException e) {
			e.printStackTrace();
		}
		catch(IllegalStateException s) {
			throw s;
		}
	}//NetAnalysis
	
	/*
	 * uses dijkstra's algorithm to find shortest path between u and w
	 */
	public ArrayList<Integer> lowestLatencyPath(int u, int w){
		//if u or w lie out of bounds, or u and w are the same, return null
		if(!inBounds(u) || !inBounds(w) || u == w) {
			return null;
		}
		
		int[] parentOf = new int[nodeCount];
		double[] distance = new double[nodeCount];
		boolean[] checked = new boolean[nodeCount];
		
		//initializes all distances (except u) to infinity
		//initializes all nodes as unseen
		//initializes all parents to -1, after complete, u and unconnected nodes will have parent of -1
		for(int i = 0; i < nodeCount; i++) {
			if(i == u) {
				distance[i] = 0;
			}
			else {
				distance[i] = Double.MAX_VALUE;
			}
			checked[i] = false;
			parentOf[i] = -1;
		}
		
		int currNode = u, adjNode;
		Edge currEdge;
		
		//iterates through all nodes once
		for(int i = 0; i < nodeCount; i++) {
			
			//iterates through edges of currNode
			for(int j = 0; j < graph[currNode].length(); j++) {
				currEdge = graph[currNode].get(j);
				adjNode = currEdge.getDestination();
				
				//if path from currNode to adjNode found is less than adjNode's current best path
				//make currNode parent of adjNode, update distance
				if(distance[currNode] + currEdge.getLatency() < distance[adjNode]) {
					distance[adjNode] = distance[currNode] + currEdge.getLatency();
					parentOf[adjNode] = currNode;
				}
			}
			
			//marks currNode as checked
			//finds unchecked node with smallest distance, makes it currNode
			checked[currNode] = true;
			int smallNode = -1;
			double smallDist = Double.MAX_VALUE;
			for(int k = 0; k < nodeCount; k++) {
				if(distance[k] <= smallDist && !checked[k]) {
					smallNode = k;
					smallDist = distance[k];
				}
			}
			
			//if smallNode = -1, no unchecked nodes left
			//else, set set currNode to smallNode, continue
			if(smallNode == -1) {
				break;
			}
			else {
				currNode = smallNode;
			}
		}
		
		//using distance array, generates path from u to w
		ArrayList<Integer> path = new ArrayList<Integer>();
		
		int node = w;
		
		while(parentOf[node] != -1) {
			path.add(0, node);
			node = parentOf[node];
		}
		
		//when node is -1, it is either u or a node unconnected to u
		if(node != u) {
			path = null;
		}
		else {
			path.add(0, u);
		}
		
		return path;
	}//lowestLatencyPath
	
	/*
	 * traverses along input path
	 * throws exception if path is invalid
	 */
	public int bandwidthAlongPath(ArrayList<Integer> p) throws IllegalArgumentException {
		//if path is empty or less than 2 nodes long, too small
		if(p == null || p.size() < 2) {
			throw new IllegalArgumentException("path too small");
		}
		
		int currNode = p.get(0), nextNode = p.get(1), bottleneck = Integer.MAX_VALUE;
		boolean edgeFound = false;
		Edge currEdge = graph[currNode].get(0);
		
		if(!inBounds(currNode)) {
			throw new IllegalArgumentException("illegal path");
		}
		
		//for each node pair in p, find edge in graph that connects them
		for(int i = 1; i < p.size(); i++) {	
			nextNode = p.get(i);
			for(int j = 0; j < graph[currNode].length(); j++) {
				//search currNode adj list for edge that connects to nextNode
				currEdge = graph[currNode].get(j);
				if(currNode == currEdge.getSource() && nextNode == currEdge.getDestination()) {
					edgeFound = true;
					break;
				}
			}
			
			//if edge connecting currNode to nextNode not found, path is not valud
			if(!edgeFound) {
				throw new IllegalArgumentException("illegal path");
			}
			
			//update currNode and bandwidth bottleneck
			currNode = nextNode;
			edgeFound = false;
			if(currEdge.getBandwidth() < bottleneck) {
				bottleneck = currEdge.getBandwidth();
			}
		}
		
		return bottleneck;
	}//bandwidthAlongPath
	
	/*
	 * uses Union Find to check if connected, considering only copper cables
	 */
	public boolean copperOnlyConnected() {
		UnionFind u = new UnionFind(nodeCount);
		Edge nextEdge;
		
		//iterates through all edges in the graph
		for(int i = 0; i < nodeCount; i++) {
			for(int j = 0; j < graph[i].length(); j++) {
				nextEdge = graph[i].get(j);
				
				//only considers edge if it is copper
				if(nextEdge.isCopper()) {
					u.union(nextEdge);
				}
			}
		}
		
		return u.connectedGraph();
	}//copperOnlyConnected
	
	/*
	 * removes 1 vertex from graph, then checks if remaining graph has an articulation point
	 * runs in O(V*E) time
	 */
	public boolean connectedTwoVertFail() {
		//if only 2 vertices, impossible to be connected given two fail
		if(nodeCount < 3) {
			return false;
		}
		
		//checks if graph is connected, if not returns false
		UnionFind u = new UnionFind(nodeCount);
		Edge nextEdge;
		for(int i = 0; i < nodeCount; i++) {
			for(int j = 0; j < graph[i].length(); j++) {
				nextEdge = graph[i].get(j);
				u.union(nextEdge);
			}
		}
		
		if(!u.connectedGraph()) {
			return false;
		}
		
		//iterates through every vertex, removing it from the graph
		boolean fails = false;
		for(int removed = 0; removed < nodeCount; removed++) {
			AdjList[] newGraph = new AdjList[nodeCount];
			
			//adds every edge from graph to newGraph
			//except ones containing removed
			for(int i = 0; i < nodeCount; i++) {
				
				AdjList nextList = new AdjList();
				
				for(int j = 0; j < graph[i].length(); j++) {
					nextEdge = graph[i].get(j);
					int source = nextEdge.getSource();
					int destination = nextEdge.getDestination();
						
					if(source != removed && destination != removed) {
						nextList.add(nextEdge);
					}
				}
				newGraph[i] = nextList;
			}
			
			//if new graph has an articulation point
			//then fails if 2 vertices are removed
			APFind a = new APFind(newGraph);
			
			if(removed == 0) {
				fails = a.hasArticulationPoint(1);
			}
			else {
				fails = a.hasArticulationPoint(0);
			}
			
			if(fails) {
				break;
			}
		}
		
		return !fails;
	}//connectedTwoVertFail
	
	/*
	 * finds minimum spanning tree with kruskal's algorithm
	 * returns null if graph is unconnected
	 */
	public ArrayList<STE> lowestAvgLatST() {
		ArrayList<STE> tree = new ArrayList<STE>();
		
		UnionFind u = new UnionFind(nodeCount);
		PQ<Edge> edges = new PQ<Edge>();
		
		//fills queue with all edges in graph
		for(int i = 0; i < nodeCount; i++) {
			for(int j = 0; j < graph[i].length(); j++) {
				edges.add(graph[i].get(j));
			}
		}
		
		//pulls smallest edge from queue
		while(edges.size() > 0) {
			Edge nextEdge = edges.get(0);
			edges.remove(0);
			
			//if edge connects 2 unconnected vertices, add to tree and union
			//if vertices are connected, do nothing
			if(!u.connected(nextEdge.getSource(), nextEdge.getDestination())) {
				u.union(nextEdge);
				STE nextSTE = nextEdge.getSTE();
				tree.add(nextSTE);
			}
		}
		
		//after checking all edges, if graph is unconnected, return null
		if(!u.connectedGraph()) {
			tree = null;
		}
		
		return tree;
	}//lowestAvgLatST
	
	/*
	 * returns true if the node is between 0 and number of nodes
	 */
	private boolean inBounds(int node) {
		return node >= 0 && node < nodeCount;
	}//inBounds
	
	public String toString() {
		String output = "";
		for(int i = 0; i < nodeCount; i++) {
			output = output + "Node: " + i + ", Adjacency List: " + graph[i].toString() + "\n"; 
		}
		return output;
	}//toString
}//NetAnalysis
