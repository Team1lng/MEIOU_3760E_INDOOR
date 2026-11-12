#ifndef SAT_WIFI_API_H
#define SAT_WIFI_API_H

#include <stdbool.h>
#include <stdint.h>

// WiFi信息结构体（原全局数组元素类型）
typedef struct
{
    int db;                      // 信号强度
    unsigned char psk_flags[64]; // 加密标识
    unsigned char name[64];      // WiFi名称（SSID）
    bool free;                   // 是否免费（无密码）
} wifi_info;

// WiFi上下文结构体（封装所有状态和资源）
typedef struct wifi_context wifi_context;

/**
 * @brief 创建WiFi上下文（初始化内部状态）
 * @return 成功返回上下文指针，失败返回NULL
 */
wifi_context *wifi_context_create(void);

/**
 * @brief 获取模块级共享WiFi上下文
 * @return 模块级共享WiFi上下文指针，调用者不得销毁或释放该指针
 */
wifi_context *wifi_context_get(void);

/**
 * @brief 销毁WiFi上下文（释放资源）
 * @param ctx WiFi上下文指针（NULL则无操作）
 */
void wifi_context_destroy(wifi_context *ctx);

/**
 * @brief 关闭WiFi设备
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_close(wifi_context *ctx);

/**
 * @brief 开启WiFi设备
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_open(wifi_context *ctx);

/**
 * @brief 连接临时缓存文件中的WiFi配置
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_temp_connecting(wifi_context *ctx);

/**
 * @brief 连接持久化配置文件中的WiFi
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_connecting(wifi_context *ctx);

/**
 * @brief 获取WiFi连接状态
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @param user 输出SSID（NULL则不输出）
 * @param free 输出是否免费（NULL则不输出）
 * @param ip 输出IP地址（NULL则不输出）
 * @param connected 输出是否已连接（NULL则不输出）
 * @param flags 输出加密标识（NULL则不输出）
 * @return true-成功，false-失败
 */
bool wifi_device_connection_status(wifi_context *ctx,
                                   unsigned char *user,
                                   bool *free,
                                   unsigned char *ip,
                                   bool *connected,
                                   unsigned char *flags);
/**
 * @brief 获取WiFi信号强度（RSSI）
 * @param ctx WiFi上下文
 * @param rssi 输出信号强度（dBm，未连接时值无意义）
 * @param connected 输出是否已连接WiFi
 * @return 成功true，失败false
 */
bool wifi_device_get_rssi(wifi_context *ctx, int *rssi, bool *connected);

/**
 * @brief 将WiFi账号密码保存到临时配置文件
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @param user SSID名称
 * @param pwd 密码（NULL表示无密码）
 * @return true-成功，false-失败
 */
bool wifi_device_user_to_temp(wifi_context *ctx, char *user, char *pwd);

/**
 * @brief 将WiFi账号密码写入持久化配置文件（不经过临时文件）
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @param user SSID名称
 * @param pwd 密码（NULL表示无密码）
 * @return true-成功，false-失败
 */
bool wifi_device_user_to_persistent(wifi_context *ctx, char *user, char *pwd);

/**
 * @brief 同步临时配置到持久化文件
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_tmp_sync(wifi_context *ctx);

/**
 * @brief 扫描周边WiFi
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_scan(wifi_context *ctx);

/**
 * @brief 获取扫描结果
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @param info 输出扫描结果数组指针（NULL则不输出）
 * @param total 输出扫描结果数量（NULL则不输出）
 * @return true-有结果，false-无结果/失败
 */
bool wifi_device_scan_info_get(wifi_context *ctx, wifi_info **info, int *total);

/**
 * @brief 获取WiFi物理地址（MAC）
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @param hwaddr 输出MAC地址（需至少64字节空间，NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_device_hwaddr_get(wifi_context *ctx, unsigned char *hwaddr);

/**
 * @brief 重置WiFi配置为默认值
 * @param ctx WiFi上下文指针（NULL则返回false）
 * @return true-成功，false-失败
 */
bool wifi_api_reset_default(wifi_context *ctx);

#endif // SAT_WIFI_API_H