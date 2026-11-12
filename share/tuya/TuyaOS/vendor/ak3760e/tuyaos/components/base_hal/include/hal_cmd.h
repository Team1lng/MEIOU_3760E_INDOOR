/**
* @file hal_cmd.h
* @brief cmd execute hal interface 
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __HAL_CMD_H__
#define __HAL_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief cmd interface for run execute program and script  
*
* @param[in] cmd: command in string format,Notice:commnad need full path foramt
* @param[out] buffer: command execute output, if no need can set NULL
* @param[in/out] in: buffer len,also return command execute output len
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/

int hal_cmd_run(const char *cmd, char *buffer,int *len);

#ifdef __cplusplus
}
#endif

#endif
