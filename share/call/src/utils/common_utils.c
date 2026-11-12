/**
 * @file        common_utils.c
 * @brief       通用工具库
 * @details     提供时间戳、System V 消息队列封装接口
 * @ingroup     common_utils
 */

#include "common_utils.h"
#include "call_log.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>

/**
 * @brief 获取当前系统时间戳（毫秒级）
 * @return 时间戳（从 1970-01-01 开始）
 */
unsigned long long common_timestamp_get(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * @brief 创建系统消息队列
 * @param[out] handle_id 消息队列句柄
 * @return 成功/失败
 */
bool common_msg_queue_create(int *handle_id)
{
    if (handle_id == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue create failed: handle_id is NULL");
        return false;
    }

    static int queue_total = 0;
    queue_total++;
    key_t key = ftok("/tmp", queue_total + 'L');

    if (key == -1)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue create failed: ftok failed, errno=%d", errno);
        return false;
    }

    *handle_id = msgget(key, IPC_CREAT | 0666);
    if (*handle_id == -1)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue create failed: msgget failed, errno=%d", errno);
        return false;
    }

    CALL_LOG_DEBUG(CALL_MODULE_UTILS, "msg queue create success, handle_id=%d", *handle_id);
    return true;
}

/**
 * @brief 向消息队列发送数据
 * @param[in] handle_id 队列句柄
 * @param[in] data 数据指针
 * @param[in] size 数据长度
 * @return 成功/失败
 */
bool common_msg_queue_send(int handle_id, void *data, int size)
{
    if (data == NULL || size <= 0 || handle_id < 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue send failed: invalid param (handle_id=%d, size=%d, data=%p)",
                       handle_id, size, data);
        return false;
    }

    int ret = msgsnd(handle_id, data, size, 0);
    if (ret == -1)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue send failed: handle_id=%d, ret=%d, errno=%d",
                       handle_id, ret, errno);
        return false;
    }
    return true;
}

/**
 * @brief 从消息队列非阻塞接收数据
 * @param[in] handle_id 队列句柄
 * @param[out] data 接收缓冲区
 * @param[in] data_size 缓冲区大小
 * @return 成功/失败
 */
bool common_msg_queue_recv(int handle_id, void *data, int data_size)
{
    if (data == NULL || data_size <= 0 || handle_id < 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue recv failed: invalid param (handle_id=%d, data_size=%d, data=%p)",
                       handle_id, data_size, data);
        return false;
    }

    ssize_t ret = msgrcv(handle_id, data, data_size, data_size, IPC_NOWAIT);
    if (ret > 0)
    {
        return true;
    }
    return false;
}

/**
 * @brief 销毁消息队列
 * @param[in] handle_id 队列句柄
 * @return 成功/失败
 */
bool common_msg_queue_destroy(int handle_id)
{
    if (handle_id < 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue destroy failed: invalid handle_id=%d", handle_id);
        return false;
    }

    int ret = msgctl(handle_id, IPC_RMID, NULL);
    if (ret == -1)
    {
        CALL_LOG_ERROR(CALL_MODULE_UTILS, "msg queue destroy failed: handle_id=%d, errno=%d",
                       handle_id, errno);
        return false;
    }

    CALL_LOG_DEBUG(CALL_MODULE_UTILS, "msg queue destroy success: handle_id=%d", handle_id);
    return true;
}