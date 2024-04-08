#include "record.h"
#include <string.h>

/* qsort comparator */
int compare_by_key(const void *a, const void *b){
	const struct record* r1 = (const struct record*)a;
	const struct record* r2 = (const struct record*)b;
	return strcmp(r1->key, r2->key);
}
