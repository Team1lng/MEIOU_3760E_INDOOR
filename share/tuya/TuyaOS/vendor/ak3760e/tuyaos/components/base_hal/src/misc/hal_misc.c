/*
 * (C) Copyright 2020
 * Cheng Lei, Tuya Software Engineering, chenglei@tuya.com.
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
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"

int hal_get_prop(char *name, char *value, char *def_value)
{
	int ret = -1;

	if (!name || !value)
		return -1;

	ret = nvram_get(name, value);
	if (!ret)
		return 0;

	if (def_value) {
		strcpy(value, def_value);
		return 0;
	}

	return -1;
}

int hal_set_prop(char *name, char *value)
{
	if (!name || !value)
		return -1;

	if (nvram_set(name, value) < 0)
		return -1;

	return nvram_commit();
}

/*
 * bluetooth zigbee watchdog motor
 */
int hal_get_device_name(char *name, char *value)
{
	char key[NVRAM_MAX_PARAM_LEN] = {0};
	char def_value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!name || !value)
		return -1;

	snprintf(key, NVRAM_MAX_PARAM_LEN, "hal_%s_devname", name);
	snprintf(def_value, NVRAM_MAX_VALUE_LEN, "/dev/%s", name);

	return hal_get_prop(key, value, def_value);
}

int hal_get_flash_type(char *value)
{
	return hal_get_prop("hal_flash_type", value, "nor");
}

int hal_reboot(int with_sync)
{
	if (with_sync)
		sync();
	return reboot(RB_AUTOBOOT);
}

/*
 * device mode
 */
int hal_get_device_mode(int *mode)
{
	int ret = 0;
	char name[NVRAM_MAX_PARAM_LEN] = {0};
	char value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!mode)
		return -1;

	snprintf(name, NVRAM_MAX_PARAM_LEN, "%s", "device.ftm.mode");
	ret = hal_get_prop(name, value, "0");
	if (ret < 0)
		return -1;
	*mode = atoi(value);

	return 0;
}

int hal_set_device_mode(int mode)
{
	char name[NVRAM_MAX_PARAM_LEN] = {0};
	char def_value[NVRAM_MAX_VALUE_LEN] = {0};

	snprintf(name, NVRAM_MAX_PARAM_LEN, "%s", "device.ftm.mode");

	return hal_set_prop(name, mode?"1":"0");
}

int hal_get_device_version(char *sw_ver, char *hw_ver)
{
	int ret = 0;
	char name[NVRAM_MAX_PARAM_LEN] = {0};
	char value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!sw_ver || !hw_ver)
		return -1;

	snprintf(name, NVRAM_MAX_PARAM_LEN, "%s", "device.sw.ver");
	ret = hal_get_prop(name, sw_ver, "0.0.0");
	if (ret < 0)
		return -1;

	snprintf(name, NVRAM_MAX_PARAM_LEN, "%s", "device.hw.ver");
	ret = hal_get_prop(name, hw_ver, "0.0.0");
	if (ret < 0)
		return -1;

	return 0;
}

int hal_set_device_version(char *sw_ver, char *hw_ver)
{
	if (sw_ver && strlen(sw_ver) > 0) {
		if (hal_set_prop("device.sw.ver", sw_ver) < 0)
			return -1;
	}

	if (hw_ver && strlen(hw_ver) > 0) {
		if (hal_set_prop("device.hw.ver", hw_ver) < 0)
			return -1;
	}

	return 0;
}

int hal_get_device_platform(int *name)
{
	int ret = 0;
	char value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!name)
		return -1;

	snprintf(name, NVRAM_MAX_PARAM_LEN, "%s", "device.cpu.name");
	ret = hal_get_prop(name, value, "common");
	if (ret < 0)
		return -1;

	return 0;
}
