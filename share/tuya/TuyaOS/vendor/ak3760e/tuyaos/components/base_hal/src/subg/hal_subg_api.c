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
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_subg.h"
#include "hal_subg_core.h"

int hal_subg_send(void *buf, int size)
{
    if (NULL == hal_if_subg.send)
        return hal_subg_send_common(buf, size);

    return hal_if_subg.send(buf, size);
}

int hal_subg_receive(void *buf, int size)
{
    if (NULL == hal_if_subg.receive)
        return hal_subg_receive_common(buf, size);

    return hal_if_subg.receive(buf, size);
}
