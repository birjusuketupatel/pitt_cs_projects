#include "include/DiGraph.h"
#include <algorithm>
#include <iostream>

using namespace std;
DiGraph::DiGraph()
{
	this->numV = 0;
}

bool DiGraph::hasNode(int V)
{
	return (this->adj.count(V) > 0);
}

void DiGraph::addNode(int V)
{
	if(!this->hasNode(V))
	{
		this->adj[V] = std::vector<int>();
		this->numV++;
		// std::cout << "Added node: " << V << std::endl;
	}
}
void DiGraph::removeNode(int V)
{
	if(this->hasNode(V))
	{
		for(auto& v : this->adj)
		{
			auto it = std::find(v.second.begin(), v.second.end(), V);
			if(it != v.second.end())
				v.second.erase(it, v.second.end());
		}
		this->adj.erase(V);
		this->numV--;
		// std::cout << "Removed node: " << V << std::endl;
	}
}

void DiGraph::addEdge(int VFrom, int VTo)
{
	if(!this->hasNode(VFrom))
		throw "ADD_EDGE = Source node does not exist";

	else if(!this->hasNode(VTo))
		throw "ADD_EDGE = Destination node does not exist";
	
	if(std::find(this->adj[VFrom].begin(), this->adj[VFrom].end(), VTo) != this->adj[VFrom].end())
		return;
	this->adj[VFrom].push_back(VTo);
	// std::cout << "Added edge: " << VFrom << " -> " << VTo << std::endl;
}

void DiGraph::removeEdge(int VFrom, int VTo)
{
	if(!this->hasNode(VFrom))
		throw "REMOVE_EDGE = Source node does not exist";
	else if(!this->hasNode(VTo))
		throw "REMOVE_EDGE = Destination node does not exist";

	auto it = std::find(this->adj[VFrom].begin(), this->adj[VFrom].end(), VTo);
	if(it == this->adj[VFrom].end())
		throw "Edge does not exist";
	this->adj[VFrom].erase(it);
	// std::cout << "Removed edge: " << VFrom << " -> " << VTo << std::endl;
}

bool DiGraph::DFSForCycle(int V, std::vector<bool>& visited, std::vector<int>& inStack)
{
	// Check if node is in recursive stack
	// if(inStack[V])
		// return true;

	// Check if node has already been visited
	if(visited[V] == false)
	{
		visited[V] = true;
		inStack[V] = true;
		// Traverse all adjacent nodes
		for(auto& v : this->adj[V])
		{
			// Recurse for `v`
			if((!visited[v]) && DFSForCycle(v, visited, inStack))
				return true;
			else if(inStack[v])
				return true;
		}
	}

	// Mark node as visited & present in recursive stack
	// visited[V] = true;
	// inStack[V] = true;

	
	inStack[V] = false;
	return false;

}

bool DiGraph::hasCycles()
{
	std::vector<bool> visited(this->numV, false);
	std::vector<int> inStack(this->numV, false);
	for(auto& v : this->adj)
	{
		if(!(visited[v.first]) && this->DFSForCycle(v.first, visited, inStack))
			return true;
	}
	return false;
}