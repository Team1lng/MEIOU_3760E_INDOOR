#ifndef __RTP_QUEUE_H__
#define __RTP_QUEUE_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp_session.h"
#include "wheel_timer.h"

#define RTP_QUEUE_INVALID_HANDLE ((void*)0)
typedef void* RTP_QUEUE_HANDLE;
typedef int (*output_rtp_cb)(void* pri_data, RTSP_SESSION_CHN_E chn, char* buf, int len);

typedef enum {
    RTP_QUEUE_LAZY_POP,                  //直到新rtp包放不下才会pop最旧的
    RTP_QUEUE_ASAP_POP,                  //as soon as possible
} RTP_QUEUE_POP_STRATEGY_E;

typedef struct {
    uint32_t                 pool_size;  //存放rtp包的缓冲区大小
    uint32_t                 queue_len;  //存放rtp包的最大个数
    RTP_QUEUE_POP_STRATEGY_E strategy;   //rtp包pop策略

    uint16_t                 begin_seq;  //起始rtp包的序列号
    
    output_rtp_cb            output_rtp; //rtp包按序输出回调
    wheel_timer_expire_cb    send_nack;  //超时重传定时器回调
    void*                    pri_data;   //回调函数参数
    RTSP_SESSION_CHN_E       chn;        //回调函数参数
} rtp_queue_param_s;

/**
 * 超时重传回调函数中把这个结构体传给上层
 */
typedef struct {
    void*                    pri_data;   //同rtp_queue_param_s中的pri_data;
    uint16_t                 seq;        //重传的rtp包的序列号
    RTSP_SESSION_CHN_E       chn;        //RTSP_VID_CTRL_CHN或RTSP_AUD_CTRL_CHN
} rtp_queue_nack_s;

RTP_QUEUE_HANDLE rtp_queue_create(rtp_queue_param_s* param);

int rtp_queue_destroy(RTP_QUEUE_HANDLE handle);

int rtp_queue_push(RTP_QUEUE_HANDLE handle, char* buf, int len);

int rtp_queue_get(RTP_QUEUE_HANDLE handle, uint16_t seq, char* buf, int* plen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTP_QUEUE_H__

