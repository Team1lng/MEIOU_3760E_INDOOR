#include "user_network.h"
#include "sat_user_common.h"
#include "user_data.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include "mxml-3.3.1/mxml.h"
#include "base64/include/libbase64.h"
#include "onvif.h"
#include "sha1/sha1.h"
#include "call_api.h"
#include "onvif_resource_xml.h"
#include "card_common_net_proto.h"
#include "password_common_net_proto.h"
#include "Amy_lvgl.h"
#include "sat_ipcamera.h"
#include "user_data.h"
#include "user_time.h"
#include "onvif/onvif.h"
// ========================= 常量定义 =========================

#define DEVICE_MAX 8
#define USER_NETWORK_MULTICAST_IP "239.255.255.250"
#define USER_NETWORK_MULTICAST_PORT 3702
#define USER_NETWORK_TCP_PORT 80
#define TCP_RECEIVE_BUFFER_SIZE (128 * 1024)
#define TCP_MAX_CONN 8 ///< 最大并发连接数，超出时拒绝新连接

/** 与 add_multicast_routes 中组播路由所用网卡一致 */
#define USER_NETWORK_LAN_IFACE "br0"
#define USER_NETWORK_BRIDGE_IFACE "br0"
#define USER_NETWORK_BRIDGE_PORT0 "eth0"
#define USER_NETWORK_BRIDGE_PORT1 "eth1"
/** 与 generate_ip 的 10.{栋}.{楼层}.* 编址匹配，覆盖同栋各楼层 */
#define USER_NETWORK_STATIC_NETMASK "255.0.0.0"

/*==========================================================================
 * 人形侦测组播接收（室外机 → 239.building.255.255）
 *==========================================================================*/

/** 人形侦测组播UDP端口号 */
#define HUMAN_DETECT_MCAST_PORT 3702

/** 人形侦测组播接收缓冲区大小（XML 报文） */
#define HD_MCAST_RECV_BUF_SIZE 4096

// ========================= 连接池信号量 =========================

static sem_t s_conn_sem; ///< 控制最大并发连接数，防止无限 pthread_create

/*==========================================================================
 * 配置组播常驻监听（栋/房变化时换组，与冲突检测框架一致）
 *==========================================================================*/

// ========================= 200 OK 响应 =========================

/**
 * @brief 向门口机回复 200 OK
 * @param fd     TCP 连接句柄
 * @param action SOAP Action 标签名
 */
static bool net_send_200_ok(int fd, const char *action)
{
        if (fd < 0)
                return true;

        char *html_fmt = (char *)malloc(TCP_RECEIVE_BUFFER_SIZE);
        if (html_fmt == NULL)
        {
                SAT_ERROR("[NET] net_send_200_ok: malloc html_fmt failed");
                return false;
        }

        strncpy(html_fmt, S200_OK_REQUEST_PATH, XML_FILE_MAX);

        char *xml_fmt = strstr(html_fmt, "<?xml version=\"1.0\"");
        if (xml_fmt == NULL)
        {
                SAT_ERROR("[NET] net_send_200_ok: xml template not found");
                free(html_fmt);
                return false;
        }

        char *xml_buffer = (char *)malloc(TCP_RECEIVE_BUFFER_SIZE);
        if (xml_buffer == NULL)
        {
                SAT_ERROR("[NET] net_send_200_ok: malloc xml_buffer failed");
                free(html_fmt);
                return false;
        }

        char *html_buffer = (char *)malloc(TCP_RECEIVE_BUFFER_SIZE);
        if (html_buffer == NULL)
        {
                SAT_ERROR("[NET] net_send_200_ok: malloc html_buffer failed");
                free(xml_buffer);
                free(html_fmt);
                return false;
        }

        sprintf(xml_buffer, xml_fmt, action);
        int xml_size = strlen(xml_buffer);

        memset(xml_fmt, 0, strlen(xml_fmt) + 1);
        strcpy(xml_fmt, xml_buffer);

        memset(html_buffer, 0, TCP_RECEIVE_BUFFER_SIZE);
        sprintf(html_buffer, html_fmt, xml_size);

        sat_socket_tcp_send(fd, (unsigned char *)html_buffer, strlen(html_buffer), 3000);

        free(html_buffer);
        free(xml_buffer);
        free(html_fmt);
        return true;
}

// ========================= XML 解析 =========================

/**
 * @brief 解析 XML 并提取指定标签的文本内容
 *
 * 原来每个 if/else if 分支都会重新调用 mxmlLoadString，
 * 现在改为：外部解析一次得到 root，再按需查找标签。
 *
 * @param root  已解析的 XML 根节点
 * @param tag   要查找的标签名
 * @param out   输出缓冲区
 * @param size  缓冲区大小
 * @return true-找到且有文本内容，false-未找到
 */
static bool xml_find_text(mxml_node_t *root, const char *tag, char *out, int size)
{
        mxml_node_t *node = mxmlFindElementSub(root, root, tag, NULL, NULL, MXML_DESCEND);
        if (node == NULL)
                return false;

        const char *text = mxmlGetText(node, NULL);
        if (text == NULL)
                return false;

        strncpy(out, text, size - 1);
        out[size - 1] = '\0';
        return true;
}

// ========================= 业务 Handler =========================

/**
 * @brief 解析通话事件公共数据（device_type-building-room）
 */
static void parse_device_info(const char *data, device_info_attr *dev)
{
        sscanf(data, "%d-%d-%d",
               &dev->device_type, &dev->building, &dev->room);
}

static bool handle_call_invite(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_invite(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_ring(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_ringing(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_busy(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_busy(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_accept(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_accept(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_end(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_end(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_heartbeat_req(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_heartbeat_request(&dev);
        return net_send_200_ok(fd, "CallManage");
}

static bool handle_call_heartbeat_resp(int fd, const char *data)
{
        device_info_attr dev = {0};
        parse_device_info(data, &dev);
        sat_call_receive_heartbeat_response(&dev);
        return net_send_200_ok(fd, "CallManage");
}

/**
 * @brief 卡片事件统一 handler
 *
 * 所有卡片推送（Scan/Add/Del/DelAll/Query/UpdateMask）共用此函数。
 * data 为 base64 编码的 comm_card_op_res_t 字节流。
 *
 */
static bool handle_card_push(int fd, const char *data)
{
        comm_card_op_res_t res = {0};
        size_t decoded_size = 0;

        base64_decode(data, strlen(data), (char *)&res, &decoded_size, 0);

        if (decoded_size < sizeof(comm_card_op_res_t))
        {
                SAT_ERROR("[NET] card push: decode size mismatch (got=%zu need=%zu)",
                          decoded_size, sizeof(comm_card_op_res_t));
                // 解码失败也要回复 200，避免门口机重试
                return net_send_200_ok(fd, "CardManage");
        }

        // 无论成功还是失败都通知 UI 层，由 UI 层根据 res_code 决定展示内容
        Amy_card_event_put(&res);

        return net_send_200_ok(fd, "CardManage");
}

/**
 * @brief 密码事件统一 handler
 *
 * 所有密码推送（Query/Add/Del/SetPerm）共用此函数。
 * data 为 base64 编码的 comm_password_op_res_t 字节流。
 */
static bool handle_password_push(int fd, const char *data)
{
        comm_password_op_res_t res = {0};
        size_t decoded_size = 0;

        base64_decode(data, strlen(data), (char *)&res, &decoded_size, 0);

        if (decoded_size < sizeof(comm_password_op_res_t))
        {
                SAT_ERROR("[NET] password push: decode size mismatch (got=%zu need=%zu)",
                          decoded_size, sizeof(comm_password_op_res_t));
                return net_send_200_ok(fd, "PasswordManage");
        }

        Amy_password_event_put(&res);

        return net_send_200_ok(fd, "PasswordManage");
}

/**
 * @brief 指纹事件统一 handler
 *
 * 所有指纹推送（Scan/Query/Add/Del/Clear/SetPerm）共用此函数。
 * data 为 base64 编码的 comm_finger_op_res_t 字节流。
 */
static bool handle_fingerprint_push(int fd, const char *data)
{
        comm_finger_op_res_t res = {0};
        size_t decoded_size = 0;

        base64_decode(data, strlen(data), (char *)&res, &decoded_size, 0);

        if (decoded_size < sizeof(comm_finger_op_res_t))
        {
                SAT_ERROR("[NET] fingerprint push: decode size mismatch (got=%zu need=%zu)",
                          decoded_size, sizeof(comm_finger_op_res_t));
                return net_send_200_ok(fd, "FingerprintManage");
        }

        Amy_fingerprint_event_put(&res);

        return net_send_200_ok(fd, "FingerprintManage");
}

static bool handle_human_detect(int fd, const char *data)
{
        int channel = 0;
        sscanf(data, "%d", &channel);
        Amy_human_detect_event_put(channel);
        return net_send_200_ok(fd, "HumanDetect");
}

// ========================= 路由表 =========================

/**
 * @brief handler 函数类型
 * @param fd   TCP 连接句柄
 * @param data XML 标签文本内容（明文字符串或 base64 字符串）
 */
typedef bool (*net_handler_fn)(int fd, const char *data);

typedef struct
{
        const char *tag;        ///< XML 标签名
        net_handler_fn handler; ///< 对应的处理函数
} net_route_t;

/**
 * @brief 路由表
 *
 * 新增事件类型只需在此处添加一行，不修改任何其他代码。
 */
static const net_route_t s_routes[] = {
    {"CallInvite", handle_call_invite},
    {"CallRing", handle_call_ring},
    {"CallBusy", handle_call_busy},
    {"CallAccept", handle_call_accept},
    {"CallEnd", handle_call_end},
    {"CallHeartbeatRequest", handle_call_heartbeat_req},
    {"CallHeartbeatResponse", handle_call_heartbeat_resp},
    {"CardScanResponse", handle_card_push},
    {"CardAddResponse", handle_card_push},
    {"CardDelResponse", handle_card_push},
    {"CardDelAllResponse", handle_card_push},
    {"CardQueryResponse", handle_card_push},
    {"CardUpdateAllResponse", handle_card_push}, // 改掩码结果推送
    {"PasswordQueryResponse", handle_password_push},
    {"PasswordAddResponse", handle_password_push},
    {"PasswordDelResponse", handle_password_push},
    {"PasswordSetPermResponse", handle_password_push},
    {"FingerprintScanResponse", handle_fingerprint_push},
    {"FingerprintQueryResponse", handle_fingerprint_push},
    {"FingerprintAddResponse", handle_fingerprint_push},
    {"FingerprintDelResponse", handle_fingerprint_push},
    {"FingerprintClearResponse", handle_fingerprint_push},
    {"FingerprintSetPermResponse", handle_fingerprint_push},
    {"HumanDetect", handle_human_detect},
};
#define ROUTES_COUNT (sizeof(s_routes) / sizeof(s_routes[0]))

// ========================= TCP 数据处理 =========================

#define POST_ONVIF_DEVICE_HTML_TEXT "POST /onvif/device_service"

/**
 * @brief 解析并分发 ONVIF SOAP 请求
 *
 * 重构要点：
 *   原来每个 if/else if 都单独调用一次 mxmlLoadString，
 *   XML 被重复解析十几次。
 *   现在改为：只解析一次，遍历路由表匹配标签，找到即分发。
 */
static bool tcp_dispatch(int fd, const unsigned char *recv_data, int recv_size)
{
        const char *pxml = strstr((const char *)recv_data, "<");
        if (pxml == NULL)
        {
                SAT_DEBUG("tcp_dispatch: no XML found\n%s", recv_data);
                return false;
        }

        // XML 只解析一次
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root == NULL)
        {
                SAT_ERROR("[NET] tcp_dispatch: mxmlLoadString failed");
                return false;
        }

        char *data = (char *)malloc(TCP_RECEIVE_BUFFER_SIZE);
        if (data == NULL)
        {
                SAT_ERROR("[NET] tcp_dispatch: malloc data failed");
                mxmlDelete(root);
                return false;
        }

        bool handled = false;

        // 遍历路由表，找到匹配的标签后分发
        for (int i = 0; i < (int)ROUTES_COUNT; i++)
        {
                if (xml_find_text(root, s_routes[i].tag, data, TCP_RECEIVE_BUFFER_SIZE))
                {
                        SAT_INFO("[dispatch] <%s>", s_routes[i].tag);
                        handled = s_routes[i].handler(fd, data);
                        break;
                }
        }

        if (!handled && strstr((const char *)recv_data, "200 OK") == NULL)
        {
                // 既非已知事件也非 200 OK 应答，打印用于调试
                SAT_DEBUG("[NET] tcp_dispatch: unhandled request\n%s", recv_data);
        }

        free(data);
        mxmlDelete(root);
        return handled;
}

static bool tcp_process(int fd, const unsigned char *recv_data, int recv_size)
{
        if (strncasecmp((const char *)recv_data,
                        POST_ONVIF_DEVICE_HTML_TEXT,
                        strlen(POST_ONVIF_DEVICE_HTML_TEXT)) == 0)
        {
                return tcp_dispatch(fd, recv_data, recv_size);
        }
        return true;
}

// ========================= TCP 连接线程 =========================

/**
 * @brief 每个 TCP 连接的处理线程
 *
 * arg 布局：[int client_fd][unsigned char recv_buf[TCP_RECEIVE_BUFFER_SIZE]]
 */
static void *tcp_conn_thread(void *arg)
{
        unsigned char *buf = (unsigned char *)arg;
        int client_fd = *(int *)buf;
        unsigned char *recv_buf = buf + sizeof(int);

        memset(recv_buf, 0, TCP_RECEIVE_BUFFER_SIZE);

        int read_len = 0;
        int remain_len = TCP_RECEIVE_BUFFER_SIZE;
        int recv_len = 0;

        while ((recv_len = sat_socket_tcp_receive(
                    client_fd, &recv_buf[read_len], remain_len, 200)) > 0)
        {
                read_len += recv_len;
                remain_len -= recv_len;
                if (remain_len <= 0)
                {
                        SAT_ERROR("[NET] tcp_conn_thread: recv buffer full");
                        break;
                }
        }

        if (read_len > 0)
                tcp_process(client_fd, recv_buf, read_len);

        sat_socket_close(client_fd);
        free(buf);

        // 释放连接槽位，允许下一个连接进入
        sem_post(&s_conn_sem);
        return NULL;
}

// ========================= TCP 服务器主循环 =========================

static void *tcp_server_task(void *arg)
{
        (void)arg;
        int server_fd = -1;
        struct sockaddr_in client_addr;

        sat_socket_tcp_open(&server_fd, USER_NETWORK_TCP_PORT, DEVICE_MAX);

        while (1)
        {
                memset(&client_addr, 0, sizeof(client_addr));
                int client_fd = sat_socket_tcp_accept(server_fd, &client_addr, 100);
                if (client_fd < 0)
                        continue;

                // 尝试获取连接槽位，获取失败说明连接数已满，拒绝本次连接
                if (sem_trywait(&s_conn_sem) != 0)
                {
                        SAT_ERROR("[NET] tcp_server: max connections (%d) reached, reject",
                                  TCP_MAX_CONN);
                        sat_socket_close(client_fd);
                        continue;
                }

                // 分配接收缓冲区，布局：[int fd][recv_buf]
                unsigned char *buf = (unsigned char *)malloc(sizeof(int) + TCP_RECEIVE_BUFFER_SIZE);
                if (buf == NULL)
                {
                        SAT_ERROR("[NET] tcp_server: malloc recv buf failed");
                        sat_socket_close(client_fd);
                        sem_post(&s_conn_sem);
                        continue;
                }
                *(int *)buf = client_fd;

                pthread_t tid;
                if (pthread_create(&tid, sat_pthread_attr_get(), tcp_conn_thread, buf) != 0)
                {
                        SAT_ERROR("[NET] tcp_server: pthread_create failed (errno=%d)", errno);
                        sat_socket_close(client_fd);
                        free(buf);
                        sem_post(&s_conn_sem);
                        continue;
                }
                pthread_detach(tid);
        }

        return NULL;
}

// ========================= 本机静态 IP（按设备编号） =========================

/* ========================= 网络命令批处理 =========================
 * 所有命令累计到一条命令后只调用一次 system()，避免每条命令都 fork 一个 shell
 * --------------------------------------------------------------------- */
static char s_net_cmd[512];
static size_t s_net_cmd_len;

static void net_cmd_reset(void)
{
        s_net_cmd[0] = '\0';
        s_net_cmd_len = 0;
}

static void net_cmd_add(const char *fmt, ...)
{
        if (s_net_cmd_len > 0)
        {
                if (s_net_cmd_len + 2 >= sizeof(s_net_cmd))
                        return;
                strcat(s_net_cmd, "; ");
                s_net_cmd_len += 2;
        }

        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf(s_net_cmd + s_net_cmd_len, sizeof(s_net_cmd) - s_net_cmd_len, fmt, ap);
        va_end(ap);
        if (n > 0)
        {
                s_net_cmd_len += n;
                if (s_net_cmd_len >= sizeof(s_net_cmd))
                        s_net_cmd_len = sizeof(s_net_cmd) - 1;
        }
}

static void net_cmd_exec(const char *tag)
{
        SAT_INFO("[NET] %s: %s", tag, s_net_cmd);
        system(s_net_cmd);
        net_cmd_reset();
}

static void user_network_setup_bridge(void)
{
        net_cmd_add("ip link add name %s type bridge", USER_NETWORK_BRIDGE_IFACE);
        net_cmd_add("ip link set %s master %s", USER_NETWORK_BRIDGE_PORT0, USER_NETWORK_BRIDGE_IFACE);
        net_cmd_add("ip link set %s master %s", USER_NETWORK_BRIDGE_PORT1, USER_NETWORK_BRIDGE_IFACE);
        net_cmd_add("ip link set %s up", USER_NETWORK_BRIDGE_PORT0);
        net_cmd_add("ip link set %s up", USER_NETWORK_BRIDGE_PORT1);
        net_cmd_add("ip link set %s up", USER_NETWORK_BRIDGE_IFACE);
}

/**
 * @brief 根据 private_cfg.device 的栋/房/分机调用 generate_ip，配置本机静态 IPv4
 */
void user_network_apply_device_static_ip(void)
{
        device_attr_info *attr = &user_data_get()->private_cfg.device.attr;
        char ipbuf[20] = {0};

        if (generate_ip(attr->building, attr->room, attr->device_type, ipbuf) != 0)
        {
                SAT_ERROR("[NET] static ip: generate_ip failed (building=%d room=%d device_type=%d)",
                          attr->building, attr->room, attr->device_type);
                return;
        }

        net_cmd_add("ifconfig %s %s netmask %s up",
                    USER_NETWORK_LAN_IFACE, ipbuf, USER_NETWORK_STATIC_NETMASK);
}

/**
 * @brief 立即重新应用静态 IP（不入队、直接执行），
 *        供设备号/房号变更后无需重启设备时调用
 */
void user_network_apply_device_static_ip_now(void)
{
        net_cmd_reset();
        user_network_apply_device_static_ip();
        net_cmd_exec("static ip re-apply");
}

// ========================= 组播路由 =========================
/* -----------------------------------------------------------------------
 * 完整版：添加 所有组播地址 + 所有广播地址 路由
 * --------------------------------------------------------------------- */
static void add_multicast_routes(void)
{
        // 1. 全覆盖 所有组播地址（224.0.0.0 ~ 239.255.255.255）
        net_cmd_add("route add -net 224.0.0.0 netmask 240.0.0.0 %s", USER_NETWORK_LAN_IFACE);
        // 2. 全网广播地址（255.255.255.255）
        net_cmd_add("route add -net 255.255.255.255 netmask 255.255.255.255 %s", USER_NETWORK_LAN_IFACE);
}

/*==========================================================================
 * 人形侦测组播常驻监听
 *==========================================================================*/

static volatile int s_hd_mcast_run;                               /**< 监听线程运行标志 */
static pthread_t s_hd_mcast_tid;                                  /**< 监听线程ID */
static pthread_mutex_t s_hd_mcast_mx = PTHREAD_MUTEX_INITIALIZER; /**< 线程安全互斥锁 */
static int s_hd_mcast_sock = -1;                                  /**< 监听socket */
static char s_hd_mcast_ip[20];                                    /**< 当前监听的组播IP */

/**
 * @brief  根据楼栋号构建人形侦测组播IP
 * @note   组播地址规则：239.楼栋.255.255（整栋楼全局广播）
 */
static void hd_mcast_build_ip(int building, char *out, size_t out_len)
{
        if (!out || out_len < 16)
                return;
        snprintf(out, out_len, "239.%d.255.255", building & 0xFF);
}

/**
 * @brief  关闭监听socket（线程安全，需已加锁）
 */
static void hd_mcast_close_sock_locked(void)
{
        if (s_hd_mcast_sock >= 0)
        {
                sat_socket_close(s_hd_mcast_sock);
                s_hd_mcast_sock = -1;
        }
        s_hd_mcast_ip[0] = '\0';
}

/**
 * @brief  确保加入正确的组播组（线程安全，需已加锁）
 */
static bool hd_mcast_ensure_join_locked(int building)
{
        char mcast[20];
        hd_mcast_build_ip(building, mcast, sizeof(mcast));

        /* 组播未变化，直接返回 */
        if (s_hd_mcast_sock >= 0 && strcmp(mcast, s_hd_mcast_ip) == 0)
                return true;

        /* 组播变化，重建socket */
        hd_mcast_close_sock_locked();

        /* 绑到组播组地址而非 0.0.0.0，避免收走同端口(3702)其它组播组
         * (如 IPC 搜索的 WS-Discovery 239.255.255.250) 的报文 */
        if (!sat_socket_udp_open_bind(&s_hd_mcast_sock, mcast, HUMAN_DETECT_MCAST_PORT, false) || s_hd_mcast_sock < 0)
        {
                SAT_ERROR("[NET-HD] udp open failed");
                return false;
        }

        if (!sat_socket_multicast_join(s_hd_mcast_sock, mcast))
        {
                SAT_ERROR("[NET-HD] multicast join %s failed", mcast);
                sat_socket_close(s_hd_mcast_sock);
                s_hd_mcast_sock = -1;
                return false;
        }

        strncpy(s_hd_mcast_ip, mcast, sizeof(s_hd_mcast_ip) - 1);
        s_hd_mcast_ip[sizeof(s_hd_mcast_ip) - 1] = '\0';
        return true;
}

/**
 * @brief  人形侦测组播常驻监听线程
 * @note   监听 239.building.255.255，接收室外机侦测包并投递 LVGL 事件
 */
static void *hd_mcast_listener_thread(void *arg)
{
        (void)arg;
        pthread_detach(pthread_self());

        int building = user_data_get()->private_cfg.device.attr.building;

        while (s_hd_mcast_run)
        {
                /* 无效楼栋，关闭socket并等待配置生效 */
                if (building < 1 || building > 255)
                {
                        pthread_mutex_lock(&s_hd_mcast_mx);
                        hd_mcast_close_sock_locked();
                        pthread_mutex_unlock(&s_hd_mcast_mx);
                        usleep(500 * 1000);
                        building = user_data_get()->private_cfg.device.attr.building;
                        continue;
                }

                /* 确保加入正确组播 */
                pthread_mutex_lock(&s_hd_mcast_mx);
                if (!hd_mcast_ensure_join_locked(building))
                {
                        pthread_mutex_unlock(&s_hd_mcast_mx);
                        usleep(1000 * 1000);
                        building = user_data_get()->private_cfg.device.attr.building;
                        continue;
                }
                int sock = s_hd_mcast_sock;
                pthread_mutex_unlock(&s_hd_mcast_mx);

                /* 阻塞接收 XML 数据（400ms超时，超时后刷新building并检查运行标志） */
                char rbuf[HD_MCAST_RECV_BUF_SIZE];
                struct sockaddr_in from;
                int n = sat_socket_udp_receive(sock, rbuf, sizeof(rbuf) - 1, &from, 400);
                if (n <= 0)
                {
                        building = user_data_get()->private_cfg.device.attr.building;
                        continue;
                }
                rbuf[n] = '\0';

                /* 通过 XML 分发流程处理（fd=-1 表示组播，无需 TCP 回复） */
                tcp_dispatch(-1, (const unsigned char *)rbuf, n);
        }

        /* 线程退出，清理资源 */
        pthread_mutex_lock(&s_hd_mcast_mx);
        hd_mcast_close_sock_locked();
        pthread_mutex_unlock(&s_hd_mcast_mx);
        return NULL;
}

/**
 * @brief  启动人形侦测组播常驻监听（全局只允许启动一次）
 */
static void human_detect_mcast_listener_start(void)
{
        static bool s_once;
        if (s_once)
                return;
        s_once = true;

        s_hd_mcast_run = 1;

        if (pthread_create(&s_hd_mcast_tid, sat_pthread_attr_get(), hd_mcast_listener_thread, NULL) != 0)
        {
                SAT_ERROR("[NET-HD] pthread_create failed");
                s_hd_mcast_run = 0;
                s_once = false;
                return;
        }
}

// ========================= 初始化入口 =========================

/**
 * @brief 室内机网络模块初始化
 *
 * 1. 按设备配置设置本机静态 IP（generate_ip）
 * 2. 添加组播路由
 * 3. 初始化连接池信号量
 * 4. 启动 TCP 服务器线程
 * 5. 启动人形侦测组播监听（239.building.255.255:37212，接收室外机侦测结果）
 */
void user_network_init(void)
{
        net_cmd_reset();
        user_network_setup_bridge();

        user_network_apply_device_static_ip();

        add_multicast_routes();

        net_cmd_exec("network setup");

        // 初始化连接池信号量，最多允许 TCP_MAX_CONN 个并发连接
        sem_init(&s_conn_sem, 0, TCP_MAX_CONN);

        pthread_t tid;
        pthread_create(&tid, sat_pthread_attr_get(), tcp_server_task, NULL);
        pthread_detach(tid);

        human_detect_mcast_listener_start();
}

#define WIRED_IFACE "eth2"
#define WLAN_IFACE "wlan0"
#define WIRED_STATIC_IP "192.168.188.1"

void user_network_pairing_init(void)
{
        user_data_info *info = user_data_get();

        if (info->public_cfg.pairing_mode == PAIRING_MODE_WLAN)
        {
                SAT_INFO("[NET] pairing mode: EZ Mode (WiFi)");
                system("ifconfig " WIRED_IFACE " down");
                system("ifconfig " WLAN_IFACE " up");
        }
#if NET_SUPPORT_CABLE
        else if (info->public_cfg.pairing_mode == PAIRING_MODE_WIRED)
        {
                SAT_INFO("[NET] pairing mode: Cable (Wired)");
                system("ifconfig " WIRED_IFACE " up");
                system("ifconfig " WLAN_IFACE " down");
                system("killall udhcpc");
                usleep(1000);

                if (info->public_cfg.allocation_mode == IP_ALLOC_DHCP)
                {
                        system("udhcpc -i " WIRED_IFACE " &");
                        SAT_INFO("[NET] Cable DHCP on %s", WIRED_IFACE);
                }
                else
                {
                        system("ifconfig " WIRED_IFACE " " WIRED_STATIC_IP);
                        SAT_INFO("[NET] Cable static IP: %s on %s", WIRED_STATIC_IP, WIRED_IFACE);
                }

                system("route add -net 224.0.0.0 netmask 224.0.0.0 " WIRED_IFACE);
        }
#endif
}
