#ifndef SHM_DATABASE_H
#define SHM_DATABASE_H

#include "payload/record.h"
#include "semaphore.h"
#include "payload/merge_res.h"

struct database;
struct database* database_create(struct sem_ids* ids);
void memory_table_persist_data(struct database* db);
void database_destroy(struct database* db);
int memory_table_is_full(struct database* db);
void memory_table_put(struct database* db, struct record* r);
int database_get(struct database* db, char key[KEY_DIGIT + 1], struct record* res);
int storage_table_is_full(struct database* db);
int storage_table_can_merge(struct database* db);
void storage_table_merge(struct database* db, struct merge_res* res);

#endif // SHM_DATABASE_H
