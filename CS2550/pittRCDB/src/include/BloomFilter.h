/**********************************************************************
 * Bloom Filter
 * To achieve a false positive rate of 2% using a 512 entry bloom filter,
 * with two hash functions, we need 6,716 bits.
 *
 **********************************************************************/

#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>

#ifndef BLOOMFILTER_HEADER
#	define BLOOMFILTER_HEADER
#	define MAX_NUM_ENTRIES 512
#	define NUM_FUNCTION 2
#	define MAX_FP_RATE 0.02

using namespace std;

enum
{
	HASH_1,
	HASH_2
};

class BloomFilter
{
private:
	vector<uint8_t> bitVector;
	int computeHash(int key, int i);
	bool getElement(int i);
	void setElement(int i);
	void load(string tableName);

public:
	BloomFilter()
		: bitVector(840, 0)
	{ }
	BloomFilter(string tableName);
	void add(int key);
	bool contains(int key);
	void dump(string tableName);
};

#endif // BLOOMFILTER_HEADER