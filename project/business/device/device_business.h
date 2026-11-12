/**
 * @file        sat_device_business.h
 * @brief       设备业务层接口（冲突检测 + 配置同步）
 * @details     提供设备冲突检测、配置同步、配置变更通知等对外接口
 * @author      系统软件组
 * @date        2025-xx-xx
 * @copyright   Copyright (c) 2025
 */

#ifndef SAT_DEVICE_BUSINESS_H
#define SAT_DEVICE_BUSINESS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 用户配置变更通知
 * @details 仅主机生效，向同房组播组推送公共配置供分机接收
 */
void device_notify_config_changed(void);

/*==========================================================================
 * 设备编号冲突（同栋+同房+同分机）：组播探测
 *==========================================================================*/

/**
 * @brief 设备冲突检测（支持同步/异步）
 *
 * @param[in]  building      楼栋号
 * @param[in]  room          房间号
 * @param[in]  device_type   设备类型
 * @param[in]  timeout_ms    检测超时时间（毫秒）
 * @param[in]  is_block      true：同步阻塞；false：异步非阻塞（线程执行）
 * @param[out] conflict_out  阻塞模式下输出是否检测到冲突；非阻塞模式传 NULL
 *
 * @retval true  启动/执行成功
 * @retval false 启动/执行失败
 *
 * @note 异步模式下，若检测到冲突，框架自动通过 LVGL 异步事件通知 UI
 */
bool device_conflict_check(int building, int room, int device_type, int timeout_ms,
                           bool is_block, bool *conflict_out);

/**
 * @brief 启动设备冲突常驻监听线程
 * @details 后台持续监听同栋、同房、同类型设备的组播包，实时检测冲突
 */
void device_conflict_listener_start(void);

/**
 * @brief 启动设备配置同步服务
 * @details 启动监听线程 + 分机定时拉取配置
 */
void device_config_sync_start(void);

#endif // SAT_DEVICE_BUSINESS_H