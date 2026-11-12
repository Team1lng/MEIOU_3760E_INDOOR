#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <hal_ir.h>

int main()
{
	int ret = 0;
	int fd = 0;
	int i = 0;

	ir_raw_code *buf = NULL;
	int size = 2048;
	unsigned int duration = 0;

	buf = malloc(sizeof(ir_raw_code));
	buf->codelist = malloc(sizeof(int)*size);
	buf->count = 0;
	buf->size = size;
	memset(buf->codelist,0,sizeof(int)*size);

	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	printf("IR rx version 1.0 \n");
	ret = hal_ir_rawcode_rx(HAL_BOARD_RK3308, buf, &timeout);
	printf("hal_ir_rawcode_rx ret = %d\n",ret);
	if(ret < 0)
		return -1;

	for(i = 0; i < buf->count; i++){
		duration += buf->codelist[i];
	}

	printf("code count:%d time:%ld\n", buf->count, duration);

	for(i = 0; i < buf->count; i++){
		printf("%u ",buf->codelist[i]);
		if((i % 10 == 0) && i >= 10)
			printf("\n");
	}
	printf("\n");

	free(buf->codelist);
	free(buf);
	return 0;
}
