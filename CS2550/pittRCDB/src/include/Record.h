#ifndef RECORD_HEADER
#define RECORD_HEADER
typedef struct
{
	int coffee_id;
	char coffee_name[8];
	int intensity;
	char country_of_origin[8];
} record;
#endif // RECORD_HEADER