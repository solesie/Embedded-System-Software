#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "ipc_keys.h"
#include "../common/logging.h"
#include "semaphore.h"

union semun { 
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct sem_ids {
	int shm_input, shm_output, shm_database;
};

static struct sembuf p = {0, -1, SEM_UNDO};
static struct sembuf v = {0, 1, SEM_UNDO};

/* returns NULL if creation fails */
struct sem_ids* sem_ids_create() {
	union semun semunarg_shm_input, semunarg_shm_output, semunarg_shm_db;
	struct sem_ids* ret = (struct sem_ids*)malloc(sizeof(struct sem_ids));
	ret->shm_input = semget(KEY_T_SEM_SHM_INPUT, 1, IPC_CREAT | IPC_EXCL | 0666); 
    ret->shm_output = semget(KEY_T_SEM_SHM_OUTPUT, 1, IPC_CREAT | IPC_EXCL| 0666);
	ret->shm_database = semget(KEY_T_SEM_SHM_DB, 1, IPC_CREAT | IPC_EXCL| 0666);

	if(ret->shm_input == -1 || ret->shm_output == -1 || ret->shm_database == -1){
		LOG(LOG_LEVEL_ERROR, "sem_ids_create semget: %d", errno);
		sem_ids_destroy(ret);
        return NULL;
	}

	semunarg_shm_input.val = 0;
	semunarg_shm_output.val = 0;
	semunarg_shm_db.val = 1;

	int status = 1;
	if(semctl(ret->shm_input, 0, SETVAL, semunarg_shm_input) == -1) status = -1;
	if(semctl(ret->shm_output, 0, SETVAL, semunarg_shm_output) == -1) status = -1;
	if(semctl(ret->shm_database, 0, SETVAL, semunarg_shm_db) == -1) status = -1;
	if(status == -1){
		LOG(LOG_LEVEL_ERROR, "sem_ids_create semctl: %d", errno);
		sem_ids_destroy(ret);
        return NULL;
	}
	return ret;
}

void sem_shm_input_down(struct sem_ids* ids) {
	if(semop(ids->shm_input, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_input_down: %d", errno);
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_input_up(struct sem_ids* ids) {
    if(semop(ids->shm_input, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_input_up: %d", errno);
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_shm_output_down(struct sem_ids* ids) {
	if(semop(ids->shm_output, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_output_down: %d", errno);
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_output_up(struct sem_ids* ids) {
    if(semop(ids->shm_output, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_output_up: %d", errno);
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_shm_database_down(struct sem_ids* ids) {
	if(semop(ids->shm_database, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_database_down: %d", errno);
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_database_up(struct sem_ids* ids) {
    if(semop(ids->shm_database, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_database_up: %d", errno);
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_ids_destroy(struct sem_ids* ids){
	int status = 1;
	if(semctl(ids->shm_input, 0, IPC_RMID, 0) == -1) status = -1;
	if(semctl(ids->shm_output, 0, IPC_RMID, 0) == -1) status = -1;
	if(semctl(ids->shm_database, 0, IPC_RMID, 0) == -1) status = -1;
	if(status == -1){
		LOG(LOG_LEVEL_ERROR, "sem_ids_destroy: %d", errno);
	}
	free(ids);
}
