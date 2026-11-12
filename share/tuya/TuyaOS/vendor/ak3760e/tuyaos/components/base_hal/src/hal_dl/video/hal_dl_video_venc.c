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
void hal_get_dlsym_for_video_venc(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_video_venc_init");
    if (sym != NULL) {
        hal_if_video.venc.init = (int (*)(hal_video_venc_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_venc_deinit");
    if (sym != NULL) {
        hal_if_video.venc.deinit = (int (*)())sym;
    }

    sym = dlsym(handle, "hal_video_venc_create_channel");
    if (sym != NULL) {
        hal_if_video.venc.create_channel = (int (*)(int, hal_video_venc_chn_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_video_venc_destroy_channel");
    if (sym != NULL) {
        hal_if_video.venc.destroy_channel = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_venc_start_channel");
    if (sym != NULL) {
        hal_if_video.venc.start_channel = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_venc_stop_channel");
    if (sym != NULL) {
        hal_if_video.venc.stop_channel = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_video_venc_wait_for_stream");
    if (sym != NULL) {
        hal_if_video.venc.wait_for_stream = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_video_venc_get_packet_num");
    if (sym != NULL) {
        hal_if_video.venc.get_packet_num = (int (*)(int, int *))sym;
    }

    sym = dlsym(handle, "hal_video_venc_get_stream");
    if (sym != NULL) {
        hal_if_video.venc.get_stream = (int (*)(int, hal_video_venc_stream_t *, int))sym;
    }

    sym = dlsym(handle, "hal_video_venc_release_stream");
    if (sym != NULL) {
        hal_if_video.venc.release_stream = (int (*)(int, hal_video_venc_stream_t *))sym;
    }
}
