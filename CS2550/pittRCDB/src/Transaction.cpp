#include "include/Transaction.h"

using namespace std;

bool operator<(const record_id& x, const record_id& y)
{
	return (x.table.compare(y.table) < 0 ||
			(x.table.compare(y.table) == 0 && x.coffeeID < y.coffeeID));
}

Transaction::Transaction(int transaction_id, ColumnBuffer* cb, map<string, Table*>* ot, ofstream *log)
{
	id = transaction_id;
	state = -1;
	col_buf = cb;
	open_tables = ot;
	mem_log = log;
}

Transaction::Transaction()
{
	id = -1;
	state = -1;
	col_buf = nullptr;
	open_tables = nullptr;
	mem_log = nullptr;
}

int Transaction::begin()
{
	if(state != -1)
	{
		throw std::runtime_error("may not start transaction");
	}

	state = RUNNING;
	return 0;
}

int Transaction::commit()
{
	if(state != RUNNING)
	{
		throw std::runtime_error("may not commit transaction");
	}

	// delete all tables marked for deletion
	string table_dir;
	for(auto it = dropped_tables.begin(); it != dropped_tables.end(); ++it)
	{
		table_dir = "tables/" + *it + "_deleted";
		std::experimental::filesystem::remove_all(table_dir);
	}

	state = COMMITTED;
	return 0;
}

int Transaction::abort()
{
	if(state != RUNNING)
	{
		throw std::runtime_error("may not abort transaction");
	}

	// drop new tables
	Table* t;
	for(auto it = new_tables.begin(); it != new_tables.end(); ++it)
	{
		if(open_tables->find(*it) == open_tables->end())
		{
			continue;
		}

		t = open_tables->at(*it);
		t->drop();
		open_tables->erase(*it);
		delete t;
	}

	// restore deleted tables if table was not created in this transaction
	for(auto it = dropped_tables.begin(); it != dropped_tables.end(); ++it)
	{
		string from = "tables/" + *it + "_deleted";
		string to = "tables/" + *it;
		rename(from.c_str(), to.c_str());

		t = new Table;
		t->open((*it), col_buf, mem_log);
		(*open_tables)[(*it)] = t;
	}

	// delete insertions on tables that have not been dropped
	for(auto it = inserted_records.begin(); it != inserted_records.end(); ++it)
	{
		if(new_tables.find(it->table) != new_tables.end())
		{
			continue;
		}

		t = open_tables->at(it->table);
		t->delete_record(it->coffeeID);

		// remove from index
		t->mem.coffee_id_index.erase(it->coffeeID);
	}

	// undo updates on records that have not been dropped or deleted
	for(auto it = modified_records.begin(); it != modified_records.end(); ++it)
	{
		if(new_tables.find(it->first.table) != new_tables.end())
		{
			continue;
		}
		if(inserted_records.find(it->first) != inserted_records.end())
		{
			continue;
		}

		t = open_tables->at(it->first.table);
		t->update(it->first.coffeeID, it->second);
	}

	state = ABORTED;
	return 0;
}

bool table_exists(string table_name)
{
	string table_dir = "tables/" + table_name;
	struct stat sb;
	return stat(table_dir.c_str(), &sb) == 0;
}

int Transaction::insert(
	string table, int coffeeID, string coffeeName, int intensity, string countryOfOrigin)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}
	if(coffeeID < 0)
	{
		throw std::invalid_argument("coffeeID must be positive");
	}
	if(coffeeName.length() > 8 || coffeeName.length() <= 0)
	{
		throw std::invalid_argument("coffeeName must be between 1 and 8 characters long");
	}
	if(intensity < 0)
	{
		throw std::invalid_argument("intensity must be positive");
	}
	if(countryOfOrigin.length() > 8 || countryOfOrigin.length() <= 0)
	{
		throw std::invalid_argument("countryOfOrigin must be between 1 and 8 characters long");
	}

	Table* t;
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		t = new Table;
		t->create(table, col_buf, mem_log);
		new_tables.insert(table);
	}

	if(t->insert(coffeeID, coffeeName, intensity, countryOfOrigin) == 0)
	{
		// save coffeeID of inserted record
		inserted_records.insert(record_id{table, coffeeID});
	}

	(*open_tables)[table] = t;
	return 0;
}

int Transaction::update(string table, int coffeeID, int intensity)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}
	if(coffeeID < 0)
	{
		throw std::invalid_argument("coffeeID must be positive");
	}
	if(intensity < 0)
	{
		throw std::invalid_argument("intensity must be positive");
	}

	Table* t;
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		return -1;
	}

	int success;
	record r;

	if(modified_records.find(record_id{table, coffeeID}) != modified_records.end())
	{
		success = t->update(coffeeID, intensity);
	}
	else if(t->retrieve(coffeeID, &r) == 0 && t->update(coffeeID, intensity) == 0)
	{
		modified_records[record_id{table, coffeeID}] = r.intensity;
		success = 0;
	}
	else
	{
		success = -1;
	}

	(*open_tables)[table] = t;
	return success;
}

int Transaction::retrieve(string table, int coffeeID, record* r)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}
	if(coffeeID < 0)
	{
		throw std::invalid_argument("coffeeID must be positive");
	}

	Table* t;
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		return -1;
	}

	int success = t->retrieve(coffeeID, r);
	(*open_tables)[table] = t;
	return success;
}

vector<record> Transaction::retrieve_all(string table)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}

	Table* t;
	vector<record> records = vector<record>();
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		return records;
	}

	records = t->retrieve_all();
	(*open_tables)[table] = t;
	return records;
}

vector<record> Transaction::retrieve_by_country(string table, string countryOfOrigin)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}
	if(countryOfOrigin.length() > 8 || countryOfOrigin.length() <= 0)
	{
		throw std::invalid_argument("countryOfOrigin must be between 1 and 8 characters long");
	}

	Table* t;
	vector<record> records = vector<record>();
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		return records;
	}

	records = t->retrieve_by_country(countryOfOrigin);
	(*open_tables)[table] = t;
	return records;
}

int Transaction::count_by_intensity(string table, int val)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}
	if(val < 0)
	{
		throw std::invalid_argument("intensity must be positive");
	}

	Table* t;
	int count = -1;
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		return count;
	}

	count = t->count_by_intensity(val);
	(*open_tables)[table] = t;
	return count;
}

int Transaction::drop_table(string table)
{
	if(state != RUNNING)
	{
		throw std::runtime_error("transaction has not been started yet");
	}
	if(table.length() > 32 || table.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}

	Table *t;
	if(open_tables->find(table) != open_tables->end())
	{
		t = open_tables->at(table);
		open_tables->erase(table);
	}
	else if(open_tables->find(table) == open_tables->end() && table_exists(table))
	{
		t = new Table;
		t->open(table, col_buf, mem_log);
	}
	else
	{
		// trying to delete non-existent table
		return -1;
	}

	if(dropped_tables.find(table) != dropped_tables.end())
	{
		// table has been recreated since drop, delete again
		t->drop();
		delete t;
		return 0;
	}

	if(new_tables.find(table) != new_tables.end())
	{
		// table has been created in this transaction only, drop and do not save
		t->drop();
		delete t;
		return 0;
	}

	// gracefully close deleted table
	t->close();
	delete t;

	// mark table as deleted
	string from = "tables/" + table;
	string to = "tables/" + table + "_deleted";
	rename(from.c_str(), to.c_str());
	dropped_tables.insert(table);

	return 0;
}
