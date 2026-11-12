/*
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tb_log.h"
#include "interface_uart.h"
#include "product_test_config.h"

static void __dump_data(unsigned char *buf,int len)
{
    int i;
    printf("###########################################\n");
    for(i=0;i<len;i++) {
        printf("%02x ",buf[i]); 
    }
    printf("\n");
    printf("###########################################\n");

}

int product_test_uart_single_loop_test(char *name,int baud) 
{
    unsigned char send[] = {0x55,0xaa,0x00,0x00,0x00,0x00,0xFF};
    unsigned char recv[32] = {0x00};
    int fd;
    int retry = 3;
    struct timeval timeout;
    fd_set u_read;
    int cnts;
    int n  = sizeof(send); 
    int iret = -1;

    if(name == NULL || baud == 0 ) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    if(interface_uart_init(name,baud,&fd)) {
        TB_LOGE("%s init fail",name); 
        return -1; 
    }

    while( retry > 0 ) {

        memset(recv,0,n);

        if(interface_uart_send(fd,(char *)send,n) != n) {
	    TB_LOGE("send fail");
	    retry --;
            continue;
	}

	usleep(100000);

	FD_ZERO(&u_read);
	FD_SET(fd, &u_read);

	timeout.tv_sec	= 3;
	timeout.tv_usec	= 0;

	iret = select(fd + 1, &u_read, NULL, NULL, &timeout);

	if (iret > 0) {
	    if (FD_ISSET(fd, &u_read)) {			
	        if ((cnts = interface_uart_recv(fd,(char *)recv,sizeof(recv))) == n) {

                    if (global_log_level >= LEVEL_OVER_LOGV) {
                        __dump_data(recv,cnts);
		    }

		    if(memcmp(send,recv,n) == 0) {
		         TB_LOGD("recv and send same");	
			 iret = 0;
			 break;
		    }else {
		        TB_LOGW("read error,retry ....\n");
		        retry --;
		    }	
	        }else {
		    TB_LOGW("read %d != %d,retry ....\n",cnts,n);
		    retry --;
		}

	    }
       }else {
	   iret  = -1;
           retry --; 
           TB_LOGW("retry ...");
       }
    }

    interface_uart_uinit(fd);

    TB_LOGD("iret = %d\n",iret);

    return iret;
}

#define UART_DEFAULT_NAME "/dev/ttySAK1"
#define UART_DEFAULT_BAUD 115200

int product_test_uart_loop_test(void) 
{
    char *name;
    int baud;
    UART_INFO_T info= {0};

    if(product_test_config_check_uart_loop_support()) {
   	return 0; 
    }

    if(product_test_get_uart_info(&info) == 0) {
        name = info.name;
        baud = info.baud;	
    }else {
	TB_LOGD("uart used default parm");
        name = UART_DEFAULT_NAME;
	baud = UART_DEFAULT_BAUD;
    }

    return product_test_uart_single_loop_test(name,baud);
}
