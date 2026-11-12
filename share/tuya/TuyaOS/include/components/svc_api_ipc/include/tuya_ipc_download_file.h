#ifndef __TUYA_IPC_DOWNLOAD_FILE_H__
#define __TUYA_IPC_DOWNLOAD_FILE_H__

#include "ty_cJSON.h"
#include "tuya_cloud_types.h"
#include "tuya_iot_internal_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef VOID (*DOWNLOAD_NOTIFY_CB)(IN CONST INT_T download_result, IN PVOID_T pri_data);
typedef VOID (*BACKGROUND_PIC_BUF_FREE)(CHAR_T *buf);

typedef struct 
{
    CHAR_T *                        url;//资源链接
    BOOL_T                          check_hmac; //是否需要校验hmac
    CHAR_T                          file_hmac[64]; //如果需要校验hmac，此处提供需要匹配的hmac 
    UINT_T                          total_file_size; //文件大小。可以设置为0
    UINT_T                          fragment_size; //内部用来下载的内存大小
    DOWNLOAD_NOTIFY_CB              notify_cb; //成功或失败的通知回调
    IOT_HTTP_GET_FILE_DATA_CB       data_cb; //数据回调
    PVOID_T                         pri_data; //客户自己关心的数据，内部通过notify_cb和data_cb回调出来
}IPC_DOWNLOAD_REQ_T;

typedef struct
{
    CHAR_T *                        pic_buf;//图片内存
    INT_T                           pic_len;//图片长度
    BACKGROUND_PIC_BUF_FREE         free_cb;//使用后，释放pic_buf的回调
}BACKGROUND_PIC_RET_T;

/**
 * 基础接口。业务无关，仅提供下载文件功能
 * 封装基础的iot下载文件功能，按照IPC_DOWNLOAD_REQ_T提供参数后，通过data_cb将数据回调出来
 * 您需要根据data_cb的数据，来自行处理数据分片
*/
OPERATE_RET tuya_ipc_download_file(IPC_DOWNLOAD_REQ_T *req);

/**
 * 带屏摄像机下载背景图功能
 * 提供要下载的文件名称
 * 结果通过BACKGROUND_PIC_RET_T返回出来
 * 请完成pic_buf的拷贝后，使用free_cb释放该内存
 * ！注意：保证单线程调用
*/
OPERATE_RET tuya_ipc_download_background_pic(CHAR_T *name, BACKGROUND_PIC_RET_T * result);

#ifdef __cplusplus
}
#endif

#endif

