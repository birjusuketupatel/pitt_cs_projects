#include "DiGraph.h"

#ifndef DEADLOCKDETECTOR_H
#	define DEADLOCKDETECTOR_H
class DeadlockDetector
{
private:
	DiGraph graph;

public:
	DeadlockDetector();

	DiGraph getGraph();

	/**
   * Creates the association between two transaction, then checks right away for
   * cycles.
   *
   * @param waitingTransactionId
   * @param waitingForTransactionId
   * @return true if there is no cycle, false otherwise
   */
	bool waitFor(int waitingTransactionId, int waitingForTransactionId);
	void addTransaction(int transactionId);
	void endTransaction(int transactionId);
};

#endif // DEADLOCKDETECTOR_H