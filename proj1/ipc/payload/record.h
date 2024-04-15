#ifndef RECORD_H
#define RECORD_H

#include <time.h>

#define KEY_DIGIT 4
#define VAL_MAX_LEN 5

struct record{
	long long pk;
	char key[KEY_DIGIT +1];
	char val[VAL_MAX_LEN +1];

	// Having the highest "st" number does not necessarily mean it represents the most recent data.
	// Similarly, having the highest record number does not imply it is the latest data within the table.
	// Moreover, according to convention, it is appropriate to add a "created_at" field.
	// Therefore, it is decided to save "created_at" in the freely defined metadata section of the specification.
	// And upon shutdown of the kernel, it was observed that time(NULL) resets to January 1, 1970. 
	// Therefore, it has been decided to designate created_at as a long long type. 
	// This can be considered a true primary key.
	long long created_at;
};

int compare_by_key(const void* a, const void* b); // comparator

#endif
