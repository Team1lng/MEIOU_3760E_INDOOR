/**
 * @file        call_action_internal.h
 * @brief       通话动作模块内部接口
 * @details     仅 core 内部调用，事件动作处理模块
 * @ingroup     call_action
 */

#ifndef __CALL_ACTION_INTERNAL_H__
#define __CALL_ACTION_INTERNAL_H__

#include "call_record.h"

/**
 * @brief 初始化通话动作模块
 * @return 成功/失败
 */
bool call_action_module_init(void);

/**
 * @brief 反初始化通话动作模块
 * @return 成功/失败
 */
bool call_action_module_uninit(void);

#endif // __CALL_ACTION_INTERNAL_H__