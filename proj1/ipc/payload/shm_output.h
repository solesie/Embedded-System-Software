#ifndef SHM_OUTPUT_H
#define SHM_OUTPUT_H

#include "record.h"
#include "merge_res.h"

/* response from the main process */
struct shm_output {
	/* for PUT/GET mode */
	struct record r;

	/* for MERGE mode */
	struct merge_res mr;
};

#endif // SHM_INPUT_H
