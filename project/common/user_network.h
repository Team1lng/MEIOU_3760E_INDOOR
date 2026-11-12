#ifndef _USER_NETWORK_H_
#define _USER_NETWORK_H_
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief 室内机网络模块初始化
 */
void user_network_init(void);
void user_network_pairing_init(void);

/**
 * @brief 根据当前设备身份重新应用静态 IP
 */
void user_network_apply_device_static_ip(void);

/**
 * @brief 立即重新应用静态 IP（不入队、直接执行），
 *        供设备号/房号变更后无需重启设备时调用
 */
void user_network_apply_device_static_ip_now(void);

#endif