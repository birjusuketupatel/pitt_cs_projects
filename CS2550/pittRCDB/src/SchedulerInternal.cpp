#include "include/Scheduler.h"
#include <algorithm>
#include <string>

void Scheduler::printToAll(std::string message)
{
	this->log << message << std::endl;
	this->sharedTableLog << message << std::endl;
	std::cout << message << std::endl;
}

bool Scheduler::scheduleProcess(int transactionID, Operation operation)
{
	switch(operation.operationType)
	{
	//The following operations require acquiring write locks on the tables
	case DROP_TABLE: {
		std::list<int> dependencies = this->tryAcquireTableLock(transactionID, operation.tableName, WRITE_LOCK);
		if(dependencies.empty())
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{ // Check for deadlock
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID))
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock. Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case INSERT: {
		std::list<int> dependencies;
		if(this->doesTableExist(operation.tableName))
			dependencies = this->tryAcquireItemLock(transactionID, operation.tableName, operation.coffeeID, WRITE_LOCK);
		else
			dependencies = this->tryAcquireTableLock(transactionID, operation.tableName, WRITE_LOCK);

		if(dependencies.empty()) // Block
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{ // Check for deadlock
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID))
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case UPDATE: {
		std::list<int> dependencies = this->tryAcquireItemLock(transactionID, operation.tableName, operation.coffeeID, WRITE_LOCK);
		if(dependencies.empty())
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		this->log << "[TX" << std::to_string(transactionID) << "]";
		for(int waitForTxID : dependencies)
		{
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID)) // Deadlock detected
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}

	// The following operations require acquiring read locks on the tables
	// But for processes, we omit the check for read
	case RETRIEVE_ENTIRE_TABLE:
	case RETRIEVE_COFFEE_NAME:
	case COUNT:
	case RETRIEVE_RECORD: {
		execute(transactionID, operation);
		return true;
	}
	case INVALID:
		throw std::runtime_error(printOperation(operation) + "is an invalid operation.");
		return false;
	case QUIT:
	case BEGIN:
	case COMMIT:
	case ABORT:
		return false;
		break;
	}

	return false;
}

bool Scheduler::scheduleTransaction(int transactionID, Operation operation)
{
	switch(operation.operationType)
	{
	//The following operations require acquiring write locks on the tables
	case DROP_TABLE: {
		std::list<int> dependencies = this->tryAcquireTableLock(transactionID, operation.tableName, WRITE_LOCK);
		if(dependencies.empty()) // Block
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{ // Check for deadlock
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID))
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case INSERT: {
		std::list<int> dependencies;
		if(this->doesTableExist(operation.tableName))
			dependencies = this->tryAcquireItemLock(transactionID, operation.tableName, operation.coffeeID, WRITE_LOCK);
		else
			dependencies = this->tryAcquireTableLock(transactionID, operation.tableName, WRITE_LOCK);

		if(dependencies.empty()) // Block
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{ // Check for deadlock
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID))
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case UPDATE: {
		std::list<int> dependencies = this->tryAcquireItemLock(transactionID, operation.tableName, operation.coffeeID, WRITE_LOCK);
		if(dependencies.empty())
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		this->log << "[TX" << std::to_string(transactionID) << "]";
		for(int waitForTxID : dependencies)
		{
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID)) // Deadlock detected
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}

	// The following operations require acquiring read locks on the tables
	case RETRIEVE_RECORD: {
		std::list<int> dependencies = this->tryAcquireItemLock(transactionID, operation.tableName, operation.coffeeID, READ_LOCK);
		if(dependencies.empty())
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{ // Check for deadlocks
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID))
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case RETRIEVE_ENTIRE_TABLE:
	case RETRIEVE_COFFEE_NAME:
	case COUNT: {
		std::list<int> dependencies = this->tryAcquireTableLock(transactionID, operation.tableName, READ_LOCK);

		if(dependencies.empty())
		{
			execute(transactionID, operation);
			return true;
		}
		// Else
		for(int waitForTxID : dependencies)
		{
			if(!this->deadlockDetector.waitFor(transactionID, waitForTxID)) // Deadlock detected
			{
				this->prematureAbort(transactionID);
				return false;
			}
		}
		// Block transaction
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << "Could not acquire needed lock" << std::endl << "Blocking " << transactionID << " and adding " << printOperation(operation) << " to buffer" << std::endl;
		this->transactions[transactionID].operationBuffer.push_back(operation);
		this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
		return true;
	}
	case INVALID:
		throw std::runtime_error(printOperation(operation) + "is an invalid operation.");
		return false;
	case QUIT:
		//TODO
		return true;
	case BEGIN:
	case COMMIT:
	case ABORT:
		return false;
		break;
	}

	return false;
}

void Scheduler::tryRunningBlocked()
{
	this->log << "Attempting to run blocked transactions" << std::endl;
	for(auto& v : this->transactions)
	{
		// Skip over non-blocked transactions
		if(v.second.transactionStatus != transactionInfo::TRANSACTION_BLOCKED)
			continue;

		bool canRun = true;
		// Mark transaction as running
		this->transactions[v.second.transactionID].transactionStatus = transactionInfo::TRANSACTION_RUNNING;
		while(canRun)
		{
			if(v.second.operationBuffer.empty()) // Exhausted operations
			{
				canRun = false;
				break;
			}
			Operation blockedOp = v.second.operationBuffer.front();
			switch(blockedOp.operationType)
			{
			case BEGIN:
				throw std::runtime_error("Attempting to begin transaction when the previous one hasn't completed.");
				break;
			case COMMIT:
				v.second.operationBuffer.pop_front();
				execute(v.second.transactionID, blockedOp);
				v.second.transactionStatus = transactionInfo::TRANSACTION_COMMITTED;
				canRun = false;
				break;
			case ABORT:
				v.second.operationBuffer.pop_front();
				execute(v.second.transactionID, blockedOp);
				v.second.transactionStatus = transactionInfo::TRANSACTION_ABORTED;
				canRun = false;
				break;
			case DROP_TABLE: {
				std::list<int> dependencies = this->tryAcquireTableLock(v.second.transactionID, blockedOp.tableName, WRITE_LOCK);
				if(dependencies.empty())
				{
					execute(v.second.transactionID, blockedOp);
					v.second.operationBuffer.pop_front();
					break;
				}
				// Else check for deadlock
				for(int waitForTxID : dependencies)
				{
					if(!this->deadlockDetector.waitFor(v.second.transactionID, waitForTxID))
					{
						this->prematureAbort(v.second.transactionID);
						canRun = false;
						break;
					}
				}
				// Block transaction
				v.second.transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
				break;
			}

			case INSERT: {
				std::list<int> dependencies;
				if(this->doesTableExist(blockedOp.tableName))
					dependencies = this->tryAcquireItemLock(v.second.transactionID, blockedOp.tableName, blockedOp.coffeeID, WRITE_LOCK);
				else
					dependencies = this->tryAcquireTableLock(v.second.transactionID, blockedOp.tableName, WRITE_LOCK);

				if(dependencies.empty())
				{
					execute(v.second.transactionID, blockedOp);
					v.second.operationBuffer.pop_front();
					break;
				}
				// Else
				for(int waitForTxID : dependencies)
				{ // Check for deadlock
					if(!this->deadlockDetector.waitFor(v.second.transactionID, waitForTxID))
					{
						this->prematureAbort(v.second.transactionID);
						canRun = false;
						break;
					}
				}
				// Block transaction
				v.second.transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
				break;
			}
			case UPDATE: {
				std::list<int> dependencies = this->tryAcquireItemLock(v.second.transactionID, blockedOp.tableName, blockedOp.coffeeID, WRITE_LOCK);

				if(dependencies.empty())
				{
					execute(v.second.transactionID, blockedOp);
					v.second.operationBuffer.pop_front();
					break;
				}
				// Else
				for(int waitForTxID : dependencies)
				{ // Check for deadlock
					if(!this->deadlockDetector.waitFor(v.second.transactionID, waitForTxID))
					{
						this->prematureAbort(v.second.transactionID);
						canRun = false;
						break;
					}
				}
				// Block transaction
				v.second.transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
				break;
			}
			case RETRIEVE_RECORD:
			case RETRIEVE_ENTIRE_TABLE:
			case RETRIEVE_COFFEE_NAME:
			case COUNT: {
				std::list<int> dependencies;
				if(blockedOp.operationType == RETRIEVE_RECORD)
				{
					dependencies = this->tryAcquireItemLock(v.second.transactionID, blockedOp.tableName, blockedOp.coffeeID, READ_LOCK);
				}
				else
				{
					dependencies = this->tryAcquireTableLock(v.second.transactionID, blockedOp.tableName, READ_LOCK);
				}
				if(!dependencies.empty())
				{
					for(int waitForTxID : dependencies)
					{
						if(!this->deadlockDetector.waitFor(v.second.transactionID,
														   waitForTxID)) // Deadlock detected
						{
							this->prematureAbort(v.second.transactionID);
							canRun = false;
							break;
						}
					}
					this->transactions[v.second.transactionID].transactionStatus = transactionInfo::TRANSACTION_BLOCKED;
					canRun = false;
					break;
				}
				v.second.operationBuffer.pop_front();
				execute(v.second.transactionID, blockedOp);
				break;
			}
			case QUIT:
			case INVALID:
				break;
			}
		}
	}
}

bool Scheduler::checkBlocked()
{
	for(auto transaction : this->transactions)
	{
		if(transaction.second.transactionStatus == transactionInfo::TRANSACTION_BLOCKED)
			return true;
	}
	return false;
}

void Scheduler::execute(int transactionID, Operation operation)
{
	this->printToAll("[TX" + std::to_string(transactionID) + "] " + printOperation(operation));
	switch(operation.operationType)
	{
	case BEGIN:
		this->transactions[transactionID].transaction = Transaction(transactionID, &(this->colBuf), &(this->open_tables), &(this->sharedTableLog));
		this->transactions[transactionID].transaction.begin();
		this->transactions[transactionID].tableLocksOwned = std::list<string>();
		this->transactions[transactionID].recordLocksOwned = std::map<string, std::list<int>>();
		this->transactions[transactionID].operationBuffer = std::list<Operation>();
		break;
	case COMMIT:
		this->transactions[transactionID].transaction.commit();
		this->transactions[transactionID].operationBuffer.clear();
		this->deadlockDetector.endTransaction(transactionID);
		this->numCommits++;
		break;
	case ABORT:
		this->transactions[transactionID].transaction.abort();
		this->transactions[transactionID].operationBuffer.clear();
		this->deadlockDetector.endTransaction(transactionID);
		break;
	case QUIT:
		break;
	case INSERT:
		if(operation.coffeeID < 0)
		{
			cerr << "Invalid coffee ID" << endl;
			break;
		}
		if(operation.intensity < 0)
		{
			cerr << "Invalid intensity" << endl;
			break;
		}
		this->transactions[transactionID].transaction.insert(operation.tableName, operation.coffeeID, operation.coffeeName.substr(0, 8), operation.intensity, operation.countryOfOrigin.substr(0, 8));
		break;
	case UPDATE:
		this->transactions[transactionID].transaction.update(operation.tableName, operation.coffeeID, operation.intensity);
		break;
	case RETRIEVE_RECORD: {
		record r = {};
		if(this->transactions[transactionID].transaction.retrieve(operation.tableName, operation.coffeeID, &r) == 0)
		{
			this->printToAll("\t>>READ: " + std::to_string(r.coffee_id) + " " + r.coffee_name + " " + std::to_string(r.intensity) + " " + r.country_of_origin);
		}
		else
		{
			this->printToAll("\t>>Record not found");
		}
		break;
	}
	case RETRIEVE_ENTIRE_TABLE: {
		std::vector<record> records = this->transactions[transactionID].transaction.retrieve_all(operation.tableName);
		this->printToAll("\t>>TABLE: " + operation.tableName);
		if(records.empty())
		{
			this->printToAll("\t>>Table is empty");
		}
		else
		{
			for(record r : records)
			{
				this->printToAll("\t>>" + std::to_string(r.coffee_id) + " " + r.coffee_name + " " + std::to_string(r.intensity) + " " + r.country_of_origin);
			}
		}
		break;
	}
	case RETRIEVE_COFFEE_NAME: {
		vector<record> records = this->transactions[transactionID].transaction.retrieve_by_country(operation.tableName, operation.countryOfOrigin);
		for(auto r : records)
		{
			printToAll("\t>>" + std::to_string(r.coffee_id) + " " + r.coffee_name + " " + std::to_string(r.intensity) + " " + r.country_of_origin);
		}
		break;
	}
	case COUNT: {
		int c = this->transactions[transactionID].transaction.count_by_intensity(operation.tableName, operation.intensity);
		{
			printToAll("\t>>Count: " + std::to_string(c));
		}
		break;
	}
	case DROP_TABLE: {
		this->transactions[transactionID].transaction.drop_table(operation.tableName);
		break;
	}
	case INVALID:
		break;
	}
	this->addEndTime(transactionID, operation);
}
opType getOperation(char command)
{
	switch(command)
	{
	case 'B':
		return BEGIN;
	case 'C':
		return COMMIT;
	case 'A':
		return ABORT;
	case 'Q':
		return QUIT;
	case 'I':
		return INSERT;
	case 'U':
		return UPDATE;
	case 'R':
		return RETRIEVE_RECORD;
	case 'T':
		return RETRIEVE_ENTIRE_TABLE;
	case 'M':
		return RETRIEVE_COFFEE_NAME;
	case 'G':
		return COUNT;
	case 'D':
		return DROP_TABLE;
	default:
		return INVALID;
	}
}
Operation parse(string command)
{
	char* dup = strdup(command.c_str());
	char* token = strtok(dup, " ");
	Operation op = {INVALID, "", 0, "", 0, ""};
	op.operationType = getOperation(*token);
	switch(op.operationType)
	{
	case INSERT: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		op.coffeeID = atoi(strtok(NULL, "( ),"));
		if(op.coffeeID < 0)
			throw std::runtime_error("Invalid Argument: coffee ID");
		op.coffeeName = string(strtok(NULL, "(),"));
		op.intensity = atoi(strtok(NULL, "(),"));
		if(op.intensity < 0)
			throw std::runtime_error("Invalid Argument: intensity");
		op.countryOfOrigin = string(strtok(NULL, "( ),"));
		break;
	}
	case UPDATE: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		op.coffeeID = atoi(strtok(NULL, "( ),"));
		if(op.coffeeID < 0)
			throw std::runtime_error("Invalid Argument: coffee ID");
		op.intensity = atoi(strtok(NULL, "( ),"));
		if(op.intensity < 0)
			throw std::runtime_error("Invalid Argument: intensity");
		break;
	}
	case RETRIEVE_RECORD: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		op.coffeeID = atoi(strtok(NULL, "( ),"));
		if(op.coffeeID < 0)
			throw std::runtime_error("Invalid Argument: coffee ID");
		break;
	}
	case DROP_TABLE: // fallthrough
	case RETRIEVE_ENTIRE_TABLE: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		break;
	}
	case RETRIEVE_COFFEE_NAME: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		op.countryOfOrigin = string(strtok(NULL, "( ),"));
		break;
	}
	case COUNT: {
		op.tableName = string(strtok(NULL, "( ),"));
		if(op.tableName.find("_deleted") != std::string::npos)
			throw std::runtime_error("Invalid Argument: table name may not contain _deleted");
		op.intensity = atoi(strtok(NULL, "( ),"));
		if(op.intensity < 0)
			throw std::runtime_error("Invalid intensity");
		break;
	}
	default:
		break;
	}
	free(dup);
	return op;
}
bool Scheduler::doesTableExist(string tableName)
{
	string tablePath = "tables/" + tableName;
	struct stat sb;
	return (stat(tablePath.c_str(), &sb) == 0);
}

void Scheduler::prematureAbort(int transactionID)
{
	this->log << "[TX" << std::to_string(transactionID) << "]";
	this->log << "Deadlock was detected. Aborting transaction " << transactionID << std::endl;

	this->releaseAllLocks(transactionID);
	this->deadlockDetector.endTransaction(transactionID);
	this->transactions[transactionID].transaction.abort();
	this->transactions[transactionID].operationBuffer.clear();
	this->transactions[transactionID].transactionStatus = transactionInfo::TRANSACTION_ABORTED;
	this->tryRunningBlocked();
	this->removeFromOperationStat(transactionID);
}

void Scheduler::closeAllTables()
{
	for(auto& table : this->open_tables)
	{
		table.second->close();
	}
}

void Scheduler::addEndTime(int transactionID, Operation operation)
{
	auto opStatList = this->operationStats[transactionID];
	for(int i = 0; static_cast<std::vector<int>::size_type>(i) < opStatList.size(); i++)
	{
		if(opStatList[i].operation == operation && opStatList[i].complete == false)
		{
			this->operationStats[transactionID][i].complete = true;
			this->operationStats[transactionID][i].end_time = clock();
			return;
		}
	}
	throw "Could not find operation in operationStats";
}

void Scheduler::removeFromOperationStat(int transactionID)
{
	auto it = this->operationStats[transactionID].begin();
	while(it != this->operationStats[transactionID].end())
	{
		if(!it->complete)
		{
			it = this->operationStats[transactionID].erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Scheduler::printStat(){
	clock_t total_time = 0;
	for(auto v : this->operationStats)
	{
		for(auto opStat : v.second)
		{
			if(opStat.complete)
			{
				total_time += opStat.end_time - opStat.start_time;
			}
		}
	}
	std::cout << "Summary Statistics" << std::endl;
	std::cout << "\t# Committed Transactions: " << this->numCommits << std::endl;	
	std::cout << "\tPercentage of Reads | Writes: " << 
	((double)(this->colBuf.read_ops) / (double)(this->colBuf.read_ops + this->colBuf.write_ops))
	<< " | "
	<< ((double)(this->colBuf.write_ops) / (double)(this->colBuf.read_ops + this->colBuf.write_ops))
	<< std::endl;
	std::cout << "\tAverage Response Time: " << (((double)total_time / CLOCKS_PER_SEC) / (double)(this->numCommits)) << std::endl;

}