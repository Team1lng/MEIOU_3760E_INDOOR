/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <pthread.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_types.h"
#include "hal_file.h"
#include "hal_subg.h"
#include "hal_subg_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_subg_t hal_if_subg = {0};


#define HAL_SUBG_DEV_NAME   "/dev/tysubg"

int hal_subg_send_common(void *buf, int size)
{
    return hal_file_write(HAL_SUBG_DEV_NAME, buf, size);
}

int hal_subg_receive_common(void *buf, int size)
{
    return hal_file_read(HAL_SUBG_DEV_NAME, buf, size);
}
