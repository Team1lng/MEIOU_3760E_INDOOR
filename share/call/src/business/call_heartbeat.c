/**
 * @file        call_heartbeat.c
 * @brief       心跳与超时检测模块
 * @details     负责设备心跳、离线检测、通话最大时长、无操作超时管理
 * @ingroup     call_heartbeat
 */
#include "call_core.h"
#include "onvif/onvif.h"
#include "call_event_task_manager_internal.h"
#include "call_heartbeat_internal.h"
#include "common_utils.h"
#include "pthread.h"
#include <unistd.h>

#define HEARTBEAT_THREAD_STACK_SIZE (32 * 1024) ///< 心跳线程栈大小(32KB)
#define HEARTBEAT_QUEUE_MAX_COUNT 16            ///< 心跳事件队列最大容量
#define HEARTBEAT_SLEEP_INTERVAL 10             ///< 线程循环休眠时间(10ms)
#define DEVICE_TIMEOUT_COUNT 3                  ///< 设备心跳最大检测失败次数
#define HEARTBEAT_REQUEST_INTERVAL 5000         ///< 心跳请求发送间隔(5s)
#define HEARTBEAT_CHECK_INTERVAL 10000          ///< 心跳检测间隔(10s)

/**
 * @brief 心跳模块全局上下文结构体
 *
 * 存储心跳模块运行所需的所有时间戳，用于定时任务调度
 */
typedef struct
{
    uint32_t last_req_time;                ///< 上次发送心跳请求时间戳(ms)
    uint32_t last_heartbeat_check_time;    ///< 上次心跳检测时间戳(ms)
    uint32_t last_max_duration_check_time; ///< 上次最大时长检测时间戳(ms)
    uint32_t last_no_interact_check_time;  ///< 上次无操作超时检测时间戳(ms)
} heartbeat_context;

static heartbeat_context heartbeat_ctx; ///< 心跳模块全局上下文

/**
 * @brief 单个会话心跳请求发送
 *
 * 对活跃会话的关联设备定时发送心跳请求，失败计数自增
 *
 * @param[in] session_ctx 目标会话上下文
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败(会话无效/空闲)
 */
static int session_heartbeat_request_send(call_session_context *session_ctx)
{
    if (session_ctx == NULL)
    {
        return -1;
    }

    if (session_ctx->call_state == CALL_IDLE || session_ctx->call_state == CALL_END)
    {
        return -1;
    }

    for (int dev_type = 0; dev_type < DEVICE_MAX; dev_type++)
    {
        if (!(session_ctx->call_flag & (0x01 << dev_type)))
        {
            continue;
        }

        device_info_attr remote_device = session_ctx->device_info;
        remote_device.device_type = dev_type;
        if (call_global_ctx.comm_interface.call_heartbeat_request != NULL)
            call_global_ctx.comm_interface.call_heartbeat_request(remote_device);

        session_ctx->online_info[dev_type].check_count++;

        CALL_LOG_DEBUG(CALL_MODULE_HEARTBEAT, "Send heartbeat req to device %d ", dev_type);
    }

    return 0;
}

/**
 * @brief 全局心跳请求发送任务
 *
 * 遍历所有会话，执行心跳请求发送
 *
 * @return 0 任务执行成功
 */
static int global_heartbeat_request_task(void)
{
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *sessions_ctx = core_get_clls();
        session_heartbeat_request_send(&sessions_ctx[i]);
    }
    return 0;
}

/**
 * @brief 单个会话心跳检测
 *
 * 检查设备心跳失败计数，超阈值标记离线，全部离线则结束会话
 *
 * @param[in] session_ctx 目标会话上下文
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int session_heartbeat_check(call_session_context *session_ctx)
{
    if (session_ctx == NULL)
    {
        return -1;
    }

    if (session_ctx->call_state == CALL_IDLE || session_ctx->call_state == CALL_END)
    {
        return -1;
    }

    bool need_end_session = false;

    for (int dev_type = 0; dev_type < DEVICE_MAX; dev_type++)
    {
        if (!(session_ctx->call_flag & (0x01 << dev_type)))
        {
            continue;
        }

        if (session_ctx->online_info[dev_type].check_count > DEVICE_TIMEOUT_COUNT)
        {
            session_ctx->online_info[dev_type].status = DEVICE_STATUS_OFFLINE;
            session_ctx->call_flag &= ~(0x01 << dev_type);
            session_ctx->online_info[dev_type].check_count = 0;
            CALL_LOG_INFO(CALL_MODULE_HEARTBEAT, "Device %d (session_id=%llu) timeout (count=%d), mark offline",
                          dev_type, session_ctx->session_id, session_ctx->online_info[dev_type].check_count);
        }
    }

    if (session_ctx->call_flag == 0x00)
    {
        need_end_session = true;
    }

    if (need_end_session)
    {
        session_ctx->reason = REASON_DEVICE_OFFLINE;
        update_current_call_state(session_ctx, CALL_END);
    }

    return 0;
}

/**
 * @brief 全局心跳检测任务
 *
 * 遍历所有会话，执行离线检测
 *
 * @return 0 任务执行成功
 */
static int global_heartbeat_check_task(void)
{
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *sessions_ctx = core_get_clls();
        session_heartbeat_check(&sessions_ctx[i]);
    }
    return 0;
}

/**
 * @brief 单个会话最大通话时长检测
 *
 * 超过最大时长自动结束会话，防止异常占用
 *
 * @param[in] session_ctx 目标会话上下文
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int session_max_duration_check(call_session_context *session_ctx)
{
    if (session_ctx == NULL)
    {
        return -1;
    }

    if (session_ctx->call_state == CALL_IDLE)
    {
        return -1;
    }

    bool need_end_session = false;
    time_t current_time = common_timestamp_get();
    time_t elapsed_time = (current_time - session_ctx->start_time);

    if (elapsed_time >= (time_t)call_global_ctx.max_call_duration_ms)
    {
        CALL_LOG_INFO(CALL_MODULE_HEARTBEAT, "session_id=%llu max duration reached (elapsed=%ldms, max=%ums)",
                      session_ctx->session_id, elapsed_time, call_global_ctx.max_call_duration_ms);
        need_end_session = true;
    }

    if (need_end_session)
    {
        session_ctx->reason = REASON_MAX_DURATION_TIMEOUT;
        update_current_call_state(session_ctx, CALL_END);
    }

    return 0;
}

/**
 * @brief 全局最大通话时长检测任务
 *
 * @return 0 任务执行成功
 */
static int global_max_duration_check_task(void)
{
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *sessions_ctx = core_get_clls();
        session_max_duration_check(&sessions_ctx[i]);
    }
    return 0;
}

/**
 * @brief 单个会话无操作超时检测
 *
 * 非通话状态下长时间无交互，自动结束会话
 *
 * @param[in] session_ctx 目标会话上下文
 * @return 执行结果
 * @retval 0 成功
 * @retval -1 失败
 */
static int session_no_interact_timeout_check(call_session_context *session_ctx)
{
    if (session_ctx == NULL)
    {
        return -1;
    }

    if (call_global_ctx.no_interact_timeout_ms == 0)
    {
        return -1;
    }

    bool need_end_session = false;

    if (session_ctx->call_state == CALL_IDLE)
    {
        return -1;
    }

    time_t current_time = common_timestamp_get();
    time_t idle_time = (current_time - session_ctx->last_interact_time);

    if (idle_time >= (time_t)call_global_ctx.no_interact_timeout_ms && session_ctx->call_state != CALL_CONNECTED)
    {
        CALL_LOG_INFO(CALL_MODULE_HEARTBEAT, "session_id=%llu no interact timeout (idle=%ldms, threshold=%ums)",
                      session_ctx->session_id, idle_time, call_global_ctx.no_interact_timeout_ms);
        need_end_session = true;
    }

    if (need_end_session)
    {
        session_ctx->reason = REASON_NO_INTERACT_TIMEOUT;
        update_current_call_state(session_ctx, CALL_END);
    }

    return 0;
}

/**
 * @brief 全局无操作超时检测任务
 *
 * @return 0 任务执行成功
 */
static int global_no_interact_check_task(void)
{
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *sessions_ctx = core_get_clls();
        session_no_interact_timeout_check(&sessions_ctx[i]);
    }
    return 0;
}

/**
 * @brief 心跳模块主循环迭代函数
 *
 * 按配置时间间隔调度4类检测任务：
 * 1. 心跳请求发送
 * 2. 设备离线检测
 * 3. 通话最大时长检测
 * 4. 无操作超时检测
 *
 * @return 0 执行成功
 */
static int call_heartbeat_iterate(void)
{
    uint32_t current_time = common_timestamp_get();

    if (current_time - heartbeat_ctx.last_req_time >= HEARTBEAT_REQUEST_INTERVAL)
    {
        global_heartbeat_request_task();
        heartbeat_ctx.last_req_time = current_time;
    }

    if (current_time - heartbeat_ctx.last_heartbeat_check_time >= HEARTBEAT_CHECK_INTERVAL)
    {
        global_heartbeat_check_task();
        heartbeat_ctx.last_heartbeat_check_time = current_time;
    }

    if (current_time - heartbeat_ctx.last_max_duration_check_time >= HEARTBEAT_CHECK_INTERVAL)
    {
        global_max_duration_check_task();
        heartbeat_ctx.last_max_duration_check_time = current_time;
    }

    if (current_time - heartbeat_ctx.last_no_interact_check_time >= HEARTBEAT_CHECK_INTERVAL)
    {
        global_no_interact_check_task();
        heartbeat_ctx.last_no_interact_check_time = current_time;
    }

    return 0;
}

/**
 * @brief 被动心跳请求处理
 *
 * 收到设备心跳请求时，回复心跳响应
 *
 * @param[in] device_info 设备信息
 * @return 0 成功 / -1 失败
 */
int call_event_passive_heartbeat_request(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_HEARTBEAT, "heartbeat request failed: NULL device info");
        return -1;
    }

    int dev_type = device_info->device_type;
    int building = device_info->building;
    int room = device_info->room;

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_HEARTBEAT, "heartbeat request failed: no active session for device %d (building=%d, room=%d)",
                       dev_type, building, room);
        return -1;
    }

    if (call_global_ctx.comm_interface.call_heartbeat_response != NULL)
        call_global_ctx.comm_interface.call_heartbeat_response(*device_info);

    return 0;
}

/**
 * @brief 被动心跳响应处理
 *
 * 收到设备心跳响应，重置失败计数，标记在线
 *
 * @param[in] device_info 设备信息
 * @return 0 成功 / -1 失败
 */
int call_event_passive_heartbeat_response(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_HEARTBEAT, "heartbeat response failed: NULL device info");
        return -1;
    }

    int dev_type = device_info->device_type;
    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_HEARTBEAT, "heartbeat response failed: no active session for device %d", dev_type);
        return -1;
    }

    session_ctx->online_info[dev_type].status = DEVICE_STATUS_ONLINE;
    session_ctx->online_info[dev_type].check_count = 0;

    return 0;
}

/**
 * @brief 心跳模块初始化
 *
 * 注册心跳事件回调、注册空闲任务、初始化时间戳
 *
 * @return 初始化结果
 * @retval true 成功
 * @retval false 失败
 */
bool heartbeat_module_init(void)
{
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_REQUEST, call_event_passive_heartbeat_request);
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_RESPONSE, call_event_passive_heartbeat_response);
    call_idle_task_handler_register(IDLE_TASK_HEARTBEAT, call_heartbeat_iterate);

    uint32_t current_time = common_timestamp_get();
    heartbeat_ctx.last_req_time = current_time;
    heartbeat_ctx.last_heartbeat_check_time = current_time;
    heartbeat_ctx.last_max_duration_check_time = current_time;
    heartbeat_ctx.last_no_interact_check_time = current_time;

    CALL_LOG_INFO(CALL_MODULE_HEARTBEAT, "success: heartbeat module ready");
    return true;
}

/**
 * @brief 心跳模块反初始化
 *
 * 注销事件与空闲任务，释放资源
 *
 * @return 反初始化结果
 * @retval true 成功
 * @retval false 失败
 */
bool heartbeat_module_uninit(void)
{
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_REQUEST);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_RESPONSE);
    call_idle_task_handler_unregister(IDLE_TASK_HEARTBEAT);

    CALL_LOG_INFO(CALL_MODULE_HEARTBEAT, "success: all resources released");
    return true;
}