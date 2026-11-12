#include "user_time.h"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "user_data.h"
#include "language.h"

/**
 * @brief 计算指定日期是星期几（基姆拉尔森公式）
 * @param date 时间结构体指针
 * @return 星期值 1~7
 */
int user_cur_time_is_week(struct tm *date)
{
	if (date == NULL)
	{
		return 0;
	}

	int m = date->tm_mon;
	int y = date->tm_year;
	int d = date->tm_mday;

	if (m == 1 || m == 2)
	{
		m += 12;
		y--;
	}

	return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7 + 1;
}

/**
 * @brief 获取当月最后一天
 * @param date 时间结构体指针
 * @return 当月最后一天日期
 */
int user_cur_time_last_day(struct tm *date)
{
	if (date == NULL)
	{
		return 0;
	}

	unsigned int year = date->tm_year;
	unsigned char mon = date->tm_mon;

	if (mon == 2)
	{
		if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
		{
			return 29;
		}
		else
		{
			return 28;
		}
	}

	switch (mon)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	default:
		break;
	}

	return 0;
}

/**
 * @brief RTC时间同步到系统时间
 */
void user_time_sync(void)
{
	system("hwclock -s");
}

/**
 * @brief 读取本地系统时间
 * @param tm 输出时间结构体
 * @return 成功true
 */
bool user_time_read(struct tm *tm)
{
	if (tm == NULL)
	{
		return false;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, tm);

	tm->tm_year += 1900;
	tm->tm_mon += 1;
	tm->tm_wday = user_cur_time_is_week(tm);

	return true;
}

/**
 * @brief 设置系统时间并同步到RTC
 * @param tm 时间结构体
 * @return 成功true
 */
bool user_time_set(struct tm *tm)
{
	if (tm == NULL)
	{
		return false;
	}

	char string[64] = {0};
	snprintf(string, sizeof(string),
					 "date -s \"%04d-%02d-%02d %02d:%02d:%02d\"",
					 tm->tm_year, tm->tm_mon, tm->tm_mday,
					 tm->tm_hour, tm->tm_min, tm->tm_sec);

	system(string);
	system("hwclock -w");

	return true;
}

/**
 * @brief 时间合法性检查
 * @param time 时间结构体
 * @return 合法true，非法false
 */
bool user_time_valid_check(struct tm *time)
{
	if (time == NULL)
	{
		return false;
	}

	if (time->tm_year < 2003 || time->tm_year > 2099)
	{
		return false;
	}
	if (time->tm_mon < 1 || time->tm_mon > 12)
	{
		return false;
	}
	if (time->tm_mday < 1 || time->tm_mday > 31)
	{
		return false;
	}
	if (time->tm_hour < 0 || time->tm_hour > 23)
	{
		return false;
	}
	if (time->tm_min < 0 || time->tm_min > 59)
	{
		return false;
	}
	if (time->tm_sec < 0 || time->tm_sec > 59)
	{
		return false;
	}

	return true;
}

/**
 * @brief 时间模块初始化
 * @return 成功true
 */
bool user_time_init(void)
{

	user_time_sync();

	struct tm tm;
	user_time_read(&tm);

	if (user_time_valid_check(&tm) == false)
	{
		tm.tm_year = 2024;
		tm.tm_mon = 1;
		tm.tm_mday = 1;
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
		user_time_set(&tm);
	}

	return true;
}

/**
 * @brief 获取格式化日期字符串
 * @param time 时间结构体
 * @return 格式化字符串指针（静态缓冲区）
 */
char *date_format_string_get(struct tm *time)
{
	static char buf[128] = {0};
	memset(buf, 0, sizeof(buf));

	if (time == NULL)
	{
		return buf;
	}

	lang_type_def curr_lang = user_data_get()->private_cfg.lang;
	int mmon = time->tm_mon + 1;
	int mday = time->tm_mday;
	int year = time->tm_year + 1900;

	if (curr_lang == LANG_CHINESE)
	{
		snprintf(buf, sizeof(buf), "%04d年 %02d月 %02d日", year, mmon, mday);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%02d/%02d/%04d", mmon, mday, year);
	}

	return buf;
}