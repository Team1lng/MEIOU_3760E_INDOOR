/**
 * @file        device_types.h
 * @brief       设备类型与设备信息结构体定义
 * @ingroup     device_types
 */

#ifndef __DEVICE_TYPES_H__
#define __DEVICE_TYPES_H__

/**
 * @brief 设备信息属性结构体
 */
typedef struct
{
  int device_type;
  int building;
  int room;
} device_info_attr;

/**
 * @brief 设备类型枚举
 */
typedef enum
{
  // ==============================
  // 全局最小 = 第一个真实设备
  // ==============================
  DEVICE_TYPE_MIN = 0x0001,

  // 室内设备区间 0x0001 ~ 0x0009
  DEVICE_TYPE_INDOOR_MIN = DEVICE_TYPE_MIN,
  DEVICE_TYPE_INDOOR_ID1 = DEVICE_TYPE_INDOOR_MIN,
  DEVICE_TYPE_INDOOR_ID2 = 0x0002,
  DEVICE_TYPE_INDOOR_ID3 = 0x0003,
  DEVICE_TYPE_INDOOR_ID4 = 0x0004,
  DEVICE_TYPE_INDOOR_ID5 = 0x0005,
  DEVICE_TYPE_INDOOR_ID6 = 0x0006,
  DEVICE_TYPE_INDOOR_ID7 = 0x0007,
  DEVICE_TYPE_INDOOR_ID8 = 0x0008,
  DEVICE_TYPE_INDOOR_MAX = DEVICE_TYPE_INDOOR_ID8,

  // 门控设备区间 0x0009 ~ 0x000a
  DEVICE_TYPE_DOOR_MIN = 0x0009,
  DEVICE_TYPE_DOOR_ID1 = DEVICE_TYPE_DOOR_MIN,
  DEVICE_TYPE_DOOR_ID2 = 0x000a,
  DEVICE_TYPE_DOOR_MAX = DEVICE_TYPE_DOOR_ID2,

  // ==============================
  // 全局最大 = 最后一个真实设备
  // ==============================
  DEVICE_TYPE_MAX = DEVICE_TYPE_DOOR_MAX,

  // ==============================
  // 以下是掩码/家族，不属于真实设备
  // ==============================
  DEVICE_TYPE_INDOOR_FAMILY = 0x0FFF,
  DEVICE_TYPE_ALL = 0xFFFF
} device_type;
#endif // __DEVICE_TYPES_H__