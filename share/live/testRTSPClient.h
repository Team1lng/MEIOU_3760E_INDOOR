#ifndef _TEST_RTSP_CLIENT_H_
#define _TEST_RTSP_CLIENT_H_
typedef struct rtsp_client_context rtsp_client_context;

// ifname: 拉流绑定的接收网卡（door=br0 / cctv=wlan0），NULL 或空串时不绑定（沿用当前全局值）
rtsp_client_context *live555_rtsp_client_start(const char *rtsp_uri, const char *ifname, int dx, int dy, int dw, int dh);

int live555_rtsp_client_close(rtsp_client_context *ctx);

int live555_rtsp_request_record_start(rtsp_client_context *ctx, int (*recvideo_cb)(const char *, int));

int live555_rtsp_request_record_stop(rtsp_client_context *ctx);

int live555_rtsp_request_snapshort_start(rtsp_client_context *ctx, int (*snapshort_cb)(const char *, int));
/* Cancel only a request not yet handed to the encoder; completion runs unlocked. */
int live555_rtsp_request_snapshort_cancel(rtsp_client_context *ctx);

int live555_rtsp_request_volume_set(rtsp_client_context *ctx, int volume);

int openRTSPServer(void);

int setMulticastInterface(const char *ipAddr);

// 客户端拉流：仅设置接收绑定接口，不修改服务端发送接口
int setReceivingInterface(const char *ipAddr);

void rtsp_server_request_restart(void);

int tuya_audio_enable(void); // 在通过手机指令进入的监控，需要调用此函数设置服务端音频输入源
int tuya_audio_disable(void);
int open_client_speaker();
int open_server_speaker();
int close_client_speaker();
int close_server_speaker();
inline void rtsp_speaker_enable(uint8_t en)
{
    if (en)
    {
        open_client_speaker();
        open_server_speaker();
    }
    else
    {
        close_client_speaker();
        close_server_speaker();
    }
}
#endif
