

#ifndef __SAT_USER_COMMON_H__
#define __SAT_USER_COMMON_H__
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#define SAT_DEBUG(x...)                                         \
        printf("\n[%s:%d]\n\033[41;37;1m", __func__, __LINE__); \
        printf(x);                                              \
        printf("\033[0m\n");

#define SAT_ERROR(x...)                                         \
        printf("\n[%s:%d]\n\033[41;37;1m", __func__, __LINE__); \
        printf(x);                                              \
        printf("\033[0m\n");

#define SAT_WARN(x...)                                          \
        printf("\n[%s:%d]\n\033[41;37;1m", __func__, __LINE__); \
        printf(x);                                              \
        printf("\033[0m\n");

#define SAT_INFO(x...)                                          \
        printf("\n[%s:%d]\n\033[41;37;1m", __func__, __LINE__); \
        printf(x);                                              \
        printf("\033[0m\n");
/**
 * @defgroup 通用工具模块
 * @brief 线程/文件/网卡/Socket/进程/字符串通用工具
 * @{
 */

/**
 * @brief 获取线程属性（设置栈大小）
 * @return 线程属性指针
 */
pthread_attr_t *sat_pthread_attr_get(void);

/**
 * @brief 获取文件大小
 * @param path 文件路径
 * @return 文件大小，失败返回0
 */
size_t sat_file_size_get(const char *path);

/**
 * @brief 读取文件内容
 * @param path 文件路径
 * @param data 接收缓存
 * @param size 最大读取长度
 * @return 实际读取大小
 */
size_t sat_file_read(const char *path, char *data, size_t size);

/**
 * @brief 获取网卡IP、MAC、子网掩码
 * @param eth 网卡名(eth0)
 * @param ip 接收IP
 * @param mac 接收MAC
 * @param mask 接收子网掩码
 * @return 成功true
 */
bool sat_ip_mac_addres_get(const char *eth, char *ip, char *mac, char *mask);

/**
 * @brief 解析 /proc/net/route 默认网关行
 * @param line 路由行
 * @param eth 网卡名
 * @param gateway 接收默认网关
 * @param size 接收缓存大小
 * @return 成功true
 */
bool sat_network_default_gateway_parse(const char *line, const char *eth, char *gateway, int size);

/**
 * @brief 获取默认网关
 * @param eth 网卡名
 * @param gateway 接收默认网关
 * @param size 接收缓存大小
 * @return 成功true
 */
bool sat_network_default_gateway_get(const char *eth, char *gateway, int size);

/**
 * @brief 调用udhcpc自动获取IP
 * @param eth 网卡名
 */
void sat_network_udhcpc_ip(const char *eth);

/**
 * @brief 写入eth0 MAC地址
 * @param mac MAC地址
 * @return 成功true
 */
bool sat_network_eth0_mac_write(const char *mac);

/**
 * @brief 写入eth1 MAC地址
 * @param mac MAC地址
 * @return 成功true
 */
bool sat_network_eth1_mac_write(const char *mac);

/**
 * @brief 网卡up/down
 * @param eth 网卡名
 * @param en true=up false=down
 */
void sat_network_enable(const char *eth, bool en);

/**
 * @brief 关闭socket
 * @param socket_fd 文件描述符
 * @return 成功true
 */
bool sat_socket_close(int socket_fd);

/**
 * @brief 加入组播
 * @param socket_fd socket
 * @param multicast_ip 组播IP
 * @return 成功true
 */
bool sat_socket_multicast_join(int socket_fd, const char *multicast_ip);

/**
 * @brief 通过指定网卡加入组播并设置组播发送出口
 * @param socket_fd socket
 * @param multicast_ip 组播IP
 * @param iface 网卡名
 * @return 成功true
 */
bool sat_socket_multicast_join_iface(int socket_fd, const char *multicast_ip, const char *iface);

/**
 * @brief 创建TCP Server
 * @param fd 输出server fd
 * @param port 监听端口
 * @param max_client 最大连接数
 * @return 成功true
 */
bool sat_socket_tcp_open(int *fd, int port, int max_client);

/**
 * @brief TCP接受连接（带超时）
 * @param server_fd server fd
 * @param client_addr 客户端地址
 * @param timeout_ms 超时时间
 * @return client fd
 */
int sat_socket_tcp_accept(int server_fd, struct sockaddr_in *client_addr, unsigned int timeout_ms);

/**
 * @brief TCP连接服务器（带超时）
 * @param socket_fd client fd
 * @param server_ip 服务器IP
 * @param port 端口
 * @param wait_ms 超时
 * @return 成功true
 */
bool sat_socket_tcp_connect(int socket_fd, const char *server_ip, int port, unsigned int wait_ms);

/**
 * @brief TCP接收数据（带超时）
 * @param client_fd client fd
 * @param data 接收缓存
 * @param data_len 最大长度
 * @param timeout_ms 超时
 * @return 实际接收长度
 */
int sat_socket_tcp_receive(int client_fd, unsigned char *data, int data_len, int timeout_ms);

/**
 * @brief TCP发送数据（带超时+分包）
 * @param socket_fd fd
 * @param data 数据
 * @param data_len 长度
 * @param timeout_ms 超时
 * @return 成功true
 */
bool sat_socket_tcp_send(int socket_fd, unsigned char *data, int data_len, unsigned int timeout_ms);

/**
 * @brief 创建UDP Socket
 * @param socket_fd 输出fd
 * @param port 端口
 * @param broadcast 是否允许广播
 * @return 成功true
 */
bool sat_socket_udp_open(int *socket_fd, int port, bool broadcast);

/**
 * @brief 创建UDP Socket并绑定到指定本地地址
 * @param socket_fd 输出fd
 * @param bind_ip 本地绑定地址（接收组播时传组播组地址，如 239.x.x.x）
 * @param port 端口
 * @param broadcast 是否允许广播
 * @return 成功true
 */
bool sat_socket_udp_open_bind(int *socket_fd, const char *bind_ip, int port, bool broadcast);

/**
 * @brief UDP发送数据
 * @param socket_fd fd
 * @param data 数据
 * @param data_len 长度
 * @param ip 目标IP
 * @param port 目标端口
 * @param timeout_ms 超时
 * @return 成功true
 */
bool sat_socket_udp_send(int socket_fd, const char *data, size_t data_len, const char *ip, int port, int timeout_ms);

/**
 * @brief UDP接收数据
 * @param socket_fd fd
 * @param data 接收缓存
 * @param data_len 最大长度
 * @param client_addr 源地址
 * @param timeout_ms 超时
 * @return 实际接收长度
 */
int sat_socket_udp_receive(int socket_fd, char *data, size_t data_len, struct sockaddr_in *client_addr, int timeout_ms);

/**
 * @brief 判断进程是否存在
 * @param process_name 进程名
 * @return 存在true
 */
bool sat_user_common_process_exists(const char *process_name);

/**
 * @brief 强制杀死进程
 * @param process_name 进程名
 * @return 成功true
 */
bool sat_kill_task_process(const char *process_name);
/**
 * @brief 计算目标值相对于1左移的位数
 * @param target 目标值
 * @param base 基础值
 * @return 左移位数（失败返回-1）
 */
int get_shift_bits(uint32_t target, uint32_t base);

/**
 * @brief 根据栋号、房号、设备号生成IP
 * @param buildingNo 栋号
 * @param roomNo 房号
 * @param deviceNo 设备号
 * @param ipBuffer 输出IP
 * @return 0成功，-1参数错误
 */
/**
 * @brief 根据栋号、房号、设备号生成IP(位拆分方案)
 * @param buildingNo  栋号 1~254
 * @param roomNo      房号 格式：楼层*100+房间，例 101(1楼01),1208(12楼08),2015(20楼15)
 * @param deviceNo    设备号 1~10
 * @param ipBuffer    输出IP 缓冲区至少16字节
 * @return 0成功，-1参数错误
 */
int generate_ip(int buildingNo, int roomNo, int deviceNo, char *ipBuffer);
/**
 * @brief 字节数组转十六进制字符串
 * @param hex_str 输出字符串
 * @param card_data 输入字节数组
 * @param len 数组长度
 * @return 输出字符串指针
 */
char *uint8_to_hex_str(char *hex_str, uint8_t *card_data, int len);

/**
 * @brief 十六进制字符串转字节数组
 * @param hex_str 输入字符串
 * @param out_card 输出数组
 * @param len 数组长度
 * @return 0成功，-1失败
 */
int hex_str_to_uint8(const char *hex_str, uint8_t *out_card, int len);

/**
 * @brief 获取指定网口的链路质量等级
 * @param dev_id 网口号（0=eth0，1=eth1）
 * @return 静态字符串：质量分数+等级
 */
const char *network_device_link_level(int dev_id);

/**
 * @brief 计算文件的MD5值
 * @param path 文件路径
 * @param out_md5_hex 输出32位小写十六进制MD5字符串（需33字节空间）
 * @return 成功true
 */
bool file_md5_get(const char *path, char *out_md5_hex);
/**
 * @}
 */
#endif