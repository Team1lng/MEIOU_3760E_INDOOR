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
#include <sys/stat.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_types.h"
#include "hal_misc.h"
#include "hal_board.h"
#include "hal_file.h"
#include "hal_config.h"
#include "cJSON.h"

static cJSON *sg_jroot = NULL;

static int hal_init_cfg_file(void)
{
    char path[MAX_PATH] = {0};
    int size;
    char *buf = NULL;
    int ret;

    // init only once
    if (sg_jroot != NULL) {
        return 0;
    }

    if (hal_board_get_config_file(path, sizeof(path)) != 0) {
        hal_log(HAL_LOG_ERR, "failed to get cfg file path\n");
        return -1;
    }

    if (hal_file_size(path, &size) != 0) {
        hal_log(HAL_LOG_ERR, "failed to get cfg file size\n");
        return -1;
    }
    hal_log(HAL_LOG_INFO, "cfg file size: %d\n", size);

    // load cfg file

    buf = (char *)malloc(size + 1);
    if (NULL == buf) {
        return -1;
    }
    memset(buf, 0, size + 1);

    ret = hal_file_read(path, buf, size);
    if (ret != size) {
        free(buf);
        buf = NULL;
        return -1;
    }

    // get json root
    sg_jroot = cJSON_Parse(buf);
    if (NULL == sg_jroot) {
        free(buf);
        buf = NULL;
        return -1;
    }

    // buf NOT released when json root is in use
    return 0;
}

static int json_node_to_string(cJSON *j_root, char *node, char *buf, int buf_len)
{
    cJSON *j_node = NULL;

    if (j_root == NULL || node == NULL || buf == NULL) {
        return -1;
    }

    j_node = cJSON_GetObjectItemCaseSensitive(j_root, node);
    if (j_node && cJSON_IsString(j_node) && (j_node->valuestring != NULL)) {
        strncpy(buf, j_node->valuestring, buf_len);
    } else {
        return -1;
    }

    return 0;
}

// get KEY's string value and save it into BUF
int hal_cfg_get_item_string(char *device, char *key, char *buf, int bufsize)
{
    cJSON *j_device = NULL;

    if ((device == NULL) || (key == NULL) || (buf == NULL)) {
        return -1;
    }

    if (hal_init_cfg_file() != 0) {
        return -1;
    }

    if (NULL == sg_jroot) {
        return -1;
    }

    j_device = cJSON_GetObjectItemCaseSensitive(sg_jroot, device);
    if (j_device == NULL) {
        return -1;
    }

    if (json_node_to_string(j_device, key, buf, bufsize) < 0) {
        return -1;
    }

    return 0;
}

// get KEY's integer value
int hal_cfg_get_item_int(char *device, char *key, int *value)
{
    char buf[32] = {0};

    if (value == NULL) {
        return -1;
    }

    if (hal_cfg_get_item_string(device, key, buf, sizeof(buf)) < 0) {
        return -1;
    }

    if ((0 == strncmp(buf, "0x", 2)) || (0 == strncmp(buf, "0X", 2))) {
        sscanf(buf + 2, "%x", value);
    } else {
        sscanf(buf, "%d", value);
    }

    return 0;
}
