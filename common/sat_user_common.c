#include "sat_user_common.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#include <ctype.h>
#include "device_types.h"
#include "md5.h"
#define NETWORK_PACKAET_MAX (100 * 1024)

/**
 * @brief 获取线程属性（设置栈大小）
 * @return 线程属性指针
 */
pthread_attr_t *sat_pthread_attr_get(void)
{
        static pthread_attr_t thread_attr;
        static bool is_init = false;
        size_t stacksize = 200 * 1024;

        if (!is_init)
        {
                pthread_attr_init(&thread_attr);
                pthread_attr_setstacksize(&thread_attr, stacksize);
                is_init = true;
        }
        return &thread_attr;
}

/**
 * @brief 获取文件大小
 * @param path 文件路径
 * @return 文件大小，失败返回0
 */
size_t sat_file_size_get(const char *path)
{
        struct stat st;

        if (path == NULL)
        {
                return 0;
        }

        if (stat(path, &st) != 0)
        {
                printf("read file size failed, (%s) errno:%d\n", path, errno);
                return 0;
        }

        return st.st_size;
}

/**
 * @brief 读取文件内容
 * @param path 文件路径
 * @param data 接收缓存
 * @param size 最大读取长度
 * @return 实际读取大小
 */
size_t sat_file_read(const char *path, char *data, size_t size)
{
        int fd;
        size_t ret;

        if (path == NULL || data == NULL || size == 0)
        {
                return 0;
        }

        fd = open(path, O_RDONLY);
        if (fd < 0)
        {
                printf("open %s failed\n", path);
                return 0;
        }

        ret = read(fd, data, size);
        close(fd);
        return ret;
}

/**
 * @brief 获取网卡IP、MAC、子网掩码
 * @param eth 网卡名(eth0)
 * @param ip 接收IP
 * @param mac 接收MAC
 * @param mask 接收子网掩码
 * @return 成功true
 */
bool sat_ip_mac_addres_get(const char *eth, char *ip, char *mac, char *mask)
{
        int sock;
        bool reslut = false;
        struct sockaddr_in sin;
        struct ifreq ifr;
        sock = socket(AF_INET, SOCK_DGRAM, 0);

        if (sock == -1)
        {
                close(sock);
                printf("Error:get local IP socket fail!\n");
                return false;
        }

        strncpy(ifr.ifr_name, eth, IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = 0;

        if (ip != NULL)
        {
                if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
                {
                        close(sock);
                        printf("[%s:%d]%s Error:get local IP ioctl fail! \n", __func__, __LINE__, eth);
                        return false;
                }

                memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
                sprintf(ip, "%s", inet_ntoa(sin.sin_addr));
                reslut = true;
        }

        if (mask != NULL)
        {
                if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
                {
                        close(sock);
                        //    printf("Error:get local IP ioctl fail! \n");
                        return false;
                }

                memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
                sprintf(mask, "%s", inet_ntoa(sin.sin_addr));
                reslut = true;
        }

        if (mac != NULL)
        {

                if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
                {
                        close(sock);
                        //  perror("Error:get local mac ioctl fail! \n");
                        return false;
                }

                struct sockaddr sa;
                memcpy(&sa, &ifr.ifr_addr, sizeof(struct sockaddr_in));
                sprintf((char *)mac, "%02X:%02X:%02X:%02X:%02X:%02X", sa.sa_data[0], sa.sa_data[1], sa.sa_data[2], sa.sa_data[3], sa.sa_data[4], sa.sa_data[5]);
                reslut = true;
        }

        close(sock);
        return reslut;
}

bool sat_network_default_gateway_parse(const char *line, const char *eth, char *gateway, int size)
{
        char iface[IFNAMSIZ] = {0};
        unsigned int destination = 0;
        unsigned int gateway_hex = 0;
        unsigned int flags = 0;
        unsigned int mask = 0;
        struct in_addr addr;

        if (line == NULL || eth == NULL || gateway == NULL || size <= 0)
        {
                return false;
        }

        if (sscanf(line, "%15s %x %x %x %*d %*d %*d %x", iface, &destination, &gateway_hex, &flags, &mask) != 5)
        {
                return false;
        }

        if (strcmp(iface, eth) != 0 || destination != 0 || mask != 0 || (flags & 0x3) != 0x3)
        {
                return false;
        }

        addr.s_addr = gateway_hex;
        snprintf(gateway, size, "%s", inet_ntoa(addr));
        return true;
}

bool sat_network_default_gateway_get(const char *eth, char *gateway, int size)
{
        FILE *fp;
        char line[256] = {0};

        if (eth == NULL || gateway == NULL || size <= 0)
        {
                return false;
        }

        fp = fopen("/proc/net/route", "r");
        if (fp == NULL)
        {
                return false;
        }

        while (fgets(line, sizeof(line), fp) != NULL)
        {
                if (sat_network_default_gateway_parse(line, eth, gateway, size))
                {
                        fclose(fp);
                        return true;
                }
        }

        fclose(fp);
        return false;
}

/**
 * @brief 调用udhcpc自动获取IP
 * @param eth 网卡名
 */
void sat_network_udhcpc_ip(const char *eth)
{
        char cmd[256] = {0};

        if (eth == NULL)
        {
                return;
        }

        snprintf(cmd, sizeof(cmd), "udhcpc");
        sat_kill_task_process(cmd);
        snprintf(cmd, sizeof(cmd), "udhcpc -i %s &", eth);
        system(cmd);
}

/**
 * @brief 网卡up/down
 * @param eth 网卡名
 * @param en true=up false=down
 */
void sat_network_enable(const char *eth, bool en)
{
        char cmd[256] = {0};

        if (eth == NULL)
        {
                return;
        }

        snprintf(cmd, sizeof(cmd), "ifconfig %s %s", eth, en ? "up" : "down");
        system(cmd);
}

/**
 * @brief 判断进程是否存在
 * @param process_name 进程名
 * @return 存在true
 */
bool sat_user_common_process_exists(const char *process_name)
{
        char cmd[256];
        FILE *pipe;
        bool found = false;
        char buffer[256];

        if (process_name == NULL)
        {
                return false;
        }

        snprintf(cmd, sizeof(cmd), "ps aux | grep -v grep | grep -F '%s'", process_name);
        pipe = popen(cmd, "r");
        if (pipe == NULL)
        {
                perror("popen");
                return false;
        }

        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
        {
                if (strstr(buffer, process_name) != NULL)
                {
                        found = true;
                        break;
                }
        }

        pclose(pipe);
        return found;
}

/**
 * @brief 强制杀死进程
 * @param process_name 进程名
 * @return 成功true
 */
bool sat_kill_task_process(const char *process_name)
{
#define MAX_BUFF_SIZE 1024
        bool result = false;
        char buffer[MAX_BUFF_SIZE];
        char cmd[128] = {0};
        FILE *pipe;

        if (process_name == NULL)
        {
                return false;
        }

        snprintf(cmd, sizeof(cmd), "ps aux | grep -v grep | grep -F \"%s\"", process_name);
        pipe = popen(cmd, "r");
        if (pipe == NULL)
        {
                return false;
        }

        while (fgets(buffer, MAX_BUFF_SIZE, pipe) != NULL)
        {
                if (strstr(buffer, process_name) != NULL)
                {
                        int pid = 0;
                        sscanf(buffer, "%d", &pid);
                        snprintf(cmd, sizeof(cmd), "kill -s 9 %d", pid);
                        system(cmd);
                        printf("kill %s pid:%d\n", process_name, pid);
                        result = true;
                }
        }

        pclose(pipe);
        return result;
}

/**
 * @brief 设置socket阻塞模式
 * @param fd socket
 * @param en true=阻塞
 * @return 成功true
 */
static bool sat_socket_block_enable(int fd, bool en)
{
        int flags;

        if (fd < 0)
        {
                return false;
        }

        flags = fcntl(fd, F_GETFL);
        if (flags == -1)
        {
                printf("[%s:%d] fcntl error\n", __func__, __LINE__);
                return false;
        }

        if (en == true)
        {
                flags &= ~O_NONBLOCK;
        }
        else
        {
                flags |= O_NONBLOCK;
        }

        if (fcntl(fd, F_SETFL, flags) == -1)
        {
                printf("[%s:%d] fcntl error\n", __func__, __LINE__);
                return false;
        }

        return true;
}

/**
 * @brief 内部创建socket并绑定到指定本地地址
 * @param socket_type SOCK_STREAM/SOCK_DGRAM
 * @param port 端口
 * @param addr 本地绑定地址（可传组播组地址，如 239.x.x.x）
 * @return fd
 */
static int sat_socket_open_addr(int socket_type, int port, uint32_t addr)
{
        struct sockaddr_in addr_in;
        int err;
        int optval = 1;
        unsigned char loop = 0;
        int sock;

        sock = socket(AF_INET, socket_type, 0);
        if (sock < 0)
        {
                printf("socket create error\n");
                return -1;
        }

        err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (err < 0)
        {
                perror("SO_REUSEADDR");
        }

        err = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if (err < 0)
        {
                perror("SO_REUSEPORT");
        }

        memset(&addr_in, 0, sizeof(addr_in));
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        addr_in.sin_addr.s_addr = addr;

        if (bind(sock, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0)
        {
                perror("bind");
                close(sock);
                return -1;
        }

        err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (err < 0)
        {
                perror("IP_MULTICAST_LOOP");
        }

        return sock;
}

/**
 * @brief 内部创建socket
 * @param socket_type SOCK_STREAM/SOCK_DGRAM
 * @param port 端口
 * @return fd
 */
static int sat_socket_open(int socket_type, int port)
{
        return sat_socket_open_addr(socket_type, port, htonl(INADDR_ANY));
}

/**
 * @brief 关闭socket
 * @param socket_fd 文件描述符
 * @return 成功true
 */
bool sat_socket_close(int socket_fd)
{
        if (socket_fd < 0)
        {
                return false;
        }

        close(socket_fd);
        return true;
}

/**
 * @brief 加入组播
 * @param socket_fd socket
 * @param multicast_ip 组播IP
 * @return 成功true
 */
bool sat_socket_multicast_join(int socket_fd, const char *multicast_ip)
{
        struct ip_mreq mreq;
        int err;

        if (socket_fd < 0 || multicast_ip == NULL)
        {
                return false;
        }

        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        err = setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if (err < 0)
        {
                perror("IP_ADD_MEMBERSHIP");
                return false;
        }

        return true;
}

bool sat_socket_multicast_join_iface(int socket_fd, const char *multicast_ip, const char *iface)
{
        struct ip_mreq mreq;
        struct ifreq ifr;
        struct sockaddr_in *addr;
        int sock;

        if (socket_fd < 0 || multicast_ip == NULL || iface == NULL)
        {
                return false;
        }

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
        {
                perror("multicast iface socket");
                return false;
        }

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
        if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
        {
                perror("SIOCGIFADDR");
                close(sock);
                return false;
        }
        close(sock);

        addr = (struct sockaddr_in *)&ifr.ifr_addr;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = addr->sin_addr.s_addr;

        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        {
                perror("IP_ADD_MEMBERSHIP");
                return false;
        }

        if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF, &addr->sin_addr, sizeof(addr->sin_addr)) < 0)
        {
                perror("IP_MULTICAST_IF");
                return false;
        }

        return true;
}

/**
 * @brief select 等待读写
 * @param fd socket
 * @param wait_ms 超时
 * @param read_flg 读检测
 * @param write_flg 写检测
 * @return 就绪true
 */
static bool sat_socket_select(int fd, unsigned int wait_ms, bool read_flg, bool write_flg)
{
        fd_set read_fdset;
        fd_set write_fdset;
        struct timeval timeout;
        int ret;

        if (fd < 0)
        {
                return false;
        }

        if (wait_ms <= 0)
        {
                return true;
        }

        FD_ZERO(&read_fdset);
        FD_ZERO(&write_fdset);

        if (read_flg)
        {
                FD_SET(fd, &read_fdset);
        }
        if (write_flg)
        {
                FD_SET(fd, &write_fdset);
        }

        timeout.tv_sec = wait_ms / 1000;
        timeout.tv_usec = (wait_ms % 1000) * 1000;

        do
        {
                ret = select(fd + 1,
                             read_flg ? &read_fdset : NULL,
                             write_flg ? &write_fdset : NULL,
                             NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        return ret > 0;
}

/**
 * @brief 创建TCP Server
 * @param fd 输出server fd
 * @param port 监听端口
 * @param max_client 最大连接数
 * @return 成功true
 */
bool sat_socket_tcp_open(int *fd, int port, int max_client)
{
        int tcp_fd;
        int err;

        if (fd == NULL)
        {
                return false;
        }

        tcp_fd = sat_socket_open(SOCK_STREAM, port);
        if (tcp_fd < 0)
        {
                printf("socket open failed\n");
                return false;
        }

        if (max_client > 0)
        {
                err = listen(tcp_fd, max_client);
                if (err < 0)
                {
                        close(tcp_fd);
                        printf("listen error\n");
                        return false;
                }
        }

        *fd = tcp_fd;
        return true;
}

/**
 * @brief TCP接受连接（带超时）
 * @param server_fd server fd
 * @param client_addr 客户端地址
 * @param timeout_ms 超时时间
 * @return client fd
 */
int sat_socket_tcp_accept(int server_fd, struct sockaddr_in *client_addr, unsigned int timeout_ms)
{
        int client_fd;
        socklen_t len;
        int ret;
        fd_set accept_fdset;
        struct timeval timeout;

        if (server_fd < 0 || client_addr == NULL)
        {
                return -1;
        }

        if (timeout_ms > 0)
        {
                FD_ZERO(&accept_fdset);
                FD_SET(server_fd, &accept_fdset);

                timeout.tv_sec = timeout_ms / 1000;
                timeout.tv_usec = (timeout_ms % 1000) * 1000;

                do
                {
                        ret = select(server_fd + 1, &accept_fdset, NULL, NULL, &timeout);
                } while (ret < 0 && errno == EINTR);

                if (ret <= 0)
                {
                        return -1;
                }
        }

        len = sizeof(struct sockaddr_in);
        client_fd = accept(server_fd, (struct sockaddr *)client_addr, &len);
        if (client_fd < 0)
        {
                printf("accept failed\n");
                return -1;
        }

        printf("client connect: %s:%d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        return client_fd;
}

/**
 * @brief TCP连接服务器（带超时）
 * @param socket_fd client fd
 * @param server_ip 服务器IP
 * @param port 端口
 * @param wait_ms 超时
 * @return 成功true
 */
bool sat_socket_tcp_connect(int socket_fd, const char *server_ip, int port, unsigned int wait_ms)
{
        int result;
        bool res = false;
        struct sockaddr_in ser_addr;
        fd_set connect_fdset;
        struct timeval timeout;
        int err;
        socklen_t socklen;

        if (socket_fd < 0 || server_ip == NULL)
        {
                return false;
        }

        if (wait_ms > 0)
        {
                sat_socket_block_enable(socket_fd, false);
        }

        memset(&ser_addr, 0, sizeof(ser_addr));
        ser_addr.sin_family = AF_INET;
        ser_addr.sin_port = htons(port);
        ser_addr.sin_addr.s_addr = inet_addr(server_ip);

        result = connect(socket_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
        if (result == 0)
        {
                res = true;
                goto finish;
        }

        if (errno == EINPROGRESS)
        {
                FD_ZERO(&connect_fdset);
                FD_SET(socket_fd, &connect_fdset);

                timeout.tv_sec = wait_ms / 1000;
                timeout.tv_usec = (wait_ms % 1000) * 1000;

                do
                {
                        result = select(socket_fd + 1, NULL, &connect_fdset, NULL, &timeout);
                } while (result < 0 && errno == EINTR);

                if (result == 1)
                {
                        socklen = sizeof(err);
                        getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
                        if (err == 0)
                        {
                                res = true;
                        }
                }
        }

finish:
        if (wait_ms > 0)
        {
                sat_socket_block_enable(socket_fd, true);
        }

        return res;
}

/**
 * @brief TCP接收数据（带超时）
 * @param client_fd client fd
 * @param data 接收缓存
 * @param data_len 最大长度
 * @param timeout_ms 超时
 * @return 实际接收长度
 */
int sat_socket_tcp_receive(int client_fd, unsigned char *data, int data_len, int timeout_ms)
{
        int len;

        if (client_fd < 0 || data == NULL || data_len <= 0)
        {
                return -1;
        }

        if (!sat_socket_select(client_fd, timeout_ms, true, false))
        {
                return -2;
        }

        len = recv(client_fd, data, data_len, 0);
        if (len == 0)
        {
                printf("client closed\n");
                return 0;
        }
        if (len < 0)
        {
                printf("recv error:%d\n", errno);
                return -1;
        }

        return len;
}

/**
 * @brief TCP发送数据（带超时+分包）
 * @param socket_fd fd
 * @param data 数据
 * @param data_len 长度
 * @param timeout_ms 超时
 * @return 成功true
 */
bool sat_socket_tcp_send(int socket_fd, unsigned char *data, int data_len, unsigned int timeout_ms)
{
        int result;
        int send_len = 0;
        int packet_size;

        if (socket_fd < 0 || data == NULL || data_len <= 0)
        {
                return false;
        }

        while (send_len < data_len)
        {
                if (!sat_socket_select(socket_fd, timeout_ms, false, true))
                {
                        return false;
                }

                packet_size = data_len - send_len;
                if (packet_size > NETWORK_PACKAET_MAX)
                {
                        packet_size = NETWORK_PACKAET_MAX;
                }

                result = send(socket_fd, &data[send_len], packet_size, MSG_NOSIGNAL);
                if (result <= 0)
                {
                        return false;
                }

                send_len += result;
        }

        return true;
}

/**
 * @brief 创建UDP Socket
 * @param socket_fd 输出fd
 * @param port 端口
 * @param broadcast 是否允许广播
 * @return 成功true
 */
bool sat_socket_udp_open(int *socket_fd, int port, bool broadcast)
{
        int fd;
        int optval = 1;

        if (socket_fd == NULL)
        {
                return false;
        }

        fd = sat_socket_open(SOCK_DGRAM, port);
        if (fd < 0)
        {
                return false;
        }

        if (broadcast)
        {
                setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
        }

        *socket_fd = fd;
        return true;
}

/**
 * @brief 创建UDP Socket并绑定到指定本地地址
 * @param socket_fd 输出fd
 * @param bind_ip 本地绑定地址（接收组播时传组播组地址，如 239.x.x.x）
 * @param port 端口
 * @param broadcast 是否允许广播
 * @return 成功true
 * @note 与 sat_socket_udp_open 绑 0.0.0.0 不同：绑到具体组播组地址后，
 *        Linux 内核只把发往该组地址的报文投递给本 socket，
 *        不会再收到同端口上其它组播组（如 WS-Discovery 239.255.255.250）的报文。
 */
bool sat_socket_udp_open_bind(int *socket_fd, const char *bind_ip, int port, bool broadcast)
{
        int fd;
        int optval = 1;

        if (socket_fd == NULL || bind_ip == NULL)
        {
                return false;
        }

        fd = sat_socket_open_addr(SOCK_DGRAM, port, inet_addr(bind_ip));
        if (fd < 0)
        {
                return false;
        }

        if (broadcast)
        {
                setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
        }

        *socket_fd = fd;
        return true;
}

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
bool sat_socket_udp_send(int socket_fd, const char *data, size_t data_len, const char *ip, int port, int timeout_ms)
{
        struct sockaddr_in addr;
        int send_len = 0;
        int packet_size;
        int res;

        if (socket_fd < 0 || data == NULL || ip == NULL)
        {
                return false;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);

        while (send_len < data_len)
        {
                if (!sat_socket_select(socket_fd, timeout_ms, false, true))
                {
                        return false;
                }

                packet_size = data_len - send_len;
                if (packet_size > NETWORK_PACKAET_MAX)
                {
                        packet_size = NETWORK_PACKAET_MAX;
                }

                res = sendto(socket_fd, data + send_len, packet_size, 0,
                             (struct sockaddr *)&addr, sizeof(addr));
                if (res < 0)
                {
                        return false;
                }

                send_len += res;
                usleep(1000);
        }

        return true;
}

/**
 * @brief UDP接收数据
 * @param socket_fd fd
 * @param data 接收缓存
 * @param data_len 最大长度
 * @param client_addr 源地址
 * @param timeout_ms 超时
 * @return 实际接收长度
 */
int sat_socket_udp_receive(int socket_fd, char *data, size_t data_len, struct sockaddr_in *client_addr, int timeout_ms)
{
        int len;
        socklen_t addr_len;

        if (socket_fd < 0 || data == NULL || client_addr == NULL)
        {
                return -1;
        }

        if (!sat_socket_select(socket_fd, timeout_ms, true, false))
        {
                return 0;
        }

        addr_len = sizeof(struct sockaddr_in);
        len = recvfrom(socket_fd, data, data_len, 0,
                       (struct sockaddr *)client_addr, &addr_len);

        return len;
}

/**
 * @brief 字节数组转十六进制字符串
 * @param hex_str 输出字符串
 * @param card_data 输入字节数组
 * @param len 数组长度
 * @return 输出字符串指针
 */
char *uint8_to_hex_str(char *hex_str, uint8_t *card_data, int len)
{
        int i;

        if (hex_str == NULL || card_data == NULL || len <= 0)
        {
                return NULL;
        }

        memset(hex_str, 0, len * 2 + 1);
        for (i = 0; i < len; i++)
        {
                snprintf(&hex_str[i * 2], 3, "%02X", card_data[i]);
        }

        return hex_str;
}

/**
 * @brief 十六进制字符串转字节数组
 * @param hex_str 输入字符串
 * @param out_card 输出数组
 * @param len 数组长度
 * @return 0成功，-1失败
 */
int hex_str_to_uint8(const char *hex_str, uint8_t *out_card, int len)
{
        int i;
        int str_len;
        char c1, c2;
        uint8_t val;

        if (hex_str == NULL || out_card == NULL || len <= 0)
        {
                return -1;
        }

        str_len = strlen(hex_str);
        if (str_len != len * 2)
        {
                return -1;
        }

        for (i = 0; i < len * 2; i += 2)
        {
                c1 = toupper(hex_str[i]);
                c2 = toupper(hex_str[i + 1]);

                if ((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F'))
                {
                        // ok
                }
                else
                {
                        return -1;
                }

                if ((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F'))
                {
                        // ok
                }
                else
                {
                        return -1;
                }

                val = 0;
                if (c1 >= '0' && c1 <= '9')
                {
                        val |= (c1 - '0') << 4;
                }
                else
                {
                        val |= (10 + c1 - 'A') << 4;
                }

                if (c2 >= '0' && c2 <= '9')
                {
                        val |= (c2 - '0');
                }
                else
                {
                        val |= (10 + c2 - 'A');
                }

                out_card[i / 2] = val;
        }

        return 0;
}

/**
 * @brief 计算目标值相对于1左移的位数
 * @param target 目标值
 * @param base 基础值
 * @return 左移位数（失败返回-1）
 */
int get_shift_bits(uint32_t target, uint32_t base)
{
        int shift_count = 0;

        if (target == 0 || (target & (target - 1)) != 0)
        {
                return -1;
        }

        while (base != target)
        {
                base <<= 1;
                shift_count++;

                if (shift_count > 31)
                {
                        return -1;
                }
        }

        return shift_count;
}

/**
 * @brief 根据栋号、房号、设备号生成IP(位拆分方案)
 * @param buildingNo  栋号 1~254
 * @param roomNo      房号 格式：楼层*100+房间，例 101(1楼01),1208(12楼08),2015(20楼15)
 * @param deviceNo    设备号 1~10
 * @param ipBuffer    输出IP 缓冲区至少16字节
 * @return 0成功，-1参数错误
 */
int generate_ip(int buildingNo, int roomNo, int deviceNo, char *ipBuffer)
{
        // 1. 空指针检查
        if (ipBuffer == NULL)
        {
                return -1;
        }
        // 2.仅前置校验栋号与设备号，移除原来复杂roomNo复合判断
        if (buildingNo < 1 || buildingNo > 254 ||
            deviceNo < DEVICE_TYPE_MIN || deviceNo > DEVICE_TYPE_MAX)
        {
                return -1;
        }

        int floor = roomNo / 100;
        int room = roomNo % 100;

        int is_indoor = (deviceNo >= DEVICE_TYPE_INDOOR_MIN && deviceNo <= DEVICE_TYPE_INDOOR_MAX);

        if (is_indoor)
        {
                // 室内机：真实房间，floor≥1；room从1开始，4bit上限15
                if (floor < 1 || floor > 255)
                {
                        return -1;
                }
                if (room < 1 || room > 15)
                {
                        return -1;
                }
        }
        else
        {
                // 门口机：允许floor=0虚拟房号；room允许0‑15；上限仍然锁死255
                if (floor < 0 || floor > 255)
                {
                        return -1;
                }
                if (room < 0 || room > 15)
                {
                        return -1;
                }
        }

        unsigned char last_byte = (unsigned char)((room << 4) | deviceNo);
        snprintf(ipBuffer, 16, "10.%d.%d.%u", buildingNo, floor, last_byte);
        return 0;
}

/**
 * @brief 获取指定网口的链路质量等级
 * @param dev_id 网口号（0=eth0，1=eth1）
 * @return 静态字符串：质量分数+等级
 */
const char *network_device_link_level(int dev_id)
{
        char buf[64] = {0};
        int level = 0;
        int quality = 0;
        FILE *fp = NULL;
        const int max_dev_id = 9;

        static const char *const link_level_str[] = {
            "Link Down",
            "A", "B", "C", "D", "E"};

        static char result_buf[32] = {0};

        if (dev_id < 0 || dev_id > max_dev_id)
        {
                snprintf(result_buf, sizeof(result_buf), "0 %s", link_level_str[0]);
                return result_buf;
        }

        snprintf(buf, sizeof(buf), "cat /proc/eth%d", dev_id);
        fp = popen(buf, "r");
        if (fp == NULL)
        {
                snprintf(result_buf, sizeof(result_buf), "0 %s", link_level_str[0]);
                goto finish;
        }

        while (fgets(buf, sizeof(buf), fp) != NULL)
        {
                buf[strcspn(buf, "\n")] = '\0';

                if (strstr(buf, "link_down") != NULL)
                {
                        snprintf(result_buf, sizeof(result_buf), "0 %s", link_level_str[0]);
                        goto finish;
                }

                if (strstr(buf, "quality") != NULL)
                {
                        sscanf(buf, "quality: %d", &level);
                        break;
                }
        }

        if (level > 0)
        {
                double ratio = 32768.0 / (3.0 * level);
                if (ratio > 0 && !isinf(ratio))
                {
                        quality = (int)(10.0 * log10(ratio));

                        if (quality >= 25)
                        {
                                snprintf(result_buf, sizeof(result_buf), "%d %s", quality, link_level_str[1]);
                        }
                        else if (quality >= 23)
                        {
                                snprintf(result_buf, sizeof(result_buf), "%d %s", quality, link_level_str[2]);
                        }
                        else if (quality >= 21)
                        {
                                snprintf(result_buf, sizeof(result_buf), "%d %s", quality, link_level_str[3]);
                        }
                        else if (quality >= 19)
                        {
                                snprintf(result_buf, sizeof(result_buf), "%d %s", quality, link_level_str[4]);
                        }
                        else
                        {
                                snprintf(result_buf, sizeof(result_buf), "%d %s", quality, link_level_str[5]);
                        }
                        goto finish;
                }
        }

        snprintf(result_buf, sizeof(result_buf), "0 %s", link_level_str[0]);

finish:
        if (fp != NULL)
        {
                pclose(fp);
        }
        return result_buf;
}

/**
 * @brief 计算文件的MD5值
 * @param path 文件路径
 * @param out_md5_hex 输出32位小写十六进制MD5字符串（需33字节空间）
 * @return 成功true
 */
bool file_md5_get(const char *path, char *out_md5_hex)
{
        FILE *fp = fopen(path, "rb");
        if (!fp)
        {
                SAT_ERROR("fopen %s failed", path);
                return false;
        }

        MD5_CTX ctx;
        MD5Init(&ctx);

        unsigned char buf[4096];
        size_t len;
        while ((len = fread(buf, 1, sizeof(buf), fp)) > 0)
        {
                MD5Update(&ctx, buf, len);
        }

        unsigned char md5_bin[16];
        MD5Final(md5_bin, &ctx);
        for (int i = 0; i < 16; i++)
        {
                sprintf(out_md5_hex + i * 2, "%02x", md5_bin[i]);
        }
        out_md5_hex[32] = 0;

        fclose(fp);
        return true;
}