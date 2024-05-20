#include "Page.h"
using namespace std;

#ifndef DISK_MANAGER_HEADER
#define DISK_MANAGER_HEADER

struct page_info
{
	char table_name[33];
	int col_num;
	int page_num;
};

struct cache_data
{
	int index;
	int last_used;
	bool dirty;
};

class ColumnBuffer
{
public:
	int pages_allocated;
	int time;
	int read_ops;
	int write_ops;
	page col_buf[COL_BUFFER_SIZE];
	map<page_info, cache_data> col_buf_index;
	ofstream *mem_log;

	ColumnBuffer();

	ColumnBuffer(ofstream *log);

	// executes cache reads and writes
	int access(string table, int column, int page_number, page* target, int operation);

	// writes all dirty pages to cache
	void flush(string table);

	// marks pages associated with table as invalid
	void delete_from_cache(string table);
};

class FileManager
{
public:
	string name;
	ColumnBuffer *col_cache;
	int pages_per_file[4];
	map<int,int> coffee_id_index;

	void init(string val, ColumnBuffer *col_buf);

	void commit_cache();

	void mark_invalid();

	// appends given number of empty pages to the end of the given column file
	void append_pages(int column, int num_pages);

	/*
   * pads column until it can store the given number of records
   */
	void pad_column(int column, int len);

	/*
   * loads individual item from column file at given offset
   */
	void get_item(int column, int offset, void* bytes);
	/*
   * sets individual item on the column file at given offset
   */
	void set_item(int column, int offset, void* bytes);

	/*
   * copies a given number of records from one position to another in a given column file
   */
	int copy_records(int column, int start_offset, int end_offset, int num_records);

	int store_page(int column, int page_number, page* target);

	int load_page(int column, int page_number, page* target);
};
#endif // DISK_MANAGER_HEADER
