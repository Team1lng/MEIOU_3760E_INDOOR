#include "user_standby.h"
#include "stddef.h"
/**
 * @brief 待机模块全部状态（单文件静态实例）
 *
 * timeout：超时秒数；timer_enable：是否参与计时；on_jump / jump_user_data：超时跳转回调；
 * timeout_timestamp：周期检测中递增的内部计数。
 */
static struct
{
  int timeout;
  bool timer_enable;
  standby_jump_callback_t on_jump;
  void *jump_user_data;
  unsigned long long timeout_timestamp;
} s_standby = {
    .timeout = 30,
    .timer_enable = false,
    .on_jump = NULL,
    .jump_user_data = NULL,
    .timeout_timestamp = 0,
};

/**
 * @brief 初始化待机检测
 * @param jump_cb 超时触发时调用的跳转回调；为 NULL 时不改写已注册的回调
 * @param user_data 传给 jump_cb 的上下文，可为 NULL
 * @param timeout 超时时间（秒）
 * @return 恒为 true
 */
bool standby_timer_init(standby_jump_callback_t jump_cb, void *user_data, int timeout)
{
  if (jump_cb != NULL)
  {
    s_standby.on_jump = jump_cb;
    s_standby.jump_user_data = user_data;
  }
  s_standby.timeout = timeout;
  s_standby.timer_enable = false;
  return true;
}

/**
 * @brief 重启或刷新待机计时
 * @param fouce_enable 为 true 时强制打开待机检测
 * @param refresh 为 true 时清零内部超时计数
 * @return 恒为 true
 */
bool standby_timer_restart(bool fouce_enable, bool refresh)
{
  if (fouce_enable)
    s_standby.timer_enable = true;
  if (refresh)
    s_standby.timeout_timestamp = 0;
  return true;
}

/**
 * @brief 重置待机超时时间
 * @param timeout 新的超时时间（秒）
 * @return 恒为 true
 */
bool standby_timer_reset(int timeout)
{
  s_standby.timeout = timeout;
  s_standby.timeout_timestamp = 0;
  return true;
}

/**
 * @brief 关闭待机检测
 * @return 当前未使能返回 false，关闭成功返回 true
 */
bool standby_timer_close(void)
{
  if (s_standby.timer_enable == false)
  {
    return false;
  }

  s_standby.timer_enable = false;
  return true;
}

/**
 * @brief 获取待机检测是否已使能
 * @return 已使能返回 true
 */
bool standby_timer_status_get(void)
{
  return s_standby.timer_enable;
}

/**
 * @brief 检测待机超时，超时则调用上层注册的跳转回调
 * @return 已调用回调返回 true，否则返回 false
 */
bool standby_timeout_check_and_process(void)
{
  if (s_standby.timer_enable == false)
  {
    return false;
  }
  s_standby.timeout_timestamp++;

  if (s_standby.timeout_timestamp > s_standby.timeout)
  {
    s_standby.timeout_timestamp = 0;
    if (s_standby.on_jump != NULL)
    {
      s_standby.on_jump(s_standby.jump_user_data);
      return true;
    }
  }
  return false;
}
