
/**
 * @file        call_record.c
 * @brief       通话记录管理模块
 * @details     实现通话记录的添加、查询、删除、持久化存储、分页翻页等功能
 * @ingroup     call_record
 */
#include "call_core.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include "pthread.h"

#define CALL_RECORD_MAX_COUNT 255               /**< 最大通话记录数，循环覆盖 */
#define CALL_RECORD_FILE_NAME "call_record.dat" /**< 持久化存储文件名 */
#define CALL_RECORD_DIR_PERM 0755               /**< 目录权限 rwxr-xr-x */
#define CALL_RECORD_FILE_PERM 0644              /**< 文件权限 rw-r--r-- */
#define RECORD_PATH_MAX_LEN 64                  /**< 存储路径最大长度 */
#define FILE_PATH_MAX_LEN 128                   /**< 完整文件路径最大长度 */
#define INVALID_PAGE_INDEX 0xFFFFFFFF           /**< 无效页索引 */

/**
 * @struct call_record_ctx
 * @brief 通话记录模块上下文
 *
 * 管理记录列表、互斥锁、分页信息、存储路径等全局资源
 */
typedef struct
{
    volatile bool inited;                                     /**< 模块初始化标志 */
    pthread_mutex_t record_mutex;                             /**< 数据访问互斥锁 */
    bool enable;                                              /**< 功能使能标志 */
    uint32_t max_per_page;                                    /**< 每页最大记录数 */
    uint32_t cur_item_index;                                  /**< 当前页起始索引 */
    uint32_t record_count;                                    /**< 当前有效记录总数 */
    char record_path[RECORD_PATH_MAX_LEN];                    /**< 记录存储目录 */
    call_record_info_attr record_list[CALL_RECORD_MAX_COUNT]; /**< 通话记录数组 */
} call_record_ctx;

/**
 * @brief 模块全局上下文
 */
static call_record_ctx s_record_ctx = {
    .inited = false,
    .enable = false,
    .max_per_page = 0,
    .cur_item_index = 0,
    .record_count = 0,
    .record_path = {0}};

/**
 * @brief 检查模块是否已初始化
 *
 * @return 初始化结果
 * @retval true 已初始化
 * @retval false 未初始化
 */
static bool check_module_inited(void)
{
    if (!s_record_ctx.inited)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "module not initialized");
    }
    return s_record_ctx.inited;
}

/**
 * @brief 拼接记录文件完整路径
 *
 * @param[out] file_path 输出完整路径缓冲区
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int get_full_file_path(char *file_path)
{
    if (file_path == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "get_full_file_path failed: file_path is NULL");
        return -1;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    bool path_valid = (s_record_ctx.record_path[0] != '\0' &&
                       strlen(s_record_ctx.record_path) < RECORD_PATH_MAX_LEN);
    char temp_path[RECORD_PATH_MAX_LEN] = {0};
    if (path_valid)
    {
        strncpy(temp_path, s_record_ctx.record_path, RECORD_PATH_MAX_LEN - 1);
        temp_path[RECORD_PATH_MAX_LEN - 1] = '\0';
    }
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (!path_valid)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "get_full_file_path failed: path not set or invalid");
        return -1;
    }

    size_t temp_len = strlen(temp_path);
    if (temp_len > 0 && temp_path[temp_len - 1] == '/')
        snprintf(file_path, FILE_PATH_MAX_LEN, "%s%s", temp_path, CALL_RECORD_FILE_NAME);
    else
        snprintf(file_path, FILE_PATH_MAX_LEN, "%s/%s", temp_path, CALL_RECORD_FILE_NAME);

    file_path[FILE_PATH_MAX_LEN - 1] = '\0';
    return 0;
}

/**
 * @brief 递归创建多级目录
 *
 * @param[in] path 目录路径
 * @param[in] mode 目录权限
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int mkdir_recursive(const char *path, mode_t mode)
{
    if (path == NULL || path[0] == '\0')
        return -1;

    char tmp[RECORD_PATH_MAX_LEN] = {0};
    strncpy(tmp, path, RECORD_PATH_MAX_LEN - 1);
    tmp[RECORD_PATH_MAX_LEN - 1] = '\0';

    size_t len = strlen(tmp);
    if (len > 1 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p != '\0'; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST)
            {
                CALL_LOG_ERROR(CALL_MODULE_RECORD, "mkdir failed: path=%s, errno=%d", tmp, errno);
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0 && errno != EEXIST)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "mkdir failed: path=%s, errno=%d", tmp, errno);
        return -1;
    }
    return 0;
}

/**
 * @brief 创建存储目录
 *
 * @param[in] dir_path 目录路径
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int create_storage_dir(const char *dir_path)
{
    if (dir_path == NULL || dir_path[0] == '\0' || strlen(dir_path) >= RECORD_PATH_MAX_LEN)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "create_storage_dir failed: invalid dir path");
        return -1;
    }

    struct stat dir_stat;
    if (stat(dir_path, &dir_stat) == 0)
    {
        if (S_ISDIR(dir_stat.st_mode))
            return 0;

        CALL_LOG_ERROR(CALL_MODULE_RECORD, "path %s exists but is not a directory", dir_path);
        return -1;
    }
    if (mkdir_recursive(dir_path, CALL_RECORD_DIR_PERM) != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "create dir failed: path=%s", dir_path);
        return -1;
    }

    if (chmod(dir_path, CALL_RECORD_DIR_PERM) != 0)
        CALL_LOG_WARN(CALL_MODULE_RECORD, "set dir perm failed: path=%s, errno=%d", dir_path, errno);

    CALL_LOG_DEBUG(CALL_MODULE_RECORD, "create dir success: %s", dir_path);
    return 0;
}

/**
 * @brief 将内存记录持久化到文件
 *
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int persist_records_to_file(void)
{
    call_record_info_attr *record_snapshot = (call_record_info_attr *)malloc(
        sizeof(call_record_info_attr) * CALL_RECORD_MAX_COUNT);
    if (record_snapshot == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: malloc snapshot failed");
        return -1;
    }
    memset(record_snapshot, 0, sizeof(call_record_info_attr) * CALL_RECORD_MAX_COUNT);

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    bool is_enable = s_record_ctx.enable;
    uint32_t record_count = s_record_ctx.record_count;
    char dir_path[RECORD_PATH_MAX_LEN] = {0};
    if (is_enable)
    {
        memcpy(record_snapshot, s_record_ctx.record_list,
               sizeof(call_record_info_attr) * record_count);
        strncpy(dir_path, s_record_ctx.record_path, RECORD_PATH_MAX_LEN - 1);
        dir_path[RECORD_PATH_MAX_LEN - 1] = '\0';
    }
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (!is_enable)
    {
        CALL_LOG_WARN(CALL_MODULE_RECORD, "persist skipped: function disabled");
        free(record_snapshot);
        return 0;
    }

    if (dir_path[0] == '\0')
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: storage dir not set");
        free(record_snapshot);
        return -1;
    }

    if (create_storage_dir(dir_path) != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: create dir error");
        free(record_snapshot);
        return -1;
    }

    char file_path[FILE_PATH_MAX_LEN] = {0};
    if (get_full_file_path(file_path) != 0)
    {
        free(record_snapshot);
        return -1;
    }

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: open %s error, errno=%d", file_path, errno);
        free(record_snapshot);
        return -1;
    }

    size_t write_ret = fwrite(&record_count, sizeof(uint32_t), 1, fp);
    if (write_ret != 1)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: write count error");
        fclose(fp);
        free(record_snapshot);
        return -1;
    }

    if (record_count > 0)
    {
        write_ret = fwrite(record_snapshot, sizeof(call_record_info_attr), record_count, fp);
        if (write_ret != record_count)
        {
            CALL_LOG_ERROR(CALL_MODULE_RECORD, "persist failed: write list error (expect=%d, actual=%zu)",
                           record_count, write_ret);
            fclose(fp);
            free(record_snapshot);
            return -1;
        }
    }

    fflush(fp);
    fsync(fileno(fp));
    chmod(file_path, CALL_RECORD_FILE_PERM);
    fclose(fp);
    free(record_snapshot);

    CALL_LOG_INFO(CALL_MODULE_RECORD, "persist success: %d records to %s", record_count, file_path);
    return 0;
}

/**
 * @brief 从文件加载历史记录到内存
 *
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int load_records_from_file(void)
{
    pthread_mutex_lock(&s_record_ctx.record_mutex);
    bool is_enable = s_record_ctx.enable;
    char dir_path[RECORD_PATH_MAX_LEN] = {0};
    if (is_enable)
    {
        strncpy(dir_path, s_record_ctx.record_path, RECORD_PATH_MAX_LEN - 1);
        dir_path[RECORD_PATH_MAX_LEN - 1] = '\0';
    }
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (!is_enable)
    {
        CALL_LOG_WARN(CALL_MODULE_RECORD, "load skipped: function disabled");
        return 0;
    }

    if (dir_path[0] == '\0')
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "load failed: storage dir not set");
        return -1;
    }

    char file_path[FILE_PATH_MAX_LEN] = {0};
    if (get_full_file_path(file_path) != 0)
        return -1;

    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0)
    {
        CALL_LOG_DEBUG(CALL_MODULE_RECORD, "load: file %s not exist (no history)", file_path);
        return 0;
    }

    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "load failed: open %s error, errno=%d", file_path, errno);
        return -1;
    }

    uint32_t load_count = 0;
    size_t read_ret = fread(&load_count, sizeof(uint32_t), 1, fp);
    if (read_ret != 1)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "load failed: read count error");
        fclose(fp);
        return -1;
    }

    load_count = (load_count > CALL_RECORD_MAX_COUNT) ? CALL_RECORD_MAX_COUNT : load_count;
    if (load_count == 0)
    {
        fclose(fp);
        CALL_LOG_DEBUG(CALL_MODULE_RECORD, "load success: no valid records in file");
        return 0;
    }

    call_record_info_attr *load_list = (call_record_info_attr *)malloc(
        sizeof(call_record_info_attr) * CALL_RECORD_MAX_COUNT);
    if (load_list == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "load failed: malloc load_list failed");
        fclose(fp);
        return -1;
    }
    memset(load_list, 0, sizeof(call_record_info_attr) * CALL_RECORD_MAX_COUNT);

    read_ret = fread(load_list, sizeof(call_record_info_attr), load_count, fp);
    fclose(fp);

    if (read_ret != load_count)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "load failed: read list error (expect=%d, actual=%zu)",
                       load_count, read_ret);
        free(load_list);
        return -1;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    s_record_ctx.record_count = load_count;
    memcpy(s_record_ctx.record_list, load_list, sizeof(call_record_info_attr) * load_count);
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    free(load_list);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "load success: %d records from %s", load_count, file_path);
    return 0;
}

/**
 * @brief 初始化通话记录模块
 *
 * 初始化互斥锁、设置初始化标志
 *
 * @return 初始化结果
 * @retval true 成功
 * @retval false 失败
 */
bool record_module_init(void)
{
    if (s_record_ctx.inited)
    {
        CALL_LOG_WARN(CALL_MODULE_RECORD, "module already initialized");
        return true;
    }

    if (pthread_mutex_init(&s_record_ctx.record_mutex, NULL) != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "init failed: mutex init error");
        return false;
    }

    s_record_ctx.inited = true;
    CALL_LOG_INFO(CALL_MODULE_RECORD, "module init success");
    return true;
}

/**
 * @brief 反初始化通话记录模块
 *
 * 持久化数据、销毁互斥锁、清空上下文
 *
 * @return 始终返回 true
 */
bool record_module_uninit(void)
{
    if (!s_record_ctx.inited)
    {
        CALL_LOG_WARN(CALL_MODULE_RECORD, "module not initialized");
        return true;
    }

    s_record_ctx.inited = false;
    persist_records_to_file();

    pthread_mutex_destroy(&s_record_ctx.record_mutex);
    memset(&s_record_ctx, 0, sizeof(call_record_ctx));

    CALL_LOG_INFO(CALL_MODULE_RECORD, "module uninit success");
    return true;
}

/**
 * @brief 添加一条通话记录
 *
 * 记录满时循环覆盖最早记录，添加后自动持久化
 *
 * @param[in] record_info 通话记录信息
 * @return 操作结果
 * @retval true 成功
 * @retval false 失败
 */
bool record_list_add(call_record_info_attr record_info)
{
    if (!check_module_inited())
        return false;

    if (record_info.duration < 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "add failed: invalid duration=%d (must be >= 0)",
                       record_info.duration);
        return false;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);

    if (!s_record_ctx.enable)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        CALL_LOG_WARN(CALL_MODULE_RECORD, "add skipped: function disabled");
        return true;
    }

    if (s_record_ctx.record_count >= CALL_RECORD_MAX_COUNT)
    {
        memmove(&s_record_ctx.record_list[0], &s_record_ctx.record_list[1],
                (CALL_RECORD_MAX_COUNT - 1) * sizeof(call_record_info_attr));
        s_record_ctx.record_list[CALL_RECORD_MAX_COUNT - 1] = record_info;
    }
    else
    {
        s_record_ctx.record_list[s_record_ctx.record_count] = record_info;
        s_record_ctx.record_count++;
    }
    uint32_t current_count = s_record_ctx.record_count;

    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (persist_records_to_file() != 0)
    {
        CALL_LOG_WARN(CALL_MODULE_RECORD, "add success but persist failed: type=%d, duration=%ds",
                      record_info.log_type, record_info.duration);
    }

    CALL_LOG_INFO(CALL_MODULE_RECORD, "add success: type=%d, duration=%ds, total_count=%d",
                  record_info.log_type, record_info.duration, current_count);
    return true;
}

/**
 * @brief 查询通话记录（按最新到最早）
 *
 * @param[out] records 输出记录缓冲区
 * @param[in] max_count 最大读取条数
 * @param[in] start_idx 起始索引
 * @return 实际读取条数
 */
uint32_t record_list_query(call_record_info_attr *records, uint32_t max_count, uint32_t start_idx)
{
    if (!check_module_inited() || records == NULL || max_count == 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "query failed: invalid parameter");
        return 0;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    uint32_t total_count = s_record_ctx.record_count;
    call_record_info_attr *snapshot = (call_record_info_attr *)malloc(
        sizeof(call_record_info_attr) * CALL_RECORD_MAX_COUNT);
    if (snapshot == NULL)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "query failed: malloc snapshot failed");
        return 0;
    }
    memcpy(snapshot, s_record_ctx.record_list,
           sizeof(call_record_info_attr) * total_count);
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (total_count == 0 || start_idx >= total_count)
    {
        CALL_LOG_DEBUG(CALL_MODULE_RECORD, "query: no records (start_idx=%d, total=%d)",
                       start_idx, total_count);
        free(snapshot);
        return 0;
    }

    uint32_t query_count = ((total_count - start_idx) < max_count)
                               ? (total_count - start_idx)
                               : max_count;

    for (uint32_t i = 0; i < query_count; i++)
    {
        uint32_t src_idx = total_count - 1 - start_idx - i;
        if (src_idx >= total_count)
            break;
        records[i] = snapshot[src_idx];
    }

    free(snapshot);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "query success: start_idx=%d, query_count=%d, total=%d",
                  start_idx, query_count, total_count);
    return query_count;
}

/**
 * @brief 删除指定索引的通话记录
 *
 * @param[in] query_idx 记录索引（0为最新）
 * @return 操作结果
 * @retval true 成功
 * @retval false 失败
 */
bool record_list_delete(uint32_t query_idx)
{
    if (!check_module_inited())
        return false;

    pthread_mutex_lock(&s_record_ctx.record_mutex);

    if (!s_record_ctx.enable)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "delete failed: function disabled");
        return false;
    }

    uint32_t total_count = s_record_ctx.record_count;
    if (total_count == 0 || query_idx >= total_count)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "delete failed: invalid query_idx=%d (total=%d)",
                       query_idx, total_count);
        return false;
    }
    uint32_t storage_idx = total_count - 1 - query_idx;
    memmove(&s_record_ctx.record_list[storage_idx],
            &s_record_ctx.record_list[storage_idx + 1],
            (total_count - storage_idx - 1) * sizeof(call_record_info_attr));
    s_record_ctx.record_count--;
    memset(&s_record_ctx.record_list[s_record_ctx.record_count], 0,
           sizeof(call_record_info_attr));

    uint32_t count_after_del = s_record_ctx.record_count;
    uint32_t max_per_page = s_record_ctx.max_per_page;
    uint32_t cur_index = s_record_ctx.cur_item_index;
    if (max_per_page > 0 &&
        cur_index == count_after_del &&
        cur_index >= max_per_page)
    {
        s_record_ctx.cur_item_index = cur_index - max_per_page;
    }

    uint32_t remaining = s_record_ctx.record_count;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (persist_records_to_file() != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "delete success but persist failed: query_idx=%d", query_idx);
        return false;
    }

    CALL_LOG_INFO(CALL_MODULE_RECORD, "delete success: query_idx=%d, remaining=%d",
                  query_idx, remaining);
    return true;
}

/**
 * @brief 删除所有通话记录
 *
 * 清空内存记录并删除存储文件
 *
 * @return 操作结果
 * @retval true 成功
 * @retval false 失败
 */
bool record_list_delete_all(void)
{
    if (!check_module_inited())
        return false;

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    bool is_enable = s_record_ctx.enable;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (!is_enable)
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "delete_all failed: function disabled");
        return false;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    s_record_ctx.record_count = 0;
    s_record_ctx.cur_item_index = 0;
    memset(&s_record_ctx.record_list, 0, sizeof(s_record_ctx.record_list));
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    char file_path[FILE_PATH_MAX_LEN] = {0};
    if (get_full_file_path(file_path) != 0)
        return false;

    if (remove(file_path) == 0)
    {
        sync();
        CALL_LOG_INFO(CALL_MODULE_RECORD, "delete_all success: file %s deleted", file_path);
    }
    else if (errno == ENOENT)
    {
        CALL_LOG_DEBUG(CALL_MODULE_RECORD, "delete_all success: file %s not exist", file_path);
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "delete_all failed: remove %s error, errno=%d",
                       file_path, errno);
        return false;
    }

    return true;
}

/**
 * @brief 配置记录功能使能及存储路径
 *
 * 使能时自动加载历史记录
 *
 * @param[in] enable 使能开关
 * @param[in] record_path 存储路径
 * @return 配置结果
 * @retval true 成功
 * @retval false 失败
 */
bool record_enable(bool enable, const char *record_path)
{
    if (!check_module_inited())
        return false;

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    if (enable)
    {
        if (record_path == NULL || strlen(record_path) == 0 ||
            strlen(record_path) >= RECORD_PATH_MAX_LEN)
        {
            CALL_LOG_ERROR(CALL_MODULE_RECORD, "enable failed: invalid path");
            pthread_mutex_unlock(&s_record_ctx.record_mutex);
            return false;
        }
        strncpy(s_record_ctx.record_path, record_path, RECORD_PATH_MAX_LEN - 1);
        s_record_ctx.record_path[RECORD_PATH_MAX_LEN - 1] = '\0';
        s_record_ctx.enable = true;
        CALL_LOG_INFO(CALL_MODULE_RECORD, "enable success: path=%s", s_record_ctx.record_path);
    }
    else
    {
        s_record_ctx.enable = false;
        CALL_LOG_INFO(CALL_MODULE_RECORD, "disable success");
    }
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (enable)
    {
        if (load_records_from_file() != 0)
            CALL_LOG_WARN(CALL_MODULE_RECORD, "enable success but load history failed");
    }

    return true;
}

/**
 * @brief 获取记录功能使能状态
 *
 * @return 使能状态
 * @retval true 已使能
 * @retval false 未使能
 */
bool record_get_enable(void)
{
    if (!check_module_inited())
        return false;

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    bool enable = s_record_ctx.enable;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    return enable;
}

/**
 * @brief 获取当前有效记录总数
 *
 * @return 有效记录条数
 */
uint32_t record_list_get_count(void)
{
    if (!check_module_inited())
        return 0;

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    uint32_t count = s_record_ctx.record_count;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    return count;
}

/**
 * @brief 跳转到第一页并查询记录
 *
 * @param[out] record_info 输出记录缓冲区
 * @param[in] max_per_page 每页最大条数
 * @param[out] query_count 实际读取条数
 */
void call_log_first_page(call_record_info_attr *record_info, uint32_t max_per_page,
                         uint32_t *query_count)
{
    if (!check_module_inited() || record_info == NULL ||
        query_count == NULL || max_per_page == 0)
    {
        if (query_count != NULL)
            *query_count = 0;
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "call_log_first_page failed: invalid parameter");
        return;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    s_record_ctx.max_per_page = max_per_page;
    s_record_ctx.cur_item_index = 0;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    *query_count = record_list_query(record_info, max_per_page, 0);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "call_log_first_page: query_count=%d", *query_count);
}

/**
 * @brief 翻到上一页（循环翻页）
 *
 * @param[out] record_info 输出记录缓冲区
 * @param[out] query_count 实际读取条数
 */
void call_log_prev_page(call_record_info_attr *record_info, uint32_t *query_count)
{
    if (!check_module_inited() || record_info == NULL || query_count == NULL)
    {
        if (query_count != NULL)
            *query_count = 0;
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "call_log_prev_page failed: invalid parameter");
        return;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    uint32_t cur_index = s_record_ctx.cur_item_index;
    uint32_t max_per_page = s_record_ctx.max_per_page;
    uint32_t record_count = s_record_ctx.record_count;

    if (max_per_page == 0 || record_count == 0)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        *query_count = 0;
        return;
    }

    uint32_t target_index;
    if (cur_index >= max_per_page)
    {
        target_index = cur_index - max_per_page;
    }
    else
    {
        uint32_t total_pages = (record_count + max_per_page - 1) / max_per_page;
        target_index = (total_pages - 1) * max_per_page;
    }
    s_record_ctx.cur_item_index = target_index;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    *query_count = record_list_query(record_info, max_per_page, target_index);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "call_log_prev_page: target_index=%d, query_count=%d",
                  target_index, *query_count);
}

/**
 * @brief 翻到下一页（循环翻页）
 *
 * @param[out] record_info 输出记录缓冲区
 * @param[out] query_count 实际读取条数
 */
void call_log_next_page(call_record_info_attr *record_info, uint32_t *query_count)
{
    if (!check_module_inited() || record_info == NULL || query_count == NULL)
    {
        if (query_count != NULL)
            *query_count = 0;
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "call_log_next_page failed: invalid parameter");
        return;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    uint32_t cur_index = s_record_ctx.cur_item_index;
    uint32_t max_per_page = s_record_ctx.max_per_page;
    uint32_t record_count = s_record_ctx.record_count;

    if (max_per_page == 0 || record_count == 0)
    {
        pthread_mutex_unlock(&s_record_ctx.record_mutex);
        *query_count = 0;
        return;
    }

    uint32_t total_pages = (record_count + max_per_page - 1) / max_per_page;
    uint32_t cur_page = (cur_index / max_per_page) + 1;
    uint32_t target_index = (cur_page >= total_pages) ? 0 : (cur_index + max_per_page);

    s_record_ctx.cur_item_index = target_index;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    *query_count = record_list_query(record_info, max_per_page, target_index);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "call_log_next_page: target_index=%d, query_count=%d",
                  target_index, *query_count);
}

/**
 * @brief 查询当前页记录
 *
 * @param[out] record_info 输出记录缓冲区
 * @param[out] query_count 实际读取条数
 */
void call_log_current_page(call_record_info_attr *record_info, uint32_t *query_count)
{
    if (!check_module_inited() || record_info == NULL || query_count == NULL)
    {
        if (query_count != NULL)
            *query_count = 0;
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "call_log_current_page failed: invalid parameter");
        return;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    uint32_t cur_index = s_record_ctx.cur_item_index;
    uint32_t page_size = s_record_ctx.max_per_page;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    if (page_size == 0)
    {
        *query_count = 0;
        CALL_LOG_WARN(CALL_MODULE_RECORD, "call_log_current_page: max_per_page not configured");
        return;
    }

    *query_count = record_list_query(record_info, page_size, cur_index);
    CALL_LOG_INFO(CALL_MODULE_RECORD, "call_log_current_page: cur_index=%d, page_size=%d, count=%d",
                  cur_index, page_size, *query_count);
}

/**
 * @brief 获取当前页索引与总记录数
 *
 * @param[out] current_index 当前页起始索引
 * @param[out] total_count 总记录条数
 */
void call_log_current_total(uint32_t *current_index, uint32_t *total_count)
{
    if (!check_module_inited() || current_index == NULL || total_count == NULL)
    {
        if (current_index != NULL)
            *current_index = INVALID_PAGE_INDEX;
        if (total_count != NULL)
            *total_count = 0;
        CALL_LOG_ERROR(CALL_MODULE_RECORD, "call_log_current_total failed: invalid parameter");
        return;
    }

    pthread_mutex_lock(&s_record_ctx.record_mutex);
    *current_index = s_record_ctx.cur_item_index;
    *total_count = s_record_ctx.record_count;
    pthread_mutex_unlock(&s_record_ctx.record_mutex);

    CALL_LOG_INFO(CALL_MODULE_RECORD, "call_log_current_total: current_index=%d, total=%d",
                  *current_index, *total_count);
}