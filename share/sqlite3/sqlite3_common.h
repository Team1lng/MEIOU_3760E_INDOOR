#ifndef _SQLITE3_COMMON_H_
#define _SQLITE3_COMMON_H_
#include "sqlite3.h"

typedef void *sqlite_context;

/*打开一个数据库表*/
sqlite_context sqlite_table_open(const char *database_path, const char *table_name);

/*创建一个数据库表*/
sqlite_context sqlite_table_create(const char *database_path, const char *table_name, const char *param);

/*创建索引*/
int sqlite_table_index_create(sqlite_context handle, const char *table_name, const char *index_name, const char *param);

/*判断表是否存在*/
int sqlite_table_exist(sqlite_context handle, const char *table_name);

/*插入预处理*/
int sqlite_table_insert_prepare(sqlite_context handle, const char *table_name, const char *param, const char *value);

/* 更新预处理*/
int sqlite_table_update_prepare(sqlite_context handle, const char *table_name, const char *where, const char *value);

/*查询预处理*/
int sqlite_table_query_prepare(sqlite_context handle, const char *select, const char *table_name, const char *where, const char *order, const char *limit, const char *offset);

/*删除预处理*/
int sqlite_table_delete_prepare(sqlite_context handle, const char *table_name, const char *where);

/*插入准备*/
int sqlite_table_insert_ready(sqlite_context handle);

/*更新准备*/
int sqlite_table_update_ready(sqlite_context handle);

/*查询查询*/
int sqlite_table_query_ready(sqlite_context handle);

/*删除准备*/
int sqlite_table_delete_ready(sqlite_context handle);

/*插入数据*/
int sqlite_table_insert_int64(sqlite_context handle, int id, sqlite_int64 value);

/*更新数据*/
int sqlite_table_update_int64(sqlite_context handle, int id, sqlite_int64 value);

/*查询数据*/
int sqlite_table_query_int64(sqlite_context handle, int id, sqlite_int64 value);

/*删除数据*/
int sqlite_table_delete_int64(sqlite_context handle, int id, sqlite_int64 value);

/*提交插入的数据到数据库*/
int sqlite_table_insert_commit(sqlite_context handle);

/*提交更新的数据到数据库*/
int sqlite_table_update_commit(sqlite_context handle);

/*提交更新的数据到数据库*/
int sqlite_table_query_commit(sqlite_context handle, void (*callback)(void *, void *, int, int), void *userdata, int base, int total);

/*删除更新的数据到数据库*/
int sqlite_table_delete_commit(sqlite_context handle);

/*执行原始SQL*/
int sqlite_table_exec(sqlite_context context, const char *sql);
/*同步数据库*/
int sqlite_table_sync_commit(sqlite_context context);

/*关闭数据库*/
int sqlite_table_close(sqlite_context handle);
#endif