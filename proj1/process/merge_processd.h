#ifndef MERGE_PROCESSD_H
#define MERGE_PROCESSD_H

#include "../ipc/shm_database.h"
#include "../ipc/msgq.h"

void merge_processd(struct database* db, struct bidir_message_queue* msgq);

#endif //MERGE_PROCESSD_H
