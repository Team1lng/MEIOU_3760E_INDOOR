#include "sat_wifi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "sat_user_common.h"

// WiFi相关路径常量
#define WLAN_DEFAULT_IFACE "wlan0"
#define TMP_WPA_CONF "/tmp/wpa_supplicant.conf"
#define APP_CONFIG_WPA_CONF_DIR "/home/data"
#define APP_CONFIG_WPA_CONF "/home/data/wpa_supplicant.conf"
#define APP_CONFIG_WPA_CONF_SYNC APP_CONFIG_WPA_CONF ".sync"
#define WPA_CTRL_INTERFACE "/var/run/wpa_supplicant"
#define UDHCP_SCRIPT_PATH "/usr/share/udhcpc/default.script"

// 扫描相关常量
#define MAX_SCAN_WIFI_NUM 20
#define MAX_CMD_LEN 256
#define MAX_BUFFER_LEN 1024

#define WIFI_LOG(fmt, ...) printf("[WIFI][%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#define CHECK_NULL_PTR(ptr)                     \
    do                                          \
    {                                           \
        if ((ptr) == NULL)                      \
        {                                       \
            WIFI_LOG("param %s is NULL", #ptr); \
            return false;                       \
        }                                       \
    } while (0)

#define CHECK_FILE_EXIST(path)                     \
    do                                             \
    {                                              \
        if (access((path), F_OK) != 0)             \
        {                                          \
            WIFI_LOG("file %s not found", (path)); \
            return false;                          \
        }                                          \
    } while (0)

#define EXEC_SYS_CMD(cmd)                   \
    do                                      \
    {                                       \
        WIFI_LOG("execute cmd: %s", (cmd)); \
        system((cmd));                      \
    } while (0)

/**********************************************************
 * 类型定义
 **********************************************************/
// WiFi上下文具体结构（仅内部可见）
struct wifi_context
{
    int scan_wifi_count;                         // 本次扫描到的 WiFi 数量（≤ MAX_SCAN_WIFI_NUM）
    wifi_info scan_wifi_info[MAX_SCAN_WIFI_NUM]; // 存储扫描到的所有 WiFi 信息的数组（最多 MAX_SCAN_WIFI_NUM 个）
    char wlan_iface[16];                         // WiFi 网卡接口名（如 "wlan0" "eth0"，长度 16 足够容纳常见接口名）
};

static wifi_context g_wifi_context;
static bool g_wifi_context_initialized = false;

/**********************************************************
 * 静态函数声明（内部使用）
 **********************************************************/
static void wifi_context_init_defaults(wifi_context *ctx);
static bool wifi_device_scan_signal(wifi_context *ctx);
static bool wifi_device_info_parsing(wifi_context *ctx, char *buffer);
static bool wifi_device_scan_result(wifi_context *ctx);
static void wifi_kill_wpa_supplicant(void);
static void wifi_iface_reset(wifi_context *ctx);
static bool wifi_start_wpa_supplicant(wifi_context *ctx, const char *conf_path);
static bool wifi_sync_directory(const char *path);
static bool wifi_psk_valid(const char *pwd);

/**********************************************************
 * 核心功能实现
 **********************************************************/
static void wifi_context_init_defaults(wifi_context *ctx)
{
    memset(ctx, 0, sizeof(wifi_context));
    strncpy(ctx->wlan_iface, WLAN_DEFAULT_IFACE, sizeof(ctx->wlan_iface) - 1);
    ctx->scan_wifi_count = 0;
}

wifi_context *wifi_context_get(void)
{
    if (!g_wifi_context_initialized)
    {
        wifi_context_init_defaults(&g_wifi_context);
        g_wifi_context_initialized = true;
    }

    return &g_wifi_context;
}

/**
 * @brief 创建WiFi上下文
 */
wifi_context *wifi_context_create(void)
{
    wifi_context *ctx = (wifi_context *)malloc(sizeof(wifi_context));
    if (ctx == NULL)
    {
        WIFI_LOG("malloc failed");
        return NULL;
    }

    // 初始化上下文默认值
    wifi_context_init_defaults(ctx);

    return ctx;
}

/**
 * @brief 销毁WiFi上下文
 */
void wifi_context_destroy(wifi_context *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    if (ctx == &g_wifi_context)
    {
        return;
    }
    // 清空扫描结果
    memset(ctx->scan_wifi_info, 0, sizeof(ctx->scan_wifi_info));
    free(ctx);
}

/**
 * @brief 关闭WiFi设备
 */
bool wifi_device_close(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    wifi_kill_wpa_supplicant();

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "ifconfig %s down", ctx->wlan_iface);
    EXEC_SYS_CMD(cmd);

    return true;
}

/**
 * @brief 开启WiFi设备
 */
bool wifi_device_open(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "ifconfig %s up", ctx->wlan_iface);
    EXEC_SYS_CMD(cmd);

    if (access(APP_CONFIG_WPA_CONF, F_OK) == 0)
    {
        wifi_start_wpa_supplicant(ctx, APP_CONFIG_WPA_CONF);
    }
    else
    {
        sat_network_udhcpc_ip(ctx->wlan_iface);
    }
    return true;
}

/**
 * @brief 结束wpa_supplicant相关进程
 */
static void wifi_kill_wpa_supplicant(void)
{
    sat_kill_task_process("wpa_supplicant");
    sat_kill_task_process("udhcpc");
}

/**
 * @brief 复位网卡，清除驱动中残留的旧关联与旧 IP
 * @note  wpa_supplicant 被 SIGKILL 时不会向驱动发 deauth，rtl8188ftv 仍保留
 *        旧热点的链路状态；此时仅重启 wpa_supplicant 会出现 wpa_state=COMPLETED
 *        但数据面不通（表现为拿不到 IP），必须 down/up 强制驱动重新初始化
 */
static void wifi_iface_reset(wifi_context *ctx)
{
    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "ifconfig %s down", ctx->wlan_iface);
    EXEC_SYS_CMD(cmd);
    snprintf(cmd, sizeof(cmd), "ifconfig %s up", ctx->wlan_iface);
    EXEC_SYS_CMD(cmd);
}

/**
 * @brief 启动wpa_supplicant进程
 * @param ctx WiFi上下文
 * @param conf_path 配置文件路径
 * @return 成功返回true，失败返回false
 */
static bool wifi_start_wpa_supplicant(wifi_context *ctx, const char *conf_path)
{
    CHECK_NULL_PTR(ctx);
    CHECK_NULL_PTR(conf_path);

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "wpa_supplicant -Dnl80211 -i %s -c %s -B", ctx->wlan_iface, conf_path);
    WIFI_LOG("execute cmd: %s", cmd);
    int status = system(cmd);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        WIFI_LOG("start wpa_supplicant failed");
        return false;
    }
    sat_network_udhcpc_ip(ctx->wlan_iface);

    return true;
}

static bool wifi_sync_directory(const char *path)
{
    CHECK_NULL_PTR(path);

    int flags = O_RDONLY;
#ifdef O_DIRECTORY
    flags |= O_DIRECTORY;
#endif
    int fd = open(path, flags);
    if (fd < 0)
    {
        WIFI_LOG("open directory %s failed", path);
        return false;
    }

    if (fsync(fd) != 0)
    {
        WIFI_LOG("fsync directory %s failed", path);
        close(fd);
        return false;
    }

    if (close(fd) != 0)
    {
        WIFI_LOG("close directory %s failed", path);
        return false;
    }

    return true;
}

/**
 * @brief 连接临时缓存文件中的WiFi配置
 */
bool wifi_device_temp_connecting(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);
    CHECK_FILE_EXIST(TMP_WPA_CONF);

    // 结束原有进程并启动新进程
    wifi_kill_wpa_supplicant();
    wifi_iface_reset(ctx);
    return wifi_start_wpa_supplicant(ctx, TMP_WPA_CONF);
}

/**
 * @brief 连接持久化配置文件中的WiFi
 */
bool wifi_device_connecting(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);
    CHECK_FILE_EXIST(APP_CONFIG_WPA_CONF);

    // 结束原有进程并启动新进程
    wifi_kill_wpa_supplicant();
    wifi_iface_reset(ctx);
    return wifi_start_wpa_supplicant(ctx, APP_CONFIG_WPA_CONF);
}

/**
 * @brief 获取WiFi连接状态
 */
bool wifi_device_connection_status(wifi_context *ctx,
                                   unsigned char *user,
                                   bool *free,
                                   unsigned char *ip,
                                   bool *connected,
                                   unsigned char *flags)
{
    CHECK_NULL_PTR(ctx);

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "wpa_cli -i %s -p %s status",
             ctx->wlan_iface, WPA_CTRL_INTERFACE);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        WIFI_LOG("popen failed (cmd: %s)", cmd);
        return false;
    }

    // 初始化输出参数
    if (connected != NULL)
        *connected = false;
    if (free != NULL)
        *free = false;

    char buffer[MAX_BUFFER_LEN] = {0};
    while (fgets(buffer, sizeof(buffer), fp) > 0)
    {
        // 去除换行符
        buffer[strcspn(buffer, "\n")] = '\0';

        // 解析SSID
        if (user != NULL && strncmp(buffer, "ssid=", 5) == 0)
        {
            strcpy((char *)user, buffer + 5);
        }
        // 解析连接状态
        else if (connected != NULL && strcmp(buffer, "wpa_state=COMPLETED") == 0)
        {
            *connected = true;
        }
        // 解析IP地址
        else if (ip != NULL && strncmp(buffer, "ip_address=", 11) == 0)
        {
            strcpy((char *)ip, buffer + 11);
        }
        // 解析加密方式
        else if (flags != NULL && strncmp(buffer, "key_mgmt=", 9) == 0)
        {
            strncpy((char *)flags, buffer + 9, 63);
            if (free != NULL)
            {
                *free = (strcmp(buffer + 9, "NONE") == 0) ? true : false;
            }
        }
    }

    int status = pclose(fp);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        WIFI_LOG("wpa_cli status failed");
        return false;
    }

    return true;
}

/**
 * @brief 获取WiFi信号强度（RSSI）
 * @param ctx WiFi上下文
 * @param rssi 输出信号强度（dBm，未连接时值无意义）
 * @param connected 输出是否已连接WiFi
 * @return 成功true，失败false
 */
bool wifi_device_get_rssi(wifi_context *ctx, int *rssi, bool *connected)
{
    CHECK_NULL_PTR(ctx);

    // 初始化输出
    if (rssi != NULL)
        *rssi = 0;
    if (connected != NULL)
        *connected = false;

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "wpa_cli -i %s -p %s signal_poll",
             ctx->wlan_iface, WPA_CTRL_INTERFACE);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        WIFI_LOG("popen failed (cmd: %s)", cmd);
        return false;
    }

    char buffer[MAX_BUFFER_LEN] = {0};
    while (fgets(buffer, sizeof(buffer), fp) > 0)
    {
        // 去掉换行
        buffer[strcspn(buffer, "\n")] = '\0';

        // 解析 RSSI
        if (rssi != NULL && strncmp(buffer, "RSSI=", 5) == 0)
        {
            *rssi = atoi(buffer + 5);
            // 能读到 RSSI 说明已连接
            if (connected != NULL)
                *connected = true;
        }
    }

    pclose(fp);
    return true;
}

/**
 * @brief 生成wpa_supplicant配置内容
 * @param buffer 输出缓冲区
 * @param buf_len 缓冲区长度
 * @param ssid 热点名称
 * @param pwd 热点密码
 * @param is_free true=开放网络(无密码) false=WPA/WPA2-PSK加密网络
 * @return 成功true，缓冲区不足/入参异常false
 */
static bool wifi_psk_valid(const char *pwd)
{
    size_t len = pwd != NULL ? strlen(pwd) : 0;

    return len >= 8 && len <= 63;
}

static bool wifi_generate_wpa_conf(char *buffer, size_t buf_len,
                                   const char *ssid, const char *pwd, bool is_free)
{
    CHECK_NULL_PTR(buffer);
    CHECK_NULL_PTR(ssid);
    if (buf_len == 0 || (!is_free && !wifi_psk_valid(pwd)))
    {
        buffer[0] = '\0';
        return false;
    }

    int ret = 0;
    if (is_free)
    {
        // 开放无密码网络
        ret = snprintf(buffer, buf_len,
                       "ctrl_interface=%s\n"
                       "update_config=1\n"
                       "network={\n"
                       "\tscan_ssid=1\n"
                       "\tkey_mgmt=NONE\n"
                       "\tssid=\"%s\"\n"
                       "}\n",
                       WPA_CTRL_INTERFACE, ssid);
    }
    else
    {
        const char *passwd = (pwd != NULL) ? pwd : "";
        ret = snprintf(buffer, buf_len,
                       "ctrl_interface=%s\n"
                       "update_config=1\n"
                       "network={\n"
                       "\tscan_ssid=1\n"
                       "\tkey_mgmt=WPA-PSK\n"
                       "\tssid=\"%s\"\n"
                       "\tpsk=\"%s\"\n"
                       "}\n",
                       WPA_CTRL_INTERFACE, ssid, passwd);
    }

    // snprintf 返回值 >= buf_len 代表缓冲区溢出
    if (ret < 0 || (size_t)ret >= buf_len)
    {
        buffer[0] = '\0';
        return false;
    }

    return true;
}

/**
 * @brief 将WiFi账号密码保存到临时配置文件
 */
bool wifi_device_user_to_temp(wifi_context *ctx, char *user, char *pwd)
{
    CHECK_NULL_PTR(ctx);
    CHECK_NULL_PTR(user);

    bool is_free = (pwd == NULL) ? true : false;
    char buffer[MAX_BUFFER_LEN] = {0};

    // 生成配置内容
    if (!wifi_generate_wpa_conf(buffer, sizeof(buffer), (char *)user, (char *)pwd, is_free))
    {
        WIFI_LOG("generate temp wpa conf failed");
        return false;
    }

    // 写入临时文件
    FILE *fp = fopen(TMP_WPA_CONF, "wb");
    if (fp == NULL)
    {
        WIFI_LOG("open %s failed", TMP_WPA_CONF);
        return false;
    }
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);

    return true;
}

/**
 * @brief 同步临时配置到持久化文件
 */
bool wifi_device_tmp_sync(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);
    CHECK_FILE_EXIST(TMP_WPA_CONF);

    bool ret = true;
    FILE *src_fp = fopen(TMP_WPA_CONF, "rb");
    FILE *dst_fp = NULL;
    unsigned char buffer[MAX_BUFFER_LEN] = {0};

    if (src_fp == NULL)
    {
        WIFI_LOG("open %s failed", TMP_WPA_CONF);
        return false;
    }

    dst_fp = fopen(APP_CONFIG_WPA_CONF_SYNC, "wb");
    if (dst_fp == NULL)
    {
        WIFI_LOG("open %s failed", APP_CONFIG_WPA_CONF_SYNC);
        fclose(src_fp);
        remove(APP_CONFIG_WPA_CONF_SYNC);
        return false;
    }

    while (ret)
    {
        size_t read_len = fread(buffer, 1, sizeof(buffer), src_fp);
        if (read_len > 0 && fwrite(buffer, 1, read_len, dst_fp) != read_len)
        {
            WIFI_LOG("write %s failed", APP_CONFIG_WPA_CONF_SYNC);
            ret = false;
            break;
        }

        if (read_len < sizeof(buffer))
        {
            if (ferror(src_fp))
            {
                WIFI_LOG("read %s failed", TMP_WPA_CONF);
                ret = false;
            }
            break;
        }
    }

    if (ret && fflush(dst_fp) != 0)
    {
        WIFI_LOG("flush %s failed", APP_CONFIG_WPA_CONF_SYNC);
        ret = false;
    }
    if (ret && fsync(fileno(dst_fp)) != 0)
    {
        WIFI_LOG("fsync %s failed", APP_CONFIG_WPA_CONF_SYNC);
        ret = false;
    }
    if (fclose(dst_fp) != 0)
    {
        WIFI_LOG("close %s failed", APP_CONFIG_WPA_CONF_SYNC);
        ret = false;
    }
    if (fclose(src_fp) != 0)
    {
        WIFI_LOG("close %s failed", TMP_WPA_CONF);
        ret = false;
    }

    if (!ret)
    {
        remove(APP_CONFIG_WPA_CONF_SYNC);
        return false;
    }

    if (rename(APP_CONFIG_WPA_CONF_SYNC, APP_CONFIG_WPA_CONF) != 0)
    {
        WIFI_LOG("rename %s to %s failed", APP_CONFIG_WPA_CONF_SYNC, APP_CONFIG_WPA_CONF);
        remove(APP_CONFIG_WPA_CONF_SYNC);
        return false;
    }

    if (!wifi_sync_directory(APP_CONFIG_WPA_CONF_DIR))
    {
        return false;
    }

    // 同步配置
    EXEC_SYS_CMD("rm -f " TMP_WPA_CONF);
    EXEC_SYS_CMD("sync");

    return true;
}

/**
 * @brief 执行WiFi扫描操作
 */
static bool wifi_device_scan_signal(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    bool scan_result = false;
    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "wpa_cli -i %s -p %s scan", ctx->wlan_iface, WPA_CTRL_INTERFACE);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        return scan_result;
    }
    char buffer[MAX_BUFFER_LEN] = {0};
    while (fgets(buffer, sizeof(buffer), fp) > 0)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "OK") == 0)
        {
            scan_result = true;
            break;
        }
    }
    pclose(fp);

    return scan_result;
}

/**
 * @brief 解析扫描结果
 */
static bool wifi_device_info_parsing(wifi_context *ctx, char *buffer)
{
    CHECK_NULL_PTR(ctx);
    CHECK_NULL_PTR(buffer);

    /* fgets 保留行尾换行；若不裁剪，strncpy(ssid) 会把 \n 带进 info->name */
    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (ctx->scan_wifi_count >= MAX_SCAN_WIFI_NUM)
    {
        return false;
    }

    wifi_info *info = &ctx->scan_wifi_info[ctx->scan_wifi_count];
    unsigned char bssid[128] = {0};
    unsigned int frequency = 0;

    // 解析基础信息
    sscanf(buffer, "%s\t%d\t%d\t%s\t%s", bssid, &frequency, &info->db,
           info->psk_flags, info->name);

    // 修正SSID解析（取最后一个制表符后的内容）
    char *ps_name = strrchr(buffer, '\t') + 1;
    memset(info->name, 0, sizeof(info->name));
    strncpy((char *)info->name, ps_name, sizeof(info->name) - 1);

    // 判断是否免费（无加密）
    info->free = true;
    if (strstr((char *)info->psk_flags, "WEP") != NULL ||
        strstr((char *)info->psk_flags, "WPA") != NULL ||
        strstr((char *)info->psk_flags, "WPA2") != NULL ||
        strstr((char *)info->psk_flags, "WPA3") != NULL)
    {
        info->free = false;
    }

    ctx->scan_wifi_count++;
    return true;
}

/**
 * @brief 获取扫描结果
 */
static bool wifi_device_scan_result(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "wpa_cli -i %s -p %s scan_result", ctx->wlan_iface, WPA_CTRL_INTERFACE);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        WIFI_LOG("popen failed");
        return false;
    }

    // 重置扫描结果
    memset(ctx->scan_wifi_info, 0, sizeof(ctx->scan_wifi_info));
    ctx->scan_wifi_count = 0;

    char buffer[MAX_BUFFER_LEN] = {0};
    int line_index = 0;
    while (fgets(buffer, sizeof(buffer), fp) > 0)
    {
        // 跳过前两行表头，最多解析MAX_SCAN_WIFI_NUM个结果
        if (line_index > 1 && ctx->scan_wifi_count < MAX_SCAN_WIFI_NUM)
        {
            wifi_device_info_parsing(ctx, buffer);
        }
        line_index++;
    }

    pclose(fp);
    return true;
}

/**
 * @brief 扫描周边WiFi
 * @note wpa_cli scan 是异步触发，返回 OK 时扫描尚未完成，立即读结果可能为空。
 *       因此结果为空时返回 false，交由上层定时重扫，避免列表一直为空。
 */
bool wifi_device_scan(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    // 执行扫描
    if (!wifi_device_scan_signal(ctx))
    {
        WIFI_LOG("scan signal failed");
        return false;
    }

    // 解析结果
    if (!wifi_device_scan_result(ctx))
    {
        WIFI_LOG("parse scan result failed");
        return false;
    }

    // 扫描结果为空：本次扫描可能尚未真正完成（或环境无热点），返回 false 触发重试
    if (ctx->scan_wifi_count == 0)
    {
        WIFI_LOG("no scan result yet");
        return false;
    }

    return true;
}

/**
 * @brief 获取扫描结果
 */
bool wifi_device_scan_info_get(wifi_context *ctx, wifi_info **info, int *total)
{
    CHECK_NULL_PTR(ctx);

    if (ctx->scan_wifi_count == 0)
    {
        WIFI_LOG("no wifi scanned");
        return false;
    }

    if (info != NULL)
        *info = ctx->scan_wifi_info;
    if (total != NULL)
        *total = ctx->scan_wifi_count;

    return true;
}

/**
 * @brief 获取WiFi物理地址（MAC）
 */
bool wifi_device_hwaddr_get(wifi_context *ctx, unsigned char *hwaddr)
{
    CHECK_NULL_PTR(ctx);
    CHECK_NULL_PTR(hwaddr);

    char cmd[MAX_CMD_LEN] = {0};
    snprintf(cmd, sizeof(cmd), "cat /sys/class/net/%s/address", ctx->wlan_iface);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        WIFI_LOG("popen failed");
        return false;
    }

    fgets((char *)hwaddr, 64, fp);
    // 去除换行符
    hwaddr[strcspn((char *)hwaddr, "\n")] = '\0';
    pclose(fp);

    return true;
}

/**
 * @brief 重置WiFi配置为默认值
 */
bool wifi_api_reset_default(wifi_context *ctx)
{
    CHECK_NULL_PTR(ctx);

    char buffer[MAX_BUFFER_LEN] = {0};
    wifi_generate_wpa_conf(buffer, sizeof(buffer), "leo_wifi", "11111111", false);

    FILE *fp = fopen(APP_CONFIG_WPA_CONF, "wb");
    if (fp == NULL)
    {
        WIFI_LOG("open %s failed", APP_CONFIG_WPA_CONF);
        return false;
    }

    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);
    EXEC_SYS_CMD("sync");

    return true;
}