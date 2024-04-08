#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "ipc_keys.h"
#include "semaphore.h"
#include "./payload/record.h"
#include "./payload/merge_res.h"
#include "../util/logging.h"

#define MEMORY_TABLE_MAX 3
#define STORAGE_TABLE_MAX 3
#define RECORDS_MAX 10000 // the st file size should be around 10KB 

static void merge(FILE* fp, struct record res[RECORDS_MAX], long long* res_size);

struct database{
	int shm_db_id;
	int memory_table_count;
	struct sem_ids* sem;

	/* metadata */
	int last_filename;
	long long last_pk;
	int file_count;
	
	/* memory table */
	struct record memory_table[MEMORY_TABLE_MAX];
};

/* returns NULL if creation fails */
struct database* database_create(struct sem_ids* ids) {
	FILE* fp = fopen("st_files/metadata.st", "r");
	if(fp == NULL){
		LOG(LOG_LEVEL_ERROR, "database_create fopen: %d", errno);
		return NULL;
	}

	int id = shmget(KEY_T_SHM_DB, sizeof(struct database), IPC_CREAT | IPC_EXCL | 0666);
	if(id == -1){
		LOG(LOG_LEVEL_ERROR, "database_create shmget: %d", errno);
		return NULL;
	}

	struct database* ret = (struct database*)shmat(id, 0, 0);
	if(ret == (void*)-1) {
		LOG(LOG_LEVEL_ERROR, "database_create shmat: %d", errno);
		return NULL;
	}
	memset(ret, 0, sizeof(struct database));
	ret->shm_db_id = id;
	ret->memory_table_count = 0;
	ret->sem = ids;
	fscanf(fp, "%d %lld %d", &ret->last_filename, &ret->last_pk, &ret->file_count);
	fclose(fp);
	
	return ret;
}

/* fork-safe function */
void persist_data(struct database* db){
	sem_shm_database_down(db->sem);

	// save memory table records to new st file
	char filename[30];
	FILE* fp;
	int i;
	if(db->memory_table_count != 0){	
		sprintf(filename, "st_files/%d.st", ++db->last_filename);
		fp = fopen(filename, "w");
		if(fp == NULL){
			LOG(LOG_LEVEL_ERROR, "persist_data fopen: %d", errno);
			sem_shm_database_up(db->sem);
			return;
		}
	
		for(i = 0; i < db->memory_table_count; ++i){
			fprintf(fp, "%lld %s %s\n", db->memory_table[i].pk, db->memory_table[i].key, db->memory_table[i].val);
			LOG(LOG_LEVEL_DEBUG, "[%d.st] %d record saved: <%lld, %s, %s>", db->last_filename, i, db->memory_table[i].pk, db->memory_table[i].key, db->memory_table[i].val);
		}
		fclose(fp);	
		
		++db->file_count;
	}

	// update metadata.st
	fp = fopen("st_files/metadata.st", "w");
	if(fp == NULL){
		LOG(LOG_LEVEL_ERROR, "persist_data fopen: %d", errno);
		sem_shm_database_up(db->sem);
		return;
	}

	fprintf(fp, "%d %lld %d", db->last_filename, db->last_pk, db->file_count);
	fclose(fp);

	// initilize memory table
	db->memory_table_count = 0;
	memset(db->memory_table, 0, sizeof(db->memory_table));

	sem_shm_database_up(db->sem);
}

void database_destroy(struct database* db){
	int status = 1;
	int id = db->shm_db_id;
	if(shmdt(db) == -1) status = -1;
	if(shmctl(id, IPC_RMID, 0) == -1) status = -1;
	if(status == -1){ 
		LOG(LOG_LEVEL_ERROR, "database_destroy: %d", errno);
	}
}

/* fork-safe function */
int memory_table_is_full(struct database* db){
	sem_shm_database_down(db->sem);
	int ret = (db->memory_table_count == MEMORY_TABLE_MAX);
	sem_shm_database_up(db->sem);
	return ret;
}

/* fork-safe function.
 * if PUT success, then r->pk will be initilized. */
void memory_table_put(struct database* db, struct record* r){
	sem_shm_database_down(db->sem);

	if(db->memory_table_count >= MEMORY_TABLE_MAX){
		LOG(LOG_LEVEL_ERROR, "database_put");
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	int idx = db->memory_table_count++;
	r->pk = ++db->last_pk;
	
	memcpy(&db->memory_table[idx], r, sizeof(struct record));
	
	sem_shm_database_up(db->sem);
}

/* fork-safe function.
 * if key is not exist, then res will be NULL.
 * return value: 1 if found, 0 if not found */
int database_get(struct database* db, char key[KEY_DIGIT +1], struct record* res){
	sem_shm_database_down(db->sem);

	// query the memory table
	int i;
	for(i = 0; i < MEMORY_TABLE_MAX; ++i){
		if(strcmp(key, db->memory_table[i].key) == 0){
			memcpy(res, &db->memory_table[i], sizeof(struct record));
			return 1;
		}
	}

	//query the storage table
	FILE* fp;
	char filename[30];
	int record_count;
	struct record rcd;
	int file, r;
	for(file = db->last_filename; file != 0; --file){
		sprintf(filename, "st_files/%d.st", file);
		fp = NULL;
		fp = fopen(filename, "r");
		if(fp == NULL) {
			LOG(LOG_LEVEL_DEBUG, "%d.st not found", file);
			continue;
		}
		LOG(LOG_LEVEL_DEBUG, "opened the file %d.st", file);

		fscanf(fp, "%d", &record_count);
		LOG(LOG_LEVEL_DEBUG, "the number of records in %d.st is %d", file, record_count);

		for(r = record_count; r > 0; --r){
			fscanf(fp, "%lld %s %s", &rcd.pk, rcd.key, rcd.val);
			LOG(LOG_LEVEL_DEBUG, "[%d.st] %d record: <%lld, %s, %s>", file, record_count - r, rcd.pk, rcd.key, rcd.val);
			if(strcmp(key, rcd.key) == 0){
				memcpy(res, &rcd, sizeof(struct record));
				return 1;
			}
		}	
	}

	sem_shm_database_up(db->sem);
	return 0;
}

/* fork-safe function */
int storage_table_count(struct database* db){
	sem_shm_database_down(db->sem);
	int ret = db->file_count;
	sem_shm_database_up(db->sem);
	return ret;
}

static void merge(FILE* fp, struct record res[RECORDS_MAX], long long* res_size){
	long long i;
	struct record r;
	char line[KEY_DIGIT + VAL_MAX_LEN + 30];
	while(fgets(line, sizeof(line), fp)){
		if(sscanf(line, "%lld %s %s", &r.pk, r.key, r.val) == 3){
			int found = 0;
			for(i = 0; i < *res_size; ++i){
				if(strcmp(res[i].key, r.key) == 0){
					found = 1;
					if(res[i].pk < r.pk){
						memcpy(&res[i], &r, sizeof(struct record));
					}
					break;
				}
			}
			if(!found){
				memcpy(&res[*res_size], &r, sizeof(struct record));
				++(*res_size);
			}
		}
	}
}

/* fork-safe function.
 * if MERGE success, then merge_res will be initilized. 
 * if MERGE fail, then parent process group will be aborted*/
void storage_table_merge(struct database* db, struct merge_res* res){
	sem_shm_database_down(db->sem);
	
	if(db->file_count != 3){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge");
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	// open two oldest st files
	FILE* oldest1_fp, * oldest2_fp;
	char oldest1_filename[30], oldest2_filename[30];
	int oldest1 = db->last_filename - 2;
	int oldest2 = db->last_filename - 1;
	sprintf(oldest1_filename, "st_files/%d.st", oldest1);
	sprintf(oldest2_filename, "st_files/%d.st", oldest2);
	oldest1_fp = fopen(oldest1_filename, "r");
	oldest2_fp = fopen(oldest2_filename, "r");
	if(oldest1_fp == NULL || oldest2_fp == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge");
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	// merge records and sort
	struct record records[RECORDS_MAX];
	long long records_size = 0;
	merge(oldest1_fp, records, &records_size);
	merge(oldest2_fp, records, &records_size);
	qsort(records, records_size, sizeof(struct record), compare_by_key);

	// save to a new st file
	FILE* fp;
	char filename[30];
	long long i;
	sprintf(filename, "st_files/%d.st", ++db->last_filename);
	fp = fopen(filename, "w");
	if(fp == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge: %d", errno);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}
	for(i = 0; i < records_size; ++i){
		fprintf(fp, "%lld %s %s\n", ++db->last_pk, records[i].key, records[i].val);
	}
	fclose(oldest1_fp);
	fclose(oldest2_fp);
	fclose(fp);

	// remove two oldest st files
	remove(oldest1_filename);
	remove(oldest2_filename);
	--db->file_count;

	// update metadata
	fp = fopen("st_files/metadata.st", "w");	
	if(fp == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge: %d", errno);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}
	fprintf(fp, "%d %lld %d", db->last_filename, db->last_pk, db->file_count);
	fclose(fp);

	// set result
	res->st_name = db->last_filename;
	res->cnt = records_size;

	sem_shm_database_up(db->sem);
}
