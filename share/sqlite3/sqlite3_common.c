#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

#include "sqlite3_common.h"

/* sqlite3_open 不会创建父目录，这里逐级创建数据库所在目录，等同 mkdir -p */
static int _sqlite_mkdir_p(const char *path)
{
    char tmp[256];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(tmp))
        return -1;

    memcpy(tmp, path, len + 1);
    /* 去掉末尾的 '/' */
    while (len > 0 && tmp[len - 1] == '/')
        tmp[--len] = '\0';
    if (len == 0)
        return 0;

    /* 去掉文件名，仅保留目录部分 */
    char *slash = strrchr(tmp, '/');
    if (slash == NULL || slash == tmp)
        return 0;
    *slash = '\0';

    /* 逐级创建目录，重复调用无副作用 */
    for (char *p = tmp + 1; *p != '\0'; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (access(tmp, F_OK) != 0)
                mkdir(tmp, 0777);
            *p = '/';
        }
    }
    if (access(tmp, F_OK) != 0)
        mkdir(tmp, 0777);
    if (access(tmp, F_OK) != 0)
    {
        printf("[%s] mkdir %s failed(%s)\n", __func__, tmp, strerror(errno));
        return -1;
    }
    return 0;
}
typedef struct {
    sqlite3* db;
    sqlite3_stmt* insert_stmt;
    sqlite3_stmt* update_stmt;
    sqlite3_stmt* query_stmt;
    sqlite3_stmt* delete_stmt;
} sqlite_pararm_contxt;

sqlite_context sqlite_table_open(const char* database_path, const char* table_name) {
    char* err_msg = NULL;
    sqlite_pararm_contxt* ctx = malloc(sizeof(sqlite_pararm_contxt));
    memset(ctx, 0, sizeof(sqlite_pararm_contxt));

    /* sqlite3_open 不会创建父目录，先确保数据库所在目录已存在 */
    _sqlite_mkdir_p(database_path);

    int ret = sqlite3_open(database_path, &ctx->db);
    if (ret != SQLITE_OK) {
        printf("[%s:%d] sqlite3 open %s failed\n", __func__, __LINE__, database_path);
        sqlite3_close(ctx->db);
        free(ctx);
        return NULL;
    }

    sqlite3_exec(ctx->db, "BEGIN TRANSACTION;", 0, 0, 0);
#if 1
    if (sqlite3_exec(ctx->db, "PRAGMA cache_size = 10000000;", 0, 0, &err_msg) != SQLITE_OK) {
        printf("[%s:%d] create %s table (%s )failed(%s)\n", __func__, __LINE__, database_path, table_name, err_msg);
        sqlite3_close(ctx->db);
        sqlite3_free(err_msg);
        return NULL;
    }

    if (sqlite3_exec(ctx->db, "PRAGMA page_size = 1000000;", 0, 0, &err_msg) != SQLITE_OK) {
        printf("[%s:%d] create %s table (%s )failed(%s)\n", __func__, __LINE__, database_path, table_name, err_msg);
        sqlite3_close(ctx->db);
        sqlite3_free(err_msg);
        return NULL;
    }
#endif
    return ctx;
}

/*打开/创建一个数据表单*/
sqlite_context sqlite_table_create(const char* database_path, const char* table_name, const char* param) {
    char* err_msg = 0;
    sqlite_pararm_contxt* ctx = sqlite_table_open(database_path, table_name);
    if (ctx == NULL) {
        printf("[%s:%d] ctx table open failed\n", __func__, __LINE__);
        return NULL;
    }

    char sql_create_table[512] = { 0 };
    snprintf(sql_create_table, sizeof(sql_create_table) - 1, "CREATE TABLE IF NOT EXISTS %s (%s);", table_name, param);
    if (sqlite3_exec(ctx->db, sql_create_table, 0, 0, &err_msg) != SQLITE_OK) {
        printf("[%s:%d] create %s table (%s )failed(%s)\n", __func__, __LINE__, table_name, param, err_msg);
        sqlite3_close(ctx->db);
        sqlite3_free(err_msg);
        return NULL;
    }

    return (sqlite_context)ctx;
}

/*创建索引*/
int sqlite_table_index_create(sqlite_context context, const char* table_name, const char* index_name, const char* param) {
    char* err_msg = 0;
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }

    char sql_create_index_string[512] = { 0 };
    snprintf(sql_create_index_string, sizeof(sql_create_index_string) - 1, "CREATE INDEX %s on %s (%s);", index_name, table_name, param);

    if (sqlite3_exec(ctx->db, sql_create_index_string, 0, 0, &err_msg) != SQLITE_OK) {
        printf("[%s:%d] create index %s on table %s param:%s failed(%s)\n", __func__, __LINE__, index_name, table_name, param, err_msg);
        sqlite3_close(ctx->db);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

/*判断表是否存在*/
int sqlite_table_exist(sqlite_context context, const char* table_name) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }

    sqlite3_stmt* stmt = NULL;
    char sql[128] = { 0 };
    snprintf(sql, sizeof(sql) - 1, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table_name);
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        if (stmt != NULL) {
            sqlite3_finalize(stmt);
        }
        return 0;
    }
    int exist = (sqlite3_step(stmt) == SQLITE_ROW) ? 1 : 0;
    sqlite3_finalize(stmt);
    return exist;
}

static int _sqlite_table_prepare(sqlite_context context, sqlite3_stmt** stmt, const char* param) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;

    if ((ctx == NULL) || (ctx->db == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }

    if (*stmt != NULL) {
        sqlite3_finalize(*stmt);
    }
    //  printf("%s\n", param);
    if (sqlite3_prepare_v2(ctx->db, param, -1, stmt, 0) != SQLITE_OK) {
        printf("[%s:%d] sqlit3 exec failed:%s\n", __func__, __LINE__, sqlite3_errmsg(ctx->db));
        return -1;
    }
    return 0;
}
/*准备向数据库中的数据表单插入数据*/
int sqlite_table_insert_prepare(sqlite_context context, const char* table_name, const char* param, const char* value) {
    char string[1024] = { 0 };
    snprintf(string, sizeof(string) - 1, "INSERT INTO %s (%s) VALUES (%s);", table_name, param, value);
    return _sqlite_table_prepare(context, &((sqlite_pararm_contxt*)context)->insert_stmt, string);
}
/* 更新预处理*/
int sqlite_table_update_prepare(sqlite_context context, const char* table_name, const char* where, const char* value) {
    char string[1024] = { 0 };
    snprintf(string, sizeof(string) - 1, "UPDATE %s SET %s WHERE %s;", table_name, value, where);
    return _sqlite_table_prepare(context, &((sqlite_pararm_contxt*)context)->update_stmt, string);
}
/*查询预处理*/
int sqlite_table_query_prepare(sqlite_context context, const char* select, const char* table_name, const char* where, const char* order, const char* limit, const char* offset) {
    char string[1024] = { 0 };
    snprintf(string, sizeof(string) - 1, "SELECT %s FROM %s WHERE %s ORDER BY %s LIMIT %s OFFSET %s;", select, table_name, where, order, limit, offset);
    return _sqlite_table_prepare(context, &((sqlite_pararm_contxt*)context)->query_stmt, string);
}
/*删除预处理*/
int sqlite_table_delete_prepare(sqlite_context context, const char* table_name, const char* where) {
    char string[1024] = { 0 };
    snprintf(string, sizeof(string) - 1, "DELETE FROM %s WHERE %s;", table_name, where);
    return _sqlite_table_prepare(context, &((sqlite_pararm_contxt*)context)->delete_stmt, string);
}
static int _sqlite_table_ready(sqlite_context context, sqlite3_stmt* stmt) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL) || (stmt == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }
    sqlite3_reset(stmt);
    return 0;
}
/*插入准备*/
int sqlite_table_insert_ready(sqlite_context context) {
    return _sqlite_table_ready(context, ((sqlite_pararm_contxt*)context)->insert_stmt);
}
/*更新准备*/
int sqlite_table_update_ready(sqlite_context context) {
    return _sqlite_table_ready(context, ((sqlite_pararm_contxt*)context)->update_stmt);
}
/*准备查询*/
int sqlite_table_query_ready(sqlite_context context) {
    return _sqlite_table_ready(context, ((sqlite_pararm_contxt*)context)->query_stmt);
}
/*准备删除*/
int sqlite_table_delete_ready(sqlite_context context) {
    return _sqlite_table_ready(context, ((sqlite_pararm_contxt*)context)->delete_stmt);
}

static int _sqlite_table_bind_int64(sqlite_context context, sqlite3_stmt* stmt, int id, sqlite_int64 value) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL) || (stmt == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }
    if (sqlite3_bind_int64(stmt, id, value) != SQLITE_OK) {
        printf("[%s:%d] sqlit3 bind failed:%s\n", __func__, __LINE__, sqlite3_errmsg(ctx->db));
        return -1;
    }
    return 0;
}

/*插入一个数据*/
int sqlite_table_insert_int64(sqlite_context context, int id, sqlite_int64 value) {
    return _sqlite_table_bind_int64(context, ((sqlite_pararm_contxt*)context)->insert_stmt, id, value);
}
/*更新一个数据*/
int sqlite_table_update_int64(sqlite_context context, int id, sqlite_int64 value) {
    return _sqlite_table_bind_int64(context, ((sqlite_pararm_contxt*)context)->update_stmt, id, value);
}
/*查询一个数据*/
int sqlite_table_query_int64(sqlite_context context, int id, sqlite_int64 value) {
    return _sqlite_table_bind_int64(context, ((sqlite_pararm_contxt*)context)->query_stmt, id, value);
}

/*删除数据*/
int sqlite_table_delete_int64(sqlite_context context, int id, sqlite_int64 value) {
    return _sqlite_table_bind_int64(context, ((sqlite_pararm_contxt*)context)->delete_stmt, id, value);
}
static int _sqlite_table_commit(sqlite_context context, sqlite3_stmt* stmt, void (*callback)(void*, void*, int, int), void* userdata, int base, int total) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL) || (stmt == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }
    int count = 0;
    int ret = 0;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (callback != NULL) {
            callback(stmt, userdata, base, total);
        }
        count++;
    }
    return count ? count : ret == SQLITE_DONE ? 0 : -1;
}

/*提交插入的数据到数据库*/
int sqlite_table_insert_commit(sqlite_context context) {
    return _sqlite_table_commit(context, ((sqlite_pararm_contxt*)context)->insert_stmt, NULL, NULL, 0, 0);
}
/*提交更新的数据到数据库*/
int sqlite_table_update_commit(sqlite_context context) {
    return _sqlite_table_commit(context, ((sqlite_pararm_contxt*)context)->update_stmt, NULL, NULL, 0, 0);
}
/*提交更新的数据到数据库*/
int sqlite_table_query_commit(sqlite_context context, void (*callback)(void*, void*, int, int), void* userdata, int base, int total) {
    return _sqlite_table_commit(context, ((sqlite_pararm_contxt*)context)->query_stmt, callback, userdata, base, total);
}
/*删除更新的数据到数据库(不是读指令，返回结果应该是SQLIT_DONE)*/
int sqlite_table_delete_commit(sqlite_context context) {
    return _sqlite_table_commit(context, ((sqlite_pararm_contxt*)context)->delete_stmt, NULL, NULL, 0, 0);
}
int sqlite_table_exec(sqlite_context context, const char *sql) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL)) {
        return -1;
    }
    return sqlite3_exec(ctx->db, sql, 0, 0, 0);
}
/*同步数据库*/
int sqlite_table_sync_commit(sqlite_context context) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }
    return sqlite3_exec(ctx->db, "COMMIT;", 0, 0, 0);
}
/*关闭数据库*/
int sqlite_table_close(sqlite_context context) {
    sqlite_pararm_contxt* ctx = (sqlite_pararm_contxt*)context;
    if ((ctx == NULL) || (ctx->db == NULL)) {
        printf("[%s:%d] ctx pararm is null\n", __func__, __LINE__);
        return -1;
    }
    sqlite3_exec(ctx->db, "COMMIT;", 0, 0, 0);

    if (ctx->insert_stmt != NULL) {
        sqlite3_finalize(ctx->insert_stmt);
        ctx->insert_stmt = NULL;
    }
    if (ctx->update_stmt != NULL) {
        sqlite3_finalize(ctx->update_stmt);
        ctx->update_stmt = NULL;
    }
    if (ctx->query_stmt != NULL) {
        sqlite3_finalize(ctx->query_stmt);
        ctx->query_stmt = NULL;
    }
    if (ctx->delete_stmt != NULL) {
        sqlite3_finalize(ctx->delete_stmt);
        ctx->delete_stmt = NULL;
    }

    sqlite3_close(ctx->db);
    ctx->db = NULL;
    free(context);
    return -1;
}
