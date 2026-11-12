#include <stdio.h>
#include "hal_backlight.h"

int main(int argc,char **argv)
{
	int brightness;
	int ret;
	if(argc != 2) {
		printf("./hal_backlight_test brightness\n");
		return -1;
	}

	brightness = atoi(argv[1]);

	ret = hal_backlight_set_value(brightness);
	if(ret < 0){
		perror("hal_backlight_set_value failed");
		return -1;
	}

	ret = hal_backlight_get_value();
	if(ret < 0){
		perror("hal_backlight_get_value failed");
		return -1;
	}

	printf("get brightness = %d\n",brightness);

	return 0;
}
