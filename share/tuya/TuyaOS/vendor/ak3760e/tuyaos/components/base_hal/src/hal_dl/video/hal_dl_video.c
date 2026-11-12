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
void hal_get_dlsym_for_video(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    hal_get_dlsym_for_video_sensor(handle);
    hal_get_dlsym_for_video_sys(handle);
    hal_get_dlsym_for_video_venc(handle);
    hal_get_dlsym_for_video_vif(handle);
    hal_get_dlsym_for_video_vpe(handle);
    hal_get_dlsym_for_video_vpp(handle);
}
