#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/types.h> 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#include "sock_util.h"

#define SA struct sockaddr 

int tcp_connect(char* host, int port)
{
	int sockfd = -1;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("create sock error\n");
		return -1;
	}

	sin.sin_family 		= AF_INET;
	sin.sin_addr.s_addr = inet_addr(host);
	sin.sin_port 		= htons(port);

    printf("connecting %s:%d\n", host, port);

    fd_set_nonblocking(sockfd);

    if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        if (errno != EINPROGRESS){
            printf("connect server failed.\n");
            close(sockfd);
            return -1;
        }
    }

	return sockfd;
}

int tcp_server_create(int port)
{
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        return -1;
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr));

    //set port reuse
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        return -1;
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        return -1;
    } 
    else
        printf("Server listening..\n"); 
    
    return sockfd;
}

int tcp_server_accept(int fd)
{
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    // Accept the data packet from client and verification 
    int connfd = accept(fd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        return -1;
    } 
    else
        printf("server acccept the client...\n"); 

    return connfd;
}

int sock_getpeername(int fd, char* buf, int size)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);

    memset(buf, 0, size);
    if (!getpeername(fd, (struct sockaddr *)&sa, &len))
    {
        snprintf(buf, size, "%s", inet_ntoa(sa.sin_addr));
        printf("peer IP：%s ", buf);
        printf("peer PORT：%d \n", ntohs(sa.sin_port));
    }

    return 0;
}

int fd_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);

    return flags;
}

int sock_read(int fd, char* buf, int size)
{
    int len = read(fd, buf, size);

    return len;
}

int sock_write(int fd, char* buf, int size)
{
    int len = write(fd, buf, size);

    return len;
}


int udp_bind(int port)
{
    int ret = 0;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0){
        return -1;
    }

    return fd;
}

//绑定两个连续的udp端口
int udp_continuous_bind(int* ports, int* fds)
{
    int ret = 0;
    
    while(1){
        //先随机获取一个10000-60000之间的偶数端口
        ports[0] = (rand() % 25000 + 5000) << 1;
        ports[1] = ports[0] + 1;

        fds[0] = udp_bind(ports[0]);
        if (fds[0] < 0){
            continue;
        }

        fds[1] = udp_bind(ports[1]);
        if (fds[1] < 0){
            close(fds[0]);
            continue;
        }

        break;
    }

    return ret;
}

int udp_sendto(int fd, char *ip, uint16_t port, char* buf, int size)
{
    struct sockaddr_in server;

    /* Set up the server name */
    server.sin_family = AF_INET;            /* Internet Domain    */
    server.sin_port = htons(port);          /* Server Port        */
    server.sin_addr.s_addr = inet_addr(ip); /* Server's Address   */

    return sendto(fd, buf, size, 0, (SA*)&server, sizeof(server));
}

int udp_recvfrom(int fd, char *ip, uint16_t port, char* buf, int size)
{
    struct sockaddr_in server;
    socklen_t len = sizeof(server);

    /* Set up the server name */
    server.sin_family = AF_INET;            /* Internet Domain    */
    server.sin_port = htons(port);          /* Server Port        */
    server.sin_addr.s_addr = inet_addr(ip); /* Server's Address   */

    return recvfrom(fd, buf, size, 0, (SA*)&server, &len);
}

int domain_to_ip(char* domain, char* ip_buf, int buf_len)
{
    int ret = -1;
    struct addrinfo hints;
    struct addrinfo *res = NULL, *res_p = NULL;

    if(domain == NULL) {
        printf("invalid params\n");
        return -1;
    }

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_CANONNAME;
    hints.ai_protocol   = 0;

    ret = getaddrinfo(domain, NULL, &hints, &res);
    if(ret != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }

    ret = -1;
    void* addr = NULL;

    for(res_p = res; res_p != NULL; res_p = res_p->ai_next) {

        if (res_p->ai_family == AF_INET){
            addr = &((struct sockaddr_in *)(res_p->ai_addr))->sin_addr;
        }
        else {
            addr = &((struct sockaddr_in6 *)(res_p->ai_addr))->sin6_addr;
        }

        if (NULL != inet_ntop(res_p->ai_family, addr, ip_buf, buf_len)){
            ret = 0;
            break;
        }
    }

    freeaddrinfo(res);
    res = NULL;

    return ret;
}

int tcp_connect_status_get(int fd)
{
    int value;
    uint32_t len = sizeof(int);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &value, &len);

    if (value == 0){
        return 0;
    }
    else {
        return -1;
    }
}