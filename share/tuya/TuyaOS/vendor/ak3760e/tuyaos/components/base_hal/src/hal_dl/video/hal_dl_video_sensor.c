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
void hal_get_dlsym_for_video_sensor(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_snr_init");
    if (sym != NULL) {
        hal_if_video.snr.init = (int (*)(int, hal_video_snr_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_snr_deinit");
    if (sym != NULL) {
        hal_if_video.snr.deinit = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_snr_enable");
    if (sym != NULL) {
        hal_if_video.snr.enable = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_snr_disable");
    if (sym != NULL) {
        hal_if_video.snr.disable = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_snr_set_fps");
    if (sym != NULL) {
        hal_if_video.snr.set_fps = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_snr_get_fps");
    if (sym != NULL) {
        hal_if_video.snr.get_fps = (int (*)(int, int *))sym;
    }

    sym = dlsym(handle, "hal_video_snr_get_res_num");
    if (sym != NULL) {
        hal_if_video.snr.get_res_num = (int (*)(int, int *))sym;
    }

    sym = dlsym(handle, "hal_video_snr_get_one_res");
    if (sym != NULL) {
        hal_if_video.snr.get_one_res = (int (*)(int, int, hal_video_res_t *))sym;
    }

    sym = dlsym(handle, "hal_video_snr_get_cur_res");
    if (sym != NULL) {
        hal_if_video.snr.get_cur_res = (int (*)(int, int *, hal_video_res_t *))sym;
    }

    sym = dlsym(handle, "hal_video_snr_set_res");
    if (sym != NULL) {
        hal_if_video.snr.set_res = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_snr_set_mirror");
    if (sym != NULL) {
        hal_if_video.snr.set_mirror = (int (*)(int, hal_video_mirror_e))sym;
    }

    sym = dlsym(handle, "hal_video_snr_get_mirror");
    if (sym != NULL) {
        hal_if_video.snr.get_mirror = (int (*)(int, hal_video_mirror_e *))sym;
    }
}
