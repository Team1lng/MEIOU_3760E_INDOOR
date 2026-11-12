#ifndef _TB_LOG_H_
#define _TB_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

enum {
    LEVEL_OVER_LOGN = 0,
    LEVEL_OVER_LOGE = 1,
    LEVEL_OVER_LOGW,
    LEVEL_OVER_LOGI,
    LEVEL_OVER_LOGD,
    LEVEL_OVER_LOGV,
};

extern int global_log_level;

#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define TB_LOGE(format,arg ...)                 \
do{                                 \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##arg); \
} while (0)

#define TB_LOGW(format,arg ...)                 \
do{                                 \
    if (global_log_level >= LEVEL_OVER_LOGW)            \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##arg);\
} while (0)

#define TB_LOGI(format,arg ...)                 \
do{                                 \
    if (global_log_level >= LEVEL_OVER_LOGI)            \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
} while (0)

#define TB_LOGD(format,arg ...)                 \
do{                                 \
    if (global_log_level >= LEVEL_OVER_LOGD)            \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
} while (0)

#define TB_LOGV(format,arg ...)                 \
do{                                 \
    if (global_log_level >= LEVEL_OVER_LOGV)            \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
} while (0)

#ifdef __cplusplus
}
#endif/* __TB_LOG_H__ */
#endif
