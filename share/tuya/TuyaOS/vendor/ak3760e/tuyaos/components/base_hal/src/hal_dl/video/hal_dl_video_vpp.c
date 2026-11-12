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
void hal_get_dlsym_for_video_vpp(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_vpp_init");
    if (sym != NULL) {
        hal_if_video.vpp.init = (int (*)(hal_video_vpp_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_vpp_deinit");
    if (sym != NULL) {
        hal_if_video.vpp.deinit = (int (*)())sym;
    }

    sym = dlsym(handle, "hal_video_vpp_create_channel");
    if (sym != NULL) {
        hal_if_video.vpp.create_channel = (int (*)(int, hal_video_vpp_chn_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_vpp_destroy_channel");
    if (sym != NULL) {
        hal_if_video.vpp.destroy_channel = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_vpp_start_channel");
    if (sym != NULL) {
        hal_if_video.vpp.start_channel = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_vpp_stop_channel");
    if (sym != NULL) {
        hal_if_video.vpp.stop_channel = (int (*)(int))sym;
    }
}
