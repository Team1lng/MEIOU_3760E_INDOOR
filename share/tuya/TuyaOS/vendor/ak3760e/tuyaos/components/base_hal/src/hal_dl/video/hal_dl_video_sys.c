/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "../hal_dl.h"
#include "hal_video_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_video_sys(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_sys_init");
    if (sym != NULL) {
        hal_if_video.sys.init = (int (*)())sym;
    }

    sym = dlsym(handle, "hal_video_sys_deinit");
    if (sym != NULL) {
        hal_if_video.sys.deinit = (int (*)())sym;
    }
}
