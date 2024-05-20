#include "include/DeadlockDetector.h"

DeadlockDetector::DeadlockDetector()
{
	graph = DiGraph(); // Creates new directed graph
}

DiGraph DeadlockDetector::getGraph()
{
	return graph;
}

/**
 * Creates the association between two transaction, then checks right away for
 * cycles.
 *
 * @param waitingTransactionId
 * @param waitingForTransactionId
 * @return true if there is no cycle, false otherwise
 */
bool DeadlockDetector::waitFor(int waitingTransactionId, int waitingForTransactionId)
{
	if(!this->graph.hasNode(waitingTransactionId))
		this->graph.addNode(waitingTransactionId);

	if(!this->graph.hasNode(waitingForTransactionId))
		this->graph.addNode(waitingForTransactionId);

	this->graph.addEdge(waitingTransactionId, waitingForTransactionId);

	if(this->graph.hasCycles())
	{
		this->graph.removeEdge(waitingTransactionId, waitingForTransactionId);
		return false;
	}
	return true;
}

/**
 * Removes the transaction from the graph.
 *
 * @param transactionId
 */
void DeadlockDetector::endTransaction(int transactionId)
{
	this->graph.removeNode(transactionId);
}

void DeadlockDetector::addTransaction(int transactionId)
{
	this->graph.addNode(transactionId);
}