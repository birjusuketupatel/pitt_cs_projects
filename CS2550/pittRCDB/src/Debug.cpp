#include "include/Scheduler.h"
#include <string>

std::string printOperation(Operation op)
{
	switch(op.operationType)
	{
	case BEGIN:
		return "B";
	case COMMIT:
		return "C";
	case ABORT:
		return "A";
	case QUIT:
		return "Q";
	case INSERT:
		return "I " + op.tableName + " (" + std::to_string(op.coffeeID) + ", " + op.coffeeName + ", " + std::to_string(op.intensity) + ", " + op.countryOfOrigin + ")";
	case UPDATE:
		return "U " + op.tableName + "(" + std::to_string(op.intensity) + ", " + std::to_string(op.coffeeID) + ")";
	case RETRIEVE_RECORD:
		return "R " + op.tableName + " " + std::to_string(op.coffeeID);
	case RETRIEVE_ENTIRE_TABLE:
		return "T " + op.tableName;
	case RETRIEVE_COFFEE_NAME:
		return "M " + op.tableName + " " + op.countryOfOrigin;
	case COUNT:
		return "G " + op.tableName + " " + std::to_string(op.intensity);
	case DROP_TABLE:
		return "D " + op.tableName;
	case INVALID:
		break;
	}

	throw "invalid operation type";
}
