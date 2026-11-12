#ifndef CALL_COMM_INTERFACE_INTERNAL_H
#define CALL_COMM_INTERFACE_INTERNAL_H

#include "call_comm_interface.h"

/** 通信接口实例（由 call_comm_init 填充，通过 call_config_t 传入 sat_call_init） */
extern call_comm_interface_t g_call_comm_interface;

/**
 * @brief 通信接口初始化（填充 g_call_comm_interface 函数指针表）
 * @return true 永远成功
 */
bool call_comm_init(void);

void call_comm_uninit(void);

#endif // CALL_COMM_INTERFACE_INTERNAL_H
