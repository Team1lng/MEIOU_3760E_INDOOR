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


typedef enum {
    RTSP_CLIENT_STATUS_INIT = 0,         //初始状态，未建立tcp连接
    RTSP_CLIENT_STATUS_CONNECTING,       //异步connect时正在连接状态
    RTSP_CLIENT_STATUS_CONNECTED,        //连接成功
    RTSP_CLIENT_STATUS_CONNECT_FAIL,     //连接失败
    RTSP_CLIENT_STATUS_OPTIONS_SENT,     //已发送OPTIONS请求
    RTSP_CLIENT_STATUS_OPTIONS_RESP,     //收到OPTIONS请求的回复
    RTSP_CLIENT_STATUS_DESCRIBE_SENT,    //已发送DESCRIBE请求的回复
    RTSP_CLIENT_STATUS_DESCRIBE_RESP,    //收到DESCRIBE请求的回复
    RTSP_CLIENT_STATUS_SETUP_VID_SENT,   //已发送视频SETUP请求
    RTSP_CLIENT_STATUS_SETUP_VID_RESP,   //收到视频SETUP请求的回复
    RTSP_CLIENT_STATUS_SETUP_AUD_SENT,   //已发送音频SETUP请求
    RTSP_CLIENT_STATUS_SETUP_AUD_RESP,   //收到音频SETUP请求的回复
    RTSP_CLIENT_STATUS_PLAY_SENT,        //已发送PLAY请求
    RTSP_CLIENT_STATUS_PLAY_RESP,        //收到PLAY请求的回复
    RTSP_CLIENT_STATUS_PLAYING,          //正在进行RTP拉流
    RTSP_CLIENT_STATUS_TEARDOWN_SENT,    //已发送TEARDOWN请求
    RTSP_CLIENT_STATUS_TEARDOWN_RESP,    //收到TEARDOWN请求的回复
    RTSP_CLIENT_STATUS_STOP,

    RTSP_CLIENT_STATUS_NUM,
} RTSP_CLIENT_STATUS_E;

typedef int (*rtsp_client_status_change_cb)(RTSP_CLIENT_HANDLE handle, RTSP_CLIENT_STATUS_E status, void* priv);

typedef int (*rtsp_client_put_data_cb)(RTSP_CLIENT_HANDLE handle, rtsp_data_packet_s* pkt, void* priv);

typedef struct {
    char                              url[RTSP_URL_LEN];
    char                              username[RTSP_USERNAME_LEN];
    char                              password[RTSP_PASSWORD_LEN];
    RTSP_TRANSPORT_MODE_E             trans_mode;
    rtsp_client_put_data_cb           put_data_cb;
    rtsp_client_status_change_cb      status_change_cb;
    void*                             priv;
} rtsp_client_param_s;

int rtsp_client_init();
int rtsp_client_deinit();
RTSP_CLIENT_HANDLE rtsp_client_start(rtsp_client_param_s* param);
int rtsp_client_stop(RTSP_CLIENT_HANDLE handle);
int rtsp_client_get_media_info(RTSP_CLIENT_HANDLE handle, rtsp_media_info_t pmedia_info[RTSP_MEDIA_TYPE_NUM]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_CLIENT_H__
