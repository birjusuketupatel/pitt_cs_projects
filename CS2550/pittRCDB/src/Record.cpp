#include "include/Record.h"
#include <cstring>

bool operator==(const record& x, const record& y)
{
	return ((x.coffee_id == y.coffee_id) &&
				 (strcmp(x.coffee_name, y.coffee_name) == 0) &&
				 (x.intensity == y.intensity) &&
				 (strcmp(x.country_of_origin, y.country_of_origin) == 0));
};

bool operator<(const record& x, const record& y)
{
	// assume no two records can have same coffee_id
	return ((x.coffee_id < y.coffee_id));
};
