#include "DeadlockDetector.h"
#include "Page.h"
#include "Transaction.h"
#include <iostream>
#include <list>
#include <time.h>
#ifndef SCHEDULER_H
#	define SCHEDULER_H

#	define EXECUTION_MODE_PROCESS 0
#	define EXECUTION_MODE_TRANSACTION 1

typedef enum
{
	BEGIN,
	COMMIT,
	ABORT,
	QUIT,
	INSERT,
	UPDATE,
	RETRIEVE_RECORD,
	RETRIEVE_ENTIRE_TABLE,
	RETRIEVE_COFFEE_NAME,
	COUNT,
	DROP_TABLE,
	INVALID
} opType;

typedef struct Operation
{
	opType operationType;
	string tableName;
	int coffeeID;
	string coffeeName;
	int intensity;
	string countryOfOrigin;
	bool operator==(Operation lhs)
	{
		return ((lhs.operationType == operationType) && (lhs.tableName == tableName) && (lhs.coffeeID == coffeeID) && (lhs.coffeeName == coffeeName) && (lhs.intensity == intensity) &&
				(lhs.countryOfOrigin == countryOfOrigin));
	}

} Operation;

typedef struct
{
	Operation operation;
	bool complete;
	clock_t start_time;
	clock_t end_time;
} operationStat;

typedef struct
{
	int transactionID;
	int executionMode;
	Transaction transaction;
	enum
	{
		TRANSACTION_RUNNING,
		TRANSACTION_BLOCKED,
		TRANSACTION_ABORTED,
		TRANSACTION_COMMITTED,
		TRANSACTION_TERMINATED
	} transactionStatus;
	std::list<Operation> operationBuffer;
	std::list<string> tableLocksOwned;				   // List of table names, locked by this transaction
	std::map<string, std::list<int>> recordLocksOwned; // <TableName, Record>

} transactionInfo;

typedef enum
{
	READ_LOCK,
	INTENT_READ_LOCK,
	WRITE_LOCK,
	INTENT_WRITE_LOCK,
	READ_INTENT_WRITE_LOCK
} lockType;

typedef struct Lock
{
	int transactionID;
	lockType type;
} Lock;

Operation parse(string command);
std::string printOperation(Operation op);

class Scheduler
{
private:
	int numCommits = 0;
	DeadlockDetector deadlockDetector;
	ColumnBuffer colBuf;
	map<string, Table*> open_tables;
	std::map<int, transactionInfo> transactions;
	std::map<string, std::map<int, Lock>> tableLevelLocks;
	std::map<string, std::map<int, std::map<int, Lock>>> recordLevelLocks;
	std::ofstream log;
	std::ofstream sharedTableLog;
	void execute(int transactionID, Operation operation);
	bool doesTableExist(string tableName);
	bool scheduleTransaction(int transactionID, Operation operation);
	bool scheduleProcess(int transactionID, Operation operation);
	void Quit(int transactionID);
	void printToAll(std::string message);
	std::map<int, std::vector<operationStat>> operationStats;
	void addEndTime(int transactionID, Operation operation);
	void removeFromOperationStat(int transactionID);

public:
	/**
     * @brief Construct a new Scheduler object
     */
	Scheduler()
	{
		this->sharedTableLog = std::ofstream("./logs/table_manager.log");
		this->colBuf = ColumnBuffer(&(this->sharedTableLog));
		this->log = std::ofstream("./logs/scheduler.log");
		this->deadlockDetector = DeadlockDetector();
		if(!log.is_open())
		{
			std::cerr << "Failed to open `scheduler.log` for writing." << std::endl;
			exit(1);
		}
	}

	/**
     * @brief Parses input and attempts to schedule the operation
	 * @file Scheduler.cpp
     *
     * @param transactionID ID of the transaction
     * @param input Input string
     * @return true Operation was scheduled
     * @return false Operation was not scheduled (transaction aborted or blocked)
     */
	bool schedule(int transactionID, string input);

	/**
 	 * @file LockManager.cpp
	 * @brief Attempts to acquire a lock on an item
	 *
	 * Note this attempts to acquire a lock on the table, prior to acquring the lock on the item.
	 * I.e., it calls `tryAcquireTableLock` internally.
	 *
	 * @param transactionID ID of the transaction
	 * @param tableName Name of the table
	 * @param coffeeID ID of the coffee
	 * @param desiredLock Desired lock type - READ_LOCK or WRITE_LOCK
	 *
	 * @return std::list<int> List of transaction IDs that are blocking the current transaction
	 * @return std::list<int> Empty list if the lock was acquired successfully
 	 */
	std::list<int> tryAcquireItemLock(int transactionID, string tableName, int coffeeID, lockType desiredLock);

	/**
	 * @file LockManager.cpp
	 * @brief Attempts to acquire a lock on a table
	 *
	 * @param transactionID ID of the transaction
	 * @param tableName Name of the table
	 * @param desiredLock Desired lock type
	 * 	- INTENT_READ_LOCK or INTENT_WRITE_LOCK
	 * 	- READ_LOCK or WRITE_LOCK
	 * 	- READ_INTENT_WRITE_LOCK
	 * @return std::list<int> List of transaction IDs that are blocking the current transaction
	 * @return std::list<int> Empty list if the lock was acquired successfully
	 */
	std::list<int> tryAcquireTableLock(int transactionID, string tableName, lockType desiredLock);

	/**
	 * @file LockManager.cpp
	 * Since this is strict 2PL, this is the only operation needed to release locks..
	 */
	void releaseAllLocks(int transactionID);
	void prematureAbort(int transactionID);
	bool checkBlocked();
	void runRemaining();
	void tryRunningBlocked();
	void closeAllTables();
	void printStat();
};

#endif // SCHEDULER_H
