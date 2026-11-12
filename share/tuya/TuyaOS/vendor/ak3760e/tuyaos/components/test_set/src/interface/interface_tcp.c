/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "interface_tcp.h"
#include "tb_log.h"

static int sg_interface_tcp_init_flag = 0;
static pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;

int interface_tcp_init(short port,int *fd)
{
    int tfd;
    int onoff = 1;
    int iret;
    int size = 0x100000;
    struct sockaddr_in sock_addr;

    if(sg_interface_tcp_init_flag) {
        return 0;
    }
    
    tfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(tfd == -1) {
        TB_LOGE("create socket  %s",strerror(errno));
        return -1;
    }

    if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, (char *)&onoff, sizeof(onoff))==-1) {
        TB_LOGE("SO_REUSEADDR %s",strerror(errno));
        close(tfd);
        return -1;
    }

    if ((setsockopt(tfd,SOL_SOCKET,SO_RCVBUF, &size ,sizeof(int))) < 0 ) {
        TB_LOGE("SO_RCVBUF %s",strerror(errno));
        close(tfd);
	return -1;
    }
/*
    if(setsockopt(tfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) == -1) {
        TB_LOGE("SO_RCVTIMEO %s",strerror(errno));
        close(tfd);
        return -1;
    }
*/

    TB_LOGD("####TCP PORT:#### %d",port);

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);    
    iret = bind(tfd,(struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
    if(iret != 0) {     
        TB_LOGE("bind error %s",strerror(errno));
        close(tfd);
        return -1;
    }

    //listen
    iret = listen(tfd, COM_TCP_MAX_SOCK);
    if(iret != 0) {
        TB_LOGE ("listen %s",strerror(errno));
        close(tfd);
        return -1;
    }

    *fd = tfd;
    sg_interface_tcp_init_flag  = 1;
    TB_LOGI("tcp init ok\n");
        
    return 0;    
}

int interface_tcp_send(int fd,char *buf,int size) 
{
    int iret;

    pthread_mutex_lock(&sg_mutex); 
    iret = write(fd,buf,size);
    pthread_mutex_unlock(&sg_mutex);   

    return iret;
}

int interface_tcp_recv(int fd,char *buf,int size) 
{
    int iret;

    pthread_mutex_lock(&sg_mutex); 
    iret =  read(fd,buf,size);
    pthread_mutex_unlock(&sg_mutex);   

    return iret;
}

void interface_tcp_uinit(int fd) 
{
    if(sg_interface_tcp_init_flag) {
        close(fd);
        sg_interface_tcp_init_flag = 0;
    }
}
