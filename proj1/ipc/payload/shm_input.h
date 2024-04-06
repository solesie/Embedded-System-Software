#ifndef SHM_INPUT_H
#define SHM_INPUT_H

#define KEY_DIGIT 4
#define VAL_MAX_LEN 5

enum mode{
	PUT = 1,
	GET,
	MERGE
};

/* request from the io process */
struct shm_input {
	enum mode m;
	char key[KEY_DIGIT + 1];
	char val[VAL_MAX_LEN + 1];
};

#endif // SHM_INPUT_H
