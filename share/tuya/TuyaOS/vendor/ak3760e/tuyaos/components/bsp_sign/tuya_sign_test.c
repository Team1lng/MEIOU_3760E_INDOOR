#include <stdio.h>
#include <string.h>
#include "tuya_sign.h"

int main(int argc, char* argv[])
{
	ota_sign sign;

	memset(&sign, 0, sizeof(sign));

	// TODO: check and help

	tuya_sign_file(argv[1], &sign);

	printf("len:%d\n", sign.len);
	printf("val:%s\n", sign.data);

	return 0;
}
