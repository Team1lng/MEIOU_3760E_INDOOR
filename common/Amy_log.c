#include "Amy_log.h"
#include "Amy_sdk.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>


#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define WHITE "\x1b[37m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

typedef struct {
    Amy_log_level level;
    Amy_mutex_t mutex;
    int mutex_inited;
    const char* level_strings[AMY_LOG_LEVEL_TOP];
}Amy_log_info;

static Amy_log_info log_context = {
    .level = AMY_LOG_LEVEL_DEBUG,
    .mutex_inited = 0,
    .level_strings[AMY_LOG_LEVEL_DEBUG] = WHITE "DEBUG",
    .level_strings[AMY_LOG_LEVEL_INFO] = GREEN "INFO",
    .level_strings[AMY_LOG_LEVEL_WARNING] = YELLOW "WARNING",
    .level_strings[AMY_LOG_LEVEL_ERROR] = RED "ERROR",
};

int Amy_log(int level, const char* file, const char* func, size_t line, const char* format, ...)
{
    va_list args;
    char buffer[1024];
    time_t now;
    struct tm* tm_info;
    char time_str[20];
    int ret = 0;

    if (log_context.mutex_inited == 0)
    {
        Amy_mutex_init(&log_context.mutex, NULL);
        log_context.mutex_inited = 1;
    }

    if (level > log_context.level)
        return 0;

    // 获取当前时间
    time(&now);
    tm_info = localtime(&now);
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    // 仅保留文件名（去除路径）
    const char* base_file = strrchr(file, '/');
    base_file = base_file ? base_file + 1 : file;
    Amy_mutex_lock(&log_context.mutex);
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ret = printf("[%s][%s] %s:%d %s() %s%s%s\n", log_context.level_strings[level], time_str, base_file, line, func, buffer, RESET, "");
    Amy_mutex_unlock(&log_context.mutex);
    return ret;
}
// 日志级别控制
void Amy_log_set_level(Amy_log_level level)
{
    if (log_context.mutex_inited == 0)
    {
        Amy_mutex_init(&log_context.mutex, NULL);
        log_context.mutex_inited = 1;
    }

    Amy_mutex_lock(&log_context.mutex);
    log_context.level = level;
    Amy_mutex_unlock(&log_context.mutex);
}

Amy_log_level Amy_log_current_level(void)
{
    return log_context.level;
}

#if 1
int ak_print(int module, int level, const char* format, ...) {
    //return 0;

    va_list args;
    char buffer[1024];
    time_t now;
    struct tm* tm_info;
    char time_str[20];
    int ret = 0;

    if (log_context.mutex_inited == 0)
    {
        Amy_mutex_init(&log_context.mutex, NULL);
        log_context.mutex_inited = 1;
    }

    if (level > log_context.level)
        return 0;

    // 获取当前时间
    time(&now);
    tm_info = localtime(&now);
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    // 仅保留文件名（去除路径）
  //  const char* base_file = strrchr(file, '/');
 //   base_file = base_file ? base_file + 1 : file;
    Amy_mutex_lock(&log_context.mutex);
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ret = printf("[%s][%s]  %s%s%s\n", log_context.level_strings[level], time_str, buffer, RESET, "");
    Amy_mutex_unlock(&log_context.mutex);
    return ret;
}

const char* ak_print_get_version(void) {
    return "leo.1.1";
}


int ak_print_set_level(int module_id, int level) {
    AMY_LOG_INFO("module id:%d set log levael:%d\n", module_id, level);
    return 0;
}

int ak_print_set_syslog_level(int module_id, int level) {
    AMY_LOG_INFO("module id:%d set sys log levael:%d\n", module_id, level);
    return 0;
}
#endif