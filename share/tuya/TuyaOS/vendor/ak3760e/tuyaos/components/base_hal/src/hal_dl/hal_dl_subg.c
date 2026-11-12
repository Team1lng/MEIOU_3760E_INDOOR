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
#include "hal_dl.h"
#include "hal_subg_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_subg(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_subg_send");
    if (sym != NULL) {
        hal_if_subg.send = (int (*)(void *, int))sym;
    }

    sym = dlsym(handle, "hal_subg_receive");
    if (sym != NULL) {
        hal_if_subg.receive = (int (*)(void *, int))sym;
    }
}
