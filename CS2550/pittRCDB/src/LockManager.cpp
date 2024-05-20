#include "include/Scheduler.h"
#include <string>

std::list<int> Scheduler::tryAcquireItemLock(int transactionID, string tableName, int coffeeID, lockType desiredLock)
{
	std::list<int> result;
	if(desiredLock == READ_LOCK)
	{
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << " Attempt to acquire READ lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
		// Attempt to get IRL on table
		std::list<int> result = this->tryAcquireTableLock(transactionID, tableName, INTENT_READ_LOCK);

		if(!result.empty())
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Failed to get top-level lock on " << tableName << std::endl;
			return result;
		}

		// Check if entry exists. If it doesn't, TX is free to grab lock
		// Note: Lock table is guaranteed to exist at this point - created by `tryAcquireTableLock`
		if(this->recordLevelLocks[tableName].count(coffeeID) == 0)
		{
			this->recordLevelLocks[tableName][coffeeID] = std::map<int, Lock>();
			this->recordLevelLocks[tableName][coffeeID][transactionID] = {transactionID, READ_LOCK};
			this->transactions[transactionID].recordLocksOwned[tableName].push_back(coffeeID);
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquired READ lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
			return result;
		}

		std::map<int, Lock> recordLock = this->recordLevelLocks[tableName][coffeeID];

		// Check if TX already has a lock on the record (any kind)
		if(recordLock.count(transactionID) != 0)
		{
			return result;
		}

		// Check if there is a write lock on the record
		for(auto v : recordLock)
		{
			if(v.second.transactionID != transactionID && v.second.type != READ_LOCK)
				result.push_back(v.second.transactionID); // Add to dependency list
		}

		// If there are no dependencies, we can acquire the lock
		if(result.empty())
		{
			this->recordLevelLocks[tableName][coffeeID][transactionID] = {transactionID, READ_LOCK};
			this->transactions[transactionID].recordLocksOwned[tableName].push_back(coffeeID);
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquired READ lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
		}
		else
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Failed to attain READ lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
		}
		return result;
	}
	else if(desiredLock == WRITE_LOCK)
	{
		// Attempt to get IWL on table
		std::list<int> result = this->tryAcquireTableLock(transactionID, tableName, INTENT_WRITE_LOCK);

		if(!result.empty())
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Failed to get top-level lock on " << tableName << std::endl;
			return result;
		}

		// Check if entry exists. If it doesn't, TX is free to grab lock
		// Note: Lock table is guaranteed to exist at this point - created by `tryAcquireTableLock`
		if(this->recordLevelLocks[tableName].count(coffeeID) == 0)
		{
			this->recordLevelLocks[tableName][coffeeID] = std::map<int, Lock>();
			this->recordLevelLocks[tableName][coffeeID][transactionID] = {transactionID, WRITE_LOCK};
			this->transactions[transactionID].recordLocksOwned[tableName].push_back(coffeeID);
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquired WRITE lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
			return result;
		}
		std::map<int, Lock> recordLock = this->recordLevelLocks[tableName][coffeeID];

		// Check if TX already has a lock on the record (write)
		if(recordLock.count(transactionID) > 0 && recordLock[transactionID].type == WRITE_LOCK)
			return result;

		// Check if there is a lock on the record (any)
		for(auto v : recordLock)
		{
			if(v.second.transactionID != transactionID)
				result.push_back(v.second.transactionID);
		}

		// Check if there are any dependencies
		if(result.empty())
		{
			// Update/Add lock
			// Note: If TX already has a read lock, upgrade it to write (exclusive) lock
			if(this->recordLevelLocks[tableName][coffeeID].count(transactionID) == 0)
				this->transactions[transactionID].recordLocksOwned[tableName].push_back(coffeeID);

			this->recordLevelLocks[tableName][coffeeID][transactionID] = {transactionID, WRITE_LOCK};
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquired WRITE lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
		}
		else
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Failed to attain WRITE lock on " << tableName << "[" << std::to_string(coffeeID) << "]" << std::endl;
		}
		return result;
	}
	else
	{
		throw "Attempting to acquire intent locks on lowest data item";
	}
}

std::list<int> Scheduler::tryAcquireTableLock(int transactionID, string tableName, lockType desiredLock)
{
	std::list<int> result;

	// Check if lock table exists
	if(this->tableLevelLocks.count(tableName) == 0)
	{

		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << " Acquired lock on " << tableName << std::endl;
		this->tableLevelLocks[tableName] = std::map<int, Lock>();
		this->tableLevelLocks[tableName][transactionID] = {transactionID, desiredLock};
		this->recordLevelLocks[tableName] = std::map<int, std::map<int, Lock>>();
		this->transactions[transactionID].tableLocksOwned.push_back(tableName);
		return result;
	}

	std::map<int, Lock> locks = this->tableLevelLocks[tableName];

	switch(desiredLock)
	{
	case INTENT_READ_LOCK:
		// Check if TX already has a lock on the table (any)
		if(locks.count(transactionID) > 0)
			return result; // Empty

		// Check if any TX has a write lock on the table
		for(auto v : locks)
		{
			if(v.second.transactionID != transactionID && v.second.type == WRITE_LOCK)
				result.push_back(v.second.transactionID);
		}
		// Check if any dependencies were found
		if(result.empty())
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquired IRL on " << tableName << std::endl;
			this->tableLevelLocks[tableName][transactionID] = {transactionID, INTENT_READ_LOCK};
			this->transactions[transactionID].tableLocksOwned.push_back(tableName);
		}

		return result;

	case INTENT_WRITE_LOCK:

		// Check if TX already has same or stronger lock (w, riw)
		if(locks.count(transactionID) > 0 && (locks[transactionID].type == INTENT_WRITE_LOCK || locks[transactionID].type == READ_INTENT_WRITE_LOCK || locks[transactionID].type == WRITE_LOCK))
			return result; // Empty

		// Check if another TX has a RL, RIW, or WL on the table
		for(auto v : locks)
		{
			if(v.second.transactionID != transactionID)
			{
				switch(v.second.type)
				{
				case READ_LOCK:
				case READ_INTENT_WRITE_LOCK:
				case WRITE_LOCK:
					result.push_back(v.second.transactionID);
					break;
				default:
					break;
				}
			}
		}

		if(result.empty()) // No dependencies
		{
			if(locks.count(transactionID) == 0)
			{
				this->log << "[TX" << std::to_string(transactionID) << "]";
				this->log << " Acquired IWL on " << tableName << std::endl;
				this->tableLevelLocks[tableName][transactionID] = {transactionID, INTENT_WRITE_LOCK};
				this->transactions[transactionID].tableLocksOwned.push_back(tableName);
			}
			else // If TX already has a lock on the table, escalate
			{
				switch(locks[transactionID].type)
				{
				case INTENT_READ_LOCK:
					this->log << "[TX" << std::to_string(transactionID) << "]";
					this->log << " Escalated IRL to IWL on " << tableName << std::endl;
					this->tableLevelLocks[tableName][transactionID] = {transactionID, INTENT_WRITE_LOCK};
					break;
				case READ_LOCK:
					this->log << "[TX" << std::to_string(transactionID) << "]";
					this->log << " Escalated RL to IWL on " << tableName << std::endl;
					this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_INTENT_WRITE_LOCK};
					break;
				case READ_INTENT_WRITE_LOCK:
				case INTENT_WRITE_LOCK:
				case WRITE_LOCK:
					break;
				}
			}
		}
		return result;

	case READ_LOCK:
		// Check if TX already has same or stronger lock
		if(locks.count(transactionID) > 0 && (locks[transactionID].type == READ_LOCK || locks[transactionID].type == READ_INTENT_WRITE_LOCK || locks[transactionID].type == WRITE_LOCK))
			return result; // Empty

		for(auto v : locks)
		{
			if(v.second.transactionID != transactionID)
			{
				switch(v.second.type)
				{
				case INTENT_READ_LOCK:
				case READ_LOCK:
					break;
				case INTENT_WRITE_LOCK:
				case READ_INTENT_WRITE_LOCK:
				case WRITE_LOCK:
					result.push_back(v.second.transactionID);
					break;
				}
			}
		}

		if(result.empty()) // No dependencies
		{
			if(locks.count(transactionID) == 0)
			{
				this->log << "[TX" << std::to_string(transactionID) << "]";
				this->log << " Acquired RL on " << tableName << std::endl;
				this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_LOCK};
				this->transactions[transactionID].tableLocksOwned.push_back(tableName);
				return result;
			}
			else // Escalate
			{
				switch(locks[transactionID].type)
				{
				case INTENT_READ_LOCK:
					this->log << "[TX" << std::to_string(transactionID) << "]";
					this->log << " Escalated IRL to RL on " << tableName << std::endl;
					this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_LOCK};
				case INTENT_WRITE_LOCK:
					this->log << "[TX" << std::to_string(transactionID) << "]";
					this->log << " Escalated IWL to RIWL on " << tableName << std::endl;
					this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_INTENT_WRITE_LOCK};
				case READ_LOCK:
				case READ_INTENT_WRITE_LOCK:
				case WRITE_LOCK:
					break;
				}
			}
		}
		return result;

	case READ_INTENT_WRITE_LOCK:
		// Check if TX already has same or stronger lock
		if(locks.count(transactionID) > 0 && (locks[transactionID].type == READ_INTENT_WRITE_LOCK || locks[transactionID].type == WRITE_LOCK))
			return result; // Empty

		for(auto v : locks)
		{
			if(v.second.transactionID != transactionID && v.second.type != INTENT_READ_LOCK)
				result.push_back(v.second.transactionID);
		}

		if(result.empty())
		{
			if(locks.count(transactionID) == 0)
			{
				this->log << "[TX" << std::to_string(transactionID) << "]";
				this->log << " Acquire RIWL on " << tableName << std::endl;
				this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_INTENT_WRITE_LOCK};
				this->transactions[transactionID].tableLocksOwned.push_back(tableName);
			}
			else if(locks[transactionID].type != WRITE_LOCK) // Escalate
			{
				this->log << "[TX" << std::to_string(transactionID) << "]";
				this->log << " Escalated existing lock to RIWL on " << tableName << std::endl;
				this->tableLevelLocks[tableName][transactionID] = {transactionID, READ_INTENT_WRITE_LOCK};
			}
		}
		return result;

	case WRITE_LOCK:
		// Check if TX already has same or stronger lock
		if(locks.count(transactionID) > 0 && locks[transactionID].type == WRITE_LOCK)
			return result; // Empty

		for(auto v : locks)
		{
			if(v.second.transactionID != transactionID)
				result.push_back(v.second.transactionID);
		}

		if(result.empty())
		{
			if(this->tableLevelLocks[tableName].count(transactionID) == 0)
				this->transactions[transactionID].tableLocksOwned.push_back(tableName);

			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Acquire WL on " << tableName << std::endl;
			this->tableLevelLocks[tableName][transactionID] = {transactionID, WRITE_LOCK};
		}
		return result;
	}

	return result;
}

void Scheduler::releaseAllLocks(int transactionID)
{
	// First release record locks
	std::map<string, std::list<int>> recordLocksOwned = this->transactions[transactionID].recordLocksOwned;
	for(auto tables : recordLocksOwned)
	{
		for(auto lock : tables.second)
		{
			this->log << "[TX" << std::to_string(transactionID) << "]";
			this->log << " Released lock on " << tables.first << "[" << std::to_string(lock) << "]" << std::endl;
			this->recordLevelLocks[tables.first][lock].erase(transactionID);
		}
	}
	this->transactions[transactionID].recordLocksOwned.clear();

	std::list<string> tableLocks = this->transactions[transactionID].tableLocksOwned;
	for(auto tableName : tableLocks)
	{
		this->log << "[TX" << std::to_string(transactionID) << "]";
		this->log << " Released lock on " << tableName << std::endl;
		this->tableLevelLocks[tableName].erase(transactionID);
	}
	this->transactions[transactionID].tableLocksOwned.clear();
}
