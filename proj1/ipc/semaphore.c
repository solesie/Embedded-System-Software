#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "ipc_keys.h"
#include "../util/logging.h"

union semun { 
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct sem_ids {
	int shm_input, shm_output, shm_memory_table, storage_table;
};

static struct sembuf p = {0, -1, SEM_UNDO};
static struct sembuf v = {0, 1, SEM_UNDO};

struct sem_ids* sem_ids_create() {
	union semun semunarg_shm_input, semunarg_shm_output, semunarg_shm_mt, semunarg_st;
	struct sem_ids* ret = (struct sem_ids*)malloc(sizeof(struct sem_ids));
    ret->shm_input = semget(KEY_T_SEM_SHM_INPUT, 1, IPC_CREAT | IPC_EXCL | 0666); 
    ret->shm_output = semget(KEY_T_SEM_SHM_OUTPUT, 1, IPC_CREAT | IPC_EXCL| 0666);
	ret->shm_memory_table = semget(KEY_T_SEM_SHM_MT, 1, IPC_CREAT | IPC_EXCL| 0666);
	ret->storage_table = semget(KEY_T_SEM_ST, 1, IPC_CREAT | IPC_EXCL| 0666);

	semunarg_shm_input.val = 0;
	semunarg_shm_output.val = 0;
	semunarg_shm_mt.val = 1;
	semunarg_st.val = 1;

	int status = 1;
	if(semctl(ret->shm_input, 0, SETVAL, semunarg_shm_input) == -1) status = -1;
	if(semctl(ret->shm_output, 0, SETVAL, semunarg_shm_output) == -1) status = -1;
	if(semctl(ret->shm_memory_table, 0, SETVAL, semunarg_shm_mt) == -1) status = -1;
	if(semctl(ret->storage_table, 0, SETVAL, semunarg_st) == -1) status = -1;
	if(status == -1){
		LOG(LOG_LEVEL_ERROR, "sem_ids_create");
		free(ret);
		exit(1);
	}
	return ret;
}

void sem_shm_input_down(struct sem_ids* ids) {
	if(semop(ids->shm_input, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_input_down");
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_input_up(struct sem_ids* ids) {
    if(semop(ids->shm_input, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_input_up");
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_shm_output_down(struct sem_ids* ids) {
	if(semop(ids->shm_output, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_output_down");
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_output_up(struct sem_ids* ids) {
    if(semop(ids->shm_output, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_output_up");
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_shm_memory_table_down(struct sem_ids* ids) {
	if(semop(ids->shm_memory_table, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_memory_table_down");
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_shm_memory_table_up(struct sem_ids* ids) {
    if(semop(ids->shm_memory_table, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_shm_memory_table_up");
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_storage_table_down(struct sem_ids* ids) {
	if(semop(ids->storage_table, &p, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_storage_table_down");
        killpg(getpgrp(), SIGABRT);
    }
}

void sem_storage_table_up(struct sem_ids* ids) {
    if(semop(ids->storage_table, &v, 1) == -1) {
        LOG(LOG_LEVEL_ERROR, "sem_storage_table_up");
        killpg(getpgrp(), SIGABRT);
    } 
}

void sem_ids_destroy(struct sem_ids* ids){
	int status = 1;
	if(semctl(ids->shm_input, 0, IPC_RMID, 0) == -1) status = -1;
	if(semctl(ids->shm_output, 0, IPC_RMID, 0) == -1) status = -1;
	if(semctl(ids->shm_memory_table, 0, IPC_RMID, 0) == -1) status = -1;
	if(semctl(ids->storage_table, 0, IPC_RMID, 0) == -1) status = -1;
	if(status == -1){
		LOG(LOG_LEVEL_ERROR, "sem_ids_destroy");
	}
	free(ids);
}
