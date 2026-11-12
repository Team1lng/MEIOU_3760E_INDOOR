/**
 * @file        call_record.h
 * @brief       通话记录模块接口头文件
 * @details     提供通话记录存储、查询、分页、删除等功能
 * @ingroup     call_record
 */

#ifndef __CALL_RECORD_H__
#define __CALL_RECORD_H__

#include <stdint.h>
#include "call_common.h"

/**
 * @brief 通话记录类型枚举
 */
typedef enum
{
    CALL_LOG_UNKNOW,              /**< 未知类型 */
    CALL_LOG_INCOMING,            /**< 呼入（未接通） */
    CALL_LOG_INCOMING_AND_ANSWER, /**< 呼入（已接通） */
    CALL_LOG_OUTGOING,            /**< 呼出（未接通） */
    CALL_LOG_OUTCOMING_AND_ANSWER /**< 呼出（已接通） */
} call_log_type;

/**
 * @brief 单条通话记录结构体
 */
typedef struct
{
    call_log_type log_type;       /**< 通话类型 */
    uint64_t start_time;          /**< 开始时间戳（毫秒） */
    uint16_t duration;            /**< 通话时长（秒） */
    device_info_attr device_info; /**< 对方设备信息 */
} call_record_info_attr;

/**
 * @brief 查询通话记录（最新 → 最早）
 * @param[out] records 记录输出缓冲区
 * @param[in] max_count 最大查询条数
 * @param[in] start_idx 起始索引
 * @return 实际获取条数
 */
uint32_t record_list_query(call_record_info_attr *records, uint32_t max_count, uint32_t start_idx);

/**
 * @brief 删除指定索引的通话记录
 * @param[in] record_index 索引
 * @return 成功/失败
 */
bool record_list_delete(uint32_t record_index);

/**
 * @brief 删除所有通话记录
 * @return 成功/失败
 */
bool record_list_delete_all(void);

/**
 * @brief 获取当前记录总数
 * @return 记录总数
 */
uint32_t record_list_get_count(void);

/**
 * @brief 跳转到通话记录第一页
 * @param[out] record_info 输出记录
 * @param[in] max_per_page 每页最大条数
 * @param[out] num 实际返回条数
 */
void call_log_first_page(call_record_info_attr *record_info, uint32_t max_per_page, uint32_t *num);

/**
 * @brief 跳转到上一页（循环）
 * @param[out] record_info 输出记录
 * @param[out] num 实际返回条数
 */
void call_log_prev_page(call_record_info_attr *record_info, uint32_t *num);

/**
 * @brief 跳转到下一页（循环）
 * @param[out] record_info 输出记录
 * @param[out] num 实际返回条数
 */
void call_log_next_page(call_record_info_attr *record_info, uint32_t *num);

/**
 * @brief 获取当前页记录
 * @param[out] record_info 输出记录
 * @param[out] query_count 实际条数
 */
void call_log_current_page(call_record_info_attr *record_info, uint32_t *query_count);

/**
 * @brief 获取当前分页索引与总记录数
 * @param[out] current 当前索引
 * @param[out] total 总记录数
 */
void call_log_current_total(uint32_t *current, uint32_t *total);

#endif // __CALL_RECORD_H__