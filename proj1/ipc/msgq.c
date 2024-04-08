#include <sys/ipc.h>
#include <sys/types.h> 
#include <sys/msg.h>  
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "ipc_keys.h"
#include "msgq.h"
#include "shm_database.h"
#include "./payload/record.h"
#include "./payload/msg.h"
#include "../util/logging.h"

struct bidir_message_queue {
	int msgq_io_2_merge;
	int msgq_merge_2_io;
};

/* returns NULL if creation fails */
struct bidir_message_queue* bidir_message_queue_create(){
	int id1 = msgget(KEY_T_MSGQ_IO_2_MERGE, IPC_CREAT | IPC_EXCL | 0666);
	int id2 = msgget(KEY_T_MSGQ_MERGE_2_IO, IPC_CREAT | IPC_EXCL | 0666);
	struct bidir_message_queue* ret = (struct bidir_message_queue*)malloc(sizeof(struct bidir_message_queue));
	if(id1 == -1 || id2 == -2){
		LOG(LOG_LEVEL_ERROR, "bidir_message_queue_create: %d", errno);
		bidir_message_queue_destroy(ret);
		return NULL;
	}
	ret->msgq_io_2_merge = id1;
	ret->msgq_merge_2_io = id2;
	return ret;
}

void bidir_message_queue_destroy(struct bidir_message_queue* msgq){
	int status = 1;
	if(msgctl(msgq->msgq_io_2_merge, IPC_RMID, 0) == -1) status = -1;
	if(msgctl(msgq->msgq_merge_2_io, IPC_RMID, 0) == -1) status = -1;
	if(status == -1){
		LOG(LOG_LEVEL_ERROR, "bidir_message_queue_destroy: %d", errno);
	}
	free(msgq);
}

/* waits until MERGE is completed. 
 * it must be called only by the io process.*/
void send_merge_req(struct bidir_message_queue* msgq){
	struct msgbuf msg1, msg2;
	msg1.mtype = 1;
	if(msgsnd(msgq->msgq_io_2_merge, (void*)&msg1, MSG_MAX_LEN, 0) == -1){
		LOG(LOG_LEVEL_ERROR, "send_merge_req: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
	if(msgrcv(msgq->msgq_merge_2_io, &msg2, MSG_MAX_LEN, 0, 0) == -1){
		LOG(LOG_LEVEL_ERROR, "send_merge_req: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
}

/* waits until MERGE request is made, then proceeds.
 * it must be called only by the merge process. */
void on_message_merge(struct bidir_message_queue* msgq, struct database* db){
	struct msgbuf msg1, msg2;
	msg2.mtype = 1;
	if(msgrcv(msgq->msgq_io_2_merge, &msg1, MSG_MAX_LEN, 0, 0) == -1){
		LOG(LOG_LEVEL_ERROR, "on_message_merge: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
	struct merge_res res;
	storage_table_merge(db, &res);
	LOG(LOG_LEVEL_DEBUG, "[BACKGROUND MERGE] new st name: %d, new st size: %ld", res.st_name, res.cnt);
	if(msgsnd(msgq->msgq_merge_2_io, (void*)&msg2, MSG_MAX_LEN, 0) == -1){
		LOG(LOG_LEVEL_ERROR, "on_message_merge: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
}
