#ifndef SHM_OUTPUT_H
#define SHM_OUTPUT_H

#include "record.h"
#include <stddef.h>

/* response from the main process */
struct shm_output {
	/* for PUT/GET mode */
	int pk;
	char key[KEY_DIGIT + 1];
	char val[VAL_MAX_LEN + 1];

	/* for MERGE mode */
	int st_name;
	size_t cnt;
};

#endif // SHM_INPUT_H
