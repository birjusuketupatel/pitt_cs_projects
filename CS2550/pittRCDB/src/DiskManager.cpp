#include "include/DiskManager.h"

using namespace std;

bool operator<(const page_info& x, const page_info& y)
{
	return (strcmp(x.table_name, y.table_name) < 0 ||
				 (strcmp(x.table_name, y.table_name) == 0 && x.col_num < y.col_num) ||
				 (strcmp(x.table_name, y.table_name) == 0 && x.col_num == y.col_num && x.page_num < y.page_num));
}

ColumnBuffer::ColumnBuffer()
{
	time = 0;
	pages_allocated = 0;
	mem_log = nullptr;
	read_ops = 0;
	write_ops = 0;
}

ColumnBuffer::ColumnBuffer(ofstream *log)
{
	time = 0;
	pages_allocated = 0;
	mem_log = log;
	write_ops = 0;
	read_ops = 0;

	if(DEBUG)
	{
		(*mem_log) << "COLUMN BUFFER INITIALIZED " << endl;
	}
}

int ColumnBuffer::access(string table, int column, int page_number, page* target, int operation)
{
	if(operation != CACHE_READ && operation != CACHE_WRITE)
	{
		throw std::invalid_argument("invalid column buffer operation");
	}

	if(operation == CACHE_READ)
	{
		read_ops++;
	}
	else
	{
		write_ops++;
	}

	if(table.length() > 32)
	{
		throw std::invalid_argument("table name must be less than 32 characters long");
	}

	char table_name[33];
	memset(table_name, 0, sizeof(table_name));
	strncpy(table_name, table.c_str(), table.length());

	page_info req_page;
	memset(req_page.table_name, 0, sizeof(req_page.table_name));
	strncpy(req_page.table_name, table.c_str(), table.length());
	req_page.col_num = column;
	req_page.page_num = page_number;

	if(col_buf_index.find(req_page) != col_buf_index.end())
	{
		// cache hit
		cache_data hit = col_buf_index.at(req_page);
		if(operation == CACHE_READ)
		{
			// read page from cache
			col_buf_index[req_page] = {hit.index, time, hit.dirty};
			memcpy(target, &col_buf[hit.index], PAGE_SIZE);
		}
		else
		{
			// write page to cache, mark page as dirty
			col_buf_index[req_page] = {hit.index, time, true};
			memcpy(&col_buf[hit.index], target, PAGE_SIZE);
		}

		time++;
		return 0;
	}

	// cache miss
	string fname = "tables/" + table + "/" + COL_NAMES[column];
	FILE* f;

	if(pages_allocated < COL_BUFFER_SIZE)
	{
		// column buffer is below capacity, allocate page to next empty space
		if(operation == CACHE_READ)
		{
			// read page from disk into column buffer
			if((f = fopen(fname.c_str(), "r")) == NULL)
			{
				throw std::runtime_error("failed to open column file " + fname);
			}

			fseek(f, page_number * PAGE_SIZE, SEEK_SET);
			fread(target, PAGE_SIZE, 1, f);
			fclose(f);
			memcpy(&col_buf[pages_allocated], target, PAGE_SIZE);
			col_buf_index[req_page] = {pages_allocated, time, false};
		}
		else
		{
			// insert dirty page into column buffer
			memcpy(&col_buf[pages_allocated], target, PAGE_SIZE);
			col_buf_index[req_page] = {pages_allocated, time, true};
		}

		if(DEBUG)
		{
			(*mem_log) << "COLD CACHE MISS, loaded page=" << page_number << ", column file=" << COL_NAMES[column] << ", table=" << table << endl;
		}
		pages_allocated++;
		time++;
		return 0;
	}

	// column buffer is at capacity, evict least recently used
	map<page_info, cache_data>::iterator it = col_buf_index.begin();
	page_info lru_page = it->first;
	int lru_time = it->second.last_used;
	while(it != col_buf_index.end())
	{
		if(lru_time > it->second.last_used)
		{
			lru_time = it->second.last_used;
			lru_page = it->first;
		}
		it++;
	}

	cache_data evict = col_buf_index[lru_page];
	string evict_table_name(lru_page.table_name);

	if(evict.dirty)
	{
		// evicted page is dirty, write back to memory
		fname = "tables/" + evict_table_name + "/" + COL_NAMES[lru_page.col_num];

		if((f = fopen(fname.c_str(), "r+")) == NULL)
		{
			throw std::runtime_error("failed to open column file " + fname);
		}

		fseek(f, lru_page.page_num * PAGE_SIZE, SEEK_SET);
		fwrite(&col_buf[evict.index], PAGE_SIZE, 1, f);
		fclose(f);

		if(DEBUG)
		{
			(*mem_log) << "CACHE MISS, evicted page=" << lru_page.page_num << ", column file=" << COL_NAMES[lru_page.col_num] << ", table=" << evict_table_name << endl;
		}
	}

	col_buf_index.erase(lru_page);

	if(operation == CACHE_READ)
	{
		fname = "tables/" + table + "/" + COL_NAMES[column];

		if((f = fopen(fname.c_str(), "r+")) == NULL)
		{
			throw std::runtime_error("failed to open column file " + fname);
		}

		fseek(f, page_number * PAGE_SIZE, SEEK_SET);
		fread(target, PAGE_SIZE, 1, f);
		fclose(f);
		memcpy(&col_buf[evict.index], target, PAGE_SIZE);
		col_buf_index[req_page] = {evict.index, time, false};
	}
	else
	{
		memcpy(&col_buf[evict.index], target, PAGE_SIZE);
		col_buf_index[req_page] = {evict.index, time, true};
	}

	if(DEBUG)
	{
		(*mem_log) << "CACHE MISS, loaded page=" << page_number << ", column file=" << COL_NAMES[column] << ", table=" << table << endl;
	}
	time++;
	return 0;
}

/*
 * marks all pages associated with a given table in the cache as not dirty to be evicted
 */
void ColumnBuffer::delete_from_cache(string table)
{
	map<page_info,cache_data> next_index;
	map<page_info,cache_data>::iterator it = col_buf_index.begin();

	string new_name = "_deleted";
	page_info next_page;
	cache_data next_cache;

	while(it != col_buf_index.end())
	{
		string curr_table(it->first.table_name);

		if(curr_table.compare(table) != 0)
		{
			next_page = it->first;
			next_cache = it->second;
		}
		else
		{
			memset(next_page.table_name, 0, sizeof(next_page.table_name));
			strncpy(next_page.table_name, new_name.c_str(), new_name.length());
			next_page.col_num = it->first.col_num;
			next_page.page_num = it->first.page_num;
			next_cache.index = it->second.index;
			next_cache.last_used = -1;
			next_cache.dirty = false;
		}

		next_index[next_page] = next_cache;
		it++;
	}

	col_buf_index = next_index;
}

/*
 * writes all dirty pages associated with a given table in the cache to file
 */
void ColumnBuffer::flush(string table)
{
	string fname;
	FILE* f;
	map<page_info, cache_data>::iterator it = col_buf_index.begin();

	while(it != col_buf_index.end())
	{
		if(it->second.dirty)
		{
			string curr_table(it->first.table_name);
			fname = "tables/" + curr_table + "/" + COL_NAMES[it->first.col_num];

			if(curr_table.compare(table) != 0)
			{
				it++;
				continue;
			}

			if((f = fopen(fname.c_str(), "r+")) == NULL)
			{
				throw std::runtime_error("failed to open column file " + fname);
			}

			fseek(f, it->first.page_num * PAGE_SIZE, SEEK_SET);
			fwrite(&col_buf[it->second.index], PAGE_SIZE, 1, f);
			fclose(f);
			if(DEBUG)
			{
				(*mem_log) << "WRITE BACK, page=" << it->first.page_num << ", column file=" << COL_NAMES[it->first.col_num]  << ", table=" << table << endl;
			}
			it->second.dirty = false;
		}
		it++;
	}
}

void FileManager::init(string val, ColumnBuffer *col_buf)
{
	name = val;
	col_cache = col_buf;
	pages_per_file[COFFEEID] = 0;
	pages_per_file[COFFEENAME] = 0;
	pages_per_file[INTENSITY] = 0;
	pages_per_file[COUNTRYOFORIGIN] = 0;
}

void FileManager::commit_cache()
{
	col_cache->flush(name);
}

void FileManager::mark_invalid()
{
	col_cache->delete_from_cache(name);
}


/*
 * appends given number of empty pages to the end of the given column file
 */
void FileManager::append_pages(int column, int num_pages)
{
	if(num_pages < 0)
	{
		throw std::invalid_argument("num_pages may not be negative");
	}
	if(column < 0 || column > 4)
	{
		throw std::invalid_argument("invalid column id");
	}

	string fname = "tables/" + name + "/" + COL_NAMES[column];

	FILE* f = fopen(fname.c_str(), "ab");
	page next_page;
	memset(&next_page, 0, sizeof(page));

	for(int i = 0; i < num_pages; i++)
	{
		fwrite(&next_page, sizeof(page), 1, f);
	}

	fclose(f);
	pages_per_file[column] += num_pages;
}

/*
 * pads column until it can store the given number of records
 */
void FileManager::pad_column(int column, int len)
{
	int required_pages = (COL_RECORD_SIZES[column] * len) / PAGE_SIZE;
	if(required_pages * PAGE_SIZE < COL_RECORD_SIZES[column] * len)
	{
		required_pages++;
	}

	if(required_pages - pages_per_file[column] > 0)
	{
		append_pages(column, required_pages - pages_per_file[column]);
	}
}

/*
 * loads individual item from column file at given offset
 */
void FileManager::get_item(int column, int offset, void* bytes)
{
	int record_size = COL_RECORD_SIZES[column];
	int page_num = (offset * record_size) / PAGE_SIZE;
	int byte_offset = (offset * record_size) % PAGE_SIZE;
	page target;

	if(column < 0 || column > 4)
	{
		throw std::invalid_argument("invalid column id");
	}

	if(offset < 0 || offset >= (pages_per_file[column] * PAGE_SIZE) / record_size)
	{
		throw std::invalid_argument("offset out of bounds");
	}

	load_page(column, page_num, &target);
	memcpy(bytes, &target.bytes[byte_offset], record_size);
}

/*
 * sets individual item on the column file at given offset
 */
void FileManager::set_item(int column, int offset, void* bytes)
{
	int record_size = COL_RECORD_SIZES[column];
	int page_num = (offset * record_size) / PAGE_SIZE;
	int byte_offset = (offset * record_size) % PAGE_SIZE;
	page target;

	if(column < 0 || column > 4)
	{
		throw std::invalid_argument("invalid column id");
	}

	if(offset < 0)
	{
		throw std::invalid_argument("offset out of bounds");
	}

	if(offset >= (pages_per_file[column] * PAGE_SIZE) / record_size)
	{
		pad_column(column, offset + 1);
	}

	load_page(column, page_num, &target);
	memcpy(&target.bytes[byte_offset], bytes, record_size);
	store_page(column, page_num, &target);
}

/*
 * copies a given number of records from one position to another in a given column file
 */
int FileManager::copy_records(int column, int start_offset, int end_offset, int num_records)
{
	int record_size = COL_RECORD_SIZES[column];
	int records_per_page = PAGE_SIZE / record_size;
	int file_len = pages_per_file[column] * records_per_page;

	if(num_records < 0)
	{
		throw std::invalid_argument("num_records may not be negative");
	}

	if(start_offset < 0 ||
		 end_offset < 0 ||
		 start_offset + num_records > file_len)
	{
		throw std::invalid_argument("offset out of bounds");
	}

	if(end_offset + num_records >= file_len)
	{
		pad_column(column, end_offset + num_records);
	}

	int old_addr, new_addr;
	page from_page, to_page;
	for(int i = 0; i < num_records; i++)
	{
		old_addr = (start_offset + i) * record_size;
		new_addr = (end_offset + i) * record_size;

		// copy record from old to new address
		load_page(column, old_addr / PAGE_SIZE, &from_page);
		load_page(column, new_addr / PAGE_SIZE, &to_page);

		// update primary key index
		if(column == COFFEEID)
		{
			int coffee_id;
			memcpy(&coffee_id, &from_page.bytes[old_addr % PAGE_SIZE], record_size);
			coffee_id_index[coffee_id] = end_offset + i;
		}

		memcpy(&to_page.bytes[new_addr % PAGE_SIZE],
			   &from_page.bytes[old_addr % PAGE_SIZE],
			   record_size);
		store_page(column, new_addr / PAGE_SIZE, &to_page);
	}

	return 0;
}

int FileManager::store_page(int column, int page_number, page* target)
{
	if(column < 0 || column > 4)
	{
		throw std::invalid_argument("invalid column id");
	}

	if(page_number < 0 || page_number >= pages_per_file[column])
	{
		throw std::invalid_argument("page_number out of bounds");
	}

	return col_cache->access(name, column, page_number, target, CACHE_WRITE);
}

int FileManager::load_page(int column, int page_number, page* target)
{
	if(column < 0 || column > 4)
	{
		throw std::invalid_argument("invalid column id");
	}
	if(page_number < 0 || page_number >= pages_per_file[column])
	{
		throw std::invalid_argument("page_number out of bounds");
	}

	return col_cache->access(name, column, page_number, target, CACHE_READ);
}
