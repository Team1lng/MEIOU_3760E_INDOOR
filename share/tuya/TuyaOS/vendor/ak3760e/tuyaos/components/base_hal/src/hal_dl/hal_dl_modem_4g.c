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
#include "hal_modem_4g_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_modem_4g(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }
#if 0
    sym = dlsym(handle, "hal_modem_4g_get_dev_attr");
    if (sym != NULL) {
        hal_if_modem_4g.get_dev_attr = (int (*)(hal_modem_4g_dev_attr_t *))sym;
    }
#endif
}
