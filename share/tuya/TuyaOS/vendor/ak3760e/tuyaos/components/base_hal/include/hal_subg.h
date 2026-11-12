/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_SUBG_H__
#define __HAL_SUBG_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Send data through subg device.
 * @param buf data buffer.
 * @param size buffer size.
 * @returns On success, the number of bytes sent is returned.
 *          On error, -1 is returned.
 */
int hal_subg_send(void *buf, int size);

/** Receive data from subg device.
 * @param buf data buffer.
 * @param size buffer size.
 * @returns On success, the number of bytes received is returned,
 *          On error, -1 is returned.
 */
int hal_subg_receive(void *buf, int size);


#ifdef __cplusplus
}
#endif

#endif
