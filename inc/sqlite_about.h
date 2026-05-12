#ifndef __SQLITE_ABOUT_H
#define __SQLITE_ABOUT_H
#include "common.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdint.h>
void DB_Init(sqlite3* db);
uint8_t get_db_count(sqlite3* db, uint16_t* out_count);
uint8_t delete_db_msg(sqlite3* db, uint16_t delete_num);
void db_save_batch(sqlite3* db, void* buffer);
#endif //__SQLITE_ABOUT_H