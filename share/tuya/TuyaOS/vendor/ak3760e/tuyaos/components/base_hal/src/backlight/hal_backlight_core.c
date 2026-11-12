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
#include "hal_backlight.h"
#include "hal_backlight_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_backlight_t hal_if_backlight = { 0 };


static int write_int(char const* path, int value)
{
    FILE *fp;
    fp = fopen(path, "w+");
    if (fp) {
        int ret = fprintf(fp, "%d", value);
        fclose(fp);
        return ret < 0 ? -errno : 0;
    } else {
        printf("write_int failed to open %s\n", path);
        return -errno;
    }
}

static int read_int(char const* path)
{
    FILE *fp;
    int value = 0;
    fp = fopen(path, "r");
    if (fp) {
        int ret = fscanf(fp, "%d", &value);
        fclose(fp);
        return ret == EOF ? 0 : value;
    } else {
        printf("read_int failed to open %s\n", path);
        return -errno;
    }
}

int hal_backlight_set_value_common(int brightness)
{
	return write_int(DISPLAY_BACKLIGHT_PATH, brightness);
}

int hal_backlight_get_value_common(void)
{
	return read_int(DISPLAY_BACKLIGHT_PATH);
}
