#include "TableManager.h"
#include <set>

using namespace std;

#ifndef TRANSACTION_HEADER
#	define TRANSACTION_HEADER
#	define RUNNING 0
#	define ABORTED 1
#	define COMMITTED 2

struct record_id
{
	string table;
	int coffeeID;
};

class Transaction
{
public:
	map<string, Table*> *open_tables;
	int id;
	int state;
	set<record_id> inserted_records;
	map<record_id, int> modified_records;
	set<string> new_tables;
	set<string> dropped_tables;
	ColumnBuffer* col_buf;
	ofstream *mem_log;

	Transaction();
	Transaction(int transaction_id, ColumnBuffer* cb, map<string, Table*>* ot, ofstream *log);
	int begin();
	int commit();
	int abort();
	int insert(string table, int coffeeID, string coffeeName, int intensity, string countryOfOrigin);
	int update(string table, int coffeeID, int intensity);
	int retrieve(string table, int coffeeID, record* r);
	vector<record> retrieve_all(string table);
	vector<record> retrieve_by_country(string table, string countryOfOrigin);
	int count_by_intensity(string table, int val);
	int drop_table(string table);
};

#endif
