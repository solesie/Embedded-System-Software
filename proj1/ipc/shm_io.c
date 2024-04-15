/*
 * Define a structure for handling inter-process Input/Output using shared memory.
 * Be aware that the functions here are not fork-safe because, 
 * while the semaphore in shm_db acts as a mutex, shm_io does not. 
 * Reading shm_io requires an order defined by external logic. 
 * It would be proper to place a mutex on each input and output buffer, but this will be omitted.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include "semaphore.h"
#include "ipc_keys.h"
#include "./payload/record.h"
#include "./payload/merge_res.h"
#include "./payload/shm_input.h"
#include "./payload/shm_output.h"
#include "../common/logging.h"

struct shm_io {
	int shm_io_id;

	// data
	struct shm_input shmi;
	struct shm_output shmo;
};

/* returns NULL if creation fails */
struct shm_io* shm_io_create(){
	int id = shmget(KEY_T_SHM_IO, sizeof(struct shm_io), IPC_CREAT | IPC_EXCL | 0666);
	if(id == -1){
		LOG(LOG_LEVEL_ERROR, "shm_io_create shmget: %d", errno);
		return NULL;
	}
	
	struct shm_io* ret = (struct shm_io*)shmat(id, 0, 0);
	if(ret == (void*)-1){
		LOG(LOG_LEVEL_ERROR, "shm_io_create shmget: %d", errno);
		shmctl(id, IPC_RMID, 0);
		return NULL;
	}
	memset(ret, 0, sizeof(struct shm_io));
	ret->shm_io_id = id;
	return ret;
}

void shm_io_destroy(struct shm_io* p){
	int status = 1;
	int id = p->shm_io_id;
	if(shmdt(p) == -1) status = -1;
	if(shmctl(id, IPC_RMID, 0) == -1) status = -1;
	if(status == -1){ 
		LOG(LOG_LEVEL_ERROR, "shm_io_destroy: %d", errno);
	}
}

void shm_io_write_input(struct shm_io* p, const struct shm_input* input){
	memcpy(&p->shmi, input, sizeof(struct shm_input));
}

/* res will be initialized */
void shm_io_read_input(struct shm_io* p, struct shm_input* res){
	memcpy(res, &p->shmi, sizeof(struct shm_input));
}

void shm_io_write_output(struct shm_io* p, const struct shm_output* output){
	memcpy(&p->shmo, output, sizeof(struct shm_output));
}

/* res will be initialized */
void shm_io_read_output(struct shm_io* p, struct shm_output* res){
	memcpy(res, &p->shmo, sizeof(struct shm_output));
}
