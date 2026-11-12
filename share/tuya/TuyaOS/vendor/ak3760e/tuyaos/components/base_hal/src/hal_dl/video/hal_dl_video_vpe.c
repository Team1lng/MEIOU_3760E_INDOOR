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
void hal_get_dlsym_for_video_vpe(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_vpe_init");
    if (sym != NULL) {
        hal_if_video.vpe.init = (int (*)(hal_video_vpe_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_vpe_deinit");
    if (sym != NULL) {
        hal_if_video.vpe.deinit = (int (*)())sym;
    }

    sym = dlsym(handle, "hal_video_vpe_create_channel");
    if (sym != NULL) {
        hal_if_video.vpe.create_channel = (int (*)(int, int, hal_video_vpe_chn_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_vpe_destroy_channel");
    if (sym != NULL) {
        hal_if_video.vpe.destroy_channel = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vpe_start_channel");
    if (sym != NULL) {
        hal_if_video.vpe.start_channel = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vpe_stop_channel");
    if (sym != NULL) {
        hal_if_video.vpe.stop_channel = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_vpe_isp_load_para_file");
    if (sym != NULL) {
        hal_if_video.vpe.isp_load_para_file = (int (*)(int, char *, unsigned int))sym;
    }
}
