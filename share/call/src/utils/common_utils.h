/**
 * @file        common_utils.h
 * @brief       通用工具模块接口
 * @details     时间戳、消息队列创建/发送/接收/销毁
 * @ingroup     common_utils
 */

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#include <stdbool.h>

/*********************************************************************
 * 函数声明（按模块规范整理）
 ********************************************************************/

/**
 * @brief  获取系统时间戳
 * @return 时间戳（unsigned long long）
 */
unsigned long long common_timestamp_get(void);

/**
 * @brief  创建消息队列
 * @param  handle_id 输出队列句柄
 * @return true=成功，false=失败
 */
bool common_msg_queue_create(int *handle_id);

/**
 * @brief  向消息队列发送数据
 * @param  handle_id 队列句柄
 * @param  data 数据指针
 * @param  size 数据长度
 * @return true=成功，false=失败
 */
bool common_msg_queue_send(int handle_id, void *data, int size);

/**
 * @brief  从消息队列接收数据
 * @param  handle_id 队列句柄
 * @param  data 接收缓冲区
 * @param  data_size 缓冲区大小
 * @return true=成功，false=失败
 */
bool common_msg_queue_recv(int handle_id, void *data, int data_size);

/**
 * @brief  销毁消息队列
 * @param  handle_id 队列句柄
 * @return true=成功，false=失败
 */
bool common_msg_queue_destroy(int handle_id);

#endif // __COMMON_UTILS_H__