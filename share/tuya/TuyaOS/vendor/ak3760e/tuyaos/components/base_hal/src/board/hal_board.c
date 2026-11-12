/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_board.h"
#include "hal_board_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_board_t hal_if_board = { 0 };


static int hal_board_get_config_file_common(char *buf, int size)
{
    if (NULL == buf) {
        return -1;
    }

    if (size < strlen(HAL_BOARD_CONFIG_FILE) + 1) {
        hal_log(HAL_LOG_ERR, "buf size too small\n");
        return -1;
    }

    strcpy(buf, HAL_BOARD_CONFIG_FILE);
    return 0;
}

int hal_board_get_config_file(char *buf, int size)
{
    if (NULL == hal_if_board.get_config_file)
        return hal_board_get_config_file_common(buf, size);

    return hal_if_board.get_config_file(buf, size);
}
