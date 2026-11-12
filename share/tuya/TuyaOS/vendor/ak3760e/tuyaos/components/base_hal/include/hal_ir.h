/*
 * Copyright (C) 2021 Tuya Inc.
 */

/*!
 * @file
 * @brief hal interface to control infrared device
 */
#ifndef __HAL_IR_H__
#define __HAL_IR_H__

#include <hal_board.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** product ID definination (deprecated) */
typedef enum hal_board_id {
    HAL_BOARD_COMMON = 0,                /**< common param, no hw related */
    HAL_BOARD_RK3308,                    /**< RK3308 AI remote and speaker*/
    HAL_BOARD_RTL3903_SC021WR2,          /**< RTL3903 IP camera */
    HAL_BOARD_A113X_SPEAKER,             /**< A113X AI speaker */
    HAL_BOARD_KAMINO18_SERCURITY,        /**< KAMINO18 sercurity gateway */
    HAL_BOARD_RTL8197_ZIGBEE_GATEWAY,    /**< RTL8197 zigbee gateway */
    HAL_BOARD_RTL8197_BLUETOOTH_GATEWAY, /**< RTL8197 bluetooth gateway */
} hal_board_id, hal_board_id_e;

typedef enum ir_status{
    IR_OFF = 0,
    IR_ON,
} ir_status, hal_ir_status_e;

/** data for store IR space and pulse time value
 *  codelist space and pulse time value
 *  the first and last code must be space and need space and pulse paired
 *  because above the count must be odd number
 */
typedef struct ir_raw_code{
    int *codelist;          /**< space and pulse time array list */
    unsigned int count;     /**< codelist real data count */
    unsigned int size;      /**< codelist buffer size */
} ir_raw_code, hal_ir_raw_code_t;

/**
 * @brief send IR rawcode signal
 * @param id board ID to open IR device
 *     The <id> is deprecated, user code can set it to any value, e.g. 0
 * @param buf IR rawcode which need to send
 * @return success return 0, failed return -errno
 */
int hal_ir_rawcode_tx(hal_board_id id, ir_raw_code *buf);

/**
 * @brief receive IR rawcode signal
 * @param id board ID to open IR device
 *     The <id> is deprecated, user code can set it to any value, e.g. 0
 * @param buf for store IR rawcode
 * @param timeout 0 is blocked, value is seconds
 * @return success return 0, failed return -errno
 * @note block function, received 1 group of ir signals each time,
 * this func will clear buf.count at the beginning
 */
int hal_ir_rawcode_rx(hal_board_id id, ir_raw_code *buf, struct timeval *timeout);

/**
 * @brief set frequency of tx carrier
 * @param id board ID to open IR device
 *     The <id> is deprecated, user code can set it to any value, e.g. 0
 * @param frequency frequency of the carrier to be set, in Hz
 * @return success return 0, failed return -errno
 */
int hal_ir_set_tx_carrier(hal_board_id id, unsigned int frequency);

/**
 * @brief open lirc device file
 * @param id number of lirc, e.g. 0
 * @param lirc_fd file handle
 * @return success return 0, failed return -errno
 */
int hal_ir_open(int id, int *lirc_fd);

/**
 * @brief close lirc device file
 * @param lirc_fd file handle
 * @return none
 */
void hal_ir_close(int lirc_fd);

/**
 * @brief receive ir raw data
 * @param lirc_fd file handle
 * @param buf  for store IR rawcode
 * @param timeout timeout for a receive
 * @return success return 0, failed return -errno
 * @note this function should be called as soon as
 * possible after hal_ir_open
 */
int hal_ir_recv(int lirc_fd, ir_raw_code *buf, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif
