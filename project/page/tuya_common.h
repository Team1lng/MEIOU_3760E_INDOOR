#ifndef _TUYA_COMMON_H_
#define _TUYA_COMMON_H_
#include "Amy_lvgl.h"
#include <stdbool.h>

typedef void (*tuya_event_handler)(int cb_attr);

bool tuya_sdk_start_from_user_data(void);

// 初始化涂鸦模块（注册总事件+细分处理器）
void tuya_event_module_init(void);

// 复位涂鸦事件默认处理
void tuya_event_callback_default(void);

void atmosphere_ctrl(bool en);

// 注册涂鸦细分事件回调
void tuya_event_handler_register(int sub_event_id, tuya_event_handler handler);
// 上传有效通道到云端
void report_valid_channel_to_tuya(void);

// 开锁生命周期（物理动作 + DP上报true → 定时器 → DP上报false）
// DP上报定时器为全局LVGL定时器，页面切换不会影响关锁状态上报
void tuya_ungate2_with_dp(void);
void tuya_unlock_lock1_with_dp(int channel);
void tuya_unlock_lock2_with_dp(int channel);

#endif