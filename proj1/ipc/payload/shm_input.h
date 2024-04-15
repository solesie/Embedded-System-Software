#ifndef SHM_INPUT_H
#define SHM_INPUT_H

#include "record.h"
#include "../../common/mode.h"

/* request from the io process */
struct shm_input {
	int terminate;
	enum mode m;
	struct record r;
};

#endif // SHM_INPUT_H
