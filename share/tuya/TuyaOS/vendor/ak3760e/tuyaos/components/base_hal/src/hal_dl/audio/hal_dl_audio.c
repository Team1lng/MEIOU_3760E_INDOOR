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
void hal_get_dlsym_for_audio(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    hal_get_dlsym_for_audio_input(handle);
    hal_get_dlsym_for_audio_output(handle);
}
