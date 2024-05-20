#include "BlockAllocator.h"
#include "BloomFilter.h"
#include "DiskManager.h"
#include "Record.h"
#include <cstring>
#include <string>
#include <sys/types.h>
#include <vector>
#include <experimental/filesystem>

using namespace std;

#ifndef TABLE_MANAGER_HEADER
#	define TABLE_MANAGER_HEADER

class Table
{
private:
	BloomFilter bf;

public:
	string name;
	FileManager mem;
	BlockAllocator b;
	map<int,int> intensity_counts;
	ofstream *mem_log;

	void open(string val, ColumnBuffer *col_buf, ofstream *log);

	void close();

	void create(string val, ColumnBuffer *col_buf, ofstream *log);

	int drop();

	bool is_free(country_of_origin block_header);

	void print_all();

	int insert(int coffeeID, string coffeeName, int intensity, string countryOfOrigin);

	int update(int coffeeID, int intensity);

	int retrieve(int coffeeID, record *val);

	int delete_record(int coffeeID);

	vector<record> retrieve_by_country(string countryOfOrigin);

	vector<record> retrieve_all();

	int count_by_intensity(int val);
};
#endif // TABLE_MANAGER_HEADER
