#ifndef _AMY_SDK_H_
#define _AMY_SDK_H_

#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include "Amy_audio_play.h"
#include "Amy_lvgl.h"
#define SOFT_VERSION "1.0.1"
#define AMY_UIN32_MAX __INT32_MAX__

#define Amy_free free
#define Amy_malloc malloc

typedef struct
{
    long size;
    unsigned char *data;
    unsigned long long timestamp;
    int keyframe;
} stream_data;

typedef pthread_t Amy_thread_t;
#define Amy_pthread_create(tid, func, param, stack_size, prio) \
    do                                                         \
    {                                                          \
        pthread_attr_t attr;                                   \
        memset(&attr, 0, sizeof(pthread_attr_t));              \
        pthread_attr_setstacksize(&attr, stack_size);          \
        pthread_create(tid, &attr, func, param);               \
    } while (0)

#define Amy_pthread_join(tid, param) pthread_join(tid, param);

#define Amy_pthread_detach(tid) pthread_detach(tid)

typedef pthread_mutex_t Amy_mutex_t;
#define Amy_mutex_init(mutex, param) pthread_mutex_init(mutex, param)
#define Amy_mutex_lock pthread_mutex_lock
#define Amy_mutex_unlock pthread_mutex_unlock
#define Amy_mutex_destroy pthread_mutex_destroy

typedef pthread_cond_t Amy_cond_t;
#define Amy_cond_init(cond, param) pthread_cond_init(cond, param)
#define Amy_cond_signal(cond) pthread_cond_signal(cond)
#define Amy_cond_broadcast(cond) pthread_cond_broadcast(cond)
#define Amy_cond_wait(cond, mutex) pthread_cond_wait(cond, mutex)
#define Amy_cond_destroy(cond) pthread_cond_destroy(cond)

typedef sem_t Amy_sem_t;
#define Amy_sem_init(sem, pshare, value) sem_init(sem, pshare, value)
#define Amy_sem_wait(sem) sem_wait(sem)
#define Amy_sem_post(sem) sem_post(sem)
#define Amy_sem_getvalue(sem, value) sem_getvalue(sem, value)
#define Amy_sem_destroy(sem) sem_destroy(sem)

#define H264_FRAME_TYPE_SPS 0X07
#define H264_FRAME_TYPE_PPS 0X08
#define H264_FRAME_TYPE_IDR 0X05
#define H264_FRAME_TYPE_PPP 0X00
static inline int h264_frame_type(char code)
{
    code = code & 0x1F;
    if ((code == 7) || (code == 8) || (code == 5))
    {
        return code;
    }
    return 0;
}
// usleep(n*1000)
// rt_thread_mdelay(n)
// usleep(n*1000)
#define Amy_timestamp(t)                                              \
    do                                                                \
    {                                                                 \
        struct timeval tv;                                            \
        gettimeofday(&tv, NULL);                                      \
        t = (unsigned long long)tv.tv_sec * 1000 + tv.tv_usec / 1000; \
    } while (0);

#define Amy_msleep(n)                                             \
    do                                                            \
    {                                                             \
        long long start_time;                                     \
        Amy_timestamp(start_time);                                \
        long long remaining = n * 1000;                           \
        while (remaining > 0)                                     \
        {                                                         \
            struct timeval tv;                                    \
            tv.tv_sec = remaining / 1000000;                      \
            tv.tv_usec = remaining % 1000000;                     \
            int reslut = select(0, NULL, NULL, NULL, &tv);        \
            if ((reslut == -1) && (errno != EINTR))               \
            {                                                     \
                AMY_LOG_ERROR("select error\n");                  \
                break;                                            \
            }                                                     \
            long long current_time;                               \
            Amy_timestamp(current_time);                          \
            remaining = (n - (current_time - start_time)) * 1000; \
        }                                                         \
    } while (0);

#define Amy_time_week(wday, year, mon, day)                                     \
    do                                                                          \
    {                                                                           \
        /* 定义局部变量，避免修改外部传入的mon/year */                          \
        int _local_mon = mon;                                                   \
        int _local_year = year;                                                 \
        /* 仅修改局部变量，不影响外部 */                                        \
        if ((_local_mon == 1) || (_local_mon == 2))                             \
        {                                                                       \
            _local_mon += 12;                                                   \
            _local_year--;                                                      \
        }                                                                       \
        /* 用局部变量计算星期几，结果赋值给wday */                              \
        wday = (day + 2 * _local_mon + 3 * (_local_mon + 1) / 5 + _local_year + \
                _local_year / 4 - _local_year / 100 + _local_year / 400) %      \
                   7 +                                                          \
               1;                                                               \
        /* 兜底：确保星期几在1-7范围内（避免计算异常） */                       \
        if (wday < 1 || wday > 7)                                               \
        {                                                                       \
            wday = (wday % 7 + 7) % 7;                                          \
            wday = (wday == 0) ? 7 : wday;                                      \
        }                                                                       \
    } while (0);

// 注意传入的tm是经过补偿计算的
#define Amy_set_time(tm)                                                         \
    do                                                                           \
    {                                                                            \
        if (tm != NULL)                                                          \
        { /* 增加空指针检查，避免程序崩溃 */                                     \
            char string[64] = {0};                                               \
            /* 修正时间字段：tm_year+1900 得到实际年份，tm_mon+1 得到实际月份 */ \
            sprintf(string, "date -s \"%04d-%02d-%02d %02d:%02d:%02d\"",         \
                    tm->tm_year, tm->tm_mon, tm->tm_mday,                        \
                    tm->tm_hour, tm->tm_min, tm->tm_sec);                        \
            system(string);                                                      \
            /***** 将系统时间与RTC同步 ******/                                   \
            system("hwclock -w");                                                \
        }                                                                        \
    } while (0)

#define Amy_localtime(tm)                                             \
    do                                                                \
    {                                                                 \
        time_t ___Amy_t____ = time(NULL);                             \
        localtime_r(&___Amy_t____, &tm);                              \
        tm.tm_year += 1900;                                           \
        tm.tm_mon += 1;                                               \
        Amy_time_week(tm.tm_wday, tm.tm_year, tm.tm_mon, tm.tm_mday); \
    } while (0);

#define Amy_time2localtime(t, tm)                                     \
    do                                                                \
    {                                                                 \
        localtime_r((const time_t *)&t, &tm);                         \
        tm.tm_year += 1900;                                           \
        tm.tm_mon += 1;                                               \
        Amy_time_week(tm.tm_wday, tm.tm_year, tm.tm_mon, tm.tm_mday); \
    } while (0)

#define tde_layer_init(layer, w, h, x, y, pw, ph, fmt, data)                       \
    do                                                                             \
    {                                                                              \
        layer.width = w;                                                           \
        layer.height = h;                                                          \
        layer.pos_left = x;                                                        \
        layer.pos_top = y;                                                         \
        layer.pos_width = pw;                                                      \
        layer.pos_height = ph;                                                     \
        layer.format_param = fmt;                                                  \
        if (data)                                                                  \
        {                                                                          \
            ak_mem_dma_vaddr2paddr((void *)data, (unsigned long *)&layer.phyaddr); \
        }                                                                          \
    } while (0)

void Amy_sdk_init(const audio_ring_resource *rings_resource, int rings_cnt, const firamware_resource *ttf);

// 执行命令，读取结果后执行比较
typedef struct
{
    const char *cmd;
    int isread;
    const void *userdata;
    int (*cb)(const void *userdata, char *output, int line);
    char *reslut_string;
    int reslut_strean_len;
} cmd_execution_param_t;

int system_cmd_execution(cmd_execution_param_t *param);

int Amy_mtd_parttion_get(const char *partition_name, char *mtd_name, int name_len, unsigned long *mtd_size);

// 获取两个空格之间的字符串
int find_string_between_spaces(const char *str, char **next, char *val, int len);

// 获取n个不连续空格后的字符串首地址
char *string_spaces_after_char(const char *string, int n);
// 获取字符串末尾开始的第一个连续数字的首地址
char *find_tail_string_number(char *string);
#endif