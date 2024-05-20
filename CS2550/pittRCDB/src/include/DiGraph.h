#ifndef DIGRAPH_H
#define DIGRAPH_H

#include <list>
#include <map>
#include <vector>

typedef struct
{
	int VFrom;
	int VTo;
} edge;

class DiGraph
{
private:
	int numV;							 // Number of vertices
	std::map<int, std::vector<int>> adj; // Adjacency list

	// helper function for findCycles
	bool DFSForCycle(int V, std::vector<bool>& visited, std::vector<int>& inStack);

public:
	DiGraph();
	bool hasNode(int V);
	void addNode(int V);
	void addEdge(int VFrom, int VTo);
	void removeEdge(int VFrom, int VTo);
	bool hasCycles();
	void removeNode(int V);
	~DiGraph() {
		this->adj.clear();
	}
};

#endif // DIGRAPH_H
