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
#include "hal_video_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_video_t hal_if_video = { 0 };
