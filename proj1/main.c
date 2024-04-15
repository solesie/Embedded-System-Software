#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "./ipc/semaphore.h"
#include "./ipc/shm_database.h"
#include "./ipc/shm_io.h"
#include "./ipc/msgq.h"
#include "./device/dev_ctrl.h"
#include "./process/io_process.h"
#include "./process/main_process.h"
#include "./process/merge_processd.h"
#include "./common/logging.h"

void root_sigint_handler(int sig){
	LOG(LOG_LEVEL_INFO, "to exit normally, must press the BACK button");
}

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
	struct shm_io* ipc_io = shm_io_create();
	if(ipc_io == NULL) {
		sem_ids_destroy(ids);
		database_destroy(db);
		bidir_message_queue_destroy(msgq);
		exit(1);
	}
	
	pid_t main_pid, io_pid, merge_pid;
	pid_t status;
	// create main process
	if((main_pid = fork()) == 0){
		// set main process as new process group
		setpgid(0, 0);

		// create io process
		if((io_pid = fork()) == 0){
			io_process(ids, db, ipc_io, msgq);
			return 0;
		}

		// create merge process
		if((merge_pid = fork()) == 0){
			merge_processd(db, msgq);
			return 0;
		}
		
		main_process(ids, db, ipc_io);
		
		// Normally terminated.
		// The main process waits until the IO process finishes the remaining tasks.
		// Only the merge process does not know when it should terminate.
		waitpid(io_pid, &status, 0);
		kill(merge_pid, SIGTERM);
		waitpid(merge_pid, &status, 0);
		return 0;
	}
	(void)signal(SIGINT, root_sigint_handler);
	waitpid(main_pid, &status, 0);
	// clean ipc resource
	sem_ids_destroy(ids);
	database_destroy(db);
	bidir_message_queue_destroy(msgq);
	shm_io_destroy(ipc_io);
	LOG(LOG_LEVEL_DEBUG, "ipc resource cleaned");
	
	return 0;
}
