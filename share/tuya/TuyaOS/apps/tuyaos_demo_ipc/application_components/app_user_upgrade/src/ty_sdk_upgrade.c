/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *FileName: tuya_ipc_dp_.c
  *
  * File Description：
  * 1. API implementation of DP point
  *
  * This file code is the basic code, users don't care it
  * Please do not modify any contents of this file at will. 
  * Please contact the Product Manager if you need to modify it.
  *
**********************************************************************************/
#include "utilities/uni_log.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_iot_config.h"
#include "ty_dp_define.h"
#include "ty_sdk_common.h"
#include "utilities/uni_log.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define TUYA_UNPACK_ENABLE  0
#if defined(TUYA_UNPACK_ENABLE) && (TUYA_UNPACK_ENABLE == 1)
#include "tuya_unpack.h"

ota_stream      *ota_obj = NULL;
#endif
/* OTA */
//Callback after downloading OTA files
OPERATE_RET __IPC_APP_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{

    PR_DEBUG("Upgrade Finish");
    PR_DEBUG("download_result:%d fw_url:%s", download_result, fw->fw_url);

    if(download_result == 0)
    {
        /* The developer needs to implement the operation of OTA upgrade,
        when the OTA file has been downloaded successfully to the specified path. [ p_mgr_info->upgrade_file_path ]*/
    }
    //TODO
    //reboot system
    return OPRT_OK;
}

//To collect OTA files in fragments and write them to local files
OPERATE_RET __IPC_APP_get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,
                             IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("total_len:%d  fw_url:%s", total_len, fw->fw_url);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    //report UPGRADE process, NOT only download percent, consider flash-write time
    //APP will report overtime fail, if uprgade process is not updated within 60 seconds
#if defined(TUYA_UNPACK_ENABLE) && (TUYA_UNPACK_ENABLE == 1)
    int ret = device_upgrade_feed(ota_obj, (char*)data, len);
	if (ret) {
		printf("upgrade feed failed\n");
		return ret;
	}
#endif
    //APP will report "uprage success" after reboot and new FW version is reported inside SDK automaticlly

    return OPRT_OK;
}

int __upgrade_status(int percent, void *user_data)
{    
	TYDEBUG("-------> %d\n", percent);
    tuya_ipc_upgrade_progress_report(percent);
#if defined(TUYA_UNPACK_ENABLE) && (TUYA_UNPACK_ENABLE == 1)
    if (100 == percent){
        device_upgrade_close(ota_obj);
    }
#endif
	return 0;

}

INT_T TUYA_IPC_Upgrade_Inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);
#if defined(TUYA_UNPACK_ENABLE) && (TUYA_UNPACK_ENABLE == 1)
    set_verify_sign_mode(1);
    ota_obj = device_upgrade_open(1, __upgrade_status, NULL);
#endif
    tuya_ipc_upgrade_sdk(fw, __IPC_APP_get_file_data_cb, __IPC_APP_upgrade_notify_cb, NULL);

    return 0;
}
#if 0


#endif