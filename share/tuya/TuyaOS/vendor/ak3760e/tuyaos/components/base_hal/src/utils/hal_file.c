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
#include <sys/stat.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_types.h"
#include "hal_misc.h"
#include "hal_file.h"

int hal_file_read(char *path, void *buf, int size)
{
    int fd;
    int ret;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "Failed to open %s\n", path);
        return -1;
    }

    ret = read(fd, buf, size);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to read %s\n", path);
        close(fd);
        return -1;
    }

    close(fd);
    return ret;
}

int hal_file_write(char *path, void *buf, int size)
{
    int fd;
    int ret;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "Failed to open %s\n", path);
        return -1;
    }

    ret = write(fd, buf, size);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to write %s\n", path);
        close(fd);
        return -1;
    }

    close(fd);
    return ret;
}

bool hal_file_exists(char *path)
{
    struct stat file_stat;

    return (stat(path, &file_stat) == 0);
}

int hal_file_size(char *path, int *size)
{
    struct stat file_stat;

    if (NULL == size) {
        return -1;
    }

    if (stat(path, &file_stat) != 0) {
        return -1;
    }

    *size = file_stat.st_size;
    return 0;
}
