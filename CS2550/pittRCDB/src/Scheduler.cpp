#include "include/Scheduler.h"
#include "include/Transaction.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

bool Scheduler::schedule(int transactionID, string input)
{
	clock_t start = clock();
	if((this->transactions.count(transactionID) > 0) && this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_TERMINATED)
	{
		this->log << "Transaction " << transactionID << " has already TERMINATED (QUIT)" << std::endl << "Ignoring " << input << std::endl;
		return false;
	}
	if(DEBUG)
	{
		this->log << "[TX" + std::to_string(transactionID) + "] " + input;
	}
	if(this->operationStats.count(transactionID) == 0)
		this->operationStats[transactionID] = std::vector<operationStat>();

	Operation op = parse(input);
	switch(op.operationType)
	{
	case BEGIN:
		// Check if transaction already exists
		if((this->transactions.count(transactionID) > 0) && ((this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_RUNNING) ||
															 (this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_BLOCKED)))
		{
			throw std::runtime_error("Attempting to BEGIN an already running transaction");
		}
		else
		{
			transactionInfo txInfo;
			txInfo.transactionID = transactionID;
			this->operationStats[transactionID].push_back(operationStat{op, false, start, 0});
			/**
             * Parse execution mode of instruction
             * This is a special case where we need to reparse the input string,
             * since `parse` does not handle returning the execution mode of a BEGIN instruction
             */
			char* dup = strdup(input.c_str());
			strtok(dup, " ");								// Extract the `B`
			txInfo.executionMode = atoi(strtok(NULL, " ")); // Extract the mode
			txInfo.transactionStatus = transactionInfo::TRANSACTION_RUNNING;
			this->transactions[transactionID] = txInfo;
			execute(transactionID, op);
			free(dup);
			this->deadlockDetector.addTransaction(transactionID);
			return true;
		}

	case COMMIT:
	case ABORT: {
		if(this->transactions.count(transactionID) == 0)
			throw std::runtime_error("Attempting to execute operations without BEGIN-ing a new transaction");

		if(this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_ABORTED || this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_COMMITTED)
		{
			this->log << "Transaction " << transactionID << " has already concluded (committed or aborted)" << std::endl << "Ignoring " << input << std::endl;
			return false;
		}
		this->operationStats[transactionID].push_back(operationStat{op, false, start, 0});
		if(this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_BLOCKED)
		{
			this->log << "Transaction " << transactionID << " is currently blocked" << std::endl << "Adding " << input << " to buffer" << std::endl;
			this->transactions[transactionID].operationBuffer.push_back(op);
			return true;
		}
		execute(transactionID, op);
		releaseAllLocks(transactionID);
		this->deadlockDetector.endTransaction(transactionID);
		this->transactions[transactionID].transactionStatus = (op.operationType == COMMIT) ? transactionInfo::TRANSACTION_COMMITTED : transactionInfo::TRANSACTION_ABORTED;
		this->tryRunningBlocked();
		return true;
	}

	case INSERT:
	case UPDATE:
	case RETRIEVE_RECORD:
	case RETRIEVE_ENTIRE_TABLE:
	case RETRIEVE_COFFEE_NAME:
	case DROP_TABLE:
	case COUNT: {
		if(this->transactions.count(transactionID) == 0)
			throw std::runtime_error("Attempting to execute operations without BEGIN-ing a new transaction");

		if(this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_ABORTED || this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_COMMITTED)
		{
			this->log << "Transaction " << transactionID << " has already concluded (committed or aborted)" << std::endl << "Ignoring\t" << input << std::endl;
			return false;
		}
		this->operationStats[transactionID].push_back(operationStat{op, false, start, 0});
		if(this->transactions[transactionID].transactionStatus == transactionInfo::TRANSACTION_BLOCKED)
		{
			this->log << "Transaction " << transactionID << " is currently blocked" << std::endl << "Adding\t\"" << input << "\"to buffer" << std::endl;
			this->transactions[transactionID].operationBuffer.push_back(op);
			return true;
		}

		if(this->transactions[transactionID].executionMode == 1)
			return scheduleTransaction(transactionID, op);
		else if(this->transactions[transactionID].executionMode == 0)
			return scheduleProcess(transactionID, op);
	}
	case INVALID:
		return false;
	case QUIT:
		this->Quit(transactionID);
		return true;
	}

	return false;
}

void Scheduler::runRemaining()
{
	while(checkBlocked())
		this->tryRunningBlocked();
}

void Scheduler::Quit(int transactionID)
{
	if(this->transactions.count(transactionID) == 0)
		throw std::runtime_error("Attempting to QUIT a non-existing transaction");

	if(this->transactions[transactionID].transactionStatus != transactionInfo::TRANSACTION_ABORTED && this->transactions[transactionID].transactionStatus != transactionInfo::TRANSACTION_COMMITTED)
		this->transactions[transactionID].transaction.abort();

	this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_TERMINATED;
}
