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
#include "hal_audio_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_audio_input(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_aud_in_init");
    if (sym != NULL) {
        hal_if_audio.in.init = (int (*)(int, int, hal_aud_in_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_aud_in_deinit");
    if (sym != NULL) {
        hal_if_audio.in.deinit = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_in_wait_for_frame");
    if (sym != NULL) {
        hal_if_audio.in.wait_for_frame = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_in_get_frame");
    if (sym != NULL) {
        hal_if_audio.in.get_frame = (int (*)(int, int, hal_aud_frame_s *, int))sym;
    }

    sym = dlsym(handle, "hal_aud_in_release_frame");
    if (sym != NULL) {
        hal_if_audio.in.release_frame = (int (*)(int, int, hal_aud_frame_s *))sym;
    }

    sym = dlsym(handle, "hal_aud_in_set_volume");
    if (sym != NULL) {
        hal_if_audio.in.set_volume = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_in_get_volume");
    if (sym != NULL) {
        hal_if_audio.in.get_volume = (int (*)(int, int, int *))sym;
    }

    sym = dlsym(handle, "hal_aud_in_mute");
    if (sym != NULL) {
        hal_if_audio.in.mute = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_in_unmute");
    if (sym != NULL) {
        hal_if_audio.in.unmute = (int (*)(int, int))sym;
    }
}
