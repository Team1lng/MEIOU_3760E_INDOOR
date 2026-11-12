#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp.h"

#define RTSP_USERNAME_LEN 64
#define RTSP_PASSWORD_LEN 64
#define RTSP_NONCE_LEN    128
#define RTSP_REALM_LEN    128

typedef void* RTSP_CLIENT_HANDLE;

typedef int (*put_data_cb)(RTSP_CLIENT_HANDLE handle, rtsp_data_packet_s* pkt);

typedef struct {
    char                   url[RTSP_URL_LEN];
    char                   username[RTSP_USERNAME_LEN];
    char                   password[RTSP_PASSWORD_LEN];
    RTSP_TRANSPORT_MODE_E  trans_mode;
    put_data_cb            put_data;
} rtsp_client_param_s;

int rtsp_client_init();
int rtsp_client_deinit();
RTSP_CLIENT_HANDLE rtsp_client_start(rtsp_client_param_s* param);
int rtsp_client_stop(RTSP_CLIENT_HANDLE handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_CLIENT_H__
