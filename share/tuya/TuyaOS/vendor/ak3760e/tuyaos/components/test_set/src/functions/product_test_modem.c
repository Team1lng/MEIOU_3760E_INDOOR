/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termio.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <time.h>
#include <sys/times.h> 
#include <sys/select.h> 
#include "tb_log.h"
#include "product_test_modem.h"

typedef struct modem_parm_s
{
    char at_dev[MODEM_BUFF_SIZE]; // at command device path
    char send_buf[MODEM_BUFF_SIZE]; // send buffer
    char recv_buf[MODEM_BUFF_SIZE]; // receive buffer
    int  at_fd; // at command fd;
    int  baud;  // at uart baudate
    pthread_mutex_t tty_mutex;
}MODEM_PARM_T;

MODEM_PARM_T sg_modem_parm = {0};

static int __modem_uart_init(MODEM_PARM_T  *param)
{
    speed_t speed;
    struct termios ttyAttr;

    if(param == NULL){
        TB_LOGE("invaild parm");
        return -1;
    }

    param->at_fd = open(param->at_dev,O_RDWR|O_NOCTTY,0);

    if(param->at_fd < 0){
        TB_LOGE("Unable to open %s",param->at_dev);
        return -1;
    }
    
    memset(&ttyAttr,0,sizeof(struct termios));

    tcgetattr(param->at_fd,&ttyAttr);

    switch(param->baud)
    {
        case 1800:
            speed = B1800;break;                        
        case 2500:                              
            speed = B2400;break;                    
        case 4800:                              
            speed = B4800;break;                        
        case 9600:                              
            speed = B9600;break;                        
        case 38400:                             
            speed = B38400;break;   
        case 57600:
            speed = B57600;break;
        case 115200:
            speed = B115200;break;
        default:
            speed = B9600;break;
    }

    cfsetospeed(&ttyAttr,speed);
    cfsetispeed(&ttyAttr,speed);

    ttyAttr.c_cflag |= CREAD | CLOCAL;
    ttyAttr.c_cflag &= ~PARENB;
    ttyAttr.c_cflag &= ~CSTOPB;
    ttyAttr.c_cflag &= ~CSIZE;
    ttyAttr.c_cflag |= CS8;
    ttyAttr.c_cflag |= HUPCL;
    ttyAttr.c_cflag &= ~OPOST;

    ttyAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    ttyAttr.c_cc[VMIN] = 1;

    if(tcsetattr(param->at_fd,TCSANOW,&ttyAttr) < 0){
        TB_LOGE("Unable to set param comm port %s",param->at_dev);
        return -1;
    }
    
    return 0;
}


static int __modem_send_then_recv(MODEM_PARM_T *param)
{
    int iret=0;
    fd_set read_set;

    struct timeval timeout = {
        .tv_sec  = 3,
        .tv_usec = 0    
    };

    if(param == NULL){
        TB_LOGE("invaild parm");
        return -1;
    }

    pthread_mutex_lock(&param->tty_mutex);
    iret = write(param->at_fd,param->send_buf,strlen(param->send_buf));

    if(iret < 0){
        TB_LOGE("write %s failed",param->at_dev);
        pthread_mutex_unlock(&param->tty_mutex);
        return -1;
    }

    usleep(10000);

    FD_ZERO(&read_set);
    FD_SET(param->at_fd,&read_set);

    if (select(param->at_fd+1,&read_set,NULL,NULL,&timeout) <= 0){
        TB_LOGE("respond timeout");
        pthread_mutex_unlock(&param->tty_mutex);
        return -1;
    }

    if(FD_ISSET(param->at_fd,&read_set)){
        if(read(param->at_fd,param->recv_buf,MODEM_BUFF_SIZE-1) <= 0){
            TB_LOGE("read %s failed",param->at_dev);
            pthread_mutex_unlock(&param->tty_mutex);
            return -1;
        }

        iret = 0;
    }
    
    if(!strstr(param->recv_buf,AT_CMD_RESULT_OK)){
        TB_LOGE("param at cmd return error");
        pthread_mutex_unlock(&param->tty_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&param->tty_mutex);

    return iret;
}

static int __modem_set_at_cmd(MODEM_PARM_T *param,char *at_cmd)
{
    if(param == NULL || at_cmd == NULL){
        TB_LOGE("invaild parm");
        return -1;
    }

    memset(param->send_buf,0,MODEM_BUFF_SIZE);
    memset(param->recv_buf,0,MODEM_BUFF_SIZE);
    strcpy(param->send_buf,at_cmd);

    return 0;
}

int product_test_modem_enter_test(void)
{
    MODEM_PARM_T *param = &sg_modem_parm;

    TB_LOGD();
    param->baud = 115200;
    strcpy(param->at_dev,"/dev/ttyUSB0");
    if(__modem_uart_init(param)) {
        TB_LOGE("param uart init fail");
        return -1;
    }

    __modem_set_at_cmd(param,AT_CMD_ECHO_OFF);
    if(__modem_send_then_recv(param)){
        TB_LOGE("turn off param echo feature fail");
        return -1;
    }

    __modem_set_at_cmd(param,AT_CMD_CMEE);
    if(__modem_send_then_recv(param)){
        TB_LOGE("enable the +CME ERROR fail");
        return -1;
    }

    __modem_set_at_cmd(param,AT_CMD_SUPPORT_HOTPLUG);
    if(__modem_send_then_recv(param)){
        TB_LOGE("enable SIM card hot plug feature fail");
        return -1;
    }

    __modem_set_at_cmd(param,AT_CMD_NET_SET);
    if(__modem_send_then_recv(param)){
        TB_LOGE("set net seq fail");
        return -1;
    }

    return 0;
}

int product_test_modem_off(void)
{
    return 0;
}

int product_test_modem_get_ip(uint32_t *ip) 
{
    return 0;
}

int product_test_modem_get_signal_type(int* type)
{
    return 0;
}

int product_test_modem_get_rssi(int *db)
{
        MODEM_PARM_T *param = &sg_modem_parm;
        int rssi,ber;
        char *str;

        TB_LOGD();
        __modem_set_at_cmd(param,AT_CMD_SIGNAL_QUALITY_CHECK);
        if(__modem_send_then_recv(param)){
        TB_LOGE("get param signal quality fail");
        return -1;
    }

        str = strstr(param->recv_buf,"+CSQ:");
        sscanf(str,"+CSQ:%d,%d",&rssi,&ber);
        if((rssi >= 0) && (rssi <=31)){
        (*db) = -113 + rssi*2;
    }else if((rssi > 31) && (rssi < 99)){
        (*db) = -51;
    } else{
        TB_LOGE("invalid rssi %d",rssi);
        return -1;
    }

        return 0;
}   

int product_test_modem_get_ccid(char *ccid)
{
    MODEM_PARM_T *param = &sg_modem_parm;
    char *str;
    
    TB_LOGD();
    __modem_set_at_cmd(param,AT_CMD_SIM_CCID_CHECK);
    if(__modem_send_then_recv(param)){
        TB_LOGE("get sim card ccid fail");
        return -1;
    }
    
    str = strstr(param->recv_buf,"+CCID:");
    sscanf(str,"+CCID: %s",ccid);
    
    return 0;
}

int product_test_modem_get_imsi(char *imsi)
{
    MODEM_PARM_T *param = &sg_modem_parm;
    char *str;

    TB_LOGD();
    __modem_set_at_cmd(param,AT_CMD_SIM_IMSI_CHECK);
    if(__modem_send_then_recv(param)){
        TB_LOGE("get sim card imsi fail");
        return -1;
    }

    str = strstr(param->recv_buf,"+CIMI:");
    sscanf(str,"+CIMI: %s",imsi);
    
    return 0;
}


int product_test_modem_get_imei(char *imei)
{
    MODEM_PARM_T *param = &sg_modem_parm;
    char *str;

    TB_LOGD();
    __modem_set_at_cmd(param,AT_CMD_LTE_IMEI_CHECK);
    if(__modem_send_then_recv(param)){
                TB_LOGE("get modem module imei fail");
                return -1;
        }

    str = strstr(param->recv_buf,"+CGSN:");
    sscanf(str,"+CGSN: %s",imei);

    return 0;
}
