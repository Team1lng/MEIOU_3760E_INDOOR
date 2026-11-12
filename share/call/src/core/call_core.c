
/**
 * @file        call_core.c
 * @brief       通话系统核心模块
 * @details     全局上下文管理、会话池、状态机、子模块初始化/反初始化
 * @ingroup     call_core
 */
#include "call_core.h"
#include "call_record_internal.h"
#include "call_action_internal.h"
#include "call_heartbeat_internal.h"
#include "call_event_internal.h"
#include "string.h"
#include "common_utils.h"
#include "pthread.h"

/**
 * @brief 全局核心上下文
 *
 * 存储全局配置、会话池、运行状态、事件队列等系统级资源
 */
call_context call_global_ctx =
    {
        .answer_auto = false,
        .max_sessions = CALL_SESSION_MAX,
        .run = false,
        .q = -1,
};

/**
 * @brief 全局上下文互斥锁
 *
 * 保护 call_global_ctx 与会话状态的并发访问
 */
static pthread_mutex_t g_call_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief 获取本地设备信息
 * @return 设备信息结构体
 */
device_info_attr core_get_local_info(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    device_info_attr device_info = call_global_ctx.local_info;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return device_info;
}

/**
 * @brief 动态更新本地设备信息（设备号/房号变更且不重启时调用）
 * @param[in] info 新的设备信息（不可为NULL）
 */
void core_set_local_info(const device_info_attr *info)
{
    if (info == NULL)
        return;
    pthread_mutex_lock(&g_call_ctx_mutex);
    call_global_ctx.local_info = *info;
    pthread_mutex_unlock(&g_call_ctx_mutex);
}

/**
 * @brief 获取自动接听开关
 * @return 0=关闭 1=开启
 */
uint8_t core_get_answer_auto(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    uint8_t val = call_global_ctx.answer_auto;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return val;
}

/**
 * @brief 设置自动接听开关
 * @param[in] auto_answer 开关状态
 */
void core_set_auto_answer(bool auto_answer)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    call_global_ctx.answer_auto = auto_answer;
    pthread_mutex_unlock(&g_call_ctx_mutex);
}

/**
 * @brief 获取系统运行状态
 * @return 0=未运行 1=运行中
 */
uint8_t core_get_run(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    uint8_t run = call_global_ctx.run;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return run;
}

/**
 * @brief 获取当前活跃通话数量
 * @return 活跃会话数
 */
uint8_t core_get_call_count(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    uint8_t count = call_global_ctx.call_nb;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return count;
}

/**
 * @brief 设置活跃通话数量
 * @param[in] call_count 数量
 */
void core_set_call_count(uint8_t call_count)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    call_global_ctx.call_nb = call_count;
    pthread_mutex_unlock(&g_call_ctx_mutex);
}

/**
 * @brief 获取会话池首地址
 * @return 会话池指针
 */
call_session_context *core_get_clls(void)
{
    return call_global_ctx.call_session_ctx;
}

/**
 * @brief 获取当前活跃通话会话
 * @return 当前通话上下文
 */
call_session_context *core_get_current_call(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    call_session_context *curren_call = call_global_ctx.currrent_call;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return curren_call;
}

/**
 * @brief 设置当前活跃通话
 * @param[in] ctx 通话上下文
 */
void core_set_current_call(call_session_context *ctx)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    call_global_ctx.currrent_call = ctx;
    pthread_mutex_unlock(&g_call_ctx_mutex);
}

/**
 * @brief 更新全局通话计数
 * @param[in] is_increase true=增加 false=减少
 */
static void update_global_call_count(bool is_increase)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    if (is_increase)
    {
        call_global_ctx.call_nb++;
    }
    else if (call_global_ctx.call_nb > 0)
    {
        call_global_ctx.call_nb--;
        if (call_global_ctx.call_nb == 0)
        {
            call_global_ctx.currrent_call = NULL;
        }
    }
    pthread_mutex_unlock(&g_call_ctx_mutex);
}

/**
 * @brief 初始化会话启动时间信息
 * @param[in,out] ctx 会话上下文
 */
static void init_session_start_info(call_session_context *ctx)
{
    if (ctx == NULL)
        return;

    ctx->start_time = common_timestamp_get();
    ctx->end_time = ctx->start_time;
    CALL_LOG_INFO(CALL_MODULE_CORE, "session_id=%llu init start info, start_time=%llu",
                  ctx->session_id, ctx->start_time);
}

/**
 * @brief 保存通话记录
 * @param[in] ctx 会话上下文
 * @param[in] reason 结束原因
 */
static void save_call_record(call_session_context *ctx, call_end_reason reason)
{
    if (ctx == NULL)
        return;

    uint64_t start_time = (ctx->start_connected_time != 0) ? ctx->start_connected_time : ctx->start_time;
    ctx->record_info_ctx.start_time = start_time;
    ctx->record_info_ctx.duration = (common_timestamp_get() - start_time) / 1000;

    if (reason == REASON_BUSY || reason == REASON_NO_RESPONSE)
    {
        ctx->record_info_ctx.duration = 0;
    }

    record_list_add(ctx->record_info_ctx);
    CALL_LOG_INFO(CALL_MODULE_CORE, "session_id=%llu save record, duration=%d",
                  ctx->session_id, ctx->record_info_ctx.duration);
}

/**
 * @brief 清理会话资源
 * @param[in,out] ctx 会话上下文
 */
static void clean_session_resources(call_session_context *ctx)
{
    if (ctx == NULL)
        return;

    memset(&ctx->online_info, 0, sizeof(ctx->online_info));
    for (int i = 0; i < DEVICE_MAX; i++)
    {
        ctx->online_info[i].status = DEVICE_STATUS_UNKNOWN;
    }

    ctx->call_flag = 0;
    ctx->session_id = 0;
    memset(&ctx->record_info_ctx, 0, sizeof(ctx->record_info_ctx));

    CALL_LOG_DEBUG(CALL_MODULE_CORE, "session_id=%llu resources cleaned", ctx->session_id);
}

/**
 * @brief 更新通话会话状态
 * @param[in,out] call_session_ctx 会话上下文
 * @param[in] new_state 新状态
 */
void update_current_call_state(call_session_context *call_session_ctx, call_state new_state)
{
    if (call_session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL session context");
        return;
    }

    call_state old_state = call_session_ctx->call_state;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "session_id=%llu, state change: %d → %d",
                   call_session_ctx->session_id, old_state, new_state);

    call_session_ctx->call_state = new_state;
    if (new_state == CALL_CONNECTED && old_state != CALL_CONNECTED)
    {
        call_session_ctx->start_connected_time = common_timestamp_get();
    }

    switch (new_state)
    {
    case CALL_INCOMING:
    case CALL_OUTGOING_PROCESS:
    case CALL_OUTGOING_RINGING:
    case CALL_CONNECTED:
        if (old_state == CALL_IDLE)
        {
            init_session_start_info(call_session_ctx);
            update_global_call_count(true);
            if (call_core_active_session_count() == 1)
                core_set_current_call(call_session_ctx);
            CALL_LOG_INFO(CALL_MODULE_CORE, "session_id=%llu started, active count=%d",
                          call_session_ctx->session_id, call_global_ctx.call_nb);
        }
        break;

    case CALL_END:
        if (old_state != CALL_IDLE)
        {
            update_global_call_count(false);
            save_call_record(call_session_ctx, new_state);
            clean_session_resources(call_session_ctx);
            call_session_ctx->call_state = CALL_IDLE;
            if (call_core_active_session_count() == 0)
                core_set_current_call(NULL);

            CALL_LOG_INFO(CALL_MODULE_CORE, "session reset to idle, active count=%d", call_global_ctx.call_nb);
        }
        break;

    case CALL_IDLE:
        clean_session_resources(call_session_ctx);
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "session forced reset to idle");
        break;

    default:
        CALL_LOG_ERROR(CALL_MODULE_CORE, "unknown state: %d", new_state);
        break;
    }
    call_state_cb cb = call_global_ctx.on_state_changed;
    call_state_cb_attr attr;
    attr.reason = call_session_ctx->reason;
    attr.device_info = call_session_ctx->device_info;
    attr.state = new_state;
    attr.prev_state = old_state;
    if (cb != NULL)
    {
        cb(&attr);
    }
}

/**
 * @brief 重置通话会话上下文
 * @param[in,out] session_ctx 会话上下文
 */
void call_session_contex_reset(call_session_context *session_ctx)
{
    if (session_ctx == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL session context");
        return;
    }

    session_ctx->session_id = 0;
    session_ctx->call_state = CALL_IDLE;
    session_ctx->call_flag = 0;
    memset(&session_ctx->device_info, 0, sizeof(session_ctx->device_info));
    memset(&session_ctx->record_info_ctx, 0, sizeof(session_ctx->record_info_ctx));
    session_ctx->start_time = 0;
    session_ctx->end_time = 0;
    session_ctx->start_connected_time = 0;
    session_ctx->last_interact_time = 0;

    for (int i = 0; i < DEVICE_MAX; i++)
    {
        session_ctx->online_info[i].status = DEVICE_STATUS_UNKNOWN;
        session_ctx->online_info[i].check_count = 0;
    }
}

/**
 * @brief 获取空闲会话
 * @return 空闲会话指针，NULL=无空闲
 */
call_session_context *new_call_session_context(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);

    if (call_global_ctx.call_nb >= call_global_ctx.max_sessions)
    {
        uint8_t cur_nb = call_global_ctx.call_nb;
        pthread_mutex_unlock(&g_call_ctx_mutex);
        CALL_LOG_ERROR(CALL_MODULE_CORE, "session pool full (max=%d, current=%d)",
                       call_global_ctx.max_sessions, cur_nb);
        return NULL;
    }

    call_session_context *found = NULL;
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        if (session_ctx->call_state == CALL_IDLE)
        {
            found = session_ctx;
            CALL_LOG_INFO(CALL_MODULE_CORE, "get free session success: pool index=%d, session_id=%llu, total_nb=%d",
                          i, session_ctx->session_id, call_global_ctx.call_nb);
            break;
        }
    }

    if (found == NULL)
    {
        uint8_t cur_nb = call_global_ctx.call_nb;
        pthread_mutex_unlock(&g_call_ctx_mutex);
        CALL_LOG_ERROR(CALL_MODULE_CORE, "no free session found (max=%d, current=%d)",
                       CALL_SESSION_MAX, cur_nb);
        return NULL;
    }

    pthread_mutex_unlock(&g_call_ctx_mutex);
    return found;
}

/**
 * @brief 根据设备信息获取活跃会话
 * @param[in] info 设备信息
 * @return 会话指针，NULL=未找到
 */
call_session_context *get_call_session(device_info_attr *info)
{
    if (info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return NULL;
    }

    pthread_mutex_lock(&g_call_ctx_mutex);
    call_session_context *found = NULL;

    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        if (session_ctx->call_state != CALL_IDLE && session_ctx->device_info.building == info->building && session_ctx->device_info.room == info->room)
        {
            if (session_ctx->device_info.device_type == info->device_type || session_ctx->call_flag & 0x1 << info->device_type)
            {
                found = session_ctx;
                break;
            }
        }
    }

    if (found != NULL)
    {
        pthread_mutex_unlock(&g_call_ctx_mutex);
        return found;
    }

    CALL_LOG_DEBUG(CALL_MODULE_CORE, "no active session for device (building=%d, room=%d, type=%d)",
                   info->building, info->room, info->device_type);
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return NULL;
}

/**
 * @brief 获取任意一个活跃会话
 * @return 会话指针，NULL=无活跃
 */
call_session_context *call_session_context_get(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        if (session_ctx->call_state != CALL_IDLE)
        {
            pthread_mutex_unlock(&g_call_ctx_mutex);
            return session_ctx;
        }
    }

    CALL_LOG_DEBUG(CALL_MODULE_CORE, "no active session exists");
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return NULL;
}
/**
 * @brief 初始化通话核心系统
 * @param[in] local_devie 本地设备信息
 * @return 0=成功，-1=失败
 */
int call_core_init(call_config_t *config)
{
    // 1. 入参检查
    if (config == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "invalid input param (config=%p)", config);
        return -1;
    }

    if (config->max_sessions == 0 || config->max_sessions > CALL_SESSION_MAX)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "invalid max_sessions=%d (must be 1~%d)",
                       config->max_sessions, CALL_SESSION_MAX);
        return -1;
    }

    // 2. 重复初始化检查
    if (call_global_ctx.run)
    {
        CALL_LOG_INFO(CALL_MODULE_CORE, "call system is already running");
        return 0;
    }

    // 所有初始化步骤（严格按启动顺序 + 回滚逆序）
    enum
    {
        INIT_STEP_NONE = 0,
        INIT_STEP_SESSION_POOL = 1 << 2, // 会话池
        INIT_STEP_LOCAL_INFO = 1 << 3,   // 本地设备信息
        INIT_STEP_RECORD = 1 << 4,       // 录音模块
        INIT_STEP_HEARTBEAT = 1 << 5,    // 心跳模块
        INIT_STEP_EVENT = 1 << 6,        // 事件模块
        INIT_STEP_ACTION = 1 << 7,       // 动作模块
    };

    uint32_t init_step = INIT_STEP_NONE;

    // -------------------------------------------------------------------------
    // 会话池初始化
    // -------------------------------------------------------------------------
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_contex_reset(&call_global_ctx.call_session_ctx[i]);
    }
    init_step |= INIT_STEP_SESSION_POOL;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "session pool initialized (max=%d)", CALL_SESSION_MAX);

    // -------------------------------------------------------------------------
    // 本地设备信息保存
    // -------------------------------------------------------------------------
    call_global_ctx.local_info = config->local_info;
    init_step |= INIT_STEP_LOCAL_INFO;
    CALL_LOG_DEBUG(CALL_MODULE_CORE,
                   "local device info initialized (building=%d, room=%d, type=%d)",
                   config->local_info.building, config->local_info.room, config->local_info.device_type);

    // -------------------------------------------------------------------------
    // 业务模块初始化
    // -------------------------------------------------------------------------
    if (!record_module_init())
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "init record module failed");
        goto INIT_FAILED;
    }
    init_step |= INIT_STEP_RECORD;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "record module initialized");

    if (!heartbeat_module_init())
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "init heartbeat module failed");
        goto INIT_FAILED;
    }
    init_step |= INIT_STEP_HEARTBEAT;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "heartbeat module initialized");

    if (!call_event_module_init())
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "init event module failed");
        goto INIT_FAILED;
    }
    init_step |= INIT_STEP_EVENT;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "event module initialized");

    if (!call_action_module_init())
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "init action module failed");
        goto INIT_FAILED;
    }
    init_step |= INIT_STEP_ACTION;
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "action module initialized");

    // -------------------------------------------------------------------------
    // 配置参数注册
    // -------------------------------------------------------------------------
    call_global_ctx.max_sessions = config->max_sessions;
    call_global_ctx.answer_auto = config->auto_answer;
    call_global_ctx.on_state_changed = config->on_state_changed;
    call_global_ctx.comm_interface = config->comm_interface;
    call_global_ctx.max_call_duration_ms = config->max_call_duration_ms != 0
                                               ? config->max_call_duration_ms
                                               : CALL_DEFAULT_MAX_DURATION_MS;
    call_global_ctx.no_interact_timeout_ms = config->no_interact_timeout_ms;

    CALL_LOG_DEBUG(CALL_MODULE_CORE,
                   "config registered (max_sessions=%d, auto_answer=%d, max_dur=%ums, no_interact=%ums)",
                   config->max_sessions, config->auto_answer,
                   call_global_ctx.max_call_duration_ms, call_global_ctx.no_interact_timeout_ms);

    if (config->record_enable)
    {
        if (!record_enable(true, config->record_path))
        {
            CALL_LOG_ERROR(CALL_MODULE_CORE, "enable record failed");
            goto INIT_FAILED;
        }
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "record enabled (path=%s)", config->record_path);
    }

    // -------------------------------------------------------------------------
    // 全部初始化成功
    // -------------------------------------------------------------------------
    call_global_ctx.run = true;
    CALL_LOG_INFO(CALL_MODULE_CORE, "call system initialized successfully");
    return 0;

    // -------------------------------------------------------------------------
    // 失败统一回滚（严格 逆序 释放）
    // -------------------------------------------------------------------------
INIT_FAILED:
    CALL_LOG_ERROR(CALL_MODULE_CORE, "init failed, rolling back all resources...");

    if (init_step & INIT_STEP_ACTION)
    {
        call_action_module_uninit();
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "action module uninitialized");
    }

    if (init_step & INIT_STEP_EVENT)
    {
        call_event_module_uninit();
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "event module uninitialized");
    }

    if (init_step & INIT_STEP_HEARTBEAT)
    {
        heartbeat_module_uninit();
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "heartbeat module uninitialized");
    }

    if (init_step & INIT_STEP_RECORD)
    {
        record_module_uninit();
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "record module uninitialized");
    }

    // 全局状态重置
    call_global_ctx.run = false;

    CALL_LOG_ERROR(CALL_MODULE_CORE, "rollback completed, init failed");
    return -1;
}

/**
 * @brief 反初始化通话核心系统
 * @return 0=成功
 */
int call_core_uninit(void)
{
    bool is_running = call_global_ctx.run;
    if (!is_running)
    {
        CALL_LOG_INFO(CALL_MODULE_CORE, "call system is already stopped");
        return 0;
    }

    CALL_LOG_INFO(CALL_MODULE_CORE, "start stopping call system");
    call_global_ctx.run = false;

    call_action_module_uninit();
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "action module uninitialized");

    call_event_module_uninit();
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "event module uninitialized");

    heartbeat_module_uninit();
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "heartbeat module uninitialized");

    record_module_uninit();
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "record module uninitialized");

    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        call_session_contex_reset(session_ctx);
    }
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "session pool cleaned up");

    memset(&call_global_ctx, 0, sizeof(call_context));
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "global context reset to zero");

    CALL_LOG_INFO(CALL_MODULE_CORE, "call system stopped successfully");
    return 0;
}

/**
 * @brief 获取当前活跃会话数
 * @return 活跃数量
 */
uint8_t call_core_active_session_count(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);
    uint8_t count = call_global_ctx.call_nb;
    pthread_mutex_unlock(&g_call_ctx_mutex);
    CALL_LOG_DEBUG(CALL_MODULE_CORE, "current active session count: %d", count);
    return count;
}

/**
 * @brief 获取当前通话对端设备信息
 * @param[out] device_info 设备信息输出
 * @return true=成功
 */
bool core_get_current_call_device(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "failed: NULL param");
        return false;
    }

    pthread_mutex_lock(&g_call_ctx_mutex);
    call_session_context *session_ctx = call_global_ctx.currrent_call;
    if (session_ctx == NULL)
    {
        pthread_mutex_unlock(&g_call_ctx_mutex);
        return false;
    }

    memcpy(device_info, &session_ctx->device_info, sizeof(device_info_attr));
    pthread_mutex_unlock(&g_call_ctx_mutex);
    return true;
}

/**
 * @brief 获取所有待处理（呼入中）通话列表（按呼入时间升序）
 * @param[out] public_info_buf 设备信息输出缓冲区
 * @param[in] max_count 缓冲区最大数量
 * @param[out] actual_count 实际获取数量
 * @return true=成功
 */
bool core_get_all_pending_calls(device_info_attr *public_info_buf, int max_count, int *actual_count)
{
    if (public_info_buf == NULL || actual_count == NULL || max_count <= 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "invalid param (buf=%p, max=%d, actual=%p)",
                       public_info_buf, max_count, actual_count);
        return false;
    }
    pthread_mutex_lock(&g_call_ctx_mutex);
    typedef struct
    {
        int session_idx;
        time_t start_time;
    } pending_session_item_t;

    // 完整初始化
    pending_session_item_t pending_items[CALL_SESSION_MAX];
    memset(pending_items, 0, sizeof(pending_items));
    int pending_item_count = 0;
    int count = 0;
    device_info_attr *buf_ptr = public_info_buf;

    // 第一步：收集所有待处理通话
    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        if (session_ctx != call_global_ctx.currrent_call &&
            session_ctx->call_state == CALL_INCOMING &&
            session_ctx->session_id != 0)
        {
            pending_items[pending_item_count].session_idx = i;
            pending_items[pending_item_count].start_time = session_ctx->start_time;
            pending_item_count++;
            if (pending_item_count >= CALL_SESSION_MAX)
            {
                CALL_LOG_WARN(CALL_MODULE_CORE, "reach max session count(%d), stop collect",
                              CALL_SESSION_MAX);
                break;
            }
        }
    }
    // 第二步：冒泡排序（按开始时间升序，修复删除无效判断）
    for (int i = 0; i < pending_item_count - 1; i++)
    {
        for (int j = 0; j < pending_item_count - i - 1; j++)
        {
            if (pending_items[j].start_time > pending_items[j + 1].start_time)
            {
                pending_session_item_t temp = pending_items[j];
                pending_items[j] = pending_items[j + 1];
                pending_items[j + 1] = temp;
            }
        }
    }

    // 第三步：填充输出缓冲区（不再重复校验，锁内数据不变）
    for (int i = 0; i < pending_item_count; i++)
    {
        int session_idx = pending_items[i].session_idx;
        if (session_idx < 0 || session_idx >= CALL_SESSION_MAX)
            continue;
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[session_idx];
        memcpy(buf_ptr, &session_ctx->device_info, sizeof(device_info_attr));
        buf_ptr++;
        count++;
        if (count >= max_count)
        {
            CALL_LOG_WARN(CALL_MODULE_CORE, "buffer full (max=%d, actual=%d)",
                          max_count, count);
            break;
        }
    }

    *actual_count = count;

    if (count == 0)
    {
        CALL_LOG_INFO(CALL_MODULE_CORE, "no pending calls (max_support=%d)", CALL_SESSION_MAX);
    }
    else
    {
        CALL_LOG_DEBUG(CALL_MODULE_CORE, "get %d pending calls (sorted by start_time asc)", count);
    }

    pthread_mutex_unlock(&g_call_ctx_mutex);
    return true;
}

/**
 * @brief 切换当前通话
 * @param[in] device_info 目标设备信息
 * @return true=成功
 */
void core_switch_call(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_SWITCH_CALL;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 主动接听指定设备
 * @param[in] device_info 目标设备信息
 */
void core_call_answer(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_ACCEPT;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 主动接听当前活跃会话
 * @return 成功/失败
 */
bool core_call_answer_current(void)
{
    device_info_attr current_device_info = {0};
    if (core_get_current_call_device(&current_device_info) == false)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "get current call device info failed");
        return false;
    }

    call_cb_attr attr = {0};
    attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_ACCEPT;
    attr.device_info = current_device_info;
    call_call_event_put(&attr);

    return true;
}

/**
 * @brief 挂断当前活跃会话
 * @return 成功/失败
 */
bool core_call_hangup_current(void)
{
    device_info_attr current_device_info = {0};

    if (core_get_current_call_device(&current_device_info) == false)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "get current call device info failed");
        return false;
    }

    call_cb_attr attr = {0};
    attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_END;
    attr.device_info = current_device_info;
    call_call_event_put(&attr);
    return true;
}

/**
 * @brief 挂断指定设备会话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
bool core_call_hangup_specified(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return false;
    }

    call_cb_attr attr = {0};
    attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_END;
    attr.device_info = *device_info;
    call_call_event_put(&attr);

    return true;
}

/**
 * @brief 挂断所有活跃会话
 * @return 成功/失败
 */
bool core_call_hangup_all(void)
{
    pthread_mutex_lock(&g_call_ctx_mutex);

    for (int i = 0; i < CALL_SESSION_MAX; i++)
    {
        call_session_context *session_ctx = &call_global_ctx.call_session_ctx[i];
        if (session_ctx->call_state == CALL_IDLE)
            continue;

        call_cb_attr attr = {0};
        attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_END;
        attr.device_info = session_ctx->device_info;
        call_call_event_put(&attr);

        CALL_LOG_INFO(CALL_MODULE_CORE, "hangup all: enqueue session[%d]\r\n", i);
    }

    pthread_mutex_unlock(&g_call_ctx_mutex);
    return true;
}

/**
 * @brief 主动拨号
 * @param[in] device_info 目标设备
 */
void core_call_dial(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_ACTIVE_INVITE;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收外部呼叫邀请
 * @param[in] device_info 呼叫方设备
 */
void core_call_receive_invite(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_INVITE;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收振铃通知
 * @param[in] device_info 设备信息
 */
void core_call_receive_ringing(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_RINGING;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收对方接听
 * @param[in] device_info 对方设备
 */
void core_call_receive_accept(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_ACCEPT;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收对方占线
 * @param[in] device_info 对方设备
 */
void core_call_receive_busy(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, " NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_BUSY;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收会话结束
 * @param[in] device_info 对方设备
 */
void core_call_receive_end(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_END;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收心跳查询
 * @param[in] device_info 发起方设备
 */
void core_call_receive_heartbeat_request(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_REQUEST;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}

/**
 * @brief 接收心跳回复
 * @param[in] device_info 回复方设备
 */
void core_call_receive_heartbeat_response(device_info_attr *device_info)
{
    if (device_info == NULL)
    {
        CALL_LOG_ERROR(CALL_MODULE_CORE, "NULL device info");
        return;
    }

    call_cb_attr attr;
    attr.call_event = CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_RESPONSE;
    attr.device_info = *device_info;
    call_call_event_put(&attr);
}
