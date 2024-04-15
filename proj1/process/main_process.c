#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "main_process.h"
#include "../common/logging.h"
#include "../common/mode.h"

void main_process(struct sem_ids* sem, struct database* db, struct shm_io* ipc_io){
	struct shm_input shmi;
	struct shm_output shmo;
	shmo.error = 0; // false
	while(1){
		// wait for the input of the io process
		sem_shm_input_down(sem);
		// read input of the io process
		shm_io_read_input(ipc_io, &shmi);

		if(shmi.terminate) {
			memory_table_persist_data(db);
			LOG(LOG_LEVEL_DEBUG, "main terminate");
			shm_io_write_output(ipc_io, &shmo);
			sem_shm_output_up(sem);
			return;
		}

		switch(shmi.m){

			case PUT:
				if(memory_table_is_full(db)){
					memory_table_persist_data(db);
				}

				memory_table_put(db, &shmi.r);
				shmo.error = 0;
				memcpy(&shmo.r, &shmi.r, sizeof(struct record));
				break;

			case GET:
				shmo.error = 0;
				if(!database_get(db, shmi.r.key, &shmo.r)){
					LOG(LOG_LEVEL_INFO, "key %s do not exist", shmi.r.key);
					shmo.error = 1; // key not found error
				}
				break;

			default:
				LOG(LOG_LEVEL_ERROR, "MERGE mode should be executed in merge process");
				break;
			
		}
		// write output to the io process
		shm_io_write_output(ipc_io, &shmo);
		// notify io process that the output has been written
		sem_shm_output_up(sem);
	}
}
