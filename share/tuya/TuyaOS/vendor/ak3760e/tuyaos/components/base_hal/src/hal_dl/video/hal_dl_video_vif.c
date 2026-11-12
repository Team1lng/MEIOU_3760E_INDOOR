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
void hal_get_dlsym_for_video_vif(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_vif_init");
    if (sym != NULL) {
        hal_if_video.vif.init = (int (*)(int, int, int, hal_video_vif_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_vif_deinit");
    if (sym != NULL) {
        hal_if_video.vif.deinit = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vif_enable");
    if (sym != NULL) {
        hal_if_video.vif.enable = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vif_disable");
    if (sym != NULL) {
        hal_if_video.vif.disable = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vif_get_info");
    if (sym != NULL) {
        hal_if_video.vif.get_info = (int (*)(int, int, int, hal_video_vif_info_t *))sym;
    }
}
