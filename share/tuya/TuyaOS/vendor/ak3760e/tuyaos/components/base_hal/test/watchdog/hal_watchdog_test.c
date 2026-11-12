#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <hal_watchdog.h>

int main()
{
	int ret = 0;
    	int fd = 0;
	int timeout = 0;
	int timeleft = 0;

	printf("watchdog test version 1.0 \n");
	fd = hal_watchdog_open();
	if(fd < 0){
        	printf("hal_watchdog_open failed %d\n", fd);
        	return -1;
    	}
	
	ret = hal_watchdog_get_timeout(fd,&timeout);
	if(ret < 0){
        	printf("hal_watchdog_get_timeout failed %d\n", ret);
        	return -1;
    	}
   	printf("current timeout %d\n", timeout);
	
    	timeout = 10;

	ret = hal_watchdog_set_timeout(fd,&timeout);
	if(ret < 0){
        	printf("hal_watchdog_set_timeout failed %d\n", ret);
        	return -1;
    	}
	printf("after set current timeout %d\n", timeout);
	
	sleep(2);
	while(1){
		ret = hal_watchdog_keepalive(fd);
		if(ret < 0){
			printf("hal_watchdog_keepalive failed %d\n", ret);
			return -1;
		}
		printf("after keepalive\n");
		
		ret = hal_watchdog_get_timeleft(fd,&timeleft);
		if(ret < 0){
			printf("hal_watchdog_get_timeleft failed %d\n", ret);
			return -1;
		}
		printf("time left:%d\n", timeleft);
		sleep(1);
	}
	
	ret = hal_watchdog_close(fd);
	if(ret < 0){
		printf("hal_watchdog_close failed %d\n", ret);
		return -1;
	}
	
    	return 0;
}
