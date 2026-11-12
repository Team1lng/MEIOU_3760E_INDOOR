/**
 * @file        call_action.c
 * @brief       呼叫动作模块
 * @details     实现呼叫、接听、挂断等核心业务逻辑，注册事件回调处理主动/被动呼叫流程
 * @ingroup     call_action
 */
#include "call_core.h"
#include "onvif/onvif.h"
#include "call_event_task_manager_internal.h"
#include "call_action_internal.h"
#include "common_utils.h"
#include "call_log.h"
#include "pthread.h"
#include <string.h>

#define CALL_THREAD_STACK_SIZE (32 * 1024) ///< 呼叫线程栈大小，防止栈溢出
#define INVITE_TCP_TIMEOUT 1000            ///< 发起邀请TCP超时时间，等待200OK判断设备在线状态
#define FAMILY_MAX_DEVICES 15              ///< Family 最大设备数(ID1~ID15)

/**
 * @brief 校验设备类型是否为合法操作对象
 *
 * 真实设备取值 [DEVICE_TYPE_MIN, DEVICE_TYPE_MAX]，
 * 家族目标 DEVICE_TYPE_INDOOR_FAMILY 同样作为合法操作对象
 *
 * @param[in] device_type 设备类型
 * @return true=合法
 */
static bool call_device_type_valid(int device_type)
{
    if (device_type == DEVICE_TYPE_INDOOR_FAMILY)
    {
        return true;
    }
    return device_type >= DEVICE_TYPE_MIN && device_type <= DEVICE_TYPE_MAX;
}

/**
 * @brief 生成唯一会话ID
 *
 * 内部静态变量自增实现，从1开始递增，0表示无效ID
 *
 * @return 唯一会话ID
 * @retval uint32_t 有效会话ID
 * @note 仅模块内部使用，不对外暴露
 */
static uint32_t get_next_session_id(void)
{
    static unsigned long long g_session_id = 1;
    return ++g_session_id;
}

/**
 * @brief 主动呼叫设备核心逻辑（同步执行）
 *
 * 完成参数校验、设备信息提取、底层呼叫接口调用、呼叫状态标记更新
 *
 * @param[in] call_session_ctx 通话会话上下文指针
 * @param[in] device_info 目标设备信息指针
 * @return 无实际业务意义，兼容函数指针签名
 * @retval void*
 * @note 仅由call_active_invite_func内部调用，禁止应用层直接调用
 * @warning 入参必须保证非空，外部已完成有效性校验
 */
static void *call_invite_device_logic(call_session_context *call_session_ctx, device_info_attr *device_info)
{
    if (call_session_ctx == NULL || device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Invite logic failed: invalid context/device info");
        return NULL;
    }

    int building = device_info->building;
    int room = device_info->room;
    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Invite logic failed: invalid device type %d", device_type);
        return NULL;
    }

    CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Invite device: type=%d, building=%d, room=%d", device_type, building, room);

    if (call_global_ctx.comm_interface.call_invite != NULL)
    {
        bool call_result = call_global_ctx.comm_interface.call_invite(*device_info);
        if (call_result == true)
        {
            call_session_ctx->call_flag |= (0x01 << device_type);
            CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Invite device %d success, call_flag=0x%02X", device_type, call_session_ctx->call_flag);
        }
        else
        {
            CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Invite device %d failed: device not response", device_type);
        }
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Invite device %d failed: call_invite interface is NULL", device_type);
    }

    return NULL;
}

/**
 * @brief Family 并行拨号线程参数
 */
typedef struct
{
    call_session_context *session_ctx;
    device_info_attr device_info;
    pthread_mutex_t *flag_mutex;
} family_invite_thread_arg;

/**
 * @brief Family 并行拨号线程函数
 *
 * 每个线程独立发起 TCP invite，通过 mutex 保护 call_flag 并发写入
 *
 * @param[in] arg family_invite_thread_arg 指针（线程内 free）
 * @return NULL
 */
static void *family_invite_thread_func(void *arg)
{
    family_invite_thread_arg *t_arg = (family_invite_thread_arg *)arg;
    int device_type = t_arg->device_info.device_type;

    CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Family invite thread: device type=%d", device_type);

    if (call_global_ctx.comm_interface.call_invite != NULL)
    {
        bool call_result = call_global_ctx.comm_interface.call_invite(t_arg->device_info);
        if (call_result == true)
        {
            pthread_mutex_lock(t_arg->flag_mutex);
            t_arg->session_ctx->call_flag |= (0x01 << device_type);
            pthread_mutex_unlock(t_arg->flag_mutex);
            CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Family invite device %d success, call_flag=0x%02X",
                           device_type, t_arg->session_ctx->call_flag);
        }
        else
        {
            CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Family invite device %d failed: no response", device_type);
        }
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Family invite device %d failed: call_invite interface is NULL", device_type);
    }

    free(t_arg);
    return NULL;
}

/**
 * @brief 主动呼叫业务（应用层触发主动呼叫时调用）
 *
 * 核心流程：创建会话→发送呼出事件→批量执行呼叫逻辑→检查设备响应
 * Family 设备使用并行拨号，单设备使用串行拨号
 *
 * @param[in] device_info 目标设备信息指针
 * @return 呼叫发起结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 仅call_core模块通过事件调度器调用，应用层不直接调用
 */
static int call_active_invite_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: NULL device info");
        return -1;
    }

    if (core_get_call_count() != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: Please close all calls");
        return -1;
    }

    int device_type = device_info->device_type;
    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: invalid device type %d", device_type);
        return -1;
    }

    /* 身份校验，禁止呼叫本机设备 */
    device_info_attr local_info = core_get_local_info();
    if (memcmp(device_info, &local_info, sizeof(device_info_attr)) == 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: cannot call local device");
        return -1;
    }

    call_session_context *session_ctx = new_call_session_context();
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: no free session (max session reached)");
        return -1;
    }

    memcpy(&session_ctx->device_info, device_info, sizeof(device_info_attr));
    session_ctx->session_id = get_next_session_id();
    memcpy(&session_ctx->record_info_ctx.device_info, device_info, sizeof(device_info_attr));
    session_ctx->record_info_ctx.log_type = CALL_LOG_OUTGOING;
    session_ctx->call_flag = 0;
    session_ctx->reason = REASON_NONE;
    session_ctx->start_time = common_timestamp_get();
    session_ctx->last_interact_time = session_ctx->start_time;
    update_current_call_state(session_ctx, CALL_OUTGOING_PROCESS);

    if (device_type == DEVICE_TYPE_INDOOR_FAMILY)
    {
        CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Active invite: family device, parallel call %d~%d",
                       DEVICE_TYPE_INDOOR_MIN, DEVICE_TYPE_INDOOR_MAX);

        pthread_mutex_t flag_mutex;
        pthread_mutex_init(&flag_mutex, NULL);

        pthread_t threads[FAMILY_MAX_DEVICES];
        int thread_count = 0;

        for (int device = DEVICE_TYPE_INDOOR_MIN; device <= DEVICE_TYPE_INDOOR_MAX; device++)
        {
            device_info_attr dev_attr = {0};
            memcpy(&dev_attr, device_info, sizeof(device_info_attr));
            dev_attr.device_type = device;

            if (memcmp(&dev_attr, &local_info, sizeof(device_info_attr)) == 0)
            {
                CALL_LOG_WARN(CALL_MODULE_ACTION, "Skip local device %d in batch call", device);
                continue;
            }

            family_invite_thread_arg *t_arg = (family_invite_thread_arg *)malloc(
                sizeof(family_invite_thread_arg));
            if (t_arg == NULL)
            {
                CALL_LOG_ERROR(CALL_MODULE_ACTION, "Family invite: malloc failed for device %d", device);
                continue;
            }

            t_arg->session_ctx = session_ctx;
            memcpy(&t_arg->device_info, &dev_attr, sizeof(device_info_attr));
            t_arg->flag_mutex = &flag_mutex;
            pthread_attr_t attr;
            memset(&attr, 0, sizeof(pthread_attr_t));
            pthread_attr_setstacksize(&attr, 32 * 1024);
            pthread_create(&threads[thread_count], &attr, family_invite_thread_func, t_arg);
            thread_count++;
        }

        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }

        pthread_mutex_destroy(&flag_mutex);
    }
    else
    {
        device_info_attr attr = {0};
        memcpy(&attr, device_info, sizeof(device_info_attr));

        if (memcmp(&attr, &local_info, sizeof(device_info_attr)) != 0)
        {
            call_invite_device_logic(session_ctx, &attr);
        }
    }

    if (session_ctx->call_flag == 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active invite failed: no device responded (session_id=%llu)", session_ctx->session_id);
        session_ctx->reason = REASON_NO_RESPONSE;
        update_current_call_state(session_ctx, CALL_END);

        return -1;
    }

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Active invite success: session_id=%llu, call_flag=0x%02X", session_ctx->session_id, session_ctx->call_flag);
    return 0;
}

/**
 * @brief 主动接听业务（应用层触发接听呼入时调用）
 *
 * 查找关联会话→校验状态→调用设备接听接口→同步状态与事件
 *
 * @param[in] device_info 呼入设备信息指针
 * @return 接听结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 仅call_core模块通过事件调度器调用
 */
static int call_active_accept_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: NULL device info");
        return -1;
    }

    int device_type = device_info->device_type;
    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: invalid device type %d", device_type);
        return -1;
    }

    /* 身份校验，禁止应答本机发起的呼叫 */
    device_info_attr local_info = core_get_local_info();
    if (memcmp(device_info, &local_info, sizeof(device_info_attr)) == 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: cannot accept local call");
        return -1;
    }

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: session not found (device type=%d, building=%d, room=%d)",
                       device_info->device_type, device_info->building, device_info->room);
        return -1;
    }

    if (session_ctx->call_state != CALL_INCOMING)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: session state invalid (current=%d, expect=%d)",
                       session_ctx->call_state, CALL_INCOMING);
        return -1;
    }

    session_ctx->record_info_ctx.log_type = CALL_LOG_INCOMING_AND_ANSWER;
    session_ctx->call_flag |= (0x01 << device_type);
    if (call_global_ctx.comm_interface.call_accept != NULL)
    {
        call_global_ctx.comm_interface.call_accept(*device_info);
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active accept failed: call_accept interface is NULL (device type=%d)", device_type);
        return -1;
    }

    update_current_call_state(session_ctx, CALL_CONNECTED);

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Active accept success: session_id=%llu, device type=%d", session_ctx->session_id, device_info->device_type);
    return 0;
}

/**
 * @brief 主动挂断业务（应用层触发挂断时调用）
 *
 * 查找关联会话→校验状态→批量挂断设备→同步状态与事件
 * 支持家庭设备批量挂断
 *
 * @param[in] device_info 目标设备信息指针
 * @return 挂断结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 仅call_core模块通过事件调度器调用
 */
static int call_active_hangup_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active hangup failed: NULL device info");
        return -1;
    }

    int result = -1;
    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active hangup failed: invalid device type %d", device_type);
        return -1;
    }

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active hangup failed: session not found (device type=%d)", device_info->device_type);
        return -1;
    }

    if (session_ctx->call_state == CALL_IDLE)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active hangup failed: session is idle (session_id=%llu)", session_ctx->session_id);
        return -1;
    }

    int device_start = device_type;
    int device_end = device_type;
    if (device_type == DEVICE_TYPE_INDOOR_FAMILY)
    {
        device_start = DEVICE_TYPE_INDOOR_MIN;
        device_end = DEVICE_TYPE_INDOOR_MAX;

        CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Active hangup: family device, batch hangup %d~%d", device_start, device_end);
    }

    for (int device = device_end; device >= device_start; device--)
    {
        if (!(session_ctx->call_flag & (0x01 << device)))
        {
            continue;
        }

        if (call_global_ctx.comm_interface.call_end != NULL)
        {
            device_info_attr remote_device = *device_info;
            remote_device.device_type = device;
            call_global_ctx.comm_interface.call_end(remote_device);

            session_ctx->call_flag &= ~(0x01 << device);
        }
        else
        {
            CALL_LOG_ERROR(CALL_MODULE_ACTION, "Active hangup failed: call_end interface is NULL (device type=%d)", device);
        }
    }

    session_ctx->reason = REASON_HANGUP_SELF;
    update_current_call_state(session_ctx, CALL_END);

    result = 0;

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Active hangup success: session_id=%llu", session_ctx->session_id);
    return result;
}

/**
 * @brief 被动呼入处理（接收设备呼入邀请时回调）
 *
 * 接收呼入请求→创建会话→发送呼入事件→响应振铃→支持自动接听
 *
 * @param[in] device_info 呼入设备信息指针
 * @return 处理结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 由事件调度器自动回调，无需手动调用
 */
static int call_invite_callback_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive invite failed: NULL device info");
        return -1;
    }

    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive invite failed: invalid device type %d", device_type);
        return -1;
    }

    /* 身份校验，拦截本机发送的呼入请求 */
    device_info_attr local_info = core_get_local_info();
    if (memcmp(device_info, &local_info, sizeof(device_info_attr)) == 0)
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive invite failed: reject local call request");
        return -1;
    }

    if (call_core_active_session_count() >= call_global_ctx.max_sessions)
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive invite failed: max sessions reached (%d/%d), send busy (device type=%d)",
                      call_global_ctx.call_nb, call_global_ctx.max_sessions, device_type);
        if (call_global_ctx.comm_interface.call_busy != NULL)
            call_global_ctx.comm_interface.call_busy(*device_info);
        return -1;
    }

    call_session_context *session_ctx = new_call_session_context();
    if (session_ctx == NULL)
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive invite failed: no free session, send busy (device type=%d)", device_type);
        if (call_global_ctx.comm_interface.call_busy != NULL)
            call_global_ctx.comm_interface.call_busy(*device_info);
        return -1;
    }
    session_ctx->call_flag = (0X01 << device_type);
    memcpy(&session_ctx->device_info, device_info, sizeof(device_info_attr));
    session_ctx->session_id = get_next_session_id();
    memcpy(&session_ctx->record_info_ctx.device_info, device_info, sizeof(device_info_attr));
    session_ctx->record_info_ctx.log_type = CALL_LOG_INCOMING;
    session_ctx->reason = REASON_NONE;
    session_ctx->start_time = common_timestamp_get();
    session_ctx->last_interact_time = session_ctx->start_time;

    update_current_call_state(session_ctx, CALL_INCOMING);

    if (call_global_ctx.comm_interface.call_ringing != NULL)
    {
        call_global_ctx.comm_interface.call_ringing(*device_info);
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive invite failed: call_ringing interface is NULL (device type=%d)", device_type);
        update_current_call_state(session_ctx, CALL_END);
        return -1;
    }

    if (core_get_answer_auto())
    {
        CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Passive invite: auto answer enabled, accept call");
        int ret = call_active_accept_func(device_info);
        if (ret != 0)
        {
            CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive invite auto accept failed (device type=%d)", device_type);
        }
    }

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Passive invite success: session_id=%llu", session_ctx->session_id);
    return 0;
}

/**
 * @brief 被动振铃处理（接收设备振铃响应时回调）
 *
 * 接收振铃响应→校验会话状态→发送振铃事件→更新会话状态
 *
 * @param[in] device_info 振铃设备信息指针
 * @return 处理结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 由事件调度器自动回调
 */
static int call_ringing_callback_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive ringing failed: NULL device info");
        return -1;
    }

    int result = -1;
    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive ringing failed: invalid device type %d", device_type);
        return -1;
    }

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive ringing failed: session not found (building=%d, room=%d, type=%d)", device_info->building, device_info->room, device_info->device_type);
        return -1;
    }

    if (session_ctx->call_state != CALL_OUTGOING_PROCESS && session_ctx->call_state != CALL_OUTGOING_RINGING)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive ringing failed: invalid session state (device=%d, state=%d)",
                       device_type, session_ctx->call_state);
        return -1;
    }

    if (!(session_ctx->call_flag & (0x01 << device_type)))
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive ringing ignored: device %d not in call list (session_id=%llu)",
                      device_type, session_ctx->session_id);
        return -1;
    }

    update_current_call_state(session_ctx, CALL_OUTGOING_RINGING);
    result = 0;

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Passive ringing success: device=%d, session_id=%llu", device_type, session_ctx->session_id);
    return result;
}

/**
 * @brief 被动繁忙处理（接收设备繁忙响应时回调）
 *
 * 接收繁忙响应→清除呼叫标记→检查所有设备状态→结束会话（可选）
 *
 * @param[in] device_info 繁忙设备信息指针
 * @return 处理结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 由事件调度器自动回调
 */
static int call_busy_callback_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive busy failed: NULL device info");
        return -1;
    }

    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive busy failed: invalid device type %d", device_type);
        return -1;
    }

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive busy failed: session not found (device=%d)", device_type);
        return -1;
    }

    if (!(session_ctx->call_flag & (0x01 << device_type)))
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive busy ignored: device %d not in call list (session_id=%llu)",
                      device_type, session_ctx->session_id);
        return -1;
    }

    session_ctx->call_flag &= ~(0x01 << device_type);
    CALL_LOG_WARN(CALL_MODULE_ACTION, "Device %d is busy, call_flag=0x%02X (session_id=%llu)",
                  device_type, session_ctx->call_flag, session_ctx->session_id);

    if (session_ctx->call_flag == 0X00)
    {
        session_ctx->reason = REASON_BUSY;
        update_current_call_state(session_ctx, CALL_END);
    }

    return 0;
}

/**
 * @brief 被动接听处理（接收设备接听响应时回调）
 *
 * 接收接听响应→校验会话状态→挂断其他设备→同步状态与事件
 * 同一时间仅允许一个设备接听
 *
 * @param[in] device_info 接听设备信息指针
 * @return 处理结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 由事件调度器自动回调
 */
static int call_accept_callback_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive accept failed: NULL device info");
        return -1;
    }

    int result = -1;
    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive accept failed: invalid device type %d", device_type);
        return -1;
    }

    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive accept failed: session not found (device=%d)", device_type);
        return -1;
    }

    if (session_ctx->call_state == CALL_CONNECTED)
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive accept failed: session already connected (device=%d, session_id=%llu)",
                      device_type, session_ctx->session_id);
        if (call_global_ctx.comm_interface.call_busy != NULL)
            call_global_ctx.comm_interface.call_busy(*device_info);
        return -1;
    }

    if (!(session_ctx->call_flag & (0x01 << device_type)))
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive accept ignored: device %d not in call list (session_id=%llu)",
                      device_type, session_ctx->session_id);
        return -1;
    }

    for (int i = DEVICE_TYPE_MIN; i < DEVICE_TYPE_MAX; i++)
    {
        if ((session_ctx->call_flag & (0x01 << i)) != 0 && (i != device_type))
        {
            if (call_global_ctx.comm_interface.call_end != NULL)
            {
                device_info_attr remote_device = session_ctx->device_info;
                remote_device.device_type = i;
                call_global_ctx.comm_interface.call_end(remote_device);
                CALL_LOG_DEBUG(CALL_MODULE_ACTION, "End call with device %d (device %d accepted, session_id=%llu)",
                               i, device_type, session_ctx->session_id);
            }
            else
            {
                CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive accept failed: call_end interface is NULL (device type=%d)", i);
            }
        }
    }

    session_ctx->record_info_ctx.log_type = CALL_LOG_OUTCOMING_AND_ANSWER;
    session_ctx->call_flag = (0x01 << device_type);
    session_ctx->device_info = *device_info;
    CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Passive invite: auto answer enabled, accept call");

    update_current_call_state(session_ctx, CALL_CONNECTED);
    result = 0;

    CALL_LOG_INFO(CALL_MODULE_ACTION, "Passive accept success: device=%d, session_id=%llu", device_type, session_ctx->session_id);
    return result;
}

/**
 * @brief 被动结束处理（接收设备挂断响应时回调）
 *
 * 接收挂断响应→清除呼叫标记→检查所有设备状态→结束会话（可选）
 *
 * @param[in] device_info 结束通话设备信息指针
 * @return 处理结果
 * @retval 0 成功
 * @retval -1 失败
 * @note 由事件调度器自动回调
 */
static int call_end_callback_func(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive end failed: NULL device info");
        return -1;
    }
    int device_type = device_info->device_type;

    if (!call_device_type_valid(device_type))
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive end failed: invalid device type %d", device_type);
        return -1;
    }
    call_session_context *session_ctx = get_call_session(device_info);
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "Passive end failed: session not found (device=%d)", device_type);
        return -1;
    }
    if (!(session_ctx->call_flag & (0x01 << device_type)))
    {
        CALL_LOG_WARN(CALL_MODULE_ACTION, "Passive end ignored: device %d not in call list (session_id=%llu)",
                      device_type, session_ctx->session_id);
        return -1;
    }
    session_ctx->call_flag &= ~(0x01 << device_type);
    CALL_LOG_DEBUG(CALL_MODULE_ACTION, "Device %d end call, call_flag=0x%02X (session_id=%llu)",
                   device_type, session_ctx->call_flag, session_ctx->session_id);

    if (session_ctx->call_flag == 0X00)
    {
        session_ctx->reason = REASON_HANGUP;
        update_current_call_state(session_ctx, CALL_END);
    }
    return 0;
}

static int call_switch_callback_func(device_info_attr *device_info)
{

    int ret = 0;
    device_info_attr current_device_info = {0};
    call_session_context *target_session = NULL;
    bool need_hangup_old_session = false;
    if (core_get_current_call_device(&current_device_info))
    {
        if (memcmp(&current_device_info, device_info, sizeof(device_info_attr)) != 0)
        {
            need_hangup_old_session = true;
        }
        else
        {
            CALL_LOG_WARN(CALL_MODULE_ACTION, "target call is current call, no need to switch");
            return -1;
        }
    }
    target_session = get_call_session(device_info);
    if (target_session != NULL)
    {
        core_set_current_call(target_session);
    }
    else
    {
        CALL_LOG_ERROR(CALL_MODULE_ACTION, "failed: Not found target call (type=%d, bld=%d, room=%d)",
                       device_info->device_type, device_info->building, device_info->room);
        ret = -1;
    }
    if (need_hangup_old_session)
    {
        call_active_hangup_func(&current_device_info);
    }
    return ret;
}

/**
 * @brief 初始化通话动作模块
 *
 * 注册所有主动/被动呼叫事件回调函数
 *
 * @return 初始化结果
 * @retval true 成功
 * @retval false false 失败
 * @note 仅call_core模块调用，不对外暴露
 */
bool call_action_module_init(void)
{
    call_event_handler_register(CALL_EVENT_ID_CALL_ACTIVE_INVITE, call_active_invite_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_ACTIVE_ACCEPT, call_active_accept_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_ACTIVE_END, call_active_hangup_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_SWITCH_CALL, call_switch_callback_func);

    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_INVITE, call_invite_callback_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_RINGING, call_ringing_callback_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_BUSY, call_busy_callback_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_ACCEPT, call_accept_callback_func);
    call_event_handler_register(CALL_EVENT_ID_CALL_PASSIVE_END, call_end_callback_func);

    CALL_LOG_INFO(CALL_MODULE_ACTION, "call_action_module_init success: all event handlers registered");
    return true;
}

/**
 * @brief 反初始化通话动作模块
 *
 * 注销所有已注册的事件回调函数
 *
 * @return 反初始化结果
 * @retval true 成功
 * @retval false 失败
 * @note 仅call_core模块调用，不对外暴露
 */
bool call_action_module_uninit(void)
{
    call_event_handler_unregister(CALL_EVENT_ID_CALL_ACTIVE_INVITE);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_ACTIVE_ACCEPT);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_ACTIVE_END);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_SWITCH_CALL);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_INVITE);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_RINGING);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_BUSY);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_ACCEPT);
    call_event_handler_unregister(CALL_EVENT_ID_CALL_PASSIVE_END);

    CALL_LOG_INFO(CALL_MODULE_ACTION, "call_action_module_uninit success: all event handlers unregistered");
    return true;
}