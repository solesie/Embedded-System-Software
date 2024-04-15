#include <unistd.h>
#include <stdio.h>
#include "merge_processd.h"

void merge_processd(struct database* db, struct bidir_message_queue* msgq){
	while(1){
		bidir_message_queue_on_message_merge(msgq, db);
	}
}
