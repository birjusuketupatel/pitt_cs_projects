#include "include/BloomFilter.h"
#include <iostream>

int BloomFilter::computeHash(int key, int i)
{
	if(i == HASH_1)
		return key % MAX_NUM_ENTRIES;
	else
		return (3 + (5 * key)) % MAX_NUM_ENTRIES;
}

bool BloomFilter::getElement(int i)
{
	int byteIndex = (i / 8);
	int bitOffset = (i % 8);
	return (bitVector[byteIndex] >> bitOffset) & 1;
}

void BloomFilter::setElement(int i)
{
	int byteIndex = (i / 8);
	int bitOffset = (i % 8);
	uint32_t mask = 1 << bitOffset;
	bitVector[byteIndex] = bitVector[byteIndex] | mask;
}

void BloomFilter::load(string tableName)
{
	string fileName = "./" + tableName + "/bloomfilter.bin";
	ifstream inFile(fileName, ios::in);
	if(!inFile.is_open())
	{
		cerr << "Error opening file" << fileName << endl;
		return;
	}
	// Get the size of the file
	inFile.seekg(0, std::ios::end);
	std::streampos fileSize = inFile.tellg();
	inFile.seekg(0, std::ios::beg);
	this->bitVector = vector<uint8_t>(fileSize);

	inFile.read(reinterpret_cast<char*>(&(this->bitVector[0])), fileSize);
	inFile.close();
}

BloomFilter::BloomFilter(string tableName)
{
	load(tableName);
}

void BloomFilter::add(int key)
{
	int k1 = this->computeHash(key, HASH_1);
	int k2 = this->computeHash(key, HASH_2);
	this->setElement(k1);
	this->setElement(k2);
}

bool BloomFilter::contains(int key)
{
	int k1 = this->computeHash(key, HASH_1);
	int k2 = this->computeHash(key, HASH_2);
	return (this->getElement(k1) || this->getElement(k2));
}

void BloomFilter::dump(string tableName)
{
	string fileName = "./" + tableName + "/bloomfilter.bin";
	ofstream outFile(fileName, ios::out);
	if(!outFile.is_open())
	{
		cerr << "Error opening file" << fileName << endl;
		return;
	}
	outFile.write(reinterpret_cast<const char*>(&(this->bitVector[0])),
				  this->bitVector.size() * sizeof(uint8_t));
	outFile.close();
}
