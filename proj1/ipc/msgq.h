#ifndef MSGQ_H
#define MSGQ_H

#include "shm_database.h"

struct bidir_message_queue;

struct bidir_message_queue* bidir_message_queue_create();
void bidir_message_queue_destroy(struct bidir_message_queue*);
void bidir_message_queue_send_merge_req(struct bidir_message_queue* msgq, struct merge_res* res);
void bidir_message_queue_on_message_merge(struct bidir_message_queue* msgq, struct database* db);

#endif // MSGQ_H
