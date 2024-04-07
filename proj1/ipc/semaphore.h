#ifndef SEMAPHORE_H
#define SEMAPHORE_H

struct sem_ids;

struct sem_ids* sem_ids_create();
void sem_ids_destroy(struct sem_ids* ids);

void sem_shm_input_down(struct sem_ids* ids);
void sem_shm_input_up(struct sem_ids* ids);
void sem_shm_output_down(struct sem_ids* ids);
void sem_shm_output_up(struct sem_ids* ids); 
void sem_shm_database_down(struct sem_ids* ids); 
void sem_shm_database_up(struct sem_ids* ids);

#endif // SEMAPHORE_H
