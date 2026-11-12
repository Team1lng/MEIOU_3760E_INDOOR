/**
 * @file        call_event_task_manager_internal.c
 * @brief       通话系统 - 事件分发与空闲任务管理模块
 * @details     提供事件处理器注册/注销/分发、空闲任务注册/遍历执行能力
 * @ingroup     call_event
 */

#include "call_event_task_manager_internal.h"
#include "call_log.h"
#include "pthread.h"

#define EVENT_HANDLER_MAP_SIZE 32 /**< 普通事件处理器映射表大小 */
#define IDLE_TASK_MAP_SIZE 8      /**< 空闲任务映射表大小 */

/**
 * @brief 普通事件处理器映射表项
 */
typedef struct
{
    int event_type;               /**< 事件类型ID */
    call_event_handler_t handler; /**< 事件处理回调函数 */
    bool is_registered;           /**< 是否已注册标记 */
} event_handler_item_t;

/**
 * @brief 空闲任务处理器映射表项
 */
typedef struct
{
    int task_id;          /**< 空闲任务唯一ID */
    int (*handler)(void); /**< 空闲任务执行函数 */
    bool is_registered;   /**< 是否已注册标记 */
} idle_task_item_t;

static event_handler_item_t event_handler_map[EVENT_HANDLER_MAP_SIZE]; /**< 普通事件处理器表 */
static idle_task_item_t idle_task_map[IDLE_TASK_MAP_SIZE];             /**< 空闲任务处理器表 */
static pthread_mutex_t idle_task_mutex = PTHREAD_MUTEX_INITIALIZER;    /**< 空闲任务访问锁 */

/**
 * @brief 注册普通事件处理器
 * @param[in] event_type 事件类型ID
 * @param[in] handler 事件处理回调
 * @return 0=成功，-1=参数错误/重复注册
 */
int call_event_handler_register(int event_type, call_event_handler_t handler)
{
    if (handler == NULL || event_type < 0 || event_type >= EVENT_HANDLER_MAP_SIZE)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "invalid register param: event_type=%d (valid range 0~%d), handler=%p",
                       event_type, EVENT_HANDLER_MAP_SIZE - 1, handler);
        return -1;
    }

    if (event_handler_map[event_type].is_registered)
    {
        CALL_LOG_WARN(CALL_MODULE_EVENT, "event_type=%d already registered, cannot register again", event_type);
        return -1;
    }

    event_handler_map[event_type].event_type = event_type;
    event_handler_map[event_type].handler = handler;
    event_handler_map[event_type].is_registered = true;
    return 0;
}

/**
 * @brief 注销普通事件处理器
 * @param[in] event_type 事件类型ID
 * @return 0=成功，-1=参数错误/未注册
 */
int call_event_handler_unregister(int event_type)
{
    if (event_type < 0 || event_type >= EVENT_HANDLER_MAP_SIZE)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "invalid unregister param: event_type=%d (valid range 0~%d)",
                       event_type, EVENT_HANDLER_MAP_SIZE - 1);
        return -1;
    }

    if (!event_handler_map[event_type].is_registered)
    {
        CALL_LOG_WARN(CALL_MODULE_EVENT, "event_type=%d not registered, cannot unregister", event_type);
        return -1;
    }

    memset(&event_handler_map[event_type], 0, sizeof(event_handler_item_t));
    return 0;
}

/**
 * @brief 分发事件到对应处理器
 * @param[in] event_attr 事件属性结构体
 * @return 0=成功，-1=参数错误/无处理器
 */
int call_event_dispatch(call_cb_attr *event_attr)
{
    if (event_attr == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "dispatch failed: event_attr is NULL");
        return -1;
    }

    int event_type = event_attr->call_event;
    if (event_type < 0 || event_type >= EVENT_HANDLER_MAP_SIZE)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "dispatch failed: invalid event_type=%d (valid range 0~%d)",
                       event_type, EVENT_HANDLER_MAP_SIZE - 1);
        return -1;
    }

    if (event_handler_map[event_type].is_registered)
    {
        call_event_handler_t handler = event_handler_map[event_type].handler;
        return handler(&event_attr->device_info);
    }
    else
    {
        CALL_LOG_WARN(CALL_MODULE_EVENT, "dispatch failed: no handler registered for event_type=%d", event_type);
        return -1;
    }
}

/**
 * @brief 注册空闲任务
 * @param[in] task_id 任务ID
 * @param[in] handler 任务执行函数
 * @return 0=成功，-1=参数错误/重复注册
 */
int call_idle_task_handler_register(int task_id, int (*handler)(void))
{
    if (handler == NULL || task_id < 0 || task_id >= IDLE_TASK_MAP_SIZE)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "invalid idle task register param: task_id=%d, handler=%p",
                       task_id, handler);
        return -1;
    }

    pthread_mutex_lock(&idle_task_mutex);

    if (idle_task_map[task_id].is_registered)
    {
        CALL_LOG_WARN(CALL_MODULE_EVENT, "idle task_id=%d already registered", task_id);
        pthread_mutex_unlock(&idle_task_mutex);
        return -1;
    }

    idle_task_map[task_id].task_id = task_id;
    idle_task_map[task_id].handler = handler;
    idle_task_map[task_id].is_registered = true;

    pthread_mutex_unlock(&idle_task_mutex);
    return 0;
}

/**
 * @brief 注销空闲任务
 * @param[in] task_id 任务ID
 * @return 0=成功，-1=参数错误/未注册
 */
int call_idle_task_handler_unregister(int task_id)
{
    if (task_id < 0 || task_id >= IDLE_TASK_MAP_SIZE)
    {
        CALL_LOG_ERROR(CALL_MODULE_EVENT, "invalid idle task unregister param: task_id=%d", task_id);
        return -1;
    }

    pthread_mutex_lock(&idle_task_mutex);

    if (!idle_task_map[task_id].is_registered)
    {
        CALL_LOG_WARN(CALL_MODULE_EVENT, "idle task_id=%d not registered", task_id);
        pthread_mutex_unlock(&idle_task_mutex);
        return -1;
    }

    memset(&idle_task_map[task_id], 0, sizeof(idle_task_item_t));

    pthread_mutex_unlock(&idle_task_mutex);
    return 0;
}

/**
 * @brief 遍历执行所有已注册的空闲任务
 * @note 通常在事件循环空闲时调用
 */
void call_idle_task_iterate(void)
{
    pthread_mutex_lock(&idle_task_mutex);

    for (int i = 0; i < IDLE_TASK_MAP_SIZE; i++)
    {
        idle_task_item_t *task = &idle_task_map[i];
        if (!task->is_registered || task->handler == NULL)
            continue;

        int ret = task->handler();
        if (ret != 0)
        {
            CALL_LOG_WARN(CALL_MODULE_EVENT, "idle task execute failed: task_id=%d, ret=%d", task->task_id, ret);
        }
    }

    pthread_mutex_unlock(&idle_task_mutex);
}