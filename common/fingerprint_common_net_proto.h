/**
 * @file    fingerprint_common_net_proto.h
 * @brief   指纹网络协议定义 -- 门口机与室内机通信数据结构
 *
 * 与 card_common_net_proto.h / password_common_net_proto.h 模式一致。
 * 本文件在室内机 app 与门口机 app 各存一份，两份必须字节级一致以保证二进制兼容。
 */

#ifndef _FINGERPRINT_COMMON_NET_PROTO_H_
#define _FINGERPRINT_COMMON_NET_PROTO_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * 宿主房号信息
 *
 * card / password / fingerprint 三个协议头文件共用同一份定义，用 guard 宏防重复，
 * 保证任意包含顺序与组合下都不会重定义。
 * 语义同业务层 device_attr_info（building / room / device_type 三要素），
 * 但独立定义、不依赖业务层。
 * ===================================================================== */

#ifndef COMMON_HOST_INFO_T_DEFINED
#define COMMON_HOST_INFO_T_DEFINED
typedef struct
{
    int32_t building;
    int32_t room;
    int32_t device_type;
} common_host_info_t;
#endif

/* ========================================================================
 * 户号限制开关：
 *   1 = 按户号（building/room/device_type）限制增/删/改/查，只能操作本户号数据（默认）；
 *   0 = 不限户号，所有数据都可操作：查询返回全部；按户号清空即清空整库；
 *       按索引删/改权限跳过归属校验。host_id 仍必填（用于落库归属与推送路由）。
 * card / password / fingerprint 三个协议头文件共用同名宏，用 guard 宏防重复，
 * 门口机与室内机侧使用同名宏，两边需保持一致。
 * ===================================================================== */

#ifndef HOST_ID_LIMIT_ENABLE
#define HOST_ID_LIMIT_ENABLE 1
#endif

/* ========================================================================
 * 指纹操作类型
 * ===================================================================== */

typedef enum
{
    COMM_FINGER_OP_NONE           = 0x00,
    COMM_FINGER_OP_ENTER_MANAGER  = 0x01,
    COMM_FINGER_OP_EXIT_MANAGER   = 0x02,
    COMM_FINGER_OP_ADD            = 0x03,
    COMM_FINGER_OP_DEL            = 0x04,
    COMM_FINGER_OP_QUERY          = 0x05,
    COMM_FINGER_OP_CLEAR          = 0x06,
    COMM_FINGER_OP_SET_PERM       = 0x07,
} comm_finger_op_e;

/* ========================================================================
 * 指纹权限（位掩码）
 * ===================================================================== */

typedef enum
{
    COMM_FINGER_PERM_NONE = 0x00,
    COMM_FINGER_PERM_LOCK = 0x01,
    COMM_FINGER_PERM_GATE = 0x02,
    COMM_FINGER_PERM_ALL  = 0x03,
} comm_finger_perm_e;

/* ========================================================================
 * 响应码
 * ===================================================================== */

typedef enum
{
    COMM_FINGER_RES_SUCCESS        = 0x00,
    COMM_FINGER_RES_FAILED         = 0x01,
    COMM_FINGER_RES_INVALID_PARAM  = 0x02,
    COMM_FINGER_RES_SENSOR_ERR     = 0x03,
    COMM_FINGER_RES_FULL           = 0x04,
    COMM_FINGER_RES_DUPLICATE      = 0x05,
    COMM_FINGER_RES_NOT_FOUND      = 0x06,
    COMM_FINGER_RES_TIMEOUT        = 0x07,
} comm_finger_res_e;

/* ========================================================================
 * 推送类型（门口机主动推送事件）
 * ===================================================================== */

typedef enum
{
    COMM_FINGER_PUSH_SCAN     = 0x01,
    COMM_FINGER_PUSH_ADD      = 0x02,
    COMM_FINGER_PUSH_DEL      = 0x03,
    COMM_FINGER_PUSH_QUERY    = 0x04,
    COMM_FINGER_PUSH_CLEAR    = 0x05,
    COMM_FINGER_PUSH_SET_PERM = 0x06,
} comm_finger_push_e;

/* ========================================================================
 * 单条指纹网络信息
 * ===================================================================== */

#define NET_FINGER_DATA_LEN 5
#define NET_MAX_FINGER_NUM  100

typedef struct
{
    uint16_t index;
    comm_finger_perm_e perm;
    uint8_t data[NET_FINGER_DATA_LEN];
    common_host_info_t host_id; /* 宿主房号信息：决定推送路由与查询归属 */
    bool is_valid;
} comm_finger_info_t;

/* ========================================================================
 * 指纹操作请求/响应结构体
 *
 * 字段顺序与命名同 comm_card_op_res_t：op_type / res_code / host_id / count / items[]
 * 使用扁平结构体（非联合体），base64编码后传输。
 * 请求和响应共用同一结构体，字段按 op_type 选择性填充。
 * ===================================================================== */

typedef struct
{
    comm_finger_push_e op_type;
    comm_finger_res_e  res_code;
    common_host_info_t host_id;
    uint16_t           finger_count;
    comm_finger_info_t fingers[NET_MAX_FINGER_NUM];
} comm_finger_op_res_t;

#endif /* _FINGERPRINT_COMMON_NET_PROTO_H_ */
