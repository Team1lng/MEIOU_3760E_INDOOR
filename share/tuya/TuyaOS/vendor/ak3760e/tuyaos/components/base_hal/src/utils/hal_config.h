/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hal_cfg_get_item_string(char *device, char *key, char *buf, int bufsize);
int hal_cfg_get_item_int(char *device, char *key, int *value);

#ifdef __cplusplus
}
#endif

#endif
