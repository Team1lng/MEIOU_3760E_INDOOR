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
void hal_get_dlsym_for_audio_output(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_aud_out_init");
    if (sym != NULL) {
        hal_if_audio.out.init = (int (*)(int, int, hal_aud_out_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_aud_out_deinit");
    if (sym != NULL) {
        hal_if_audio.out.deinit = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_out_write_frame");
    if (sym != NULL) {
        hal_if_audio.out.write_frame = (int (*)(int, int, hal_aud_frame_s *, int))sym;
    }

    sym = dlsym(handle, "hal_aud_out_set_volume");
    if (sym != NULL) {
        hal_if_audio.out.set_volume = (int (*)(int, int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_out_get_volume");
    if (sym != NULL) {
        hal_if_audio.out.get_volume = (int (*)(int, int, int *))sym;
    }

    sym = dlsym(handle, "hal_aud_out_mute");
    if (sym != NULL) {
        hal_if_audio.out.mute = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_out_unmute");
    if (sym != NULL) {
        hal_if_audio.out.unmute = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_aud_out_speaker_switch");
    if (sym != NULL) {
        hal_if_audio.out.speaker_switch = (int (*)(int, bool))sym;
    }
}
