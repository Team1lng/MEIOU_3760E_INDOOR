#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include "Amy_sdk.h"
#include "Amy_log.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "Amy_media_mgr.h"
#include "sqlite3/sqlite3_common.h"

#define FILE_DOT_VIDEO "avi"
#define FILE_DOT_PHOTO "jpg"
#define FILE_DOT_AUDIO "wav"
#define FILE_DOT_UNKNOW "unknow"
#define MEDIA_DATA_TABLE "MediaTable"
#define MEDIA_DATA_INDEX "MediaIndex"
#define MEDIA_DATABASES "media.db"

#define FILE_DATABASES "file.db"
#define FILE_DATA_TABEL "FileTable"
#define FILE_DATA_INDEX "FileIndex"

#define MEDIA_MASK_NUMBERS_BASE 3

#define MEDIA_MASK_NUMBER_TOTAL (MEDIA_MASK_NUMBERS_BASE + 0)
/*新旧标志*/
#define MEDIA_MASK_NUMBERS_OLD (MEDIA_MASK_NUMBERS_BASE + 1)
#define MEDIA_MASK_NUMBERS_NEW (MEDIA_MASK_NUMBERS_BASE + 2)
/*文件类型*/
#define MEDIA_MASK_NUMBERS_TYPE_PHOTO (MEDIA_MASK_NUMBERS_BASE + 3)
#define MEDIA_MASK_NUMBERS_TYPE_VIDEO (MEDIA_MASK_NUMBERS_BASE + 4)
#define MEDIA_MASK_NUMBERS_TYPE_AUDIO (MEDIA_MASK_NUMBERS_BASE + 5)
#define MEDIA_MASK_NUMBERS_TYPE_CUTOM1 (MEDIA_MASK_NUMBERS_BASE + 6)
#define MEDIA_MASK_NUMBERS_TYPE_CUTOM2 (MEDIA_MASK_NUMBERS_BASE + 7)
#define MEDIA_MASK_NUMBERS_TYPE_CUTOM3 (MEDIA_MASK_NUMBERS_BASE + 8)
#define MEDIA_MASK_NUMBERS_TYPE_CUTOM4 (MEDIA_MASK_NUMBERS_BASE + 9)
/*记录模式*/
#define MEDIA_MASK_NUMBER_MODE_MUAUTAL (MEDIA_MASK_NUMBERS_BASE + 10)
#define MEDIA_MASK_NUMBER_MODE_AUTO (MEDIA_MASK_NUMBERS_BASE + 11)
#define MEDIA_MASK_NUMBER_MODE_MOTION (MEDIA_MASK_NUMBERS_BASE + 12)
#define MEDIA_MASK_NUMBER_MODE_ALARM (MEDIA_MASK_NUMBERS_BASE + 13)
#define MEDIA_MASK_NUMBER_MODE_CUTOM1 (MEDIA_MASK_NUMBERS_BASE + 14)
#define MEDIA_MASK_NUMBER_MODE_CUTOM2 (MEDIA_MASK_NUMBERS_BASE + 15)
#define MEDIA_MASK_NUMBER_MODE_CUTOM3 (MEDIA_MASK_NUMBERS_BASE + 16)
/*通道*/
#define MEDIA_MASK_NUMBER_CHANNEL_0 (MEDIA_MASK_NUMBERS_BASE + 17)
#define MEDIA_MASK_NUMBER_CHANNEL_1 (MEDIA_MASK_NUMBERS_BASE + 18)
#define MEDIA_MASK_NUMBER_CHANNEL_2 (MEDIA_MASK_NUMBERS_BASE + 19)
#define MEDIA_MASK_NUMBER_CHANNEL_3 (MEDIA_MASK_NUMBERS_BASE + 20)
#define MEDIA_MASK_NUMBER_CHANNEL_4 (MEDIA_MASK_NUMBERS_BASE + 21)
#define MEDIA_MASK_NUMBER_CHANNEL_5 (MEDIA_MASK_NUMBERS_BASE + 22)
#define MEDIA_MASK_NUMBER_CHANNEL_6 (MEDIA_MASK_NUMBERS_BASE + 23)
#define MEDIA_MASK_NUMBER_CHANNEL_7 (MEDIA_MASK_NUMBERS_BASE + 24)
#define MEDIA_MASK_NUMBER_CHANNEL_8 (MEDIA_MASK_NUMBERS_BASE + 25)
#define MEDIA_MASK_NUMBER_CHANNEL_9 (MEDIA_MASK_NUMBERS_BASE + 26)
#define MEDIA_MASK_NUMBER_CHANNEL_10 (MEDIA_MASK_NUMBERS_BASE + 27)
#define MEDIA_MASK_NUMBER_CHANNEL_11 (MEDIA_MASK_NUMBERS_BASE + 28)
#define MEDIA_MASK_NUMBER_CHANNEL_12 (MEDIA_MASK_NUMBERS_BASE + 29)
#define MEDIA_MASK_NUMBER_CHANNEL_13 (MEDIA_MASK_NUMBERS_BASE + 30)
#define MEDIA_MASK_NUMBER_CHANNEL_14 (MEDIA_MASK_NUMBERS_BASE + 31)
#define MEDIA_MASK_NUMBER_CHANNEL_15 (MEDIA_MASK_NUMBERS_BASE + 32)
#define MEDIA_MASK_MAX (MEDIA_MASK_NUMBERS_BASE + 33)

#define TABLE_1 "Year"
#define TABLE_2 "Month"
#define TABLE_3 "Day"
#define TABLE_4 "TOTAL"
#define TABLE_5 "OLD"
#define TABLE_6 "NEW"
#define TABLE_7 "PHOTO"
#define TABLE_8 "VIDEO"
#define TABLE_9 "AUDIO"
#define TABLE_10 "TCUTOM1"
#define TABLE_11 "TCUTOM2"
#define TABLE_12 "TCUTOM3"
#define TABLE_13 "TCUTOM4"
#define TABLE_14 "MUAUTAL"
#define TABLE_15 "AUTO"
#define TABLE_16 "MOTION"
#define TABLE_17 "ALARM"
#define TABLE_18 "MCUTOM1"
#define TABLE_19 "MCUTOM2"
#define TABLE_20 "MCUTOM3"
#define TABLE_21 "CHANNEL0"
#define TABLE_22 "CHANNEL1"
#define TABLE_23 "CHANNEL2"
#define TABLE_24 "CHANNEL3"
#define TABLE_25 "CHANNEL4"
#define TABLE_26 "CHANNEL5"
#define TABLE_27 "CHANNEL6"
#define TABLE_28 "CHANNEL7"
#define TABLE_29 "CHANNEL8"
#define TABLE_30 "CHANNEL9"
#define TABLE_31 "CHANNEL10"
#define TABLE_32 "CHANNEL11"
#define TABLE_33 "CHANNEL12"
#define TABLE_34 "CHANNEL13"
#define TABLE_35 "CHANNEL14"
#define TABLE_36 "CHANNEL15"

#define TABLE_INT " INT"
#define MEDIA_TABLE_CREATE(INT) \
    TABLE_1 INT "," TABLE_2 INT "," TABLE_3 INT "," TABLE_4 INT "," TABLE_5 INT "," TABLE_6 INT "," TABLE_7 INT "," TABLE_8 INT "," TABLE_9 INT "," TABLE_10 INT "," TABLE_11 INT "," TABLE_12 INT "," TABLE_13 INT "," TABLE_14 INT "," TABLE_15 INT "," TABLE_16 INT "," TABLE_17 INT "," TABLE_18 INT "," TABLE_19 INT "," TABLE_20 INT "," TABLE_21 INT "," TABLE_22 INT "," TABLE_23 INT "," TABLE_24 INT "," TABLE_25 INT "," TABLE_26 INT "," TABLE_27 INT "," TABLE_28 INT "," TABLE_29 INT "," TABLE_30 INT "," TABLE_31 INT "," TABLE_32 INT "," TABLE_33 INT "," TABLE_34 INT "," TABLE_35 INT "," TABLE_36 INT
#define MEDIA_TABLE_BYTE_SPACE(B) \
    B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B "," B

#define FILE_TALBEL_TIMESTAMP "TIME"
#define FILE_TALBEL_DURATION "DURATION"
#define FILE_TABLE_MASK "MASK"

#define FILE_TABLE_CREATE(INT) \
    FILE_TALBEL_TIMESTAMP INT "," FILE_TALBEL_DURATION INT "," FILE_TABLE_MASK INT
#define FILE_TABLE_BYTE_SPACE(B) \
    B "," B "," B

#define _def_media_type_get(mask) mask &MEDIA_TYPE_PHOTO     ? FILE_DOT_PHOTO  \
                                  : mask &MEDIA_TYPE_VIDEO   ? FILE_DOT_VIDEO  \
                                  : mask &MEDIA_TYPE_AUDIO   ? FILE_DOT_AUDIO  \
                                  : mask &MEDIA_TYPE_CUSTOM1 ? FILE_DOT_UNKNOW \
                                  : mask &MEDIA_TYPE_CUSTOM2 ? FILE_DOT_UNKNOW \
                                  : mask &MEDIA_TYPE_CUSTOM3 ? FILE_DOT_UNKNOW \
                                  : mask &MEDIA_TYPE_CUSTOM4 ? FILE_DOT_UNKNOW \
                                                             : NULL;
#define _def_media_file_get(base_dir, tm, duration, dot, file, file_size, timestamp) \
    tm.tm_year -= 1900;                                                              \
    tm.tm_mon -= 1;                                                                  \
    timestamp = mktime(&tm);                                                         \
    tm.tm_year += 1900;                                                              \
    tm.tm_mon += 1;                                                                  \
    snprintf(file, file_size - 1, "%s%s%d/%d/%d/%lu_%d.%s", base_dir, MEDIA_FILE_PATH, tm.tm_year, tm.tm_mon, tm.tm_mday, timestamp, duration, dot);

#define _def_media_mask_init(info, new, lock, type, mode, channel) \
    info.mask = (0xFFFFFFFF & ((new << 0) | (lock << 1) | (type << 2) | (mode << 9) | (channel < 17)));

#define _def_media_info_init(info, timestamp, duration, new, lock, type, mode, channel) \
    snprintf(info.timestamp, MEDIA_TIMESTAP_LEN, "%010ld", timestamp);                  \
    snprintf(info.duration, MEDIA_DURTION_LEN, "%03d", duration);                       \
    _def_media_mask_init(info, new, lock, type, mode, channel);

#define _def_data_ture_adding(nembers, node, yes) \
    if (yes)                                      \
    {                                             \
        numbers[node]++;                          \
    }
/*格式:总数(9999) 已读(0010) 未读(0020) 未锁(0050) 已锁(0030) 照片(0010) 视频(0030) 音频(0034) 预留1(0000) 预留2(0000) 预留3 预留4 手动0020 自动0040 侦测0100 警报5000 预留1(0000) 预留2(0000) 预留3(0000) 通道0 通道1 通道2 通道3 通道4 通道5 通道6 通道7 通道8 通道9 通道10 通道11 通道12 通道13 通道14 通道15*/
#define _def_media_mask_adding(numbers, year, month, day, mask)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
    numbers[0] = year;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
    numbers[1] = month;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
    numbers[2] = day;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
    _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_TOTAL, true);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
    _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_OLD, (mask & MEDIA_ATTR_OLD)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_NEW, (mask & MEDIA_ATTR_NEW));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
    _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_PHOTO, mask &MEDIA_TYPE_PHOTO) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_VIDEO, (mask & MEDIA_TYPE_VIDEO)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_AUDIO, (mask & MEDIA_TYPE_AUDIO)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_CUTOM1, (mask & MEDIA_TYPE_CUSTOM1)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_CUTOM2, (mask & MEDIA_TYPE_CUSTOM2)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_CUTOM3, (mask & MEDIA_TYPE_CUSTOM3)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBERS_TYPE_CUTOM4, (mask & MEDIA_TYPE_CUSTOM4)); \
    _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_MUAUTAL, (mask & MEDIA_MODE_MANUAL)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_AUTO, (mask & MEDIA_MODE_AUTO)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_MOTION, (mask & MEDIA_MODE_MOTION)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_ALARM, (mask & MEDIA_MODE_ALARM)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_CUTOM1, (mask & MEDIA_MODE_CUSTOM1)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_CUTOM2, (mask & MEDIA_MODE_CUSTOM2)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_MODE_CUTOM3, (mask & MEDIA_MODE_CUSTOM3));     \
    _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_0, (mask & MEDIA_CHANNEL_0)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_1, (mask & MEDIA_CHANNEL_1)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_2, (mask & MEDIA_CHANNEL_2)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_3, (mask & MEDIA_CHANNEL_3)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_4, (mask & MEDIA_CHANNEL_4)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_5, (mask & MEDIA_CHANNEL_5)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_6, (mask & MEDIA_CHANNEL_6)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_7, (mask & MEDIA_CHANNEL_7)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_8, (mask & MEDIA_CHANNEL_8)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_9, (mask & MEDIA_CHANNEL_9)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_10, (mask & MEDIA_CHANNEL_10)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_11, (mask & MEDIA_CHANNEL_11)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_12, (mask & MEDIA_CHANNEL_12)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_13, (mask & MEDIA_CHANNEL_13)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_14, (mask & MEDIA_CHANNEL_14)) else _def_data_ture_adding(numbers, MEDIA_MASK_NUMBER_CHANNEL_15, (mask & MEDIA_CHANNEL_15));

#define _def_media_string_adding_format(string, n, year, month, day, numbers)                                                                                                                                                                                                                                                \
    snprintf(string, n, "%lu,%lu,%lu,"                                                                                                                                                                                                                                                                                       \
                        "%lu,"                                                                                                                                                                                                                                                                                               \
                        "%lu,%lu,"                                                                                                                                                                                                                                                                                           \
                        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,"                                                                                                                                                                                                                                                                       \
                        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,"                                                                                                                                                                                                                                                                       \
                        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu",                                                                                                                                                                                                                                   \
             numbers[0], numbers[1], numbers[2],                                                                                                                                                                                                                                                                             \
             numbers[MEDIA_MASK_NUMBER_TOTAL],                                                                                                                                                                                                                                                                               \
             numbers[MEDIA_MASK_NUMBERS_OLD], numbers[MEDIA_MASK_NUMBERS_NEW],                                                                                                                                                                                                                                               \
             numbers[MEDIA_MASK_NUMBERS_TYPE_PHOTO], numbers[MEDIA_MASK_NUMBERS_TYPE_VIDEO], numbers[MEDIA_MASK_NUMBERS_TYPE_AUDIO], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM1], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM2], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM3], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM4],                     \
             numbers[MEDIA_MASK_NUMBER_MODE_MUAUTAL], numbers[MEDIA_MASK_NUMBER_MODE_AUTO], numbers[MEDIA_MASK_NUMBER_MODE_MOTION], numbers[MEDIA_MASK_NUMBER_MODE_ALARM], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM1], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM2], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM3],                           \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_0], numbers[MEDIA_MASK_NUMBER_CHANNEL_1], numbers[MEDIA_MASK_NUMBER_CHANNEL_2], numbers[MEDIA_MASK_NUMBER_CHANNEL_3], numbers[MEDIA_MASK_NUMBER_CHANNEL_4], numbers[MEDIA_MASK_NUMBER_CHANNEL_5], numbers[MEDIA_MASK_NUMBER_CHANNEL_6], numbers[MEDIA_MASK_NUMBER_CHANNEL_7], \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_8], numbers[MEDIA_MASK_NUMBER_CHANNEL_9], numbers[MEDIA_MASK_NUMBER_CHANNEL_10], numbers[MEDIA_MASK_NUMBER_CHANNEL_11], numbers[MEDIA_MASK_NUMBER_CHANNEL_12], numbers[MEDIA_MASK_NUMBER_CHANNEL_13],                                                                         \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_14], numbers[MEDIA_MASK_NUMBER_CHANNEL_15]);

#define _def_media_string_update_format(string, n, numbers)                                                                                                                                                                                                                                              \
    snprintf(string, n, MEDIA_TABLE_CREATE("=%lu"),                                                                                                                                                                                                                                                      \
             numbers[0], numbers[1], numbers[2],                                                                                                                                                                                                                                                         \
             numbers[MEDIA_MASK_NUMBER_TOTAL],                                                                                                                                                                                                                                                           \
             numbers[MEDIA_MASK_NUMBERS_OLD], numbers[MEDIA_MASK_NUMBERS_NEW],                                                                                                                                                                                                                           \
             numbers[MEDIA_MASK_NUMBERS_TYPE_PHOTO], numbers[MEDIA_MASK_NUMBERS_TYPE_VIDEO], numbers[MEDIA_MASK_NUMBERS_TYPE_AUDIO], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM1], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM2], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM3], numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM4], \
             numbers[MEDIA_MASK_NUMBER_MODE_MUAUTAL], numbers[MEDIA_MASK_NUMBER_MODE_AUTO], numbers[MEDIA_MASK_NUMBER_MODE_MOTION], numbers[MEDIA_MASK_NUMBER_MODE_ALARM], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM1], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM2], numbers[MEDIA_MASK_NUMBER_MODE_CUTOM3],       \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_0], numbers[MEDIA_MASK_NUMBER_CHANNEL_1], numbers[MEDIA_MASK_NUMBER_CHANNEL_2], numbers[MEDIA_MASK_NUMBER_CHANNEL_3], numbers[MEDIA_MASK_NUMBER_CHANNEL_4], numbers[MEDIA_MASK_NUMBER_CHANNEL_5], numbers[MEDIA_MASK_NUMBER_CHANNEL_6],                   \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_7], numbers[MEDIA_MASK_NUMBER_CHANNEL_8], numbers[MEDIA_MASK_NUMBER_CHANNEL_9], numbers[MEDIA_MASK_NUMBER_CHANNEL_10], numbers[MEDIA_MASK_NUMBER_CHANNEL_11], numbers[MEDIA_MASK_NUMBER_CHANNEL_12], numbers[MEDIA_MASK_NUMBER_CHANNEL_13],               \
             numbers[MEDIA_MASK_NUMBER_CHANNEL_14], numbers[MEDIA_MASK_NUMBER_CHANNEL_15]);

#define _def_string_filter_data(string, mask, attri, column, filter, count) \
    if (mask & attri)                                                       \
    {                                                                       \
        if (count != 0)                                                     \
        {                                                                   \
            strcat(string, " OR ");                                         \
        }                                                                   \
        count++;                                                            \
        strcat(string, column);                                             \
        strcat(string, filter);                                             \
    }

/* 四种掩码类型之间为 AND 关系，同种类型内部为 OR 关系 */
/* 若某类别掩码为全选(ALL)或无(0)，则跳过该类别的过滤 */
#define _def_media_category_and_filter(string, mask, filter, cat_all, cat_name) \
    do                                                                          \
    {                                                                           \
        unsigned long _cat = (mask) & (cat_all);                                \
        if (_cat != 0 && _cat != (cat_all))                                     \
        {                                                                       \
            int _c = 0;                                                         \
            strcat(string, " AND (");                                           \
            cat_name                                                            \
                strcat(string, ")");                                            \
        }                                                                       \
    } while (0)

#define _def_media_string_filter_format(string, mask, filter)                                                             \
    if (mask & 0xFFFFFFFF)                                                                                                \
    {                                                                                                                     \
        strcat(string, TABLE_4 ">0");                                                                                     \
        _def_media_category_and_filter(string, mask, filter, MEDIA_ATTR_ALL,                                              \
                                       _def_string_filter_data(string, mask, MEDIA_ATTR_OLD, TABLE_5, filter, _c);        \
                                       _def_string_filter_data(string, mask, MEDIA_ATTR_NEW, TABLE_6, filter, _c););      \
        _def_media_category_and_filter(string, mask, filter, MEDIA_TYPE_ALL,                                              \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_PHOTO, TABLE_7, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_VIDEO, TABLE_8, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_AUDIO, TABLE_9, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_CUSTOM1, TABLE_10, filter, _c);   \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_CUSTOM2, TABLE_11, filter, _c);   \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_CUSTOM3, TABLE_12, filter, _c);   \
                                       _def_string_filter_data(string, mask, MEDIA_TYPE_CUSTOM4, TABLE_13, filter, _c);); \
        _def_media_category_and_filter(string, mask, filter, MEDIA_MODE_ALL,                                              \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_MANUAL, TABLE_14, filter, _c);    \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_AUTO, TABLE_15, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_MOTION, TABLE_16, filter, _c);    \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_ALARM, TABLE_17, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_CUSTOM1, TABLE_18, filter, _c);   \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_CUSTOM2, TABLE_19, filter, _c);   \
                                       _def_string_filter_data(string, mask, MEDIA_MODE_CUSTOM3, TABLE_20, filter, _c);); \
        _def_media_category_and_filter(string, mask, filter, MEDIA_CHANNEL_ALL,                                           \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_0, TABLE_21, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_1, TABLE_22, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_2, TABLE_23, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_3, TABLE_24, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_4, TABLE_25, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_5, TABLE_26, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_6, TABLE_27, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_7, TABLE_28, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_8, TABLE_29, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_9, TABLE_30, filter, _c);      \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_10, TABLE_31, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_11, TABLE_32, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_12, TABLE_33, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_13, TABLE_34, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_14, TABLE_35, filter, _c);     \
                                       _def_string_filter_data(string, mask, MEDIA_CHANNEL_15, TABLE_36, filter, _c););   \
    }
#define _def_file_databases_foramt(string, len, year, month, day) \
    snprintf(string, len - 1, "%s%s/%d/%d/%d/%s", ctx->base_dir, MEDIA_FILE_PATH, year, month, day, FILE_DATABASES);

typedef struct
{
    unsigned long media_total;
    unsigned long media_index;
    char *media_sqlite_where;
    char *media_sqlite_select;
    sqlite_context media_sqlite_ctx;

    unsigned long new_total;

    unsigned long all_total;
    unsigned long all_index;

    unsigned long page_total;
    unsigned long page_index;

    unsigned long file_total;
    unsigned long file_index;
    int page_file_num;
    int page_file_num_max;
    char *file_sqlite_dir;
    char *file_sqlite_where;
    char *file_sqlite_select;
    char *file_sqlite_limit;
    int duration;
    unsigned long timestmap;
    sqlite_context file_sqlite_ctx;

    char *base_dir;
    pthread_mutex_t mutex;
    media_file_info_t *pinfo;
    unsigned long mask;
    struct tm tm;

    /* 日期范围过滤（0 表示不过滤） */
    int filter_year;
    int filter_month;
    int filter_day;
} media_context_t;
static void _Amy_media_date_count_callback(void *stmt, void *userdata, int base, int total);
/*******************************************************************
 * @brief  : 构建完整 WHERE 子句（mask 过滤 + 日期范围过滤）
 *******************************************************************/
static void _Amy_media_where_build(media_context_t *ctx, char *where, int size)
{
    char temp[128];
    _def_media_string_filter_format(where, ctx->mask, ">0");

    if (ctx->filter_year > 0)
    {
        snprintf(temp, sizeof(temp) - 1, " AND %s=%d ", TABLE_1, ctx->filter_year);
        strcat(where, temp);
    }
    if (ctx->filter_month > 0)
    {
        snprintf(temp, sizeof(temp) - 1, " AND %s=%d ", TABLE_2, ctx->filter_month);
        strcat(where, temp);
    }
    if (ctx->filter_day > 0)
    {
        snprintf(temp, sizeof(temp) - 1, " AND %s=%d ", TABLE_3, ctx->filter_day);
        strcat(where, temp);
    }
}

#define media_mutex_lock(ctx)            \
    if (ctx != NULL)                     \
    {                                    \
        pthread_mutex_lock(&ctx->mutex); \
    }
#define media_mutex_unlock(ctx)            \
    if (ctx != NULL)                       \
    {                                      \
        pthread_mutex_unlock(&ctx->mutex); \
    }
#define media_mutex_init(ctx)                  \
    if (ctx != NULL)                           \
    {                                          \
        pthread_mutex_init(&ctx->mutex, NULL); \
    }
#define media_mutex_destroy(ctx)            \
    if (ctx != NULL)                        \
    {                                       \
        pthread_mutex_destroy(&ctx->mutex); \
    }

const media_file_info_t *_Amy_media_prev_media_info(media_context_t *ctx);
const media_file_info_t *_Amy_media_next_media_info(media_context_t *ctx);

static int _Amy_media_file_path_get(const char *base_dir, int duration, struct tm *t, const char *dot, char *path, int path_len, unsigned long *timestamp)
{
    int count = 0;
    do
    {
        memset(path, 0, path_len);
        _def_media_file_get(base_dir, (*t), duration, dot, path, path_len - 1, (*timestamp));
        if (access(path, F_OK) != 0)
        {
            break;
        }
        t->tm_sec++;
        t->tm_sec %= 60;
        if ((count++) == 60)
        {
            AMY_LOG_ERROR("[%s:%d] Too many duplicate file names\n");
            return -1;
        }
    } while (1);
    return 0;
}

static int _Amy_media_create_param_init(media_context_t *ctx, int duration, const struct tm *tm, char *path, int path_len)
{
    struct tm t;
    if (tm == NULL)
    {
        Amy_localtime(t);
    }
    else
    {
        t = *tm;
    }
    char *dot = _def_media_type_get(ctx->mask);
    if (dot == NULL)
    {
        AMY_LOG_ERROR("edia type failed(%lx)\n", ctx->mask);
        return -1;
    }
    if (_Amy_media_file_path_get(ctx->base_dir, duration, &t, dot, path, path_len, &ctx->timestmap) != 0)
    {
        AMY_LOG_ERROR("get path failed\n");
        return -1;
    }

    ctx->tm = t;
    ctx->duration = duration;
    return 0;
}

static sqlite_context _Amy_media_sqlite_ctx_open(media_context_t *ctx)
{
    char media_dir[64] = {0};
    char databases_dir[128] = {0};
    snprintf(media_dir, sizeof(media_dir) - 1, "%s%s", ctx->base_dir, MEDIA_FILE_PATH);
    snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s", media_dir, MEDIA_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open sqlite ctx failed\n");
        return NULL;
    }
    if (!sqlite_table_exist(sqlite_ctx, MEDIA_DATA_TABLE))
    {
        sqlite_table_close(sqlite_ctx);
        char *param = MEDIA_TABLE_CREATE(TABLE_INT);
        sqlite_ctx = sqlite_table_create(databases_dir, MEDIA_DATA_TABLE, param);
        if (sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR("Media table create failed\n");
            return NULL;
        }
        if (sqlite_table_index_create(sqlite_ctx, MEDIA_DATA_TABLE, MEDIA_DATA_INDEX, "Year,Month,Day,TOTAL") != 0)
        {
            sqlite_table_close(sqlite_ctx);
            AMY_LOG_ERROR("Media table index create failed\\n");
            return NULL;
        }
    }
    return sqlite_ctx;
}

static sqlite_context _Amy_media_context_create(media_context_t *ctx)
{
    sqlite_context sqlite_cxt = _Amy_media_sqlite_ctx_open(ctx);
    if (sqlite_cxt == NULL)
    {
        AMY_LOG_ERROR("open sqlite ctx failed\n");
        return NULL;
    }
    sqlite_table_insert_prepare(sqlite_cxt, MEDIA_DATA_TABLE, MEDIA_TABLE_CREATE(""), MEDIA_TABLE_BYTE_SPACE("?"));
    sqlite_table_query_prepare(sqlite_cxt, "*", MEDIA_DATA_TABLE, TABLE_1 "=? AND " TABLE_2 "=? AND " TABLE_3 "=?", TABLE_1 " DESC", "1", "0");
    sqlite_table_update_prepare(sqlite_cxt, MEDIA_DATA_TABLE, TABLE_1 "=? AND " TABLE_2 "=? AND " TABLE_3 "=?", MEDIA_TABLE_CREATE("=?"));
    return sqlite_cxt;
}

static int _Amy_file_sqlite_database_dir_get(media_context_t *ctx, char *string, int len)
{
    return snprintf(string, len - 1, "%s%s%d/%d/%d/", ctx->base_dir, MEDIA_FILE_PATH, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday);
}

static sqlite_context _Amy_file_sqlite_ctx_open(media_context_t *ctx)
{
    char db_path[256] = {0};
    sprintf(db_path, "%s%s", ctx->file_sqlite_dir, FILE_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(db_path, FILE_DATA_TABEL);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open sqlite ctx failed\n");
        return NULL;
    }
    if (!sqlite_table_exist(sqlite_ctx, FILE_DATA_TABEL))
    {
        sqlite_table_close(sqlite_ctx);
        char param[1024] = FILE_TABLE_CREATE(TABLE_INT);
        sqlite_ctx = sqlite_table_create(db_path, FILE_DATA_TABEL, param);
        if (sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR("File tabel create failed\n");
            return NULL;
        }

        if (sqlite_table_index_create(sqlite_ctx, FILE_DATA_TABEL, FILE_DATA_INDEX, FILE_TALBEL_TIMESTAMP "," FILE_TABLE_MASK) != 0)
        {
            sqlite_table_close(sqlite_ctx);
            AMY_LOG_ERROR("File tabel index create failed\n");
            return NULL;
        }
    }
    return sqlite_ctx;
}

static sqlite_context _Amy_file_context_create(media_context_t *ctx)
{
    sqlite_context sqlite_cxt = _Amy_file_sqlite_ctx_open(ctx);
    if (sqlite_cxt == NULL)
    {
        AMY_LOG_ERROR("open sqlite ctx failed\n");
        return NULL;
    }
    sqlite_table_insert_prepare(sqlite_cxt, FILE_DATA_TABEL, FILE_TABLE_CREATE(""), FILE_TABLE_BYTE_SPACE("?"));
    return sqlite_cxt;
}

static int _Amy_media_sqlite_context_init(media_context_t *ctx)
{
    /*获取media sqlite*/
    if (ctx->media_sqlite_ctx == NULL)
    {
        ctx->media_sqlite_ctx = _Amy_media_context_create(ctx);
        if (ctx->media_sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR("open sqlite ctx failed\n");
            return -1;
        }
    }
    char file_sqlite_dir[128] = {0};
    _Amy_file_sqlite_database_dir_get(ctx, file_sqlite_dir, sizeof(file_sqlite_dir));
    if ((ctx->file_sqlite_ctx == NULL) || (ctx->file_sqlite_dir == NULL) || strcmp(ctx->file_sqlite_dir, file_sqlite_dir))
    {
        if (ctx->file_sqlite_ctx != NULL)
        {
            sqlite_table_close(ctx->file_sqlite_ctx);
            ctx->file_sqlite_ctx = NULL;
        }
        if (ctx->file_sqlite_dir != NULL)
        {
            free(ctx->file_sqlite_dir);
            ctx->file_sqlite_dir = NULL;
        }
        ctx->file_sqlite_dir = strdup(file_sqlite_dir);

        ctx->file_sqlite_ctx = _Amy_file_context_create(ctx);
        if (ctx->file_sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR("open sqlite ctx failed\n");
            return -1;
        }
    }
    return 0;
}
static void _Amy_media_sqlite_table_query_callback(void *stmt, void *userdata, int base, int total)
{
    unsigned long *numbers = (unsigned long *)userdata;
    for (int i = base; i < total; i++)
    {
        numbers[i] = sqlite3_column_int64(stmt, i);
    }
#if 0
    printf("\nYear=%lu, Month=%lu, Day=%lu,"
        "TOTAL=%lu,"
        "OLD=%lu,NEW=%lu,"
        "PHOTO=%lu,VIDEO=%lu,AUDIO=%lu,TCUTOM1=%lu,TCUTOM2=%lu,TCUTOM3=%lu,"
        "MUAUTAL=%lu,AUTO=%lu,MOTION=%lu,ALARM=%lu,MCUTOM1=%lu,MCUTOM2=%lu,MCUTOM3=%lu,"
        "CHANNEL0=%lu,CHANNEL1=%lu,CHANNEL2=%lu,CHANNEL3=%lu,CHANNEL4=%lu,CHANNEL5=%lu,CHANNEL6=%lu,CHANEL7=%lu,CHANNEL8=%lu,CHANNEL9=%lu,CHANNEL10=%lu,CHANNEL11=%lu,CHANNEL12=%lu,CHANNEL13=%lu,CHANNEL14=%lu,CHANNEL15=%lu\n",
        numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], numbers[8], numbers[9], numbers[10], numbers[11], numbers[12], numbers[13], numbers[14], numbers[15], numbers[16], numbers[17], numbers[18], numbers[19], numbers[20], numbers[21], numbers[22], numbers[23], numbers[24], numbers[25], numbers[26], numbers[27], numbers[28], numbers[29], numbers[30], numbers[31], numbers[32], numbers[33], numbers[34]);
#endif
}

static unsigned long _Amy_media_type_count_from_numbers(const unsigned long *numbers, unsigned long mask)
{
    unsigned long value = 0;
    if (mask & MEDIA_TYPE_ALL)
    {
        if (mask & MEDIA_TYPE_PHOTO)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_PHOTO];
        if (mask & MEDIA_TYPE_VIDEO)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_VIDEO];
        if (mask & MEDIA_TYPE_AUDIO)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_AUDIO];
        if (mask & MEDIA_TYPE_CUSTOM1)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM1];
        if (mask & MEDIA_TYPE_CUSTOM2)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM2];
        if (mask & MEDIA_TYPE_CUSTOM3)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM3];
        if (mask & MEDIA_TYPE_CUSTOM4)
            value += numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM4];
    }
    else
    {
        value = numbers[MEDIA_MASK_NUMBER_TOTAL];
    }
    return value;
}

struct _media_date_file_total_ctx
{
    unsigned long mask;
    int total;
};

static void _Amy_media_date_file_total_callback(void *stmt, void *userdata, int base, int total)
{
    struct _media_date_file_total_ctx *d = (struct _media_date_file_total_ctx *)userdata;
    unsigned long value = 0;
    if (d->mask & MEDIA_TYPE_ALL)
    {
        if (d->mask & MEDIA_TYPE_PHOTO)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_PHOTO);
        if (d->mask & MEDIA_TYPE_VIDEO)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_VIDEO);
        if (d->mask & MEDIA_TYPE_AUDIO)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_AUDIO);
        if (d->mask & MEDIA_TYPE_CUSTOM1)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_CUTOM1);
        if (d->mask & MEDIA_TYPE_CUSTOM2)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_CUTOM2);
        if (d->mask & MEDIA_TYPE_CUSTOM3)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_CUTOM3);
        if (d->mask & MEDIA_TYPE_CUSTOM4)
            value += sqlite3_column_int64(stmt, MEDIA_MASK_NUMBERS_TYPE_CUTOM4);
    }
    else
    {
        value = sqlite3_column_int64(stmt, MEDIA_MASK_NUMBER_TOTAL);
    }
    d->total = (int)value;
}

/*******************************************************************
 * @brief  : 构建文件级 WHERE 子句（四种掩码类型 AND 关系，同类 OR）
 *******************************************************************/
static void _Amy_media_file_where_build(char *where, int size, unsigned long mask)
{
    unsigned long cat_masks[] = {
        mask & MEDIA_ATTR_ALL,
        mask & MEDIA_TYPE_ALL,
        mask & MEDIA_MODE_ALL,
        mask & MEDIA_CHANNEL_ALL,
    };
    unsigned long cat_alls[] = {
        MEDIA_ATTR_ALL,
        MEDIA_TYPE_ALL,
        MEDIA_MODE_ALL,
        MEDIA_CHANNEL_ALL,
    };
    int pos = 0;
    for (int i = 0; i < 4; i++)
    {
        if (cat_masks[i] != 0 && cat_masks[i] != cat_alls[i])
        {
            if (pos > 0)
                pos += snprintf(where + pos, size - pos, " AND ");
            pos += snprintf(where + pos, size - pos, FILE_TABLE_MASK "&%lu", cat_masks[i]);
        }
    }
}

/*******************************************************************
 * @brief  : 从 file.db 统计指定日期下匹配掩码的文件数
 *******************************************************************/
static unsigned long _Amy_media_file_count_by_date_mask(media_context_t *ctx, int year, int month, int day, unsigned long mask)
{
    char file_where[256] = {0};
    char databases_dir[128] = {0};
    _def_file_databases_foramt(databases_dir, sizeof(databases_dir) - 1, year, month, day);
    _Amy_media_file_where_build(file_where, sizeof(file_where), mask);

    if (access(databases_dir, F_OK) != 0)
        return 0;

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, FILE_DATA_TABEL);
    if (sqlite_ctx == NULL)
        return 0;

    int count = 0;
    if (sqlite_table_query_prepare(sqlite_ctx, "COUNT(*)", FILE_DATA_TABEL,
                                   file_where, FILE_TALBEL_TIMESTAMP, "1", "0") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_count_callback, &count, 0, 1);
    }
    sqlite_table_close(sqlite_ctx);
    return (unsigned long)count;
}

static unsigned long _Amy_media_file_count_by_date(media_context_t *ctx, int year, int month, int day)
{
    return _Amy_media_file_count_by_date_mask(ctx, year, month, day, ctx->mask);
}

/*******************************************************************
 * @brief  : 判断是否需要多类别交叉过滤（TYPE 以外的类别有部分过滤）
 *******************************************************************/
static bool _Amy_media_need_file_level_count(unsigned long mask)
{
    unsigned long mode = mask & MEDIA_MODE_ALL;
    unsigned long chan = mask & MEDIA_CHANNEL_ALL;
    unsigned long attr = mask & MEDIA_ATTR_ALL;
    if (mode != 0 && mode != MEDIA_MODE_ALL)
        return true;
    if (chan != 0 && chan != MEDIA_CHANNEL_ALL)
        return true;
    if (attr != 0 && attr != MEDIA_ATTR_ALL)
        return true;
    return false;
}

/*******************************************************************
 * @brief  : 计算当前日期匹配掩码的文件总数
 *           与 all_total 保持同一套判定：存在部分 MODE/CHANNEL/ATTR
 *           过滤时走文件级计数，否则直接累加类型计数
 *******************************************************************/
static unsigned long _Amy_media_date_file_total_get(media_context_t *ctx, const unsigned long *numbers)
{
    if (_Amy_media_need_file_level_count(ctx->mask))
    {
        return _Amy_media_file_count_by_date(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday);
    }
    return _Amy_media_type_count_from_numbers(numbers, ctx->mask);
}

static void _Amy_media_sqlite_table_total_query_callback(void *stmt, void *userdata, int base, int total)
{
    media_context_t *ctx = (media_context_t *)userdata;
    unsigned long value = 0;

    int year = (int)sqlite3_column_int64(stmt, 0);
    int month = (int)sqlite3_column_int64(stmt, 1);
    int day = (int)sqlite3_column_int64(stmt, 2);

    if (_Amy_media_need_file_level_count(ctx->mask))
    {
        value = _Amy_media_file_count_by_date(ctx, year, month, day);
        ctx->new_total += _Amy_media_file_count_by_date_mask(ctx, year, month, day, ctx->mask | MEDIA_ATTR_NEW);
    }
    else
    {
        if (ctx->mask & MEDIA_TYPE_ALL)
        {
            if (ctx->mask & MEDIA_TYPE_PHOTO)
                value += sqlite3_column_int64(stmt, 6);
            if (ctx->mask & MEDIA_TYPE_VIDEO)
                value += sqlite3_column_int64(stmt, 7);
            if (ctx->mask & MEDIA_TYPE_AUDIO)
                value += sqlite3_column_int64(stmt, 8);
            if (ctx->mask & MEDIA_TYPE_CUSTOM1)
                value += sqlite3_column_int64(stmt, 9);
            if (ctx->mask & MEDIA_TYPE_CUSTOM2)
                value += sqlite3_column_int64(stmt, 10);
            if (ctx->mask & MEDIA_TYPE_CUSTOM3)
                value += sqlite3_column_int64(stmt, 11);
            if (ctx->mask & MEDIA_TYPE_CUSTOM4)
                value += sqlite3_column_int64(stmt, 12);
        }
        else
        {
            value = sqlite3_column_int64(stmt, 3);
        }
        ctx->new_total += sqlite3_column_int64(stmt, 5);
    }

    if (value > 0)
    {
        ctx->media_total++;
        ctx->all_total += value;
        ctx->page_total += (value / ctx->page_file_num_max) + (value % ctx->page_file_num_max ? 1 : 0);
    }
}
static int _Amy_media_databases_update_data(media_context_t *ctx)
{
    int i = 0;
    unsigned long numbers[MEDIA_MASK_MAX] = {0};
    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, ctx->tm.tm_year);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, ctx->tm.tm_mon);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 3, ctx->tm.tm_mday);

    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        _def_media_mask_adding(numbers, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->mask);
        sqlite_table_update_ready(ctx->media_sqlite_ctx);

        for (i = 0; i < MEDIA_MASK_MAX; i++)
        {
            sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, numbers[i]);
        }
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, ctx->tm.tm_year);
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 2, ctx->tm.tm_mon);
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 3, ctx->tm.tm_mday);
        return sqlite_table_update_commit(ctx->media_sqlite_ctx);
    }

    _def_media_mask_adding(numbers, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->mask);
    sqlite_table_insert_ready(ctx->media_sqlite_ctx);
    for (int i = 0; i < MEDIA_MASK_MAX; i++)
    {
        sqlite_table_insert_int64(ctx->media_sqlite_ctx, i + 1, numbers[i]);
    }
    return sqlite_table_insert_commit(ctx->media_sqlite_ctx);
}
static int _Amy_file_databases_update_data(media_context_t *ctx)
{
    sqlite_table_insert_ready(ctx->file_sqlite_ctx);
    sqlite_table_insert_int64(ctx->file_sqlite_ctx, 1, ctx->timestmap);
    sqlite_table_insert_int64(ctx->file_sqlite_ctx, 2, ctx->duration);
    sqlite_table_insert_int64(ctx->file_sqlite_ctx, 3, ctx->mask);
    return sqlite_table_insert_commit(ctx->file_sqlite_ctx);
}

static int _Amy_databases_insert_data(media_context_t *ctx)
{
    if (_Amy_media_databases_update_data(ctx) < 0)
    {
        AMY_LOG_ERROR("failed\n");
        return -1;
    }
    return _Amy_file_databases_update_data(ctx);
}

/*
 * 创建一个文件
 * media_type:文件类型，查看Amy_media_file.h
 * media_type num:文件类型数目,最大值为7
 * record_mode:记录模式，请看Amy_media_file.h
 * record_mode_num:记录模式数目，最大值为8
 * channel:通道号，请看Amy_media_file.h
 * channel_num:通道数目，最大值为16
 * duration:录制时长，最大值999秒
 * path:创建的文件路径
 * path_max:文件路径buffer的最大值
 * return :0 success,非0初始化失败
 */
int Amy_media_insert(Amy_media_context context, int duration, const struct tm *tm, char *path, int path_max)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    if (_Amy_media_create_param_init(ctx, duration, tm, path, path_max) != 0)
    {
        AMY_LOG_ERROR("get media tm failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }
    if (_Amy_media_sqlite_context_init(ctx) != 0)
    {
        AMY_LOG_ERROR(" _Amy_media_sqlite_context_init failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }
    int rc = _Amy_databases_insert_data(ctx);
    media_mutex_unlock(ctx);
    return rc;
}

/*
 * 依据掩码打开媒体文件句柄
 *  bit[0]:1已读，0未读;
 *  bit[1]:0未锁，1已锁，
 *  bit[2:8]:1照片，2视频，4音频，8、16、32预留
 *  bit[9:16]:0手动，2自动，4侦测，8警报，16、32、64、128预留
 *  bit[17:32]
 */
Amy_media_context Amy_media_open(const char *base_dir, unsigned int mask, int page_num_max)
{
    if ((base_dir == NULL) || (base_dir[0] == '\0'))
    {
        return NULL;
    }
    media_context_t *ctx = (media_context_t *)malloc(sizeof(media_context_t));
    memset(ctx, 0, sizeof(media_context_t));
    media_mutex_init(ctx);

    ctx->base_dir = (char *)malloc(strlen(base_dir) + 1);
    strcpy(ctx->base_dir, base_dir);
    ctx->mask = mask;
    ctx->page_file_num_max = page_num_max;
    ctx->page_file_num = 0;
    ctx->media_index = 0;
    ctx->file_index = 0;
    ctx->all_index = 0;
    ctx->page_index = 0;
    ctx->media_sqlite_ctx = NULL;
    ctx->file_sqlite_ctx = NULL;
    if (page_num_max > 0)
    {
        ctx->pinfo = (media_file_info_t *)malloc(sizeof(media_file_info_t) * page_num_max);
        if (ctx->pinfo == NULL)
        {
            AMY_LOG_ERROR("param failed\n");
            return NULL;
        }
    }

    return (Amy_media_context)ctx;
}
/*
 * 修改掩码
 */
int Amy_media_mask_change(Amy_media_context *context, unsigned int mask)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    ctx->mask = mask;
    media_mutex_unlock(ctx);
    return 0;
}

static void _Amy_file_sqlite_table_query_callback(void *stmt, void *userdata, int base, int total)
{
    media_context_t *ctx = (media_context_t *)userdata;
    snprintf(ctx->pinfo[ctx->page_file_num].timestamp, MEDIA_TIMESTAP_LEN, "%llu", sqlite3_column_int64(stmt, 0));
    snprintf(ctx->pinfo[ctx->page_file_num].duration, MEDIA_DURTION_LEN, "%llu", sqlite3_column_int64(stmt, 1));
    ctx->pinfo[ctx->page_file_num].mask = sqlite3_column_int64(stmt, 2);
    ctx->page_file_num++;
}

static int _Amy_databases_file_context_get(media_context_t *ctx, const char *databases_dir, const char *where, const char *select, const char *limit)
{
    bool reset_query = false;
    if (ctx->file_sqlite_ctx == NULL)
    {
        reset_query = true;
    }

    if ((ctx->file_sqlite_dir == NULL) || (strcmp(ctx->file_sqlite_dir, databases_dir)))
    {
        if (ctx->file_sqlite_dir != NULL)
        {
            free(ctx->file_sqlite_dir);
        }
        if (ctx->file_sqlite_ctx != NULL)
        {
            sqlite_table_close(ctx->file_sqlite_ctx);
            ctx->file_sqlite_ctx = NULL;
        }

        ctx->file_sqlite_dir = strdup(databases_dir);
        reset_query = true;
    }

    if ((ctx->file_sqlite_where == NULL) || (strcmp(ctx->file_sqlite_where, where)))
    {
        if (ctx->file_sqlite_where != NULL)
        {
            free(ctx->file_sqlite_where);
        }
        ctx->file_sqlite_where = strdup(where);
        reset_query = true;
    }

    if ((ctx->file_sqlite_select == NULL) || (strcmp(ctx->file_sqlite_select, select)))
    {
        if (ctx->file_sqlite_select != NULL)
        {
            free(ctx->file_sqlite_select);
        }
        ctx->file_sqlite_select = strdup(select);
        reset_query = true;
    }

    if ((ctx->file_sqlite_limit == NULL) || (strcmp(ctx->file_sqlite_limit, limit)))
    {
        if (ctx->file_sqlite_limit != NULL)
        {
            free(ctx->file_sqlite_limit);
        }
        ctx->file_sqlite_limit = strdup(limit);
        reset_query = true;
    }
    if (ctx->file_sqlite_ctx == NULL)
    {
        // printf("%s\n", ctx->file_sqlite_dir);
        ctx->file_sqlite_ctx = sqlite_table_open(ctx->file_sqlite_dir, FILE_DATA_TABEL);
        if (ctx->file_sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR("param failed\n");
            return -1;
        }
        sqlite_table_update_prepare(ctx->file_sqlite_ctx, FILE_DATA_TABEL, FILE_TALBEL_TIMESTAMP "=? AND " FILE_TALBEL_DURATION "=? ", FILE_TABLE_CREATE("=?"));
        sqlite_table_delete_prepare(ctx->file_sqlite_ctx, FILE_DATA_TABEL, FILE_TALBEL_TIMESTAMP "=? AND " FILE_TALBEL_DURATION "=? ");
    }
    if (reset_query == true)
    {
        sqlite_table_query_prepare(ctx->file_sqlite_ctx, ctx->file_sqlite_select, FILE_DATA_TABEL, ctx->file_sqlite_where, FILE_TALBEL_TIMESTAMP " DESC", ctx->file_sqlite_limit, "?");
    }
    return 0;
}

static int _Amy_databases_file_reset_param(media_context_t *ctx, char *databases_dir, int size)
{
    char where[512] = {0};
    char limit[10] = {0};
    _def_file_databases_foramt(databases_dir, size - 1, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday);
    if (access(databases_dir, F_OK) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    if (ctx->file_sqlite_ctx != NULL)
    {
        sqlite_table_close(ctx->file_sqlite_ctx);
        ctx->file_sqlite_ctx = NULL;
    }
    ctx->file_index = ctx->all_index = ctx->page_index = 0;
    _Amy_media_file_where_build(where, sizeof(where), ctx->mask);
    sprintf(limit, "%d", ctx->page_file_num_max);
    return _Amy_databases_file_context_get(ctx, databases_dir, where, FILE_TALBEL_TIMESTAMP "," FILE_TALBEL_DURATION "," FILE_TABLE_MASK, limit);
}
/*
 * 获取最后一个记录的文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
static const media_file_info_t *_Amy_file_first_page(media_context_t *ctx, int *num)
{
    char databases_dir[128] = {0};
    // char param[128] = { 0 };

    if (ctx->pinfo == NULL)
    {
        AMY_LOG_ERROR("media pinfo is nullptr\n");
        return NULL;
    }

    if (_Amy_databases_file_reset_param(ctx, databases_dir, sizeof(databases_dir)) != 0)
    {
        AMY_LOG_ERROR("failed\n");
        return NULL;
    }
    sqlite_table_query_ready(ctx->file_sqlite_ctx);
    sqlite_table_query_int64(ctx->file_sqlite_ctx, 1, ctx->media_index);
    int rc = sqlite_table_query_commit(ctx->file_sqlite_ctx, _Amy_file_sqlite_table_query_callback, ctx, 0, 1);
    if (rc > 0)
    {
        if (num)
            *num = ctx->page_file_num;

        for (int i = 0; i < ctx->page_file_num; i++)
        {
            ctx->pinfo[i].year = ctx->tm.tm_year;
            ctx->pinfo[i].month = ctx->tm.tm_mon;
            ctx->pinfo[i].day = ctx->tm.tm_mday;
        }
        return ctx->pinfo;
    }
    return NULL;
}
static int _Amy_databases_media_context_get(media_context_t *ctx, unsigned long mask, const char *where, const char *select)
{
    bool reset_query = false;
    char databases_dir[128] = {0};
    if (ctx->media_sqlite_ctx == NULL)
    {
        snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);
        ctx->media_sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
        if (ctx->media_sqlite_ctx == NULL)
        {
            AMY_LOG_ERROR(" param failed(%s)\n", databases_dir);
            return -1;
        }
        sqlite_table_update_prepare(ctx->media_sqlite_ctx, MEDIA_DATA_TABLE, TABLE_1 "=? AND " TABLE_2 "=? AND " TABLE_3 "=?", MEDIA_TABLE_CREATE("=?"));
        sqlite_table_delete_prepare(ctx->media_sqlite_ctx, MEDIA_DATA_TABLE, TABLE_1 "=? AND " TABLE_2 "=? AND " TABLE_3 "=?");
    }

    if ((ctx->media_sqlite_where == NULL) || (strcmp(ctx->media_sqlite_where, where)))
    {
        if (ctx->media_sqlite_where)
        {
            free(ctx->media_sqlite_where);
        }
        ctx->media_sqlite_where = strdup(where);
        reset_query = true;
    }

    if ((ctx->media_sqlite_select == NULL) || (strcmp(ctx->media_sqlite_select, select)))
    {
        if (ctx->media_sqlite_select)
        {
            free(ctx->media_sqlite_select);
        }
        ctx->media_sqlite_select = strdup(select);
        reset_query = true;
    }
    if (ctx->mask != mask)
    {
        ctx->mask = mask;
        reset_query = true;
    }

    if (reset_query == true)
    {
        if (sqlite_table_query_prepare(ctx->media_sqlite_ctx, ctx->media_sqlite_select, MEDIA_DATA_TABLE, ctx->media_sqlite_where, TABLE_1 " DESC, " TABLE_2 " DESC, " TABLE_3 " DESC ", "?", "?") != 0)
        {
            AMY_LOG_ERROR("param failed\n");
            return -1;
        }
    }
    return 0;
}

static int _Amy_databases_media_reset_param(media_context_t *ctx, const char *where)
{
    ctx->media_index = 0;
#if 0
    if (ctx->media_sqlite_ctx != NULL) {
        sqlite_table_close(ctx->media_sqlite_ctx);
        ctx->media_sqlite_ctx = NULL;
    }
#endif
    char *select = MEDIA_TABLE_CREATE("");
    if (_Amy_databases_media_context_get(ctx, ctx->mask, where, select) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    return 0;
}

static int _Amy_media_file_databases_query(media_context_t *ctx, int year, int month, int day, int index, int nummax)
{
    char where[512] = {0};
    char limit[10] = {0};
    char databases_dir[128] = {0};
    _def_file_databases_foramt(databases_dir, sizeof(databases_dir) - 1, year, month, day);
    if (access(databases_dir, F_OK) != 0)
    {
        AMY_LOG_ERROR(" param failed:%s\n", databases_dir);
        return -1;
    }

    _Amy_media_file_where_build(where, sizeof(where), ctx->mask);
    sprintf(limit, "%d", nummax);

    if (_Amy_databases_file_context_get(ctx, databases_dir, where, FILE_TALBEL_TIMESTAMP "," FILE_TALBEL_DURATION "," FILE_TABLE_MASK, limit) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }

    sqlite_table_query_ready(ctx->file_sqlite_ctx);
    sqlite_table_query_int64(ctx->file_sqlite_ctx, 1, index);
    ctx->page_file_num = 0;
    if (sqlite_table_query_commit(ctx->file_sqlite_ctx, _Amy_file_sqlite_table_query_callback, ctx, 0, 1) > 0)
    {
        return 0;
    }
    return -1;
}

static int _Amy_media_next_databases_proc(media_context_t *ctx)
{
    unsigned long numbers[MEDIA_MASK_MAX] = {0};

    int index = ctx->media_index + 1;
    // 此处判断有问题。
    if (index >= ctx->media_total)
    {
        ctx->page_index = ctx->all_index = index = 0;
    }
    else
    {
        ctx->all_index = ctx->all_index - ctx->file_index + ctx->file_total;
        ctx->page_index = ctx->page_index - (ctx->file_index / ctx->page_file_num_max) + (ctx->file_total / ctx->page_file_num_max + (ctx->file_total % ctx->page_file_num_max ? 1 : 0));
    }

    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, index);
    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        ctx->tm.tm_year = (int)numbers[0];
        ctx->tm.tm_mon = (int)numbers[1];
        ctx->tm.tm_mday = (int)numbers[2];
        ctx->file_total = _Amy_media_date_file_total_get(ctx, numbers);
        ctx->media_index = index;
        return 0;
    }
    return -1;
}

const media_file_info_t *Amy_media_first_page(Amy_media_context context, int *num)
{
    // char param[128] = { 0 };
    // char values[1024] = { 0 };
    char where[512] = {0};
    unsigned long numbers[MEDIA_MASK_MAX] = {0};
    const media_file_info_t *media_info = NULL;

    if (num)
        *num = 0;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    _Amy_media_where_build(ctx, where, sizeof(where));
    if (_Amy_databases_media_reset_param(ctx, where) != 0)
    {
        AMY_LOG_ERROR("failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    // 获取文件最新文件的信息
    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, ctx->media_index);
    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        ctx->tm.tm_year = (int)numbers[0];
        ctx->tm.tm_mon = (int)numbers[1];
        ctx->tm.tm_mday = (int)numbers[2];
        ctx->file_total = _Amy_media_date_file_total_get(ctx, numbers);
        media_info = _Amy_file_first_page(ctx, num);
    }

    // 获取媒体的总个数
    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 65535);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, 0);
    ctx->media_total = ctx->all_total = ctx->page_total = ctx->new_total = 0;
    sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_total_query_callback, ctx, 0, 1);
    AMY_LOG_INFO("all_total:[%lu] page_total:[%lu] media_total:[%lu]", ctx->all_total, ctx->page_total, ctx->media_total);
    media_mutex_unlock(ctx);
    return media_info;
}
/*
 * 获取当前记录文件的下一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t *Amy_media_next_page(Amy_media_context context, int *num)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    /*翻到下一页*/
    if ((ctx->file_index + ctx->page_file_num_max) >= ctx->file_total)
    {
        if (_Amy_media_next_databases_proc(ctx) != 0)
        {
            AMY_LOG_ERROR("param failed\n");
            media_mutex_unlock(ctx);
            return NULL;
        }
        ctx->file_index = 0;
    }
    /*正常翻页*/
    else
    {
        ctx->all_index = ctx->all_index + ctx->page_file_num_max;
        ctx->page_index++;
        ctx->file_index = ctx->file_index + ctx->page_file_num_max;
    }

    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, ctx->page_file_num_max) != 0)
    {
        AMY_LOG_ERROR(" param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    if (num)
        *num = ctx->page_file_num;
    for (int i = 0; i < ctx->page_file_num; i++)
    {
        ctx->pinfo[i].year = ctx->tm.tm_year;
        ctx->pinfo[i].month = ctx->tm.tm_mon;
        ctx->pinfo[i].day = ctx->tm.tm_mday;
    }

    media_mutex_unlock(ctx);
    return ctx->pinfo;
}

/*
 * 获取当前记录文件的下一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t *Amy_media_current_page(Amy_media_context context, int *num)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    // 重新调整显示页面，归为到页面的最初的索引号
    //  Amy_warning_log("file index:%d,max:%d\n", ctx->file_index, ctx->page_file_num_max);
    ctx->all_index = ctx->all_index - ctx->file_index;
    ctx->file_index = ctx->file_index / ctx->page_file_num_max * ctx->page_file_num_max;
    ctx->all_index = ctx->all_index + ctx->file_index;

    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, ctx->page_file_num_max) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    *num = ctx->page_file_num;
    for (int i = 0; i < ctx->page_file_num; i++)
    {
        ctx->pinfo[i].year = ctx->tm.tm_year;
        ctx->pinfo[i].month = ctx->tm.tm_mon;
        ctx->pinfo[i].day = ctx->tm.tm_mday;
    }

    media_mutex_unlock(ctx);
    return ctx->pinfo;
}

static int _Amy_media_prev_databases_proc(media_context_t *ctx)
{
    unsigned long numbers[MEDIA_MASK_MAX] = {0};

    int index = ctx->media_index - 1;
    if (index < 0)
    {
        index = ctx->media_total - 1;
        ctx->all_index = ctx->all_total;
        AMY_LOG_DEBUG(" ctx->all_index is %lu\r\n", ctx->all_index);
        ctx->page_index = ctx->page_total;
    }

    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, index);
    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        ctx->tm.tm_year = (int)numbers[0];
        ctx->tm.tm_mon = (int)numbers[1];
        ctx->tm.tm_mday = (int)numbers[2];
        ctx->file_total = _Amy_media_date_file_total_get(ctx, numbers);
        ctx->media_index = index;
        ctx->page_index = ctx->page_index - (ctx->file_total / ctx->page_file_num_max) - (ctx->file_total % ctx->page_file_num_max ? 1 : 0);
        return 0;
    }
    return -1;
}
/*
 * 获取当前记录文件的上一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t *Amy_media_prev_page(Amy_media_context context, int *num)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    /*翻到上一页*/
    if (ctx->file_index == 0)
    {
        if (_Amy_media_prev_databases_proc(ctx) != 0)
        {
            AMY_LOG_ERROR("param failed\n");
            media_mutex_unlock(ctx);
            return NULL;
        }
        if (ctx->file_total > ctx->page_file_num_max)
        {
            unsigned long total_index = ctx->file_total - 1;
            ctx->file_index = total_index - total_index % ctx->page_file_num_max;
        }
        else
        {
            ctx->file_index = 0;
        }
        ctx->all_index = ctx->all_index - ctx->file_total + ctx->file_index;
        ctx->page_index = ctx->page_index + (ctx->file_index / ctx->page_file_num_max);
    }
    /*翻到第一个文件*/
    else if (ctx->file_index < ctx->page_file_num_max)
    {
        ctx->all_index = ctx->all_index - ctx->file_index;
        ctx->file_index = 0;
    }
    /*正常翻页*/
    else
    {
        ctx->all_index = ctx->all_index - ctx->page_file_num_max;
        ctx->page_index--;
        ctx->file_index = ctx->file_index - ctx->page_file_num_max;
    }

    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, ctx->page_file_num_max) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    if (num)
        *num = ctx->page_file_num;
    for (int i = 0; i < ctx->page_file_num; i++)
    {
        ctx->pinfo[i].year = ctx->tm.tm_year;
        ctx->pinfo[i].month = ctx->tm.tm_mon;
        ctx->pinfo[i].day = ctx->tm.tm_mday;
    }

    media_mutex_unlock(ctx);
    return ctx->pinfo;
}
static int _Amy_media_attri_new_file_clean(media_context_t *ctx)
{
    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, 1) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }

    if ((ctx->page_file_num > 0) && (ctx->mask & MEDIA_ATTR_NEW))
    {
        ctx->pinfo[0].mask &= ~(MEDIA_ATTR_NEW);
        ctx->pinfo[0].mask |= MEDIA_ATTR_OLD;
        sqlite_table_update_ready(ctx->file_sqlite_ctx);
        // 4,5当满足的情况请设置1,2,3,timestamp和duration以及mask的值
        sqlite_table_update_int64(ctx->file_sqlite_ctx, 1, strtoul(ctx->pinfo[0].timestamp, NULL, 10));
        sqlite_table_update_int64(ctx->file_sqlite_ctx, 2, strtoul(ctx->pinfo[0].duration, NULL, 10));
        sqlite_table_update_int64(ctx->file_sqlite_ctx, 3, ctx->pinfo[0].mask);
        sqlite_table_update_int64(ctx->file_sqlite_ctx, 4, strtoul(ctx->pinfo[0].timestamp, NULL, 10));
        sqlite_table_update_int64(ctx->file_sqlite_ctx, 5, strtoul(ctx->pinfo[0].duration, NULL, 10));
        sqlite_table_update_commit(ctx->file_sqlite_ctx);
        return 1;
    }
    return 0;
}

static int _Amy_media_attri_new_media_clean(media_context_t *ctx)
{
    int i = 0;
    unsigned long numbers[MEDIA_MASK_MAX] = {0};
    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, ctx->media_index);
    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        if ((numbers[MEDIA_MASK_NUMBERS_NEW] + numbers[MEDIA_MASK_NUMBERS_OLD]) != numbers[MEDIA_MASK_NUMBER_TOTAL])
        {
            AMY_LOG_ERROR(" media databares faieled \n");
        }
        else
        {
            numbers[MEDIA_MASK_NUMBERS_OLD]++;
        }
        if (numbers[MEDIA_MASK_NUMBERS_NEW] > 0)
        {
            numbers[MEDIA_MASK_NUMBERS_NEW]--;
        }
        sqlite_table_update_ready(ctx->media_sqlite_ctx);
        for (i = 0; i < MEDIA_MASK_MAX; i++)
        {
            sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, numbers[i]);
        }
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, ctx->tm.tm_year);
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 2, ctx->tm.tm_mon);
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 3, ctx->tm.tm_mday);
        return sqlite_table_update_commit(ctx->media_sqlite_ctx);
    }

    return 0;
}

static int _Amy_media_delete_file(media_context_t *ctx)
{
    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, 1) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    sqlite_table_delete_ready(ctx->file_sqlite_ctx);
    // 4,5当满足的情况,数据库将删除文件
    sqlite_table_delete_int64(ctx->file_sqlite_ctx, 1, strtoul(ctx->pinfo[0].timestamp, NULL, 10));
    sqlite_table_delete_int64(ctx->file_sqlite_ctx, 2, strtoul(ctx->pinfo[0].duration, NULL, 10));
    if (sqlite_table_delete_commit(ctx->file_sqlite_ctx) >= 0)
    {
        sqlite_table_sync_commit(ctx->file_sqlite_ctx);
        ctx->file_total--;
        ctx->all_total--;
        return 0;
    }
    AMY_LOG_INFO("delete media index:%lu  file index:%lu failed\n", ctx->media_index, ctx->file_index);
    return -1;
}

#define VOLUE_MASK_SUB(els, mask, attr, number)           \
    els if (mask & attr)                                  \
    {                                                     \
        if (number > 0)                                   \
        {                                                 \
            number--;                                     \
        }                                                 \
        else                                              \
        {                                                 \
            AMY_LOG_ERROR(" media databares faieled \n"); \
        }                                                 \
    }

static int _Amy_media_delete_update_media(media_context_t *ctx, const media_file_info_t *info, unsigned long *numbers)
{
    // 新旧
    VOLUE_MASK_SUB(, info->mask, MEDIA_ATTR_OLD, numbers[MEDIA_MASK_NUMBERS_OLD])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_ATTR_NEW, numbers[MEDIA_MASK_NUMBERS_NEW]);

    VOLUE_MASK_SUB(, info->mask, MEDIA_TYPE_PHOTO, numbers[MEDIA_MASK_NUMBERS_TYPE_PHOTO])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_VIDEO, numbers[MEDIA_MASK_NUMBERS_TYPE_VIDEO])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_AUDIO, numbers[MEDIA_MASK_NUMBERS_TYPE_AUDIO])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_CUSTOM1, numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM1])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_CUSTOM2, numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM2])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_CUSTOM3, numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM3])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_TYPE_CUSTOM4, numbers[MEDIA_MASK_NUMBERS_TYPE_CUTOM4]);

    VOLUE_MASK_SUB(, info->mask, MEDIA_MODE_MANUAL, numbers[MEDIA_MASK_NUMBER_MODE_MUAUTAL])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_AUTO, numbers[MEDIA_MASK_NUMBER_MODE_AUTO])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_MOTION, numbers[MEDIA_MASK_NUMBER_MODE_MOTION])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_ALARM, numbers[MEDIA_MASK_NUMBER_MODE_ALARM])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_CUSTOM1, numbers[MEDIA_MASK_NUMBER_MODE_CUTOM1])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_CUSTOM2, numbers[MEDIA_MASK_NUMBER_MODE_CUTOM2])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_MODE_CUSTOM3, numbers[MEDIA_MASK_NUMBER_MODE_CUTOM3]);

    VOLUE_MASK_SUB(, info->mask, MEDIA_CHANNEL_0, numbers[MEDIA_MASK_NUMBER_CHANNEL_0])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_1, numbers[MEDIA_MASK_NUMBER_CHANNEL_1])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_2, numbers[MEDIA_MASK_NUMBER_CHANNEL_2])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_3, numbers[MEDIA_MASK_NUMBER_CHANNEL_3])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_4, numbers[MEDIA_MASK_NUMBER_CHANNEL_4])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_5, numbers[MEDIA_MASK_NUMBER_CHANNEL_5])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_6, numbers[MEDIA_MASK_NUMBER_CHANNEL_6])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_7, numbers[MEDIA_MASK_NUMBER_CHANNEL_7])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_8, numbers[MEDIA_MASK_NUMBER_CHANNEL_8])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_9, numbers[MEDIA_MASK_NUMBER_CHANNEL_9])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_10, numbers[MEDIA_MASK_NUMBER_CHANNEL_10])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_11, numbers[MEDIA_MASK_NUMBER_CHANNEL_11])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_12, numbers[MEDIA_MASK_NUMBER_CHANNEL_12])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_13, numbers[MEDIA_MASK_NUMBER_CHANNEL_13])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_14, numbers[MEDIA_MASK_NUMBER_CHANNEL_14])
    VOLUE_MASK_SUB(else, info->mask, MEDIA_CHANNEL_15, numbers[MEDIA_MASK_NUMBER_CHANNEL_15]);

    sqlite_table_update_ready(ctx->media_sqlite_ctx);
    int i = 0;
    for (i = 0; i < MEDIA_MASK_MAX; i++)
    {
        sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, numbers[i]);
    }
    sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 1, ctx->tm.tm_year);
    sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 2, ctx->tm.tm_mon);
    sqlite_table_update_int64(ctx->media_sqlite_ctx, i + 3, ctx->tm.tm_mday);
    return sqlite_table_update_commit(ctx->media_sqlite_ctx);
}
static int _Amy_media_delete_delete_media(media_context_t *ctx, unsigned long *numbers)
{
    sqlite_table_delete_ready(ctx->media_sqlite_ctx);
    sqlite_table_delete_int64(ctx->media_sqlite_ctx, 1, ctx->tm.tm_year);
    sqlite_table_delete_int64(ctx->media_sqlite_ctx, 2, ctx->tm.tm_mon);
    sqlite_table_delete_int64(ctx->media_sqlite_ctx, 3, ctx->tm.tm_mday);
    return sqlite_table_delete_commit(ctx->media_sqlite_ctx);
}

static int _Amy_media_delete_media(media_context_t *ctx, const media_file_info_t *info)
{
    // int i = 0;
    unsigned long numbers[MEDIA_MASK_MAX] = {0};
    sqlite_table_query_ready(ctx->media_sqlite_ctx);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
    sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, ctx->media_index);
    if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, MEDIA_MASK_MAX) > 0)
    {
        if ((numbers[MEDIA_MASK_NUMBERS_NEW] + numbers[MEDIA_MASK_NUMBERS_OLD]) != numbers[MEDIA_MASK_NUMBER_TOTAL])
        {
            AMY_LOG_ERROR(" media databares faieled \n");
        }
        if (numbers[MEDIA_MASK_NUMBER_TOTAL] > 1)
        {
            // 仅仅更新
            numbers[MEDIA_MASK_NUMBER_TOTAL]--;
            _Amy_media_delete_update_media(ctx, info, numbers);
        }
        else
        {

            if (_Amy_media_delete_delete_media(ctx, numbers) >= 0)
            {
                ctx->media_total--;
            }
        }
        sqlite_table_sync_commit(ctx->media_sqlite_ctx);
    }
    return 0;
}
/*
 * 清除媒体新文件属性
 * return :0 success,非0初始化失败
 */
int Amy_media_attr_new_clean(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    if (_Amy_media_attri_new_file_clean(ctx) > 0)
    {
        _Amy_media_attri_new_media_clean(ctx);
    }
    media_mutex_unlock(ctx);
    return 0;
}

/*
 * 删除当前文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t *Amy_media_delete_file(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    const media_file_info_t *info = NULL;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    if (_Amy_media_delete_file(ctx) >= 0)
    {
        _Amy_media_delete_media(ctx, &ctx->pinfo[0]);
        if (ctx->media_total > 0)
        {
            if (ctx->file_total <= 0)
            {
                ctx->file_index = 0;
                ctx->page_total--;
            }
            if (ctx->all_index == ctx->all_total)
            {
                info = _Amy_media_prev_media_info(ctx);
            }
            else
            {
                media_mutex_unlock(ctx);
                return Amy_media_current_media_info(ctx);
            }
        }
    }
    media_mutex_unlock(ctx);
    return info;
}
/*
 * 关闭媒体文件
 * return :0 success,非0初始化失败
 */
int Amy_media_close(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        return 0;
    }
    media_mutex_lock(ctx);
    if (ctx->base_dir != NULL)
    {
        free(ctx->base_dir);
        ctx->base_dir = NULL;
    }
    if (ctx->media_sqlite_where != NULL)
    {
        free(ctx->media_sqlite_where);
        ctx->media_sqlite_where = NULL;
    }
    if (ctx->file_sqlite_dir != NULL)
    {
        free(ctx->file_sqlite_dir);
        ctx->file_sqlite_dir = NULL;
    }

    if (ctx->file_sqlite_where != NULL)
    {
        free(ctx->file_sqlite_where);
        ctx->file_sqlite_where = NULL;
    }

    if (ctx->file_sqlite_select != NULL)
    {
        free(ctx->file_sqlite_select);
        ctx->file_sqlite_select = NULL;
    }
    if (ctx->file_sqlite_limit != NULL)
    {
        free(ctx->file_sqlite_limit);
        ctx->file_sqlite_limit = NULL;
    }

    if (ctx->media_sqlite_select != NULL)
    {
        free(ctx->media_sqlite_select);
        ctx->media_sqlite_select = NULL;
    }
    if (ctx->media_sqlite_ctx)
    {
        sqlite_table_close(ctx->media_sqlite_ctx);
        ctx->media_sqlite_ctx = NULL;
    }
    if (ctx->file_sqlite_ctx)
    {
        sqlite_table_close(ctx->file_sqlite_ctx);
        ctx->file_sqlite_ctx = NULL;
    }
    if (ctx->pinfo)
    {
        free(ctx->pinfo);
        ctx->pinfo = NULL;
    }
    media_mutex_unlock(ctx);
    free(ctx);
    return 0;
}

/*
 * 文件信息转为文件路径
 */
int Amy_media_info_conver_path(Amy_media_context context, const media_file_info_t *info, char *path, int size)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    char *dot = _def_media_type_get(info->mask);
    media_mutex_lock(ctx);
    snprintf(path, size - 1, "%s%s%d/%d/%d/%s_%s.%s", ctx->base_dir, MEDIA_FILE_PATH, info->year, info->month, info->day, info->timestamp, info->duration, dot);
    media_mutex_unlock(ctx);

    AMY_LOG_INFO("path:%s\n", path);
    return 0;
}

/*
 *	依据时间戳获取文件路径
 */
int Amy_media_timestamp_conver_path(Amy_media_context context, unsigned int start, unsigned int end, const char *dot, char *path, int size)
{
    struct tm tm;
    int rc = 0;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }

    unsigned int duration = end - start;
    localtime_r((time_t *)&start, &tm);
    tm.tm_year += 1900;
    tm.tm_mon += 1;
    media_mutex_lock(ctx);
    snprintf(path, size - 1, "%s%s%d/%d/%d/%d_%d.%s", ctx->base_dir, MEDIA_FILE_PATH, tm.tm_year, tm.tm_mon, tm.tm_mday, start, duration, dot);
    if (access(path, F_OK) != 0)
    {
        AMY_LOG_ERROR(" %s not exist\n", path);
        rc = -1;
    }
    media_mutex_unlock(ctx);
    return rc;
}

/*
 * 依据年月查询最新的视频日期
 */
int Amy_media_query_by_month(Amy_media_context context, struct tm *tm, int index, int *day)
{
    unsigned long numbers[3] = {0};
    char where[512] = {0};
    char temp[128] = {0};
    unsigned long offset = 0;
    int rc = 0;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    // 设置查询方法
    {
        _def_media_string_filter_format(where, ctx->mask, ">0");
        sprintf(temp, " AND %s=%d AND %s=%d ", TABLE_1, tm->tm_year, TABLE_2, tm->tm_mon);
        strcat(where, temp);
        if (_Amy_databases_media_reset_param(ctx, where) != 0)
        {
            media_mutex_unlock(ctx);
            AMY_LOG_ERROR(" param failed\n");
            return -1;
        }
    }

    do
    {
        sqlite_table_query_ready(ctx->media_sqlite_ctx);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, index);
        if ((offset = sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, 3)) > 0)
        {
            AMY_LOG_ERROR(" %lu %lu %lu\n", numbers[0], numbers[1], numbers[2]);
            if ((numbers[0] == tm->tm_year) && (numbers[1] == tm->tm_mon))
            {
                AMY_LOG_ERROR("numbers[0]:%lu tm->tm_year:%d numbers[1]:%lu tm->tm_mon:%d\n", numbers[0], tm->tm_year, numbers[1], tm->tm_mon);
                ctx->tm.tm_year = numbers[0];
                ctx->tm.tm_mon = numbers[1];
                ctx->tm.tm_mday = numbers[2];
                *day = ctx->tm.tm_mday;
                ctx->media_index = index;
                break;
            }
            index += offset;
        }
        else
        {
            rc = -1;
            break;
        }
    } while (1);
    media_mutex_unlock(ctx);
    return rc;
}

/*
 * 依据年月查询当月有媒体的日期，一个bit表示一个日期，day从bit1~bit31表示1日~31日
 */
int Amy_media_query_everyday_by_month(Amy_media_context context, struct tm *tm, int index, unsigned int *day)
{
    unsigned long numbers[3] = {0};
    char where[512] = {0};
    char temp[128] = {0};
    unsigned long offset = 0;
    int rc = 0;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    // 设置查询方法
    {
        _def_media_string_filter_format(where, ctx->mask, ">0");
        sprintf(temp, " AND %s=%d AND %s=%d ", TABLE_1, tm->tm_year, TABLE_2, tm->tm_mon);
        strcat(where, temp);
        if (_Amy_databases_media_reset_param(ctx, where) != 0)
        {
            AMY_LOG_ERROR(" param failed\n");
            media_mutex_unlock(ctx);
            return -1;
        }
    }

    do
    {
        sqlite_table_query_ready(ctx->media_sqlite_ctx);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, index);
        if ((offset = sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback, numbers, 0, 3)) > 0)
        {
            // AMY_LOG_ERROR(" %lu %lu %lu\n", numbers[0], numbers[1], numbers[2]);
            if ((numbers[0] == tm->tm_year) && (numbers[1] == tm->tm_mon))
            {
                // AMY_LOG_ERROR("numbers[0]:%lu numbers[1]:%lu numbers[2]:%lu\n", numbers[0], numbers[1], numbers[2]);
                ctx->tm.tm_year = numbers[0];
                ctx->tm.tm_mon = numbers[1];
                ctx->tm.tm_mday = numbers[2];
                *day |= 1 << ctx->tm.tm_mday;
                ctx->media_index = index;
                // break;
            }
            index += offset;
        }
        else
        {
            rc = -1;
            break;
        }
    } while (1);
    media_mutex_unlock(ctx);
    return rc;
}

/*
 * 查询年月日的文件信息个数及时间戳
 */
media_file_info_t *Amy_media_query_by_day(Amy_media_context context, struct tm *tm, int index, int *count)
{
    //  unsigned long numbers[3] = { 0 };
    //  char where[512] = { 0 };
    //   char temp[128] = { 0 };
    //   unsigned long offset = 0;
    //   int rc = 0;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    if (_Amy_media_file_databases_query(ctx, tm->tm_year, tm->tm_mon, tm->tm_mday, index, ctx->page_file_num_max) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    *count = ctx->page_file_num;
    media_mutex_unlock(ctx);
    return ctx->pinfo;
}

/*
 * 获取当前媒体的总数
 */
int Amy_media_current_total(Amy_media_context context, unsigned long *current, unsigned long *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    *current = ctx->file_index;
    *total = ctx->file_total;
    media_mutex_unlock(ctx);
    return 0;
}

/*
 * 获取当前媒体的日期
 */
int Amy_media_current_time(Amy_media_context context, struct tm *tm)
{
    struct tm t;
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR(" param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    unsigned long timestamp = 0;
    timestamp = strtoul(ctx->pinfo[0].timestamp, NULL, 10);
    localtime_r((time_t *)&timestamp, &t);
    t.tm_year += 1900;
    t.tm_mon += 1;
    *tm = t;
    media_mutex_unlock(ctx);
    return 0;
}

/*
 *设置文件读取索引
 */
int Amy_media_file_index_set(Amy_media_context context, unsigned long index)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    if (index >= ctx->file_total)
    {
        AMY_LOG_ERROR("file total :%lu,index:%lu\n", ctx->file_total, index);
        media_mutex_unlock(ctx);
        return -1;
    }
    ctx->all_index = ctx->all_index - ctx->file_index + index;
    ctx->file_index = index;
    media_mutex_unlock(ctx);
    return 0;
}

/*
 * 按跨天全局序号（all_index，0 为最新）定位读取位置
 * 定位到索引所在日期并同步 tm/file_total/media_index，
 * 避免调用方把全局序号当成当天内偏移使用
 */
int Amy_media_all_index_set(Amy_media_context context, unsigned long index)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    if (index >= ctx->all_total)
    {
        AMY_LOG_ERROR("all total :%lu,index:%lu\n", ctx->all_total, index);
        media_mutex_unlock(ctx);
        return -1;
    }

    unsigned long numbers[MEDIA_MASK_MAX] = {0};
    unsigned long base = 0; /* 当前日期之前累计的文件数（全局基准） */
    int day_index = 0;

    while ((unsigned long)day_index < ctx->media_total)
    {
        sqlite_table_query_ready(ctx->media_sqlite_ctx);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 1, 1);
        sqlite_table_query_int64(ctx->media_sqlite_ctx, 2, day_index);
        if (sqlite_table_query_commit(ctx->media_sqlite_ctx, _Amy_media_sqlite_table_query_callback,
                                      numbers, 0, MEDIA_MASK_MAX) <= 0)
        {
            break;
        }
        ctx->tm.tm_year = (int)numbers[0];
        ctx->tm.tm_mon = (int)numbers[1];
        ctx->tm.tm_mday = (int)numbers[2];
        unsigned long day_total = _Amy_media_date_file_total_get(ctx, numbers);
        if (index < base + day_total)
        {
            ctx->media_index = day_index;
            ctx->file_total = day_total;
            ctx->all_index = index;
            ctx->file_index = index - base;
            media_mutex_unlock(ctx);
            return 0;
        }
        base += day_total;
        day_index++;
    }

    AMY_LOG_ERROR("index:%lu locate failed\n", index);
    media_mutex_unlock(ctx);
    return -1;
}

/*
 *获取文件读取索引
 */
unsigned long Amy_media_file_index_get(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);
    unsigned long index = ctx->file_index;
    media_mutex_unlock(ctx);
    return index;
}
/*
 * 获取当前媒体信息
 */
const media_file_info_t *Amy_media_current_media_info(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    if ((ctx->media_total == 0) || (ctx->file_total == 0))
    {
        AMY_LOG_ERROR("current meida total or file total zero(media tatal:%lu,file total:%lu)\n", ctx->media_total, ctx->file_total);
        media_mutex_unlock(ctx);
        return NULL;
    }
    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, 1) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    ctx->pinfo[0].year = ctx->tm.tm_year;
    ctx->pinfo[0].month = ctx->tm.tm_mon;
    ctx->pinfo[0].day = ctx->tm.tm_mday;
    media_mutex_unlock(ctx);
    return &ctx->pinfo[0];
}

/*
 * 下一个媒体信息
 */
const media_file_info_t *_Amy_media_next_media_info(media_context_t *ctx)
{
    /*翻到下一页*/
    if ((ctx->file_index + 1) >= ctx->file_total)
    {
        if (_Amy_media_next_databases_proc(ctx) != 0)
        {
            AMY_LOG_ERROR("param failed\n");
            media_mutex_unlock(ctx);
            return NULL;
        }
        ctx->file_index = 0;
    }
    /*正常翻页*/
    else
    {
        ctx->all_index++;
        ctx->file_index++;
        if (ctx->file_index % ctx->page_file_num_max == 0)
        {
            ctx->page_index++;
        }
    }

    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, 1) != 0)
    {
        AMY_LOG_ERROR(" param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }

    ctx->pinfo[0].year = ctx->tm.tm_year;
    ctx->pinfo[0].month = ctx->tm.tm_mon;
    ctx->pinfo[0].day = ctx->tm.tm_mday;
    return &ctx->pinfo[0];
}
const media_file_info_t *Amy_media_next_media_info(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);
    const media_file_info_t *info = _Amy_media_next_media_info(ctx);
    media_mutex_unlock(ctx);
    return info;
}
const media_file_info_t *_Amy_media_prev_media_info(media_context_t *ctx)
{
    /*翻到上一个日期的数据库*/
    if (ctx->file_index == 0)
    {
        if (_Amy_media_prev_databases_proc(ctx) != 0)
        {
            AMY_LOG_ERROR("param failed\n");
            media_mutex_unlock(ctx);
            return NULL;
        }
        ctx->file_index = ctx->file_total - 1;
        ctx->all_index = ctx->all_index - ctx->file_total + ctx->file_index;
        ctx->page_index = ctx->page_index + (ctx->file_index / ctx->page_file_num_max);
    }
    /*翻到第一个文件*/
    else
    {
        ctx->all_index--;
        if (ctx->file_index % ctx->page_file_num_max == 0)
        {
            ctx->page_index--;
        }
        ctx->file_index--;
    }

    if (_Amy_media_file_databases_query(ctx, ctx->tm.tm_year, ctx->tm.tm_mon, ctx->tm.tm_mday, ctx->file_index, 1) != 0)
    {
        AMY_LOG_ERROR("param failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }
    ctx->pinfo[0].year = ctx->tm.tm_year;
    ctx->pinfo[0].month = ctx->tm.tm_mon;
    ctx->pinfo[0].day = ctx->tm.tm_mday;

    return &ctx->pinfo[0];
}
/*
 * 上一个媒体信息
 */
const media_file_info_t *Amy_media_prev_media_info(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx); //  const media_file_info_t* info =
    _Amy_media_prev_media_info(ctx);
    media_mutex_unlock(ctx);
    return &ctx->pinfo[0];
}

/*******************************************************************
 * @brief  : 获取全部文件的索引和总数
 * @return  {*}
 * @param {Amy_media_context} context
 * @param {unsigned long} *index
 * @param {unsigned long} *total
 *******************************************************************/
void Amy_media_all_index_total_get(Amy_media_context context, unsigned long *index, unsigned long *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return;
    }
    media_mutex_lock(ctx);
    if (index)
        *index = ctx->all_index;
    if (total)
        *total = ctx->all_total;
    media_mutex_unlock(ctx);
}
/*******************************************************************
 * @brief  : 获取页数的索引和总数
 * @return  {*}
 * @param {Amy_media_context} context
 * @param {unsigned long} *index
 * @param {unsigned long} *total
 *******************************************************************/
void Amy_media_page_index_total_get(Amy_media_context context, unsigned long *index, unsigned long *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return;
    }
    media_mutex_lock(ctx);
    if (index)
        *index = ctx->page_index;
    if (total)
        *total = ctx->page_total;
    media_mutex_unlock(ctx);
}
/*******************************************************************
 * @brief  : 删除所有文件
 * @return  {*}
 * @param {Amy_media_context} context
 *******************************************************************/
void Amy_media_delete_all_file(Amy_media_context context)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return;
    }
    char cmd[128] = {0};
    media_mutex_lock(ctx);
    snprintf(cmd, sizeof(cmd) - 1, "rm -rf %s%s", ctx->base_dir, MEDIA_FILE_PATH);
    system(cmd);
    system("sync");
    media_mutex_unlock(ctx);
}

void Amy_media_delete_by_date(Amy_media_context context, int year, int month, int day)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return;
    }
    char cmd[256] = {0};
    media_mutex_lock(ctx);

    /* 先清理 media.db 中的聚合记录（year=0 时 rm -rf 会整体删除 media.db，无需 SQL）
     * 若 ctx->media_sqlite_ctx 已初始化（FILE 层级浏览时会触发），需先提交其事务，
     * 否则独立连接无法获取写锁，DELETE 会静默失败 */
    if (year != 0)
    {
        if (ctx->media_sqlite_ctx)
        {
            sqlite_table_sync_commit(ctx->media_sqlite_ctx);
        }

        char databases_dir[128];
        char sql[128];
        snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s",
                 ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);
        sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
        if (sqlite_ctx)
        {
            if (month == 0)
            {
                snprintf(sql, sizeof(sql) - 1, "DELETE FROM %s WHERE %s=%d",
                         MEDIA_DATA_TABLE, TABLE_1, year);
            }
            else if (day == 0)
            {
                snprintf(sql, sizeof(sql) - 1,
                         "DELETE FROM %s WHERE %s=%d AND %s=%d",
                         MEDIA_DATA_TABLE, TABLE_1, year, TABLE_2, month);
            }
            else
            {
                snprintf(sql, sizeof(sql) - 1,
                         "DELETE FROM %s WHERE %s=%d AND %s=%d AND %s=%d",
                         MEDIA_DATA_TABLE, TABLE_1, year, TABLE_2, month, TABLE_3, day);
            }
            sqlite_table_exec(sqlite_ctx, sql);
            sqlite_table_close(sqlite_ctx);
        }
    }

    /* 删除文件系统目录 */
    if (year == 0)
    {
        snprintf(cmd, sizeof(cmd) - 1, "rm -rf %s%s", ctx->base_dir, MEDIA_FILE_PATH);
    }
    else if (month == 0)
    {
        snprintf(cmd, sizeof(cmd) - 1, "rm -rf %s%s%d", ctx->base_dir, MEDIA_FILE_PATH, year);
    }
    else if (day == 0)
    {
        snprintf(cmd, sizeof(cmd) - 1, "rm -rf %s%s%d/%d", ctx->base_dir, MEDIA_FILE_PATH, year, month);
    }
    else
    {
        snprintf(cmd, sizeof(cmd) - 1, "rm -rf %s%s%d/%d/%d", ctx->base_dir, MEDIA_FILE_PATH, year, month, day);
    }
    system(cmd);
    system("sync");
    media_mutex_unlock(ctx);
}

/*******************************************************************
 * @brief  : 获取新文件总数
 * @return  {*}
 * @param {Amy_media_context} context
 *******************************************************************/
void Amy_media_new_total_get(Amy_media_context context, unsigned long *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return;
    }
    media_mutex_lock(ctx);
    if (total)
        *total = ctx->new_total;
    media_mutex_unlock(ctx);
}

/*******************************************************************
 * 日期层级查询辅助结构与回调
 *******************************************************************/
struct _date_multi_row_ctx
{
    int *buf;
    int buf_size;
    int count;
};

static void _Amy_media_date_multi_row_callback(void *stmt, void *userdata, int base, int total)
{
    struct _date_multi_row_ctx *d = (struct _date_multi_row_ctx *)userdata;
    if (d->count < d->buf_size)
    {
        d->buf[d->count++] = (int)sqlite3_column_int64(stmt, 0);
    }
}

static void _Amy_media_date_count_callback(void *stmt, void *userdata, int base, int total)
{
    int *count = (int *)userdata;
    *count = (int)sqlite3_column_int64(stmt, 0);
}

/*******************************************************************
 * @brief  : 查询有媒体文件的年份列表（分页）
 * @return  : 实际返回的年份数量，<0 失败
 *******************************************************************/
int Amy_media_query_years(Amy_media_context context, int *years_buf, int buf_size, int offset, int *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL || years_buf == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    char where[512] = {0};
    char databases_dir[128] = {0};
    char limit_str[16], offset_str[16];

    _def_media_string_filter_format(where, ctx->mask, ">0");
    snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open media.db failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }

    /* 查询总数 */
    if (sqlite_table_query_prepare(sqlite_ctx, "COUNT(DISTINCT " TABLE_1 ")", MEDIA_DATA_TABLE, where, TABLE_1, "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, 1);
        sqlite_table_query_int64(sqlite_ctx, 2, 0);
        int cnt = 0;
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_count_callback, &cnt, 0, 1);
        if (total)
            *total = cnt;
    }

    /* 查询年份数据 */
    snprintf(limit_str, sizeof(limit_str), "%d", buf_size);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    struct _date_multi_row_ctx cb_data = {years_buf, buf_size, 0};

    if (sqlite_table_query_prepare(sqlite_ctx, "DISTINCT " TABLE_1, MEDIA_DATA_TABLE, where,
                                   TABLE_1 " DESC", "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, buf_size);
        sqlite_table_query_int64(sqlite_ctx, 2, offset);
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_multi_row_callback, &cb_data, 0, 1);
    }

    sqlite_table_close(sqlite_ctx);
    media_mutex_unlock(ctx);
    return cb_data.count;
}

/*******************************************************************
 * @brief  : 查询指定年份下有媒体文件的月份列表（分页）
 *******************************************************************/
int Amy_media_query_months(Amy_media_context context, int year, int *months_buf, int buf_size, int offset, int *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL || months_buf == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    char where[512] = {0};
    char temp[128] = {0};
    char databases_dir[128] = {0};
    char limit_str[16], offset_str[16];

    _def_media_string_filter_format(where, ctx->mask, ">0");
    snprintf(temp, sizeof(temp) - 1, " AND %s=%d ", TABLE_1, year);
    strcat(where, temp);
    snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open media.db failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }

    if (sqlite_table_query_prepare(sqlite_ctx, "COUNT(DISTINCT " TABLE_2 ")", MEDIA_DATA_TABLE, where, TABLE_1, "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, 1);
        sqlite_table_query_int64(sqlite_ctx, 2, 0);
        int cnt = 0;
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_count_callback, &cnt, 0, 1);
        if (total)
            *total = cnt;
    }

    snprintf(limit_str, sizeof(limit_str), "%d", buf_size);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    struct _date_multi_row_ctx cb_data = {months_buf, buf_size, 0};

    if (sqlite_table_query_prepare(sqlite_ctx, "DISTINCT " TABLE_2, MEDIA_DATA_TABLE, where,
                                   TABLE_2 " DESC", "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, buf_size);
        sqlite_table_query_int64(sqlite_ctx, 2, offset);
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_multi_row_callback, &cb_data, 0, 1);
    }

    sqlite_table_close(sqlite_ctx);
    media_mutex_unlock(ctx);
    return cb_data.count;
}

/*******************************************************************
 * @brief  : 查询指定年月下有媒体文件的日期列表（分页）
 *******************************************************************/
int Amy_media_query_days(Amy_media_context context, int year, int month, int *days_buf, int buf_size, int offset, int *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL || days_buf == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    char where[512] = {0};
    char temp[128] = {0};
    char databases_dir[128] = {0};
    char limit_str[16], offset_str[16];

    _def_media_string_filter_format(where, ctx->mask, ">0");
    snprintf(temp, sizeof(temp) - 1, " AND %s=%d AND %s=%d ", TABLE_1, year, TABLE_2, month);
    strcat(where, temp);
    snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open media.db failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }

    if (sqlite_table_query_prepare(sqlite_ctx, "COUNT(DISTINCT " TABLE_3 ")", MEDIA_DATA_TABLE, where, TABLE_1, "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, 1);
        sqlite_table_query_int64(sqlite_ctx, 2, 0);
        int cnt = 0;
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_count_callback, &cnt, 0, 1);
        if (total)
            *total = cnt;
    }

    snprintf(limit_str, sizeof(limit_str), "%d", buf_size);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    struct _date_multi_row_ctx cb_data = {days_buf, buf_size, 0};

    if (sqlite_table_query_prepare(sqlite_ctx, "DISTINCT " TABLE_3, MEDIA_DATA_TABLE, where,
                                   TABLE_3 " DESC", "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, buf_size);
        sqlite_table_query_int64(sqlite_ctx, 2, offset);
        sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_multi_row_callback, &cb_data, 0, 1);
    }

    sqlite_table_close(sqlite_ctx);
    media_mutex_unlock(ctx);
    return cb_data.count;
}

/*******************************************************************
 * @brief  : 查询指定日期的文件总数
 *******************************************************************/
int Amy_media_query_date_file_total(Amy_media_context context, int year, int month, int day, int *total)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL || total == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    char where[512] = {0};
    char temp[128] = {0};
    char databases_dir[128] = {0};

    _def_media_string_filter_format(where, ctx->mask, ">0");
    snprintf(temp, sizeof(temp) - 1, " AND %s=%d AND %s=%d AND %s=%d ", TABLE_1, year, TABLE_2, month, TABLE_3, day);
    strcat(where, temp);
    snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);

    sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
    if (sqlite_ctx == NULL)
    {
        AMY_LOG_ERROR("open media.db failed\n");
        media_mutex_unlock(ctx);
        return -1;
    }

    int rc = -1;
    if (sqlite_table_query_prepare(sqlite_ctx, "*", MEDIA_DATA_TABLE, where, TABLE_1, "?", "?") == 0)
    {
        sqlite_table_query_ready(sqlite_ctx);
        sqlite_table_query_int64(sqlite_ctx, 1, 1);
        sqlite_table_query_int64(sqlite_ctx, 2, 0);
        struct _media_date_file_total_ctx cb_data = {ctx->mask, 0};
        if (sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_file_total_callback, &cb_data, 0, 1) > 0)
        {
            *total = cb_data.total;
            rc = 0;
        }
    }

    sqlite_table_close(sqlite_ctx);
    media_mutex_unlock(ctx);
    return rc;
}

/*******************************************************************
 * @brief  : 跳转到指定日期并加载文件首页
 *******************************************************************/
const media_file_info_t *Amy_media_goto_date(Amy_media_context context, int year, int month, int day, int *num, int *total_files)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return NULL;
    }
    media_mutex_lock(ctx);

    if (_Amy_media_file_databases_query(ctx, year, month, day, 0, ctx->page_file_num_max) != 0)
    {
        AMY_LOG_ERROR("query file db failed\n");
        media_mutex_unlock(ctx);
        return NULL;
    }

    ctx->tm.tm_year = year;
    ctx->tm.tm_mon = month;
    ctx->tm.tm_mday = day;
    ctx->file_index = 0;
    ctx->file_total = 0;

    if (num)
        *num = ctx->page_file_num;

    /* 查询当日文件总数 */
    {
        char where[512] = {0};
        char temp[128] = {0};
        char databases_dir[128] = {0};
        _def_media_string_filter_format(where, ctx->mask, ">0");
        snprintf(temp, sizeof(temp) - 1, " AND %s=%d AND %s=%d AND %s=%d ", TABLE_1, year, TABLE_2, month, TABLE_3, day);
        strcat(where, temp);
        snprintf(databases_dir, sizeof(databases_dir) - 1, "%s%s%s", ctx->base_dir, MEDIA_FILE_PATH, MEDIA_DATABASES);

        sqlite_context sqlite_ctx = sqlite_table_open(databases_dir, MEDIA_DATA_TABLE);
        if (sqlite_ctx != NULL)
        {
            if (sqlite_table_query_prepare(sqlite_ctx, "*", MEDIA_DATA_TABLE, where, TABLE_1, "?", "?") == 0)
            {
                sqlite_table_query_ready(sqlite_ctx);
                sqlite_table_query_int64(sqlite_ctx, 1, 1);
                sqlite_table_query_int64(sqlite_ctx, 2, 0);
                struct _media_date_file_total_ctx cb_data = {ctx->mask, 0};
                if (sqlite_table_query_commit(sqlite_ctx, _Amy_media_date_file_total_callback, &cb_data, 0, 1) > 0)
                {
                    ctx->file_total = (unsigned long)cb_data.total;
                    if (total_files)
                        *total_files = cb_data.total;
                }
            }
            sqlite_table_close(sqlite_ctx);
        }
    }

    /* 填充年月日信息 */
    if (ctx->pinfo && ctx->page_file_num > 0)
    {
        for (int i = 0; i < ctx->page_file_num; i++)
        {
            ctx->pinfo[i].year = year;
            ctx->pinfo[i].month = month;
            ctx->pinfo[i].day = day;
        }
    }

    media_mutex_unlock(ctx);
    return ctx->pinfo;
}

/*******************************************************************
 * @brief  : 设置日期范围过滤，0 表示不过滤该层级
 *******************************************************************/
int Amy_media_set_date_filter(Amy_media_context context, int year, int month, int day)
{
    media_context_t *ctx = (media_context_t *)context;
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("param failed\n");
        return -1;
    }
    media_mutex_lock(ctx);

    ctx->filter_year = year;
    ctx->filter_month = month;
    ctx->filter_day = day;

    char where[512] = {0};
    _Amy_media_where_build(ctx, where, sizeof(where));

    int rc = _Amy_databases_media_reset_param(ctx, where);
    media_mutex_unlock(ctx);
    return rc;
}