#ifndef __SOCK_UTIL_H__
#define __SOCK_UTIL_H__


#ifdef __cplusplus
extern "C" {
#endif


int tcp_connect(char* host, int port);
int tcp_connect_status_get(int fd);
int tcp_server_create(int port);
int fd_set_nonblocking(int fd);
int sock_read(int fd, char* buf, int size);
int sock_write(int fd, char* buf, int size);
int sock_getpeername(int fd, char* buf, int size);
int tcp_server_accept(int fd);

int udp_bind(int port);
//绑定两个连续的udp端口
int udp_continuous_bind(int* ports, int* fds);
int udp_sendto(int fd, char *ip, uint16_t port, char* buf, int size);
int udp_recvfrom(int fd, char *ip, uint16_t port, char* buf, int size);

int domain_to_ip(char* domain, char* ip_buf, int buf_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SOCK_UTIL_H__

