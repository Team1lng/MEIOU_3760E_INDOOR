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
#include "hal_watchdog_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_watchdog(void *handle)
{
    void *sym = NULL;
    char *err = NULL;

    if (NULL == handle) {
        return;
    }

    /* clear any old errors */
    dlerror();

    sym = dlsym(handle, "hal_watchdog_open");
    if (sym != NULL) {
        hal_if_watchdog.open = (int (*)())sym;
    } else {
        if ((err = dlerror()) != NULL) {
            //hal_log(HAL_LOG_ERR, "dlsym: sym=%s, %s\n",
            //        "hal_watchdog_open", err);
        }
    }

    sym = dlsym(handle, "hal_watchdog_close");
    if (sym != NULL) {
        hal_if_watchdog.close = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_watchdog_keepalive");
    if (sym != NULL) {
        hal_if_watchdog.keepalive = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_watchdog_set_timeout");
    if (sym != NULL) {
        hal_if_watchdog.set_timeout = (int (*)(int, int*))sym;
    }

    sym = dlsym(handle, "hal_watchdog_get_timeout");
    if (sym != NULL) {
        hal_if_watchdog.get_timeout = (int (*)(int, int*))sym;
    }

    sym = dlsym(handle, "hal_watchdog_get_timeleft");
    if (sym != NULL) {
        hal_if_watchdog.get_timeleft = (int (*)(int, int*))sym;
    }
}
