#include "include/TableManager.h"
#include <cstring>
#include <fstream>
#include <iostream>

// #include <filesystem>
#include <sys/types.h>
#include <vector>

using namespace std;

const char* COL_NAMES[] = {
	"coffeeID.bin", "coffeeName.bin", "intensity.bin", "countryOfOrigin.bin"};
const int COL_RECORD_SIZES[] = {sizeof(int), 8, sizeof(int), sizeof(country_of_origin)};

/*
 * Writes current state to file
 */
void Table::close()
{
	// saves block allocations to countryOfOrigin.bin
	Node *next = b.head, *temp;
	country_of_origin block;
	int index = 0;
	int blocks_allocated = b.len;

	this->bf.dump("tables/" + name);

	while(next != NULL)
	{
		block = next->key;
		mem.set_item(COUNTRYOFORIGIN, index, (void*)&block);
		index++;
		temp = next;
		next = next->next;
		b.delete_node(temp);
	}

	/*
	 * saves table level data to "tables/{name}/{name}.txt"
	 *
	 * data saved:
	 * number of pages in coffeeID.bin
	 * number of pages in coffeeName.bin
	 * number of pages in intensity.bin
	 * number of pages in countryOfOrigin.bin
	 * number of records in countryOfOrigin.bin
	 */
	ofstream f;
	string fname = "tables/" + name + "/" + name + ".txt";
	f.open(fname);
	f << mem.pages_per_file[COFFEEID] << endl;
	f << mem.pages_per_file[COFFEENAME] << endl;
	f << mem.pages_per_file[INTENSITY] << endl;
	f << mem.pages_per_file[COUNTRYOFORIGIN] << endl;
	f << blocks_allocated << endl;
	f.close();

	// flushes column cache to disk
	mem.commit_cache();
}

/*
 * Loads existing table from disk
 */
void Table::open(string val, ColumnBuffer* col_buf, ofstream *log)
{
	if(val.length() > 32 || val.length() <= 0)
	{
		throw std::invalid_argument("table name must be between 1 and 32 characters long");
	}

	mem.init(val, col_buf);
	b = BlockAllocator();
	name = val;
	mem_log = log;

	// load table level data
	ifstream f;
	string fname = "tables/" + name + "/" + name + ".txt";
	string line;
	int blocks_allocated;

	f.open(fname);

	if(!f.good())
	{
		throw std::invalid_argument("table of this name does not exist");
	}

	getline(f, line);
	mem.pages_per_file[COFFEEID] = stoi(line);
	getline(f, line);
	mem.pages_per_file[COFFEENAME] = stoi(line);
	getline(f, line);
	mem.pages_per_file[INTENSITY] = stoi(line);
	getline(f, line);
	mem.pages_per_file[COUNTRYOFORIGIN] = stoi(line);
	getline(f, line);
	blocks_allocated = stoi(line);
	f.close();

	// load in block allocations
	country_of_origin block;
	alloc_info block_info;
	string block_id;
	int offset = 0;
	Node* next;

	for(int i = 0; i < blocks_allocated; i++)
	{
		mem.get_item(COUNTRYOFORIGIN, i, (void*)&block);
		block_id = block.val;
		next = b.insert_tail(block);
		block_info = alloc_info{offset, next};
		b.block_map[block_id] = block_info;
		offset += block.block_size;
	}

	// load bloomfilter
	this->bf = BloomFilter("tables/" + name);

	// reconstruct coffeeID index and intensity count cache
	int coffeeID, intensity, old_count;
	next = b.head;
	offset = 0;

	while(next != NULL)
	{
		block = next->key;
		if(is_free(block))
		{
			offset += block.block_size;
			next = next->next;
			continue;
		}

		for(int i = 0; i < block.record_count; i++)
		{
			mem.get_item(COFFEEID, offset + i, (void*)&coffeeID);
			mem.get_item(INTENSITY, offset + i, (void*)&intensity);

			// skip deleted records
			if(coffeeID == -1)
			{
				continue;
			}

			// skip if primary key is duplicate
			if(mem.coffee_id_index.find(coffeeID) != mem.coffee_id_index.end())
			{
				continue;
			}

			mem.coffee_id_index[coffeeID] = offset + i;

			if(intensity_counts.find(intensity) == intensity_counts.end())
			{
				intensity_counts[intensity] = 1;
			}
			else
			{
				old_count = intensity_counts.at(intensity);
				intensity_counts[intensity] = old_count + 1;
			}
		}

		offset += block.block_size;
		next = next->next;
	}
}

/*
 * Provisions space for new table in filesystem
 */
void Table::create(string val, ColumnBuffer* col_buf, ofstream *log)
{
	if(val.length() > 32)
	{
		throw std::invalid_argument("table name must be less than 32 characters long");
	}

	mem.init(val, col_buf);
	b = BlockAllocator();
	name = val;
	mem_log = log;

	string table_dir = "tables/" + name;
	string fname;
	FILE* f;

	// makes table directory
	mkdir("tables", 0777);
	if(mkdir(table_dir.c_str(), 0777) == -1)
	{
		throw std::runtime_error("failed to create table directory");
	}

	// makes column files
	for(int i = 0; i < NUM_COLS; i++)
	{
		fname = table_dir + "/" + COL_NAMES[i];
		if((f = fopen(fname.c_str(), "wb")) == NULL)
		{
			throw std::runtime_error("failed to create column file " + fname);
		}
		fclose(f);
	}

	// allocate 1 page to each column file
	mem.append_pages(COFFEEID, 1);
	mem.append_pages(COFFEENAME, 1);
	mem.append_pages(INTENSITY, 1);
	mem.append_pages(COUNTRYOFORIGIN, 1);

	if(DEBUG)
	{
		(*mem_log) << "CREATED FILE " << name << endl;
	}
}

/*
 * Deletes table from filesystem
 */
int Table::drop()
{
	mem.mark_invalid();

	string table_dir = "tables/" + name;
	if(std::experimental::filesystem::remove_all(table_dir) > 0)
	{
		if(DEBUG)
		{
			(*mem_log) << "DELETED FILE " << name << endl;
		}
		return 0;
	}
	else
	{
		return -1;
	}
}

bool Table::is_free(country_of_origin block_header)
{
	return strcmp("", block_header.val) == 0 && block_header.record_count == -1;
}

int Table::insert(int coffeeID, string coffeeName, int intensity, string countryOfOrigin)
{
	// input validation
	if(coffeeID < 0)
	{
		return -1;
	}
	if(coffeeName.length() <= 0 || coffeeName.length() > 8)
	{
		return -1;
	}

	if(intensity < 0)
	{
		return -1;
	}

	if(countryOfOrigin.length() <= 0 || countryOfOrigin.length() > 8)
	{
		return -1;
	}

	if(this->bf.contains(coffeeID) && mem.coffee_id_index.find(coffeeID) != mem.coffee_id_index.end())
	{
		// duplicate coffeeID
		return -1;
	}

	int offset = b.block_insert(countryOfOrigin, &mem);

	char name[8];
	strncpy(name, coffeeName.c_str(), 8);

	mem.set_item(COFFEEID, offset, (void*)&coffeeID);
	mem.set_item(COFFEENAME, offset, (void*)name);
	mem.set_item(INTENSITY, offset, (void*)&intensity);

	// update primary key index
	mem.coffee_id_index[coffeeID] = offset;
	this->bf.add(coffeeID);

	// update counts of intensity values
	if(intensity_counts.find(intensity) == intensity_counts.end())
	{
		intensity_counts[intensity] = 1;
	}
	else
	{
		int old_count = intensity_counts.at(intensity);
		intensity_counts[intensity] = old_count + 1;
	}

	return 0;
}

int Table::update(int coffeeID, int intensity)
{
	// ignore if bloom filter indicates record is definitely not present
	if(!this->bf.contains(coffeeID))
	{
		return -1;
	}

	// update intensity if record with given coffeeID is in this table
	if(mem.coffee_id_index.find(coffeeID) == mem.coffee_id_index.end())
	{
		return -1;
	}
	else
	{
		int offset = mem.coffee_id_index.at(coffeeID);
		int old_intensity;
		mem.get_item(INTENSITY, offset, (void*)&old_intensity);
		mem.set_item(INTENSITY, offset, (void*)&intensity);

		// update counts of intensity values
		int old_count = intensity_counts.at(old_intensity);
		if(old_count > 0)
		{
			intensity_counts[old_intensity] = old_count - 1;
		}

		if(intensity_counts.find(intensity) == intensity_counts.end())
		{
			intensity_counts[intensity] = 1;
		}
		else
		{
			old_count = intensity_counts.at(intensity);
			intensity_counts[intensity] = old_count + 1;
		}
	}

	return 0;
}

int Table::retrieve(int coffeeID, record* val)
{
	// ignore if bloom filter indicates record is definitely not present
	if(!this->bf.contains(coffeeID))
	{
		return -1;
	}

	if(mem.coffee_id_index.find(coffeeID) == mem.coffee_id_index.end())
	{
		return -1;
	}

	int offset = mem.coffee_id_index.at(coffeeID);

	memset(val->coffee_name, 0, 8);
	mem.get_item(COFFEEID, offset, (void*)&val->coffee_id);
	mem.get_item(COFFEENAME, offset, (void*)val->coffee_name);
	mem.get_item(INTENSITY, offset, (void*)&val->intensity);

	country_of_origin block;
	if(b.get_block_from_offset(offset, &block) == -1)
	{
		return -1;
	}

	strncpy(val->country_of_origin, block.val, 8);

	return 0;
}

int Table::delete_record(int coffeeID)
{
	// ignore if bloom filter indicates record is definitely not present
	if(!this->bf.contains(coffeeID))
	{
		return -1;
	}

	if(mem.coffee_id_index.find(coffeeID) == mem.coffee_id_index.end())
	{
		return -1;
	}

	int offset = mem.coffee_id_index.at(coffeeID);

	// update counts of intensity values
	int intensity;
	mem.get_item(INTENSITY, offset, (void*)&intensity);
	int old_count = intensity_counts.at(intensity);
	if(old_count > 0)
	{
		intensity_counts[intensity] = old_count - 1;
	}

	// mark record as deleted
	int new_id = -1;
	mem.set_item(COFFEEID, offset, (void*)&new_id);

	return 0;
}

vector<record> Table::retrieve_by_country(string countryOfOrigin)
{
	vector<record> records;
	record r;

	if(b.block_map.find(countryOfOrigin) == b.block_map.end())
	{
		return records;
	}

	alloc_info block = b.block_map.at(countryOfOrigin);
	int offset = block.offset;
	int num_records = block.loc->key.record_count;

	strncpy(r.country_of_origin, countryOfOrigin.c_str(), countryOfOrigin.length());

	for(int i = 0; i < num_records; i++)
	{
		memset(r.coffee_name, 0, sizeof(r.coffee_name));
		mem.get_item(COFFEEID, offset + i, (void*)&(r.coffee_id));
		mem.get_item(COFFEENAME, offset + i, (void*)r.coffee_name);
		mem.get_item(INTENSITY, offset + i, (void*)&(r.intensity));

		// skip deleted records
		if(r.coffee_id == -1)
		{
			continue;
		}

		records.push_back(r);
	}

	return records;
}

vector<record> Table::retrieve_all()
{
	vector<record> records;
	record r;
	Node* next = b.head;
	country_of_origin block;
	int offset = 0;

	while(next != NULL)
	{
		block = next->key;
		if(!is_free(block))
		{
			strncpy(r.country_of_origin, block.val, 8);

			for(int i = 0; i < block.record_count; i++)
			{
				memset(r.coffee_name, 0, sizeof(r.coffee_name));
				mem.get_item(COFFEEID, offset + i, (void*)&(r.coffee_id));
				mem.get_item(COFFEENAME, offset + i, (void*)r.coffee_name);
				mem.get_item(INTENSITY, offset + i, (void*)&(r.intensity));

				// skip deleted records
				if(r.coffee_id == -1)
				{
					continue;
				}

				records.push_back(r);
			}
		}

		offset += block.block_size;
		next = next->next;
	}

	return records;
}

int Table::count_by_intensity(int intensity)
{
	int count;

	if(intensity_counts.find(intensity) == intensity_counts.end())
	{
		count = 0;
	}
	else
	{
		count = intensity_counts.at(intensity);
	}

	return count;
}
