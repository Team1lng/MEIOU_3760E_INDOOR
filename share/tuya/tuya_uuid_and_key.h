#ifndef __TUYA_UUID_AND_KEY_H__
#define __TUYA_UUID_AND_KEY_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 涂鸦UUID/KEY初始化入口函数
 * @return true-已存在有效配置或SD卡有可用XLS文件（可正常使用），false-无有效配置且无可用XLS文件（需注册）
 */
bool tuya_key_and_uuid_init(void);

/**
 * @brief 从SD卡XLS文件注册指定索引的涂鸦UUID和KEY（生成本地配置文件）
 * @param index 要注册的XLS行索引（从1开始，对应表头下的行）
 * @return true-注册成功，false-注册失败（已注册/文件打开失败/格式错误等）
 */
bool tuya_key_and_key_xls_register(int index);

/**
 * @brief 从本地配置文件读取已注册的涂鸦UUID和KEY
 * @param uuid 存储读取到的UUID的缓冲区（需提前分配足够空间，建议至少36字节）
 * @param key 存储读取到的KEY的缓冲区（需提前分配足够空间，建议至少16字节）
 * @return true-读取成功，false-未注册/缓冲区为空/文件读取失败
 */
bool tuya_uuid_and_key_read(unsigned char *uuid, unsigned char *key);

/**
 * @brief 获取已注册的涂鸦序列号（配置文件名前缀，不含.conf后缀）
 * @param serial 存储序列号的缓冲区（需提前分配足够空间，建议至少20字节）
 * @param serial_len 缓冲区最大长度（需大于0）
 * @return true-获取成功，false-未注册/缓冲区无效/解析失败
 */
bool tuya_serial_number_get(char *serial, int serial_len);

#endif  // __TUYA_UUID_AND_KEY_H__