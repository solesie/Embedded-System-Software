#ifndef IO_PROCESS_H
#define IO_PROCESS_H

#include "../ipc/shm_io.h"
#include "../ipc/semaphore.h"
#include "../ipc/shm_database.h"
#include "../ipc/msgq.h"
#include "../device/dev_ctrl.h"

void io_process(struct sem_ids* sem, struct database* db, struct shm_io* ipc_io, struct bidir_message_queue* msgq);

#endif // IO_PROCESS_H
