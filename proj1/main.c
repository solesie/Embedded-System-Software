#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "./ipc/semaphore.h"
#include "./ipc/shm_database.h"
#include "./ipc/msgq.h"
#include "./util/logging.h"

/* root process */
int main(void){
	struct sem_ids* ids = sem_ids_create();
	if(ids == NULL) {
		exit(1);
	}
	struct database* db = database_create(ids);
	if(ids == NULL) {
		sem_ids_destroy(ids);
		exit(1);
	}
	struct bidir_message_queue* msgq = bidir_message_queue_create();
	if(msgq == NULL) {
		sem_ids_destroy(ids);
		database_destroy(db);
		exit(1);
	}
	
	int parent_pid, main_pid, io_pid, merge_pid;
	int status;
	// create parent process
	if((parent_pid = fork()) == 0){
		// set parent process as new process group
		setpgid(0, 0);

		// create main process
		if((main_pid = fork()) == 0){
			// main_process();
			return 0;
		}

		// create io process
		if((io_pid = fork()) == 0){
			// io_process();
			return 0;
		}

		// create merge process
		if((merge_pid = fork()) == 0){
			// merge_process();
			return 0;
		}

		// wait for normal termination
		waitpid(io_pid, &status, 0);
		kill(main_pid, SIGTERM);
		kill(merge_pid, SIGTERM);
		while(wait(NULL) > 0);
		persist_data(db);
		return 0;
	}

	waitpid(parent_pid, &status, 0);
	sem_ids_destroy(ids);
	database_destroy(db);
	
	return 0;
}
