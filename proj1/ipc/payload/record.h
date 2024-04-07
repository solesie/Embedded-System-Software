#ifndef RECORD_H
#define RECORD_H

#define KEY_DIGIT 4
#define VAL_MAX_LEN 5

struct record{
	long long pk;
	char key[KEY_DIGIT +1];
	char val[VAL_MAX_LEN +1];
};

#endif
