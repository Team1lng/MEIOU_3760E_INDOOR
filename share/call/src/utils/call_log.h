/**
 * @file        call_log.h
 * @brief       通话系统日志模块
 * @details     带颜色、分级、分模块的日志输出接口
 * @ingroup     call_log
 */

#ifndef __CALL_LOG_H__
#define __CALL_LOG_H__

#include <stdio.h>

// 可选：关闭颜色输出（定义此宏则日志无颜色）
// #define CALL_LOG_DISABLE_COLOR

// ANSI 颜色控制码定义
#ifndef CALL_LOG_DISABLE_COLOR
#define CALL_COLOR_RED "\033[31m"
#define CALL_COLOR_YELLOW "\033[33m"
#define CALL_COLOR_GREEN "\033[32m"
#define CALL_COLOR_BLUE "\033[34m"
#define CALL_COLOR_RESET "\033[0m"
#else
#define CALL_COLOR_RED ""
#define CALL_COLOR_YELLOW ""
#define CALL_COLOR_GREEN ""
#define CALL_COLOR_BLUE ""
#define CALL_COLOR_RESET ""
#endif

/**
 * @brief 日志等级枚举
 */
typedef enum
{
    CALL_LOG_LEVEL_ERROR = 0,
    CALL_LOG_LEVEL_WARN = 1,
    CALL_LOG_LEVEL_INFO = 2,
    CALL_LOG_LEVEL_DEBUG = 3
} call_log_level_e;

#ifndef CALL_LOG_GLOBAL_LEVEL
#define CALL_LOG_GLOBAL_LEVEL CALL_LOG_LEVEL_DEBUG
#endif

/**
 * @brief 日志输出模块枚举
 */
typedef enum
{
    CALL_MODULE_CORE,
    CALL_MODULE_ACTION,
    CALL_MODULE_HEARTBEAT,
    CALL_MODULE_EVENT,
    CALL_MODULE_RECORD,
    CALL_MODULE_UTILS,
    CALL_MODULE_API
} call_module_e;

/**
 * @brief 获取模块名字符串
 * @param module 模块枚举
 * @return 字符串
 */
static inline const char *call_module_name(call_module_e module)
{
    switch (module)
    {
    case CALL_MODULE_CORE:
        return "CORE";
    case CALL_MODULE_ACTION:
        return "ACTION";
    case CALL_MODULE_HEARTBEAT:
        return "HEARTBEAT";
    case CALL_MODULE_EVENT:
        return "EVENT";
    case CALL_MODULE_RECORD:
        return "RECORD";
    case CALL_MODULE_UTILS:
        return "UTILS";
    case CALL_MODULE_API:
        return "API";
    default:
        return "UNKNOWN";
    }
}

#define CALL_LOG_LEVEL_FILTER(level) ((level) <= CALL_LOG_GLOBAL_LEVEL)

#define CALL_LOG_ERROR(module, fmt, ...)                                          \
    do                                                                            \
    {                                                                             \
        if (CALL_LOG_LEVEL_FILTER(CALL_LOG_LEVEL_ERROR))                          \
        {                                                                         \
            printf(CALL_COLOR_RED "[CALL-%s] [ERROR] " fmt CALL_COLOR_RESET "\n", \
                   call_module_name(module), ##__VA_ARGS__);                      \
        }                                                                         \
    } while (0)

#define CALL_LOG_WARN(module, fmt, ...)                                             \
    do                                                                              \
    {                                                                               \
        if (CALL_LOG_LEVEL_FILTER(CALL_LOG_LEVEL_WARN))                             \
        {                                                                           \
            printf(CALL_COLOR_YELLOW "[CALL-%s] [WARN] " fmt CALL_COLOR_RESET "\n", \
                   call_module_name(module), ##__VA_ARGS__);                        \
        }                                                                           \
    } while (0)

#define CALL_LOG_INFO(module, fmt, ...)                                            \
    do                                                                             \
    {                                                                              \
        if (CALL_LOG_LEVEL_FILTER(CALL_LOG_LEVEL_INFO))                            \
        {                                                                          \
            printf(CALL_COLOR_GREEN "[CALL-%s] [INFO] " fmt CALL_COLOR_RESET "\n", \
                   call_module_name(module), ##__VA_ARGS__);                       \
        }                                                                          \
    } while (0)

#define CALL_LOG_DEBUG(module, fmt, ...)                                           \
    do                                                                             \
    {                                                                              \
        if (CALL_LOG_LEVEL_FILTER(CALL_LOG_LEVEL_DEBUG))                           \
        {                                                                          \
            printf(CALL_COLOR_BLUE "[CALL-%s] [DEBUG] " fmt CALL_COLOR_RESET "\n", \
                   call_module_name(module), ##__VA_ARGS__);                       \
        }                                                                          \
    } while (0)

#endif // __CALL_LOG_H__