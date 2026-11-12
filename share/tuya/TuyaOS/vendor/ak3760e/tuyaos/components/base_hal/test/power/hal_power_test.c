#include <stdio.h>
#include "hal_power.h"

int main(int argc,char **argv)
{
	char mode[20] = {0};
	unsigned int value;
	int ret;

	
	ret = hal_get_wakeup_src(&value);
        if(ret < 0){
                perror("hal_get_wakeup_src failed");
                return -1;
        }
	
        if(value == 0x40){
                printf("the wakeup src is gpio24,8720cs\n");
        }else if(value == 0x10000){
                printf("the wakeup src is gpio67\n");
        }else if(value == 0x800000){
                printf("the wakeup src is gpio85\n");
        }else{
                printf("illegal wakeup src,value = 0x%x\n",value);
        }
	
	if(argc != 2) {
		printf("./hal_power_test standby\n");
		return -1;
	}

	strcpy(mode,argv[1]);

	ret = hal_set_power_state(mode);
	if(ret < 0){
		perror("hal_set_power_state failed");
		return -1;
	}

	ret = hal_get_power_state(mode);
	if(ret < 0){
		perror("hal_get_power_state failed");
		return -1;
	}
	printf("get mode = %s\n",mode);
	
	return 0;
}
