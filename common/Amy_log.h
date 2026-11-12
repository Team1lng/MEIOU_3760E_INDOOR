#ifndef _AYM_COMMON_H_
#define _AYM_COMMON_H_
#include <string.h>
#include <fcntl.h>

typedef enum {
    AMY_LOG_LEVEL_ERROR = 1,
    AMY_LOG_LEVEL_WARNING,
    AMY_LOG_LEVEL_INFO,
    AMY_LOG_LEVEL_DEBUG,
    AMY_LOG_LEVEL_TOP = 6
} Amy_log_level;
int Amy_log(int level, const char* file, const char* func, size_t line, const char* format, ...);

#define AMY_LOG(level, format, ...) \
    do { \
        if ((level) <= Amy_log_current_level()) { \
            Amy_log((level), __FILE__, __func__, __LINE__, (format), ##__VA_ARGS__); \
        } \
    } while(0)

// 各等级快捷宏
#define AMY_LOG_ERROR(format, ...) \
    AMY_LOG(AMY_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#define AMY_LOG_WARNING(format, ...) \
    AMY_LOG(AMY_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

#define AMY_LOG_INFO(format, ...) \
    AMY_LOG(AMY_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

#define AMY_LOG_DEBUG(format, ...) \
    AMY_LOG(AMY_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

// 日志级别控制API
void Amy_log_set_level(Amy_log_level level);
Amy_log_level Amy_log_current_level(void);



#endif
