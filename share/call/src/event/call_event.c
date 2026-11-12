/**
 * @file        call_event.c
 * @brief       通话系统 - 事件队列处理线程模块
 * @details     负责事件接收、队列管理、事件分发、空闲任务调度
 * @ingroup     call_event
 */

#include "call_core.h"
#include "call_log.h"
#include "call_event_task_manager_internal.h"
#include "call_event_internal.h"
#include "pthread.h"
#include "common_utils.h"
#include "call_heartbeat_internal.h"
#include <unistd.h>

#define EVENT_THREAD_STACK_SIZE (32 * 1024) /**< 事件线程栈大小 32KB */
#define EVENT_THREAD_PRIORITY 12            /**< 事件线程优先级 */
#define EVENT_PROCESS_SLEEP_INTERVAL 10000  /**< 线程空闲休眠 10ms */

/**
 * @brief 事件模块全局上下文
 */
typedef struct
{
    int q;                 /**< 消息队列句柄 */
    uint8_t run;           /**< 线程运行标志 */
    pthread_mutex_t mutex; /**< 上下文保护锁 */
    pthread_t tid;         /**< 事件线程ID */
} event_context;

/**
 * @brief 模块全局上下文（默认0初始化）
 */
static event_context event_ctx;

/**
 * @brief 事件处理入口（分发到注册的业务handler）
 * @param[in] attr 事件属性结构体
 */
static void call_event_handle(call_cb_attr *attr)
{
    if (attr == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "invalid event attr (NULL pointer)");
        return;
    }

    int ret = call_event_dispatch(attr);
    if (ret != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "event process failed - event_type=%d, ret=%d",
                       attr->call_event, ret);
    }
}

/**
 * @brief 空闲任务调度入口
 */
static void call_idel_task_handle(void)
{
    call_idle_task_iterate();
}

/**
 * @brief 事件处理主线程
 * @param[in] arg 线程入参（未使用）
 * @return 退出码
 */
void *call_handle_thread(void *arg)
{
    call_msg msg;
    msg.msg_type = sizeof(msg.attr);

    while (1)
    {
        pthread_mutex_lock(&event_ctx.mutex);
        bool run = event_ctx.run;
        pthread_mutex_unlock(&event_ctx.mutex);

        if (!run)
            break;

        if (common_msg_queue_recv(event_ctx.q, &msg, msg.msg_type) > 0)
        {
            call_event_handle(&msg.attr);
        }
        else
        {
            call_idel_task_handle();
        }

        usleep(EVENT_PROCESS_SLEEP_INTERVAL);
    }

    CALL_LOG_INFO(CALL_MODULE_EVENT, "event processing thread exit successfully");
    return NULL;
}

/**
 * @brief 初始化事件模块（队列 + 线程 + 锁）
 * @return true=成功
 */
bool call_event_module_init(void)
{
    memset(&event_ctx, 0, sizeof(event_context));
    event_ctx.q = -1;

    if (pthread_mutex_init(&event_ctx.mutex, NULL) != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "failed to create module mutex");
        return false;
    }

    if (!common_msg_queue_create(&event_ctx.q))
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "failed to create msg queue");
        pthread_mutex_destroy(&event_ctx.mutex);
        event_ctx.q = -1;
        return false;
    }

    call_msg msg;
    msg.msg_type = sizeof(msg.attr);
    while (event_ctx.q >= 0 && common_msg_queue_recv(event_ctx.q, &msg, msg.msg_type) == true)
        ;

    pthread_mutex_lock(&event_ctx.mutex);
    event_ctx.run = true;
    pthread_mutex_unlock(&event_ctx.mutex);

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, EVENT_THREAD_STACK_SIZE);

    int ret = pthread_create(&event_ctx.tid, &thread_attr, call_handle_thread, NULL);
    pthread_attr_destroy(&thread_attr);

    if (ret != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "failed to create event thread (ret=%d)", ret);

        pthread_mutex_lock(&event_ctx.mutex);
        event_ctx.run = false;
        pthread_mutex_unlock(&event_ctx.mutex);

        common_msg_queue_destroy(event_ctx.q);
        pthread_mutex_destroy(&event_ctx.mutex);
        memset(&event_ctx, 0, sizeof(event_context));
        event_ctx.q = -1;
        return false;
    }

    CALL_LOG_INFO(CALL_MODULE_EVENT, "event module initialized successfully");
    return true;
}

/**
 * @brief 反初始化事件模块（安全停止线程 + 释放资源）
 * @return true=成功
 */
bool call_event_module_uninit(void)
{
    call_msg msg;
    msg.msg_type = sizeof(msg.attr);

    pthread_mutex_lock(&event_ctx.mutex);
    event_ctx.run = false;
    pthread_mutex_unlock(&event_ctx.mutex);

    pthread_join(event_ctx.tid, NULL);

    while (event_ctx.q >= 0 && common_msg_queue_recv(event_ctx.q, &msg, msg.msg_type) == true)
        ;

    if (event_ctx.q >= 0)
    {
        common_msg_queue_destroy(event_ctx.q);
        event_ctx.q = -1;
    }

    pthread_mutex_destroy(&event_ctx.mutex);
    memset(&event_ctx, 0, sizeof(event_context));
    event_ctx.q = -1;

    CALL_LOG_INFO(CALL_MODULE_EVENT, "event module uninitialized successfully");
    return true;
}

/**
 * @brief 外部事件入队接口
 * @param[in] attr 事件结构体
 * @return 0=成功
 */
int call_call_event_put(call_cb_attr *attr)
{
    if (attr == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "failed: NULL event attr");
        return -1;
    }

    pthread_mutex_lock(&event_ctx.mutex);
    bool run = event_ctx.run;
    int q = event_ctx.q;
    pthread_mutex_unlock(&event_ctx.mutex);

    if (!run)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "failed: call system not running");
        return -1;
    }

    if (q < 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "failed:queue not initialized");
        return -1;
    }

    call_msg msg;
    msg.msg_type = sizeof(msg.attr);
    msg.attr = *attr;

    if (common_msg_queue_send(q, &msg, msg.msg_type) == false)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "failed: queue put event error");
        return -1;
    }

    CALL_LOG_DEBUG(CALL_MODULE_CORE, "success: event_type=%d, device_type=%d",
                   msg.attr.call_event, msg.attr.device_info.device_type);
    return 0;
}