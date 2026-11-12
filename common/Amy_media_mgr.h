#ifndef _Amy_MEDIA_MGR_H_
#define _Amy_MEDIA_MGR_H_

#include <time.h>
#include "Amy_tfcard.h"
#define MEDIA_FILE_PATH "/media/"
#define TFCARD_MEDIA_PATH TFCARD_MOUNT_PATH MEDIA_FILE_PATH

#define MEDIA_TIMESTAP_LEN 16
#define MEDIA_DURTION_LEN 5

typedef void* Amy_media_context;
#define MEDIA_ATTR_OLD (1 << 0)
#define MEDIA_ATTR_NEW (1 << 1)
#define MEDIA_ATTR_ALL (0X3 << 0)
/*文件类型*/
#define MEDIA_TYPE_PHOTO (1 << 2)
#define MEDIA_TYPE_VIDEO (1 << 3)
#define MEDIA_TYPE_AUDIO (1 << 4)
#define MEDIA_TYPE_CUSTOM1 (1 << 5)
#define MEDIA_TYPE_CUSTOM2 (1 << 6)
#define MEDIA_TYPE_CUSTOM3 (1 << 7)
#define MEDIA_TYPE_CUSTOM4 (1 << 8)
#define MEDIA_TYPE_ALL (0X7F << 2)
/*记录模式*/
#define MEDIA_MODE_MANUAL (1 << 9)
#define MEDIA_MODE_AUTO (1 << 10)
#define MEDIA_MODE_MOTION (1 << 11)
#define MEDIA_MODE_ALARM (1 << 12)
#define MEDIA_MODE_CUSTOM1 (1 << 13)
#define MEDIA_MODE_CUSTOM2 (1 << 14)
#define MEDIA_MODE_CUSTOM3 (1 << 15)
#define MEDIA_MODE_ALL (0X7F << 9)
/*通道号*/
#define MEDIA_CHANNEL_0 (1 << 16)
#define MEDIA_CHANNEL_1 (1 << 17)
#define MEDIA_CHANNEL_2 (1 << 18)
#define MEDIA_CHANNEL_3 (1 << 19)
#define MEDIA_CHANNEL_4 (1 << 20)
#define MEDIA_CHANNEL_5 (1 << 21)
#define MEDIA_CHANNEL_6 (1 << 22)
#define MEDIA_CHANNEL_7 (1 << 23)
#define MEDIA_CHANNEL_8 (1 << 24)
#define MEDIA_CHANNEL_9 (1 << 25)
#define MEDIA_CHANNEL_10 (1 << 26)
#define MEDIA_CHANNEL_11 (1 << 27)
#define MEDIA_CHANNEL_12 (1 << 28)
#define MEDIA_CHANNEL_13 (1 << 29)
#define MEDIA_CHANNEL_14 (1 << 30)
#define MEDIA_CHANNEL_15 (1 << 31)
#define MEDIA_CHANNEL_ALL (0XFFFF << 16)
#define MEDIA_SELECT_ALL (0XFFFFFFFF)

static inline int Amy_media_mask_to_channel(unsigned long mask) {
	int channel = 0;
	mask = mask >> 16;
	// if (mask & 0xFFFF0000) { channel += 16; mask >>= 16; }
	if (mask & 0xFF00) { channel += 8;  mask >>= 8; }
	if (mask & 0xF0) { channel += 4;  mask >>= 4; }
	if (mask & 0xC) { channel += 2;  mask >>= 2; }
	if (mask & 0x2) { channel += 1; }
	return channel;
}

typedef struct {
	char timestamp[MEDIA_TIMESTAP_LEN + 1];
	char duration[MEDIA_DURTION_LEN + 1];
	// bit[0:1]:0已读，1未读;
	//// bit[1]:0未锁，1已锁，
	// bit[2:8]:1照片，2视频，4音频，8、16、32预留
	// bit[9:15]:1手动，2自动，4侦测，8警报，16、32、64预留
	// bit[17:31] bit0-16分别代表通道0到通道16
	unsigned long mask;
	int year;
	int month;
	int day;
} media_file_info_t;

/*
 * 创建一个文件
 * media_type:文件类型，查看Amy_media_mgr.h
 * media_type num:文件类型数目,最大值为7
 * record_mode:记录模式，请看Amy_media_mgr.h
 * record_mode_num:记录模式数目，最大值为8
 * channel:通道号，请看Amy_media_mgr.h
 * channel_num:通道数目，最大值为16
 * path:创建的文件路径
 * path_max:文件路径buffer的最大值
 * return :0 success,非0初始化失败
 */
int Amy_media_insert(Amy_media_context context, int duration, const struct tm* tm, char* path, int path_max);
/*
 * 依据掩码打开媒体文件句柄
 *  bit[0]:1已读，0未读;
 *  bit[1]:0未锁，1已锁，
 *  bit[2:8]:1照片，2视频，4音频，8、16、32 64预留
 *  bit[9:16]:0手动，2自动，4侦测，8警报，16、32、64、128预留
 *  bit[17:32]
 */
Amy_media_context Amy_media_open(const char* base_dir, unsigned int mask, int page_num_max);

/*
 * 修改掩码
 */
int Amy_media_mask_change(Amy_media_context* ctx, unsigned int mask);
/*
 * 获取最后一个记录的文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t* Amy_media_first_page(Amy_media_context context, int* num);
/*
 * 获取当前记录文件的下一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t* Amy_media_next_page(Amy_media_context context, int* num);

/*
 * 获取当前记录文件的下一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t* Amy_media_current_page(Amy_media_context context, int* num);

/*
 * 获取当前记录文件的上一个文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :NULL失败，非空获取到信息
 */
const media_file_info_t* Amy_media_prev_page(Amy_media_context context, int* num);

/*
 * 清除媒体新文件属性
 * return :0 success,非0初始化失败
 */
int Amy_media_attr_new_clean(Amy_media_context context);

/*
 * 删除当前文件
 * Amy_media_handle : Amy_media_open后的句柄
 * return :0 success,非0初始化失败
 */
const media_file_info_t* Amy_media_delete_file(Amy_media_context context);

/*
 * 关闭媒体文件
 * return :0 success,非0初始化失败
 */
int Amy_media_close(Amy_media_context context);

/*
 * 文件信息转为文件路径
 */
int Amy_media_info_conver_path(Amy_media_context context, const media_file_info_t* info, char* path, int size);

/*
 *	依据时间戳获取文件路径
 */
int Amy_media_timestamp_conver_path(Amy_media_context context, unsigned int start, unsigned int end, const char* dot, char* path, int size);

/*
 * 依据年月查询最新的视频日期
 */
int Amy_media_query_by_month(Amy_media_context context, struct tm* tm, int index, int* day);

/*
 * 依据年月查询当月有媒体的日期，一个bit表示一个日期，day从bit1~bit31表示1日~31日
 */
int Amy_media_query_everyday_by_month(Amy_media_context context, struct tm* tm, int index, unsigned int* day);

/*
 * 查询年月日的文件信息个数及时间戳
 */
media_file_info_t* Amy_media_query_by_day(Amy_media_context context, struct tm* tm, int index, int* count);

/*
* 获取当前媒体的总数
*/
int Amy_media_current_total(Amy_media_context context, unsigned long* current, unsigned long* total);
/*
* 获取当前媒体的日期
*/
int Amy_media_current_time(Amy_media_context context, struct tm* tm);
/*
*设置文件读取索引
*/
int Amy_media_file_index_set(Amy_media_context context, unsigned long index);
/*
*按跨天全局序号定位读取位置（all_index，0 为最新）
*/
int Amy_media_all_index_set(Amy_media_context context, unsigned long index);
/*
*获取文件读取索引
*/
unsigned long Amy_media_file_index_get(Amy_media_context context);
/*
* 获取当前媒体信息
*/
const media_file_info_t* Amy_media_current_media_info(Amy_media_context context);
/*
* 下一个媒体信息
*/
const media_file_info_t* Amy_media_next_media_info(Amy_media_context context);
/*
* 上一个媒体信息
*/
const media_file_info_t* Amy_media_prev_media_info(Amy_media_context context);
/*
* 所有文件媒体总数，当前索引
*/
void Amy_media_all_index_total_get(Amy_media_context context, unsigned long* index, unsigned long* total);
/*
* 当前日期的媒体总数，当前索引
*/
void Amy_media_page_index_total_get(Amy_media_context context, unsigned long* index, unsigned long* total);
/*
* 删除所有文件
*/
void Amy_media_delete_all_file(Amy_media_context context);
/*
* 按日期范围删除文件（year/month/day=0 表示该层级不过滤）
* year=0 删除全部，month=0 删除整年，day=0 删除整月
*/
void Amy_media_delete_by_date(Amy_media_context context, int year, int month, int day);
/*
* 所有文件媒体新文件总数
*/
void Amy_media_new_total_get(Amy_media_context context, unsigned long* total);

/*
 * 查询有媒体文件的年份列表（分页）
 * years_buf: 输出年份数组，buf_size: 数组大小，offset: 起始偏移，total: 输出总年份数
 * return: 实际返回的年份数量，<0 失败
 */
int Amy_media_query_years(Amy_media_context context, int *years_buf, int buf_size, int offset, int *total);

/*
 * 查询指定年份下有媒体文件的月份列表（分页）
 * return: 实际返回的月份数量，<0 失败
 */
int Amy_media_query_months(Amy_media_context context, int year, int *months_buf, int buf_size, int offset, int *total);

/*
 * 查询指定年月下有媒体文件的日期列表（分页）
 * return: 实际返回的日期数量，<0 失败
 */
int Amy_media_query_days(Amy_media_context context, int year, int month, int *days_buf, int buf_size, int offset, int *total);

/*
 * 查询指定日期的文件总数
 * return: 0 成功，<0 失败
 */
int Amy_media_query_date_file_total(Amy_media_context context, int year, int month, int day, int *total);

/*
 * 跳转到指定日期并加载文件首页
 * return: 文件信息指针，NULL 失败；num 输出文件数量，total_files 输出当日文件总数
 */
const media_file_info_t *Amy_media_goto_date(Amy_media_context context, int year, int month, int day, int *num, int *total_files);

/*
 * 设置日期范围过滤（0 表示不过滤该层级）
 * year=0 清除所有日期过滤
 * return: 0 成功，<0 失败
 */
int Amy_media_set_date_filter(Amy_media_context context, int year, int month, int day);

#endif