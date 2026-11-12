/*
 * (C) Copyright 2020
 * Cheng Lei, Tuya Software Engineering, chenglei@tuya.com.
 */

#ifndef HAL_MISC_H
#define HAL_MISC_H

#include "hal_gpio.h"	/* for legacy codes */
#include "hal_led.h"	/* for legacy codes */

int hal_get_device_name(char *name, char *value);

int hal_get_flash_type(char *value);

int hal_reboot(int sync);

int hal_get_device_mode(int *mode);

int hal_set_device_mode(int mode);

int hal_get_device_version(char *sw_ver, char *hw_ver);

int hal_set_device_version(char *sw_ver, char *hw_ver);

int hal_get_device_platform(int *name);

int hal_get_prop(char *name, char *value, char *def_value);
int hal_set_prop(char *name, char *value);

#endif
