#ifndef _CARD_COMMON_NET_PROTO_H_
#define _CARD_COMMON_NET_PROTO_H_
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// -------------------------- 基础配置宏（复用+补充） --------------------------
#define NET_CARD_ID_LEN 4    // 卡片ID长度（字节，与业务层保持一致但独立定义）
#define NET_MAX_CARD_NUM 500 // 单次查询最大返回卡片数（避免数据量过大）

// -------------------------- 功能操作枚举（保留原有） --------------------------
typedef enum
{
  COMM_CARD_OP_NONE = 0x00,             // 无效操作
  COMM_CARD_OP_ADD = 0x01,              // 添加卡片
  COMM_CARD_OP_DEL = 0x02,              // 删除卡片
  COMM_CARD_OP_QUERY_COUNT = 0x03,      // 查询白名单数量（轻量查询）
  COMM_CARD_OP_QUERY_ALL = 0x07,        // 查询所有卡片详情（全量查询）
  COMM_CARD_OP_CLEAR = 0x04,            // 清空白名单（需二次确认）
  COMM_CARD_OP_EXIT_MANAGER = 0x05,     // 退出管理员模式
  COMM_CARD_OP_MODIFY_DOOR_MASK = 0x06, // 批量修改卡片开门属性
  COMM_CARD_OP_ENTER_MANAGER = 0x08     // 进入管理员模式（仅含 host_id，触发扫描等待）
} comm_card_op_param_e;

// -------------------------- 响应结果枚举 --------------------------
// 室内机可通过错误码快速定位问题，无需解析错误描述
typedef enum
{
  COMM_RES_SUCCESS = 0x00,         // 执行成功
  COMM_RES_FAILED = 0x01,          // 通用失败
  COMM_RES_INVALID_OP = 0x02,      // 无效操作类型（如未知的comm_card_op_param_e）
  COMM_RES_INVALID_PARAM = 0x03,   // 无效参数（如宿主房号信息为空、卡片ID长度错误）
  COMM_RES_NO_PERMISSION = 0x04,   // 无权限（如非本宿主查询其他住户卡片）
  COMM_RES_CARD_NOT_FOUND = 0x05,  // 卡片不存在（删除/查询单张卡片时）
  COMM_RES_WHITELIST_EMPTY = 0x06, // 白名单为空（查询时）
  COMM_RES_DATA_TOO_MUCH = 0x07,   // 卡片数量超出单次返回上限
  COMM_RES_SYSTEM_ERR = 0x08       // 系统内部错误（如白名单读取失败）
} comm_res_code_e;

// -------------------------- 卡片类型枚举 --------------------------
// 与业务层语义一致，但独立枚举（避免依赖业务层）
typedef enum
{
  COMM_CARD_PERM_NORMAL = 0x01,  // 普通权限卡（住户日常使用）
  COMM_CARD_PERM_MANAGER = 0x02, // 管理员权限卡（用于配置操作）
  COMM_CARD_PERM_TEMP = 0x03     // 临时权限卡（可选：如访客卡，可扩展过期时间）
} comm_card_perm_e;

// -------------------------- 卡片开锁类型枚举 --------------------------
typedef enum
{
  COMM_CARD_LOCK_GATE = 0X01,
  COMM_CARD_GARAGE_GATE = 0X02,
} comm_card_lock_e;

// 门口机主动推送的操作类型枚举（与comm_card_op_param_e区分，仅用于推送）
typedef enum
{
  PUSH_OP_NONE = 0x00,             // 无操作（默认值，避免脏数据）
  PUSH_OP_SCAN_CARD = 0x01,        // 单卡搜索结果推送
  PUSH_OP_ADD_CARD = 0x02,         // 单卡添加结果推送
  PUSH_OP_DEL_CARD = 0x03,         // 单卡删除结果推送
  PUSH_OP_QUERY_CARD_BATCH = 0x04, // 批量查询（宿主下所有卡）结果推送
  PUSH_OP_DEL_CARD_BATCH = 0x05,   // 批量删除（宿主下所有卡）结果推送
  PUSH_OP_UPDATE_MASK = 0x06,      // 更改卡片开门属性结果推送（单卡或批量，由 update_mask 分支区分）
} push_op_type_e;

// -------------------------- 加/删卡操作响应结构体（反馈操作结果） --------------------------
// 宿主房号信息：card / password / fingerprint 三个协议头文件共用同一份定义，
// 用 guard 宏保证任意包含顺序与组合下都不会重复定义。
#ifndef COMMON_HOST_INFO_T_DEFINED
#define COMMON_HOST_INFO_T_DEFINED
typedef struct
{
  int32_t building;
  int32_t room;
  int32_t device_type;
} common_host_info_t;
#endif

// -------------------------- 户号限制开关 --------------------------
// 1 = 按户号（building/room/device_type）限制增/删/改/查，只能操作本户号数据（默认）；
// 0 = 不限户号，所有数据都可操作：查询返回全部；按户号清空即清空整库；
//     按索引删/改权限跳过归属校验。host_id 仍必填（用于落库归属与推送路由）。
// card / password / fingerprint 三个协议头共用同名宏，用 guard 防重复，
// 门口机与室内机两侧需保持一致。
#ifndef HOST_ID_LIMIT_ENABLE
#define HOST_ID_LIMIT_ENABLE 1
#endif

typedef struct
{
  uint8_t card_id[NET_CARD_ID_LEN]; // 卡片ID（二进制，与室外机存储一致）
  comm_card_lock_e card_lock_type;  // 卡开锁对应的锁类型
  common_host_info_t host_id;       // 宿主房号信息（关联的住户编号，便于室内机校验）
  bool is_valid;                    // 是否已被注册
} comm_card_info_t;

// -------------------------- 推送结果结构体）--------------------------

// -------------------------- 统一的卡片管理操作结果结构体 --------------------------
// 覆盖所有场景：扫描/加卡/删卡/批量查询/批量删除/批量改属性。
//
// 设计决策：使用扁平结构而非联合体。
// 原因：结构体整体做 base64 编码后传输，传输大小固定为 sizeof(comm_card_op_res_t)，
// 联合体节省的内存在传输层没有意义，反而增加了访问字段时的心智负担。
//
// 字段使用约定：
//   host_id    — 所有操作均填写，室内机据此定位住户
//   card_count — 单卡操作填 1，批量操作填实际数量，无卡操作（如批量删除）填 0
//   cards[]    — 按 card_count 依次填写，未使用的槽位保持零值
typedef struct
{
  push_op_type_e op_type;                   // 推送类型
  comm_res_code_e res_code;                 // 响应码
  common_host_info_t host_id;               // 关联宿主（所有操作必填）
  uint16_t card_count;                      // 本次操作涉及的卡片数量
  comm_card_info_t cards[NET_MAX_CARD_NUM]; // 卡片列表
} comm_card_op_res_t;
#endif