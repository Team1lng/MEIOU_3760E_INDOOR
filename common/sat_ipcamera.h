#ifndef SAT_IPCAMERA_H
#define SAT_IPCAMERA_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#define USER_NETWORK_AUTH_USER "admin"
#define USER_NETWORK_AUTH_PASSWORD "123456789"
/*==========================================================================
 * 常量定义
 *==========================================================================*/
#define IPCAMERA_NUM_MAX 8
#define IPCAMERA_PROFILE_MAX 8
/* AMY_EVENT_ID_IPCAMERA 事件类型 */
#define IPCAMERA_EVENT_TYPE_SEARCH_DONE 1
#define IPCAMERA_EVENT_TYPE_RTSP_DONE 2
#define IPCAMERA_EVENT_TYPE_DOOR_REGISTER_DONE 3
/*==========================================================================
 * 枚举定义
 *==========================================================================*/
typedef enum
{
        RTSP_URL_TYPE_BASE,
        RTSP_URL_TYPE_FULL,
} rtsp_url_type_e;
/*==========================================================================
 * 数据结构定义
 *==========================================================================*/
struct ipcamera_rtsp_info
{
        char profile_token[64];
        char rtsp_url[128];
        int rtsp_url_type;
};

struct ipcamera_network
{
        bool udhcp;
        char ipaddr[24];
        char mask[24];
        char dns[24];
        char gateway[24];
        char net_token[24];
};

struct ipcamera_info
{
        char ipaddr[32];
        int port;
        char username[64];
        char password[64];
        char door_name[64];
        /*注意此处：因为判断sizeof长度使sip，与rtsp一致，故
rtsp url与 sip url的数组长度要一致，否则会出现越界处理*/
        char sip_url[128];
        struct ipcamera_rtsp_info rtsp[IPCAMERA_PROFILE_MAX];
        int profile_token_num;
        /*0:token,1:digest*/
        char auther_flag;
        char video_token[32];
};

// 事件回调属性
typedef struct
{
        int type;
        int num;
} ipcamera_cb_attr;

// 在线检测回调属性
typedef struct
{
        int index;
        bool is_online;
} ipcamera_online_check_attr;

// 版本查询回调属性
typedef struct
{
        int index;
        bool result;
        char version[32];
} door_version_cb_attr;

/*==========================================================================
 * discover 层对外接口（依赖内部 ipc_device[]，搜索阶段使用）
 *==========================================================================*/

// 初始化与配置
bool sat_ipcamera_initialization_parameters(struct ipcamera_info *device, int total);
bool sat_ipcamera_user_password_set(int index, const char *username, const char *password);

// 设备搜索
bool sat_ipcamera_device_discover_search(char type, const char *ifname);

// RTSP 获取
bool sat_ipcamera_rtsp_url_get(int index);

// 状态查询
bool sat_ipcamera_device_status_get(void);
void sat_ipcamera_device_status_reset(void);

// 搜索/注册流程占用 ipc_device[]，防止门口机状态轮询覆盖搜索结果
bool sat_ipcamera_discovery_reserve(void);
void sat_ipcamera_discovery_release(void);
int sat_ipcamera_online_num_get(void);
const char *sat_ipcamera_ipaddr_get(int index);
int sat_ipcamera_port_get(int index);
const char *sat_ipcamera_username_get(int index);
const char *sat_ipcamera_password_get(int index);
const char *sat_ipcamera_door_name_get(int index);
const char *sat_ipcamera_rtsp_addr_get(int index, int ch);
const char *sat_ipcamera_sip_addr_get(int index);
int sat_ipcamera_profile_token_num_get(int index);
struct ipcamera_info *sat_ipcamera_node_data_get(int index);

// 搜索阶段设备操作（通过索引访问 ipc_device[]）
bool sat_ipcamera_device_delete(int index, int timeout, bool is_block);
bool sat_ipcamera_device_name_set(char *name, int index, int timeout, bool is_block);
bool sat_ipcamera_device_password_set(char *new_pwd, int index, int timeout, bool is_block);
bool sat_ipcamera_online_checkt(int index, int timeout, bool is_block);
bool sat_ipcamera_device_version_get(int index, int timeout, bool is_block, char *version_buf);

/*==========================================================================
 * ctrl 层对外接口（无状态，直接传 ip/port/user/password，绑定后使用）
 *==========================================================================*/

// 系统时间
bool sat_ipcamera_system_time_get(const char *ip, int port, const char *user,
                                  const char *password, char auther_flag,
                                  int timeout, bool is_block);

// Shell 命令
bool sat_ipcamera_report_shellcmd(char *ip, int port, const char *user, const char *password,
                                  char *cmd, int timeout, bool is_block);

// 图像参数
bool sat_ipcamera_image_get(const char *ip, int port, const char *user, const char *password,
                            char auther_flag, const char *video_token, int brightness[3],
                            int saturation[3], int contrast[3], int timeout, bool is_block);
bool sat_ipcamera_image_setting(const char *ip, int port, const char *user, const char *password,
                                char auther_flag, const char *video_token, int brightness,
                                int saturation, int contrast, int timeout, bool is_block);

// 网络配置
bool sat_ipcamera_network_get(const char *ip, int port, const char *user, const char *password,
                              char auther_flag, struct ipcamera_network *info,
                              int timeout, bool is_block);
bool sat_ipcamera_network_setting(const char *ip, int port, const char *user, const char *password,
                                  char auther_flag, struct ipcamera_network *info,
                                  int timeout, bool is_block);

// 工作模式
bool sat_ipcamera_device_mode_get(const char *ip, int port, const char *user, const char *password,
                                  char auther_flag, char *status, int timeout, bool is_block);
bool sat_ipcamera_device_mode_setting(const char *ip, int port, const char *user, const char *password,
                                      char auther_flag, char status, int timeout, bool is_block);

// 视频通道
bool sat_ipcamera_device_channel_get(const char *ip, int port, const char *user, const char *password,
                                     char auther_flag, int *channel, int timeout, bool is_block);
bool sat_ipcamera_device_channel_setting(const char *ip, int port, const char *user, const char *password,
                                         char auther_flag, int channel, int timeout, bool is_block);

// MAC 更新
bool sat_ipcamera_device_mac_update(const char *ip, int port, const char *user, const char *password,
                                    char auther_flag, char *mac, int timeout, bool is_block);

// 通话状态上报
bool sat_ipcamera_device_talking_status_report(int status, const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block);

// 通话信令
bool sat_ipcamera_device_call_invite(int device_type, int building, int room, const char *ip,
                                     int port, const char *user, const char *password,
                                     int timeout, bool is_block);
bool sat_ipcamera_device_call_ringing(int device_type, int building, int room, const char *ip,
                                      int port, const char *user, const char *password,
                                      int timeout, bool is_block);
bool sat_ipcamera_device_call_busy(int device_type, int building, int room, const char *ip,
                                   int port, const char *user, const char *password,
                                   int timeout, bool is_block);
bool sat_ipcamera_device_call_error(int device_type, int building, int room, const char *ip,
                                    int port, const char *user, const char *password,
                                    int timeout, bool is_block);
bool sat_ipcamera_device_call_accept(int device_type, int building, int room, const char *ip,
                                     int port, const char *user, const char *password,
                                     int timeout, bool is_block);
bool sat_ipcamera_device_call_end(int device_type, int building, int room, const char *ip,
                                  int port, const char *user, const char *password,
                                  int timeout, bool is_block);
bool sat_ipcamera_device_call_heartbeat_request(int device_type, int building, int room, const char *ip,
                                                int port, const char *user, const char *password,
                                                int timeout, bool is_block);
bool sat_ipcamera_device_call_heartbeat_response(int device_type, int building, int room, const char *ip,
                                                 int port, const char *user, const char *password,
                                                 int timeout, bool is_block);
/* 门口机注册到指定房间（data: device_type-building-room） */
bool sat_ipcamera_device_register(int device_type, int building, int room, const char *ip,
                                  int port, const char *user, const char *password,
                                  int timeout, bool is_block);
bool sat_ipcamera_device_card_manage_exit(const void *data, size_t data_len,
                                          const char *ip, int port,
                                          const char *user, const char *password,
                                          int timeout, bool is_block);
/*
 * sat_ipcamera_card —— 卡片管理结果推送
 *
 * 设计原则：
 *   本模块不引入任何卡片业务数据类型（comm_card_op_res_t 等）。
 *   调用方负责准备好原始数据（任意结构体），以 void* 传入；
 *   本模块内部统一完成 base64 编码并以对应 XML 标签发送。
 *
 * 所有接口签名完全一致，仅 XML 标签不同。
 */
bool sat_ipcamera_device_card_manage_enter(const void *data, size_t data_len,
                                           const char *ip, int port,
                                           const char *user, const char *password,
                                           int timeout, bool is_block);
/* 添加单卡结果推送 */
bool sat_ipcamera_device_card_manage_add_card(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block);

/* 删除单卡结果推送 */
bool sat_ipcamera_device_card_manage_del_card(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block);

/* 单卡扫描（预添加）结果推送 */
bool sat_ipcamera_device_card_manage_scan_card(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block);

/* 批量查询白名单结果推送 */
bool sat_ipcamera_device_card_manage_query_card_whitelist(const void *data, size_t data_len,
                                                          const char *ip, int port,
                                                          const char *user, const char *password,
                                                          int timeout, bool is_block);

/* 批量删除白名单结果推送 */
bool sat_ipcamera_device_card_manage_del_card_whitelist(const void *data, size_t data_len,
                                                        const char *ip, int port,
                                                        const char *user, const char *password,
                                                        int timeout, bool is_block);

/* 批量修改开门属性结果推送 */
bool sat_ipcamera_device_card_manage_modify_door_mask(const void *data, size_t data_len,
                                                      const char *ip, int port,
                                                      const char *user, const char *password,
                                                      int timeout, bool is_block);

/*
 * sat_ipcamera_password —— 密码管理
 *
 * 设计原则与卡片管理一致：
 *   不引入密码业务数据类型，调用方负责准备原始数据以 void* 传入；
 *   内部统一完成 base64 编码并以对应 XML 标签发送。
 */
/* 进入密码管理模式 */
bool sat_ipcamera_device_password_manage_enter(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block);
/* 退出密码管理模式 */
bool sat_ipcamera_device_password_manage_exit(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block);
/* 查询密码列表 */
bool sat_ipcamera_device_password_manage_query(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block);
/* 添加密码 */
bool sat_ipcamera_device_password_manage_add(const void *data, size_t data_len,
                                             const char *ip, int port,
                                             const char *user, const char *password,
                                             int timeout, bool is_block);
/* 删除密码 */
bool sat_ipcamera_device_password_manage_del(const void *data, size_t data_len,
                                             const char *ip, int port,
                                             const char *user, const char *password,
                                             int timeout, bool is_block);
/* 设置密码权限 */
bool sat_ipcamera_device_password_manage_set_perm(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block);

/*
 * sat_ipcamera_fingerprint —— 指纹管理
 *
 * 设计原则与卡片/密码管理一致：
 *   不引入指纹业务数据类型，调用方负责准备原始数据以 void* 传入；
 *   内部统一完成 base64 编码并以对应 XML 标签发送。
 */
/* 进入指纹管理模式 */
bool sat_ipcamera_device_fingerprint_manage_enter(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block);
/* 退出指纹管理模式 */
bool sat_ipcamera_device_fingerprint_manage_exit(const void *data, size_t data_len,
                                                 const char *ip, int port,
                                                 const char *user, const char *password,
                                                 int timeout, bool is_block);
/* 查询指纹列表 */
bool sat_ipcamera_device_fingerprint_manage_query(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block);
/* 添加指纹（触发门口机录入流程） */
bool sat_ipcamera_device_fingerprint_manage_add(const void *data, size_t data_len,
                                                const char *ip, int port,
                                                const char *user, const char *password,
                                                int timeout, bool is_block);
/* 删除指纹 */
bool sat_ipcamera_device_fingerprint_manage_del(const void *data, size_t data_len,
                                                const char *ip, int port,
                                                const char *user, const char *password,
                                                int timeout, bool is_block);
/* 清空指纹库 */
bool sat_ipcamera_device_fingerprint_manage_clear(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block);
/* 设置指纹权限 */
bool sat_ipcamera_device_fingerprint_manage_set_perm(const void *data, size_t data_len,
                                                     const char *ip, int port,
                                                     const char *user, const char *password,
                                                     int timeout, bool is_block);
// 解锁
bool sat_ipcamera_device_unlock(const char *ip, int port, const char *user,
                                const char *password, int lock_time, int lock_type,
                                int language_id, int timeout, bool is_block);
// 解锁参数
bool sat_ipcamera_device_unlock_param(const char *ip, int port, const char *user,
                                      const char *password, int lock_time, int lock_type, int timeout, bool is_block);

// 通话音量
bool sat_ipcamera_device_talk_volume(int device_type, int building, int room,
                                     const char *ip, int port, const char *user,
                                     const char *password, int volume, int timeout, bool is_block);

// 语音播报：无人在家
bool sat_ipcamera_device_no_one_home_broadcast(const char *ip, int port,
                                               const char *user, const char *password,
                                               int language_id, int timeout, bool is_block);

// 移动侦测
bool sat_ipcamera_device_human_detect_ctrl(const char *ip, int port, const char *user,
                                           const char *password, int enable,
                                           int timeout, bool is_block);

// 系统重启（门口机复位）
bool sat_ipcamera_device_system_reboot(const char *ip, int port, const char *user,
                                       const char *password, int timeout, bool is_block);

// 系统复位（门口机恢复出厂配置）
bool sat_ipcamera_device_factory_reset(const char *ip, int port, const char *user,
                                       const char *password, int timeout, bool is_block);

#endif /* SAT_IPCAMERA_H */