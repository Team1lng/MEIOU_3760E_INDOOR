/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <poll.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_power.h"
#include "hal_power_core.h"


static int write_char(char const* path,const char *mode)
{
	FILE *fp;
	fp = fopen(path, "w+");
	if(fp){
		int ret = fprintf(fp, "%s", mode);
		fclose(fp);
		return ret < 0 ? -errno : 0;
	} else {
		printf("write_char failed to open %s\n", path);
		return -errno;
	}
}

static int read_char(char const* path,char *mode)
{
	FILE *fp;
	int value = 0;
	fp = fopen(path, "r");
	if (fp) {
		int ret = fscanf(fp, "%s", mode);
		fclose(fp);
		return ret < 0 ? -errno : 0;
	} else {
		printf("read_char failed to open %s\n",path);
		return -errno;
	}
}

static int read_int(char const* path,unsigned int *value)
{
	FILE *fp;
	fp = fopen(path,"r");
	if (fp) {
		printf("fopen fp success.\n");
		int ret = fscanf(fp, "%d", value);
		fclose(fp);
		return ret < 0 ? -errno : 0;
	} else {
		printf("read_int failed to open %s\n", path);
		return -errno;
	}
}

/*
 * For generic instance, set all the members to NULL
 */
hal_if_power_t hal_if_power = { 0 };

int hal_set_power_state_common(const char *mode)
{
	return write_char(POWER_MODE_PATH,mode);
}

int hal_get_power_state_common(char *mode)
{
	return read_char(POWER_MODE_PATH,mode);
}

int hal_get_wakeup_src_common(unsigned int *value)
{
	return read_int(WAKEUP_SRC_PATH,value);
}
