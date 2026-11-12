/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include <pthread.h>
#include"interface_uart.h"
#include "tb_log.h"


struct uart_param_s {
    int baud;     /* uart baud rate   */
    int flow_ctrl;/* uart flow control*/
    int data_bits;/* uart data bits   */
    int stop_bits;/* uart stop bits   */
    int parity;   /* uart parity bit  */
};

static int sg_interface_uart_init_flag = 0;
static pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;

static int __interface_uart_parm_init(int fd, struct uart_param_s *param)
{
    int i;    
    const int speed_arr[] = { B1500000, B115200, B19200, B9600, B4800, B2400, B1200, B300};    
    const int name_arr[] = {1500000, 115200, 19200, 9600, 4800, 2400, 1200, 300};    

    struct termios options;    

    if(param == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }
       
    if( tcgetattr(fd,&options)  !=  0) {    
        TB_LOGE("tcgetattr %s",strerror(errno));        
        return -1;     
    }    
      
    for ( i= 0;i < sizeof(speed_arr)/sizeof(int);i++) {    
        if(param->baud == name_arr[i]) {                 
            cfsetispeed(&options, speed_arr[i]);     
            cfsetospeed(&options, speed_arr[i]);      
        }    
    }         
       
    options.c_cflag |= CLOCAL;    
    options.c_cflag |= CREAD;    
      
    switch(param->flow_ctrl) {    
        case 0: 
            options.c_cflag &= ~CRTSCTS;    
            break;       
        case 1 :
            options.c_cflag |= CRTSCTS;    
            break;    
        case 2: 
            options.c_cflag |= IXON | IXOFF | IXANY;    
        default:
            TB_LOGE("Unsupported flow control"); 
            return -1;
    }

    options.c_cflag &= ~CSIZE;    
    switch (param->data_bits) {      
        case 5:    
            options.c_cflag |= CS5;    
            break;    
        case 6:    
            options.c_cflag |= CS6;    
            break;    
        case 7:   
            options.c_cflag |= CS7;    
            break;    
        case 8:        
            options.c_cflag |= CS8;    
            break;      
        default:       
            TB_LOGE("Unsupported data size");    
            return -1;     
    }

    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    switch (param->parity) {      
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;     
            options.c_iflag &= ~INPCK;        
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);     
            options.c_iflag |= INPCK;                 
            break;     
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;           
            options.c_cflag &= ~PARODD;           
            options.c_iflag |= INPCK;          
            break;
        case 's':    
        case 'S':
            options.c_cflag &= ~PARENB;    
            options.c_cflag &= ~CSTOPB;    
            break;     
        default:      
            TB_LOGE("Unsupported parity");        
            return -1;     
    } 

    switch (param->stop_bits) {      
        case 1:
            options.c_cflag &= ~CSTOPB; break;     
        case 2:
            options.c_cflag |= CSTOPB; break;    
        default:       
            TB_LOGE("Unsupported stop bits");     
            return -1;    
    }
       
    //raw data 
    options.c_oflag &= ~OPOST;    
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);    
       
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 1;
       
    tcflush(fd,TCIFLUSH);    
       
    if (tcsetattr(fd,TCSANOW,&options) != 0) {    
        TB_LOGE("com set %s",strerror(errno));      
        return -1;     
    }    
    
    return 0;     
}

int interface_uart_init(char *name,int baud,int *fd)
{
    int ufd;
    struct uart_param_s param = {0};

    if(sg_interface_uart_init_flag)
        return 0;   

    ufd  = open(name, O_RDWR|O_NOCTTY|O_NDELAY);

    if (ufd < 0) {
        TB_LOGE("Can't Open Serial Port %s",strerror(errno));
        return -1;
    }
/*
    if(fcntl(ufd, F_SETFL, 0) < 0) {
        TB_LOGE("fcntl failed");
        return -1;
    }
*/

    param.baud = baud;
    param.flow_ctrl = 0;
    param.data_bits = 8;
    param.stop_bits = 1;
    param.parity =  'N';

    if(__interface_uart_parm_init(ufd,&param)) {
        TB_LOGE("interface_uart_init fail\n");
        return -1;
    }
    
    *fd = ufd;
    sg_interface_uart_init_flag = 1;
    return 0;    
}    
    
int interface_uart_send(int fd,char *buf,int size) 
{
    int iret;
    
    pthread_mutex_lock(&sg_mutex);  
    iret =  write(fd,buf,size);

    if(iret != size) {
        tcflush(fd,TCOFLUSH);  
        iret = 0;
    }
    pthread_mutex_unlock(&sg_mutex);
    return iret;
}

int interface_uart_recv(int fd,char *buf,int size) 
{
    int iret = 0;
    int cnts = 0;;
    struct timeval timeout; 

    pthread_mutex_lock(&sg_mutex);  
    cnts =  read(fd,buf,size);
    pthread_mutex_unlock(&sg_mutex);

    if(cnts < 8) {
           return cnts;
    }

    timeout.tv_sec  = 0;
    timeout.tv_usec = 1000000;

    while(cnts < size) {
        int n;
        fd_set fds; 

        FD_ZERO(&fds);
        FD_SET(fd,&fds);

        iret = select(fd + 1, &fds, NULL, NULL, &timeout);

        if(iret > 0) {
            n =  read(fd,buf+cnts,size-cnts);

            if(n > 0) {
                cnts += n;
            }

            if(n < 8) {
                break;
        }
        } else {
            TB_LOGD("iret:%d",iret);
            break;  
        }
    }

    TB_LOGD("ret %d bytes\n",cnts);

    return cnts;
}

void interface_uart_uinit(int fd) 
{
    if(sg_interface_uart_init_flag) {
        close(fd);
    }
}
