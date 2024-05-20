#include "include/DiskManager.h"
#include "include/Scheduler.h"
#include "include/TableManager.h"
#include "include/Transaction.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

extern void runTests(string arg);

enum
{
	RANDOM,
	ROUND_ROBIN
} mode;

enum
{
	NEWTRANSACTION,
	VALID,
	NOTVALID
};

int seed = 0;

typedef struct
{
	int transactionID;
	std::ifstream* fileptr;
} transactionFile;

bool isValidInput(const std::string& input)
{
	for(char c : input)
	{
		if(!std::isspace(c))
		{
			if(c == 'B' || c == 'C' || c == 'A' || c == 'Q' || c == 'I' || c == 'U' || c == 'R' || c == 'T' || c == 'M' || c == 'G' || c == 'D')
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

void run_random(std::list<transactionFile> inputFiles, int seed)
{
	Scheduler scheduler = Scheduler();
	std::ofstream logFile("./logs/random.log");
	if(!logFile.is_open())
	{
		std::cerr << "Failed to open `random.log` for writing." << std::endl;
		exit(1);
	}
	int target;
	srand(seed);
	while(inputFiles.size() > 0)
	{
		target = rand() % inputFiles.size();
		auto iter = inputFiles.begin();
		std::advance(iter, target);
		std::string line;
		int readNext = 1;
		while(readNext != 0)
		{
			if(((iter->fileptr))->eof())
			{
				inputFiles.erase(iter);
				break;
			}
			std::getline(*(iter->fileptr), line);
			if(line.empty())
				continue;
			if(isValidInput(line))
			{
				logFile << line << std::endl;
				scheduler.schedule(iter->transactionID, line);
			}
			else
				std::cerr << "Invalid command detected " << line << std::endl;
			readNext = rand() % 2;
		}
	}
	scheduler.printStat();
	scheduler.closeAllTables();
}

void run_roundrobin(std::list<transactionFile> inputFiles)
{
	Scheduler scheduler = Scheduler();
	// Create Log File
	std::ofstream logFile("./logs/roundrobin.log");
	if(!logFile.is_open())
	{
		std::cerr << "Failed to open `roundrobin.log` for writing." << std::endl;
		exit(1);
	}
	bool allFilesClosed;
	while(!allFilesClosed)
	{
		allFilesClosed = true;
		for(auto iter : inputFiles)
		{
			std::string line;
			if(std::getline(*(iter.fileptr), line))
			{
				if(isValidInput(line))
				{
					line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
					logFile << line << std::endl;
					scheduler.schedule(iter.transactionID, line);
					allFilesClosed = false;
				}
				else
					std::cerr << "Invalid command detected " << line << std::endl;
			}
			else if(!(iter.fileptr->eof()))
			{
				std::cerr << "Error reading from file." << std::endl;
			}
		}
	}
	scheduler.printStat();
	scheduler.runRemaining(); // Try to see if there are any blocked operations
	scheduler.closeAllTables();
}

int main(int argc, char* argv[])
{
	// Check for correct number of arguments
	if(argc < 2)
	{
		// Print usage
		std::cout << "Usage: " << argv[0] << " [MODE] [PROGRAM] [PROGRAM ...]\n\n"
				  << "Mode :\n"
				  << "  random [SEED]\t\n"
				  << "  roundrobin\n\n"
				  << "Example: " << argv[0] << " random 1234 ./tests/prog1.txt ./tests/prog2.txt\n"
				  << std::endl;
		return 0;
	}

	// Parse mode (random or roundrobin)
	if(strcmp(argv[1], "random") == 0)
	{
		mode = RANDOM;
		seed = atoi(argv[2]);
	}
	else if(strcmp(argv[1], "roundrobin") == 0)
	{
		mode = ROUND_ROBIN;
	}
	else if(strcmp(argv[1], "test") == 0)
	{
		if (argc < 3)
		{
			std::cerr << "Must proide test name" << std::endl;
			return 0;
		}
		runTests(argv[2]);
		return 0;
	}
	else
	{
		std::cerr << "Invalid mode" << std::endl;
		return 0;
	}

	// List of file pointers (as streams)
	std::list<transactionFile> inputs;
	int transactionID = 0;
	// Load the programs
	for(int i = 2; i < argc; i++) // Begin at 2 to skip mode
	{
		if(mode == RANDOM && i == 2) // Skip seed if mode is random
			continue;

		std::ifstream* filePtr = new std::ifstream(argv[i], std::ios::in);
		if(filePtr->is_open())
		{
			inputs.push_back({transactionID, filePtr});
			transactionID++;
		}
		else
		{
			std::cerr << "Failed to open file: " << argv[i] << std::endl;
			delete filePtr; // Clean up if failed to open
		}
	}
	// Run the transaction manager
	if(mode == RANDOM)
	{
		run_random(inputs, seed);
	}
	else if(mode == ROUND_ROBIN)
	{
		run_roundrobin(inputs);
	}

	// Close and clean up file pointers
	for(auto iter : inputs)
	{
		iter.fileptr->close();
		delete iter.fileptr;
	}
	inputs.clear(); // Clear the list
}

/*
#include <iostream>
#include <random>

using namespace std;

int main()
{
	string table_names[] = {"Giant", "Acme", "Kroger", "Shop-Rite", "Patel Bros", "Gentile's"};
	string countries[] = {"US", "US", "US", "US", "US", "MX", "US", "YE", "MX", "BR", "BR", "BR", "EC", "EC", "UK",
												"UK", "UK", "IN", "CN", "CN", "CA", "CA", "RU", "RU", "JP", "JP", "JP", "SA", "SA", "SA"};
	string names[] = {"Lemon", "Peach", "Apple", "Orange", "Plum", "Banana", "Grape", "Cherry", "Guava", "Kiwi", "Blueberry",
										"Pineapple", "Watermelon", "Dragonfruit", "Cantaloupe", "Pepper", "Eggplant", "Cucumber"};
	int table_index, country_index, name_index, intensity;

	for(int i = 0; i < 256; i++)
	{
		table_index = rand() % (sizeof(table_names) / sizeof(table_names[0]));
		country_index = rand() % (sizeof(countries) / sizeof(countries[0]));
		name_index = rand() % (sizeof(names) / sizeof(names[0]));
		intensity = rand() % 10;
		cout << "I " << table_names[table_index] << " (" << i << ", " << names[name_index] << ", " << intensity << ", " << countries[country_index] << ")" << endl;
	}
}
*/
