/**
 * @file        call_record_internal.h
 * @brief       通话记录模块内部接口
 * @details     仅 core 内部调用，不对外暴露
 * @ingroup     call_record
 */

#ifndef __CALL_RECORD_INTERNAL_H__
#define __CALL_RECORD_INTERNAL_H__

#include "call_record.h"

/**
 * @brief 配置通话记录使能与存储路径
 * @param[in] enable 使能开关
 * @param[in] record_path 存储路径
 * @return 成功/失败
 */
bool record_enable(bool enable, const char *record_path);

/**
 * @brief 初始化通话记录模块
 * @return 成功/失败
 */
bool record_module_init(void);

/**
 * @brief 添加一条通话记录
 * @param[in] record_info 记录信息
 * @return 成功/失败
 */
bool record_list_add(call_record_info_attr record_info);

/**
 * @brief 反初始化通话记录模块
 * @return 成功/失败
 */
bool record_module_uninit(void);

#endif // __CALL_RECORD_INTERNAL_H__