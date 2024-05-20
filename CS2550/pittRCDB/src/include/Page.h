#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/stat.h>


#pragma once

#ifndef PAGE_H
#	define PAGE_H

# define DEBUG 1

#	define PAGE_SIZE 512
#	define COL_BUFFER_SIZE 16
#	define MIN_BLOCK_SIZE 16

#	define NUM_COLS 4
#	define COFFEEID 0
#	define COFFEENAME 1
#	define INTENSITY 2
#	define COUNTRYOFORIGIN 3

#	define CACHE_READ 0
#	define CACHE_WRITE 1

struct country_of_origin
{
	char val[8];
	int block_size;
	int record_count;
};

struct page
{
	char bytes[PAGE_SIZE];
};

extern const char* COL_NAMES[];
extern const int COL_RECORD_SIZES[];

#endif // PAGE_H
