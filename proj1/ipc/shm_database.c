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
#include "shm_database.h"
#include "../common/logging.h"

#define MEMORY_TABLE_MAX 3
#define STORAGE_TABLE_MAX 3
#define STORAGE_TABLE_CAN_MERGE 2
#define RECORDS_MAX 10000 // the st file size should be around 10KB 

static void merge(FILE* st_fp, FILE* st_meta_fp, struct record res[RECORDS_MAX], long long* res_size);

struct database{
	int shm_db_id;
	int memory_table_count;
	struct sem_ids* sem;

	// metadata
	int last_filename;
	long long last_pk;
	int file_count;
	
	// memory table
	struct record memory_table[MEMORY_TABLE_MAX];
};

/* returns NULL if creation fails */
struct database* database_create(struct sem_ids* ids) {
	FILE* fp = fopen("st_files/db.meta", "r");
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
		shmctl(id, IPC_RMID, 0);
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
void memory_table_persist_data(struct database* db){
	sem_shm_database_down(db->sem);

	// save memory table records to new st file
	char filename_st[30];
	char filename_st_meta[30];
	FILE* st_fp, *st_meta_fp, *db_meta_fp;
	int i;
	if(db->memory_table_count != 0){
		sprintf(filename_st, "st_files/%d.st", ++db->last_filename);
		sprintf(filename_st_meta, "st_files/%d.meta", db->last_filename);
		st_fp = fopen(filename_st, "w");
		st_meta_fp = fopen(filename_st_meta, "w");
		if(st_fp == NULL || st_meta_fp == NULL){
			LOG(LOG_LEVEL_ERROR, "memory_table_persist_data fopen: %d", errno);
			sem_shm_database_up(db->sem);
			return;
		}
	
		for(i = 0; i < db->memory_table_count; ++i){
			fprintf(st_fp, "%lld %s %s\n", db->memory_table[i].pk, db->memory_table[i].key, db->memory_table[i].val);
			fprintf(st_meta_fp, "%lld\n", db->memory_table[i].created_at);
			LOG(LOG_LEVEL_DEBUG, "[%d.st] %d record saved: <%lld, %s, %s>, <%lld>", db->last_filename, i, db->memory_table[i].pk, db->memory_table[i].key, db->memory_table[i].val, db->memory_table[i].created_at);
		}
		fclose(st_fp);
		fclose(st_meta_fp);
		
		++db->file_count;
	}

	// update db.meta
	db_meta_fp = fopen("st_files/db.meta", "w");
	if(db_meta_fp == NULL){
		LOG(LOG_LEVEL_ERROR, "memory_table_persist_data fopen: %d", errno);
		sem_shm_database_up(db->sem);
		return;
	}

	fprintf(db_meta_fp, "%d %lld %d ", db->last_filename, db->last_pk, db->file_count);
	fclose(db_meta_fp);

	// initilize memory table
	db->memory_table_count = 0;
	memset(db->memory_table, 0, sizeof(db->memory_table));

	sem_shm_database_up(db->sem);
}

/* fork-safe function */
int memory_table_is_full(struct database* db){
	sem_shm_database_down(db->sem);
	int ret = (db->memory_table_count == MEMORY_TABLE_MAX);
	sem_shm_database_up(db->sem);
	return ret;
}

/* fork-safe function.
 * if PUT success, then r->pk and r->created_at will be initilized. */
void memory_table_put(struct database* db, struct record* r){
	sem_shm_database_down(db->sem);

	if(db->memory_table_count >= MEMORY_TABLE_MAX){
		LOG(LOG_LEVEL_ERROR, "database_put");
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	int idx = db->memory_table_count++;
	r->pk = ++db->last_pk;
	r->created_at = db->last_pk;
	
	memcpy(&db->memory_table[idx], r, sizeof(struct record));
	
	sem_shm_database_up(db->sem);
}

/* fork-safe function.
 * if key exists, then res will be initilized.
 * return value: 1 if found, 0 if not found */
int database_get(struct database* db, char key[KEY_DIGIT +1], struct record* res){
	sem_shm_database_down(db->sem);
	
	// query the memory table
	int i;
	for(i = db->memory_table_count - 1; i >= 0; --i){
		if(strcmp(key, db->memory_table[i].key) == 0){
			memcpy(res, &db->memory_table[i], sizeof(struct record));
			sem_shm_database_up(db->sem);
			return 1;
		}
	}

	// query the storage table
	FILE* st_fp, *st_meta_fp;
	char st_filename[30], st_meta_filename[30];
	char st_line[KEY_DIGIT + VAL_MAX_LEN + 30], st_meta_line[KEY_DIGIT + VAL_MAX_LEN + 30];
	struct record r;
	int found = 0; // false
	int f;
	int first_filename = db->last_filename - db->file_count + 1;
	for(f = db->last_filename; f >= first_filename; --f){
		sprintf(st_filename, "st_files/%d.st", f);
		sprintf(st_meta_filename, "st_files/%d.meta", f);
		st_fp = fopen(st_filename, "r");
		st_meta_fp = fopen(st_meta_filename, "r");
		if(st_fp == NULL || st_meta_fp == NULL) {
			LOG(LOG_LEVEL_DEBUG, "%d.st not found", f);
			sem_shm_database_up(db->sem);
			killpg(getpgrp(), SIGABRT);
		}
		LOG(LOG_LEVEL_DEBUG, "opened the file %d.st", f);

		while(fgets(st_line, sizeof(st_line), st_fp) && fgets(st_meta_line, sizeof(st_meta_line), st_meta_fp)){
			if(sscanf(st_line, "%lld %s %s", &r.pk, r.key, r.val) == 3 && sscanf(st_meta_line, "%lld", &r.created_at) == 1){
				LOG(LOG_LEVEL_DEBUG, "[%d.st] record: <%lld, %s, %s>, <%lld>", f, r.pk, r.key, r.val, r.created_at);
				if(strcmp(key, r.key) == 0){
					if(!found){
						found = 1;
						memcpy(res, &r, sizeof(struct record));
						continue;
					}
					// store the most recent value.
					if(res->created_at <= r.created_at){
						memcpy(res, &r, sizeof(struct record));
					}
				}
			}
		}
	}

	sem_shm_database_up(db->sem);
	return found;
}

/* fork-safe function */
int storage_table_is_full(struct database* db){
	sem_shm_database_down(db->sem);
	int ret = (db->file_count == STORAGE_TABLE_MAX);
	sem_shm_database_up(db->sem);
	return ret;
}

/* fork-safe function */
int storage_table_can_merge(struct database* db){
	sem_shm_database_down(db->sem);
	int ret = (db->file_count == STORAGE_TABLE_CAN_MERGE);
	sem_shm_database_up(db->sem);
	return ret;
}

/* read records from fp and save to res[RECORDS_MAX].
 * res_size is initialized to the actual data size of res[RECORDS_MAX]. */
static void merge(FILE* st_fp, FILE* st_meta_fp,struct record res[RECORDS_MAX], long long* res_size){
	long long i;
	struct record r;
	char st_line[KEY_DIGIT + VAL_MAX_LEN + 30];
	char st_meta_line[KEY_DIGIT + VAL_MAX_LEN + 30];
	while(fgets(st_line, sizeof(st_line), st_fp) && fgets(st_meta_line, sizeof(st_meta_line), st_meta_fp)){
		if(sscanf(st_line, "%lld %s %s", &r.pk, r.key, r.val) == 3 && sscanf(st_meta_line, "%lld", &r.created_at) == 1){
			int found = 0;
			for(i = 0; i < *res_size; ++i){
				if(strcmp(res[i].key, r.key) == 0){
					found = 1;
					// store the most recent value.
					if(res[i].created_at <= r.created_at)
						memcpy(&res[i], &r, sizeof(struct record));
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
	
	if(db->file_count < STORAGE_TABLE_CAN_MERGE || db->file_count > STORAGE_TABLE_MAX){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge storage_table size: %d", db->file_count);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	// open two oldest st files
	FILE* oldest1_st, *oldest2_st, *oldest1_st_meta, *oldest2_st_meta;
	char oldest1_st_filename[30], oldest2_st_filename[30], oldest1_st_meta_filename[30], oldest2_st_meta_filename[30];
	int first_filename = db->last_filename - db->file_count + 1;
	int oldest1 = first_filename;
	int oldest2 = first_filename + 1;
	sprintf(oldest1_st_filename, "st_files/%d.st", oldest1);
	sprintf(oldest2_st_filename, "st_files/%d.st", oldest2);
	sprintf(oldest1_st_meta_filename, "st_files/%d.meta", oldest1);
	sprintf(oldest2_st_meta_filename, "st_files/%d.meta", oldest2);
	oldest1_st = fopen(oldest1_st_filename, "r");
	oldest2_st = fopen(oldest2_st_filename, "r");
	oldest1_st_meta = fopen(oldest1_st_meta_filename, "r");
	oldest2_st_meta = fopen(oldest2_st_meta_filename, "r");
	if(oldest1_st == NULL || oldest2_st == NULL || oldest1_st_meta == NULL || oldest2_st_meta == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge: file %d %d not found", oldest1, oldest2);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}

	// merge records and sort
	struct record records[RECORDS_MAX];
	long long records_size = 0;
	merge(oldest1_st, oldest1_st_meta, records, &records_size);
	merge(oldest2_st, oldest2_st_meta, records, &records_size);
	qsort(records, records_size, sizeof(struct record), compare_by_key);

	// save to a new st file
	FILE* st_fp, *st_meta_fp, *db_meta_fp;
	char st_filename[30], st_meta_filename[30];
	long long i;
	sprintf(st_filename, "st_files/%d.st", ++db->last_filename);
	sprintf(st_meta_filename, "st_files/%d.meta", db->last_filename);
	st_fp = fopen(st_filename, "w");
	st_meta_fp = fopen(st_meta_filename, "w");
	if(st_fp == NULL || st_meta_fp == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge: %d", errno);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}
	for(i = 0; i < records_size; ++i){
		fprintf(st_fp, "%lld %s %s\n", i + 1, records[i].key, records[i].val);
		fprintf(st_meta_fp, "%lld\n", records[i].created_at);
	}
	fclose(oldest1_st);
	fclose(oldest2_st);
	fclose(oldest1_st_meta);
	fclose(oldest2_st_meta);
	fclose(st_fp);
	fclose(st_meta_fp);

	// remove two oldest st files
	remove(oldest1_st_filename);
	remove(oldest2_st_filename);
	remove(oldest1_st_meta_filename);
	remove(oldest2_st_meta_filename);
	--db->file_count;

	// update metadata
	db_meta_fp = fopen("st_files/db.meta", "w");	
	if(db_meta_fp == NULL){
		LOG(LOG_LEVEL_ERROR, "storage_table_merge: %d", errno);
		sem_shm_database_up(db->sem);
		killpg(getpgrp(), SIGABRT);
	}
	fprintf(db_meta_fp, "%d %lld %d", db->last_filename, db->last_pk, db->file_count);
	fclose(db_meta_fp);

	// set result
	res->st_name = db->last_filename;
	res->cnt = records_size;

	sem_shm_database_up(db->sem);
}
