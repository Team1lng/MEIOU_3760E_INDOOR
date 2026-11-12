/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_FILE_H__
#define __HAL_FILE_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hal_file_read(char *path, void *buf, int size);
int hal_file_write(char *path, void *buf, int size);
bool hal_file_exists(char *path);
int hal_file_size(char *path, int *size);

#ifdef __cplusplus
}
#endif

#endif
