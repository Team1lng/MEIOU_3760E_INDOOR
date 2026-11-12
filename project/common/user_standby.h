#ifndef _USER_STANDBY_H_
#define _USER_STANDBY_H_

#include <stdbool.h>

/**
 * @brief 待机超时后由上层实现的页面跳转（勿长时间阻塞）
 * @param user_data 初始化时传入的上下文指针，可为 NULL
 */
typedef void (*standby_jump_callback_t)(void *user_data);

/**
 * @brief 初始化待机检测
 * @param jump_cb 超时触发时调用的跳转回调；为 NULL 时不改写已注册的回调
 * @param user_data 传给 jump_cb 的上下文，可为 NULL
 * @param timeout 超时时间（秒）
 * @return 恒为 true
 */
bool standby_timer_init(standby_jump_callback_t jump_cb, void *user_data, int timeout);

/**
 * @brief 重启或刷新待机计时
 * @param fouce_enable 为 true 时强制打开待机检测
 * @param refresh 为 true 时清零内部超时计数
 * @return 恒为 true
 */
bool standby_timer_restart(bool fouce_enable, bool refresh);

/**
 * @brief 关闭待机检测
 * @return 当前未使能返回 false，关闭成功返回 true
 */
bool standby_timer_close(void);

/**
 * @brief 获取待机检测是否已使能
 * @return 已使能返回 true
 */
bool standby_timer_status_get(void);

/**
 * @brief 重置待机超时时间
 * @param timeout 新的超时时间（秒）
 * @return 恒为 true
 */
bool standby_timer_reset(int timeout);

/**
 * @brief 检测待机超时，超时则调用上层注册的跳转回调
 * @return 已调用回调返回 true，否则返回 false
 */
bool standby_timeout_check_and_process(void);
#endif
