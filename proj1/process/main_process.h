#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include "../ipc/semaphore.h"
#include "../ipc/shm_database.h"
#include "../ipc/shm_io.h"

void main_process(struct sem_ids* sem, struct database* db, struct shm_io* ipc_io);

#endif // MAIN_PROCESS_H
