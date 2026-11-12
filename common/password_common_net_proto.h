/**
 * @file    password_common_net_proto.h
 * @brief   密码网络协议定义 —— 门口机与室内机通信数据结构
 *
 * 与 card_common_net_proto.h 模式一致，独立定义避免业务层依赖。
 * 本文件在室内机 app 与门口机 app 各存一份，两份必须字节级一致以保证二进制兼容。
 */

#ifndef _PASSWORD_COMMON_NET_PROTO_H_
#define _PASSWORD_COMMON_NET_PROTO_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * 密码管理模式开关：
 *   1 = 启用室内机与门口机之间的"进入/退出密码管理模式"握手，
 *       增/删/改权限需先进入管理模式；
 *   0 = 关闭握手，密码增删改查直接下发，不校验管理模式。
 * 门口机与室内机侧使用同名宏，两边需保持一致。
 */
#ifndef PASSWORD_ADMIN_MODE_ENABLE
#define PASSWORD_ADMIN_MODE_ENABLE 0
#endif

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
 * 密码操作类型（网络指令对应 password_net_op_e）
 * ===================================================================== */

typedef enum
{
    COMM_PASSWORD_OP_NONE          = 0x00, /**< 无效操作 */
    COMM_PASSWORD_OP_ADD           = 0x01, /**< 添加密码 */
    COMM_PASSWORD_OP_DEL           = 0x02, /**< 删除密码（按索引） */
    COMM_PASSWORD_OP_QUERY         = 0x03, /**< 查询所有密码信息 */
    COMM_PASSWORD_OP_SET_PERM      = 0x04, /**< 设置密码权限 */
    COMM_PASSWORD_OP_ENTER_MANAGE  = 0x05, /**< 进入管理模式 */
    COMM_PASSWORD_OP_EXIT_MANAGE   = 0x06, /**< 退出管理模式 */
} comm_password_op_e;

/* ========================================================================
 * 密码权限（网络传输用，位掩码）
 * ===================================================================== */

typedef enum
{
    COMM_PASSWORD_PERM_NONE   = 0x00, /**< 无权限（空闲槽位） */
    COMM_PASSWORD_PERM_LOCK_1 = 0x01, /**< 开锁1权限（门锁） */
    COMM_PASSWORD_PERM_LOCK_2 = 0x02, /**< 开锁2权限（闸机） */
    COMM_PASSWORD_PERM_ALL    = 0x03, /**< 全部权限 */
} comm_password_perm_e;

/* ========================================================================
 * 响应码
 * ===================================================================== */

typedef enum
{
    COMM_PASSWORD_RES_SUCCESS       = 0x00, /**< 成功 */
    COMM_PASSWORD_RES_FAILED        = 0x01, /**< 通用失败 */
    COMM_PASSWORD_RES_INVALID_PARAM = 0x02, /**< 参数无效 */
    COMM_PASSWORD_RES_NOT_FOUND     = 0x03, /**< 密码不存在 */
    COMM_PASSWORD_RES_FULL          = 0x04, /**< 密码库已满 */
    COMM_PASSWORD_RES_SYSTEM_ERR    = 0x05, /**< 系统内部错误 */
} comm_password_res_e;

/* ========================================================================
 * 推送类型（门口机主动推送事件）
 * ===================================================================== */

typedef enum
{
    COMM_PASSWORD_PUSH_ADD      = 0x01, /**< 添加结果 */
    COMM_PASSWORD_PUSH_DEL      = 0x02, /**< 删除结果 */
    COMM_PASSWORD_PUSH_QUERY    = 0x03, /**< 查询结果 */
    COMM_PASSWORD_PUSH_SET_PERM = 0x04, /**< 权限设置结果 */
} comm_password_push_e;

/* ========================================================================
 * 单条密码网络信息
 * ===================================================================== */

#define NET_PASSWORD_CODE_LEN 4  /**< 密码存储长度（字节） */
#define NET_MAX_PASSWORD_NUM  10 /**< 单次查询最大返回密码数 */

typedef struct
{
    uint16_t index;                          /**< 密码槽位索引 (0~9) */
    comm_password_perm_e perm;               /**< 权限位掩码 */
    uint8_t password[NET_PASSWORD_CODE_LEN]; /**< 密码数据 */
    common_host_info_t host_id;              /**< 宿主房号信息：决定推送路由与查询归属 */
    bool is_valid;                           /**< 是否有效 */
} comm_password_info_t;

/* ========================================================================
 * 密码操作请求/响应结构体
 *
 * 字段顺序与命名同 comm_card_op_res_t：op_type / res_code / host_id / count / items[]
 * 使用扁平结构体（非联合体），base64编码后传输
 * ===================================================================== */

typedef struct
{
    comm_password_push_e op_type;                          /**< 推送类型 */
    comm_password_res_e  res_code;                         /**< 响应码 */
    common_host_info_t   host_id;                          /**< 关联宿主（所有操作必填） */
    uint16_t             password_count;                   /**< 密码数量 */
    comm_password_info_t passwords[NET_MAX_PASSWORD_NUM];  /**< 密码列表 */
} comm_password_op_res_t;

#endif /* _PASSWORD_COMMON_NET_PROTO_H_ */
