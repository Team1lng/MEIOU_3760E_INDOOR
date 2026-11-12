#ifndef _TY_NET_LOG_H_
#define _TY_NET_LOG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LEVEL_OVER_LOGN = 0,
    LEVEL_OVER_LOGE = 1,
    LEVEL_OVER_LOGW,
    LEVEL_OVER_LOGI,
    LEVEL_OVER_LOGD,
    LEVEL_OVER_LOGV,
};

#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#ifndef DEBUG_LEVEL
#define TNLOG_DEBUG_LEVEL LEVEL_OVER_LOGD
#else
#define TNLOG_DEBUG_LEVEL LEVEL_OVER_LOGV       // DEBUG_LEVELÖµÊÇ5
#endif

#define TN_LOGE(format,arg ...)                                 \
do{                                                             \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##arg); \
} while (0)

#define TN_LOGW(format,arg ...)                                 \
do{                                                             \
    if (TNLOG_DEBUG_LEVEL >= LEVEL_OVER_LOGW)                   \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##arg);\
} while (0)

#define TN_LOGI(format,arg ...)                                 \
do{                                                             \
    if (TNLOG_DEBUG_LEVEL >= LEVEL_OVER_LOGI)                   \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);       \
} while (0)

#define TN_LOGD(format,arg ...)                                 \
do{                                                             \
    if (TNLOG_DEBUG_LEVEL >= LEVEL_OVER_LOGD)                   \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);       \
} while (0)

#define TN_LOGV(format,arg ...)                                 \
do{                                                             \
    if (TNLOG_DEBUG_LEVEL >= LEVEL_OVER_LOGV)                   \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);       \
} while (0)

#ifdef __cplusplus
}

#endif/* __TY_NET_LOG_H__ */
#endif
