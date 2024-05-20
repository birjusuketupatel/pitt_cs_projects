/*
 * based on implementation from https://www.geeksforgeeks.org/articulation-points-or-cut-vertices-in-a-graph/, modified for my purposes
 */
package cs1501_p4;

public class APFind {
	private int time, nodeCount;
	private AdjList[] graph;
	
	public APFind(AdjList[] newGraph) {
		graph = newGraph;
		nodeCount = graph.length;
		time = 0;
	}//APFind
	
	/*
	 * wrapper function, calls dfs implementation of articulation point finder
	 * assumes graph is completely connected
	 * runs in O(V+E) time
	 */
	public boolean hasArticulationPoint(int root) {
		time = 0;
		boolean[] checked = new boolean[nodeCount];
		int[] parentOf = new int[nodeCount];
		int[] discovered = new int[nodeCount];
		int[] low = new int[nodeCount];
		
		//all nodes are initially unvisited and have a parent of -1
		for(int i = 0; i < nodeCount; i++) {
			parentOf[i] = -1;
			checked[i] = false;
		}
		
		//checks if dfs tree rooted at root contains articulation points
		boolean containsAP = recursiveAP(root, checked, parentOf, discovered, low);
		
		return containsAP;
	}//hasArticulationPoint
	
	/*
	 * utilizes dfs to recursively find articulation points
	 */
	private boolean recursiveAP(int currNode, boolean[] checked, int[] parentOf, int[] discovered, int[] low) {
		//currNode is seen, stores time when currNode was seen
		time++;
		discovered[currNode] = time;
		low[currNode] = time;
		checked[currNode] = true;
		boolean containsAP = false;
		int children = 0, nextNode;
		
		//iterates through adjacent nodes
		AdjList edges = graph[currNode];
		for(int i = 0; i < edges.length(); i++) {
			nextNode = edges.get(i).getDestination();
			
			//if nextNode has not been seen, set currNode as its parent, run dfs nextNode
			//if nextNode has been seen, back edge exists
			if(!checked[nextNode]) {
				children++;
				parentOf[nextNode] = currNode;
				containsAP = recursiveAP(nextNode, checked, parentOf, discovered, low);
				
				//if nextNode can reach a lower node, update low of currNode
				if(low[nextNode] < low[currNode]) {
					low[currNode] = low[nextNode];
				}
				
				//if nodes below contain an articulation point, return true
				//else, currNode is an articulation point if
				//it is the root node and has two or more children
				//or low value of its child is >= its discovered value
				if(containsAP) {
					break;
				}
				else if((parentOf[currNode] == -1 && children >= 2) || (parentOf[currNode] != -1 && low[nextNode] >= discovered[currNode])) {
					containsAP = true;
					break;
				}
			}
			//ignores if nextNode is parent of currNode
			//if back edge connects to a lower discovered node, update low
			else if(parentOf[currNode] != nextNode && low[currNode] > discovered[nextNode]) {
				low[currNode] = discovered[nextNode];
			}	
		}
			
		return containsAP;
	}//recursiveAP
}//APFind
