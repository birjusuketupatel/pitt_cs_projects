package cs1501_p4;

public class Edge extends STE implements Comparable<Edge> {
	
	private boolean copper;
	private int bandwidth;
	private double length, latency;
	private static final double COPPER = 230000000.0, OPTICAL = 200000000.0;
	
	/*
	 * directed edge, data flows from vertex u (source) to vertex w (destination)
	 */
	public Edge(int source, int destination, boolean is_copper, int band, double len) {
		super(source, destination);
		
		copper = is_copper;
		bandwidth = band;
		length = len;
		
		if(copper) {
			latency = length / COPPER;
		}
		else {
			latency = length / OPTICAL;
		}
	}//Edge
	
	public STE getSTE() {
		STE edge = new STE(u, w);
		return edge;
	}//getSTE
	
	public int getSource() {
		return u;
	}//getSource
	
	public int getDestination() {
		return w;
	}//getDestination
	
	public int getBandwidth() {
		return bandwidth;
	}//getBandwidth
	
	public double getLatency() {
		return latency;
	}//getLatency
	
	public boolean isCopper() {
		return copper;
	}//isCopper
	
	/*
	 * returns an edge that goes from destination to source
	 */
	public Edge getReverse() {
		Edge reverseEdge = new Edge(this.w, this.u, this.copper, this.bandwidth, this.length);
		return reverseEdge;
	}//getReverse
	
	/*
	 * returns -1 if this edge has lower latency than other
	 * returns 0 if this edge has same latency as other
	 * returns 1 if this edge has higher latency as other
	 */
	public int compareTo(Edge other) {
		if(this.latency < other.latency) {
			return -1;
		}
		else if(this.latency == other.latency) {
			return 0;
		}
		else {
			return 1;
		}
	}//compareTo
	
	/*
	 * returns info in the form "source destination type bandwidth length"
	 */
	public String toString() {
		String type;
		
		if(copper) {
			type = "copper";
		}
		else {
			type = "optical";
		}
		
		return u + " " + w + " " + type + " " + bandwidth + " " + length;
	}//toString
	
}//Edge
