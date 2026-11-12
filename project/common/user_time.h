#ifndef _USER_TIME_H_
#define _USER_TIME_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/**
 * @brief 将RTC时间同步到系统时间
 */
void user_time_sync(void);

/**
 * @brief 时间模块初始化
 * @return 初始化成功返回true
 */
bool user_time_init(void);

/**
 * @brief 读取系统当前时间
 * @param tm 输出时间结构体
 * @return 读取成功返回true
 */
bool user_time_read(struct tm *tm);

/**
 * @brief 设置系统时间并同步到RTC
 * @param tm 时间结构体指针
 * @return 设置成功返回true
 */
bool user_time_set(struct tm *tm);

/**
 * @brief 获取指定日期是星期几
 * @param date 时间结构体
 * @return 星期几（1~7）
 */
int user_cur_time_is_week(struct tm *date);

/**
 * @brief 获取当月最后一天
 * @param date 时间结构体
 * @return 当月最后一天日期
 */
int cur_time_last_day(struct tm *date);

/**
 * @brief 检查时间是否合法
 * @param tm 时间结构体
 * @return 合法返回true
 */
bool user_time_valid_check(struct tm *tm);

/**
 * @brief 获取毫秒级时间戳
 * @return 毫秒时间戳
 */
unsigned long long user_timestamp_ms_get(void);

/**
 * @brief 获取格式化日期字符串
 * @param time 时间结构体
 * @return 格式化字符串
 */
char *date_format_string_get(struct tm *time);

#endif