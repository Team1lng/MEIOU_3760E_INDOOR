#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "rtp_queue.h"
#include "rtp.h"
#include "tlsf.h"
#include "wheel_timer.h"

typedef enum {
    RTP_QUEUE_PKT_INIT = 0,
    RTP_QUEUE_PKT_RETRANSMITTING = 1,
    RTP_QUEUE_PKT_RECEIVED,
    RTP_QUEUE_PKT_POPPED,
} RTP_QUEUE_PKT_STATUS_E;

typedef struct {
    RTP_QUEUE_PKT_STATUS_E   status;
    uint16_t                 seq;
    union{
        //当status == RTP_QUEUE_PKT_RETRANSMITTING时thdl保存定时器handle，
        //当status == RTP_QUEUE_PKT_RECEIVED时ptr指向数据包
        struct {
            char*            ptr;
            int              len;
        };
        struct {
            WHEEL_TIMER_HANDLE thdl;
            rtp_queue_nack_s   nack_data;
        };
    };
} rtp_queue_slot_s;

typedef struct {
    uint32_t                 pool_size;  //存放rtp包的缓冲区大小
    uint32_t                 queue_len;  //存放rtp包的最大个数
    
    output_rtp_cb            output_rtp; //rtp包按序输出回调
    wheel_timer_expire_cb    send_nack;  //超时重传回调
    void*                    pri_data;   //回调函数参数
    RTSP_SESSION_CHN_E       chn;        //回调函数参数

    uint16_t                 next_seq;   //按序输出的下一个序列号
    uint16_t                 ack_seq;    //最近收到的包或重传的包的序列号
    rtp_queue_slot_s*        ring;

    tlsf_t                   pool;       //存放rtp包的内存池
    int32_t                  alloc_size;

    pthread_mutex_t          mutex;
    RTP_QUEUE_POP_STRATEGY_E strategy;
} rtp_queue_info_s;

static RTP_QUEUE_HANDLE __rtp_queue_info_to_handle(rtp_queue_info_s* prqi)
{
    return (RTP_QUEUE_HANDLE)prqi;
}

static rtp_queue_info_s* __rtp_queue_info_from_handle(RTP_QUEUE_HANDLE handle)
{
    if (RTP_QUEUE_INVALID_HANDLE == handle){
        return NULL;
    }

    return (rtp_queue_info_s*)handle;
}

static inline int __slot_index(rtp_queue_info_s* prqi, uint16_t seq)
{
    return seq % prqi->queue_len;
}

RTP_QUEUE_HANDLE rtp_queue_create(rtp_queue_param_s* param)
{
    if (NULL == param){
        return RTP_QUEUE_INVALID_HANDLE;
    }

    int len = sizeof(rtp_queue_info_s);
    rtp_queue_info_s* prqi = (rtp_queue_info_s*)malloc(len);
    if (NULL == prqi){
        return RTP_QUEUE_INVALID_HANDLE;
    }

    memset(prqi, 0, sizeof(rtp_queue_info_s));
    prqi->chn = param->chn;
    prqi->pool_size = param->pool_size;
    prqi->output_rtp = param->output_rtp;
    prqi->send_nack = param->send_nack;
    prqi->pri_data = param->pri_data;
    prqi->queue_len = param->queue_len;
    prqi->next_seq = param->begin_seq;
    prqi->ack_seq = param->begin_seq;
    prqi->strategy = param->strategy;

    void* mem = malloc(prqi->pool_size);
    if (NULL == mem)
    {
        free(prqi);
        return RTP_QUEUE_INVALID_HANDLE;
    }
    prqi->pool = tlsf_create_with_pool(mem, prqi->pool_size);

    len = sizeof(rtp_queue_slot_s) * param->queue_len;
    prqi->ring = (rtp_queue_slot_s*)malloc(len);
    if (NULL == prqi->ring){
        printf("alloc ring failed\n");
        free(mem);
        free(prqi);
        return RTP_QUEUE_INVALID_HANDLE;
    }

    memset(prqi->ring, 0, len);

    pthread_mutex_init(&prqi->mutex, NULL);

    return __rtp_queue_info_to_handle(prqi);
}

int rtp_queue_destroy(RTP_QUEUE_HANDLE handle)
{
    rtp_queue_info_s* prqi = __rtp_queue_info_from_handle(handle);
    if (NULL == prqi){
        return -1;
    }

    pthread_mutex_destroy(&prqi->mutex);

printf("free %p\n", prqi->ring);
    free(prqi->ring);
    tlsf_destroy(prqi->pool);
    free(prqi->pool);

    free(prqi);

    return 0;
}

/**
 * 从idx开始将连续的包pop出去，返回pop的包数量
 **/
static int __queue_pop_continuous(rtp_queue_info_s* prqi, int idx)
{
    int i = idx;
    int pop_count = 0;
    do{
        rtp_queue_slot_s *slot = &prqi->ring[i];
        if (prqi->next_seq == slot->seq && slot->status == RTP_QUEUE_PKT_RECEIVED){
            if (prqi->output_rtp){
                prqi->output_rtp(prqi->pri_data, prqi->chn, slot->ptr, slot->len);
            }

            tlsf_free(prqi->pool, slot->ptr);
            prqi->alloc_size -= slot->len;
            slot->status = RTP_QUEUE_PKT_POPPED;
            prqi->next_seq += 1;
            pop_count++;

            if (prqi->strategy == RTP_QUEUE_LAZY_POP){
                break;
            }
        }
        else{
            break;
        }

        i = (i + 1) % prqi->queue_len;

    } while (i != idx);

    return pop_count;
}

static int __force_queue_pop(rtp_queue_info_s* prqi)
{
    //在ASAP策略下，next_seq对应的Pkt肯定没收到，所以从下一个slot开始遍历
    //在LAZY策略下，从next_seq开始遍历
    if (prqi->strategy == RTP_QUEUE_ASAP_POP){
        prqi->next_seq++;
    }

    int begin = __slot_index(prqi, prqi->next_seq);
    int end = __slot_index(prqi, prqi->next_seq - 1);
    int i = begin;
    for(; i != end; i = (i + 1) % prqi->queue_len){
        rtp_queue_slot_s* slot = &prqi->ring[i];
        if (prqi->next_seq == slot->seq && slot->status == RTP_QUEUE_PKT_RECEIVED){
            break;
        }
        else if (slot->status == RTP_QUEUE_PKT_RETRANSMITTING){
            slot->status = RTP_QUEUE_PKT_INIT;
            wheel_timer_remove(slot->thdl);
        }
        prqi->next_seq += 1;
    }

    if (i == end){
        printf("force pop failed\n");
        return -1;
    }
    else {
        __queue_pop_continuous(prqi, i);
    }

    return 0;
}

static int __queue_push(rtp_queue_info_s* prqi, uint16_t seq, char* ptr, char* buf, int len)
{
    memcpy(ptr, buf, len);

    int idx = __slot_index(prqi, seq);
    rtp_queue_slot_s* slot = &prqi->ring[idx];
    slot->ptr = ptr;
    slot->len = len;
    slot->seq = seq;
    slot->status = RTP_QUEUE_PKT_RECEIVED;

    return 0;
}

static int __rtp_output(rtp_queue_info_s* prqi, char* buf, int len)
{
    uint16_t seq = prqi->next_seq;
    if (prqi->output_rtp){
        prqi->output_rtp(prqi->pri_data, prqi->chn, buf, len);
    }

    rtp_queue_slot_s* slot = &prqi->ring[__slot_index(prqi, seq)];
    if (RTP_QUEUE_PKT_RETRANSMITTING == slot->status){
        slot->status = RTP_QUEUE_PKT_INIT;
        wheel_timer_remove(slot->thdl);
    }

    slot->seq = seq;
    slot->status = RTP_QUEUE_PKT_POPPED;
    prqi->next_seq += 1;
    //尝试pop后续连续的包
    __queue_pop_continuous(prqi, __slot_index(prqi, prqi->next_seq));
    return 0;
}

static int __queue_prepare_and_push(rtp_queue_info_s* prqi, rtp_header_t* phdr, char* buf, int len)
{
    int idx = __slot_index(prqi, phdr->seq);
    rtp_queue_slot_s* slot = &prqi->ring[idx];
    if (slot->seq == phdr->seq && slot->status >= RTP_QUEUE_PKT_RECEIVED){
        //重复包，忽略
        return 0;
    }

    char* ptr = NULL;
    do {
        ptr = tlsf_malloc(prqi->pool, len);
        if (NULL == ptr){
            //printf("mem pool exhausted, alloc(%d), force pop\n", prqi->alloc_size);
            if (-1 == __force_queue_pop(prqi)){
                return -1;
            }
        }
    } while(NULL == ptr);
    prqi->alloc_size += len;

    //如果环形数组被套圈，则强制pop出去
    while(phdr->seq - prqi->next_seq >= prqi->queue_len){
        int idx = __slot_index(prqi, prqi->next_seq);
        if (0 == __queue_pop_continuous(prqi, idx)){
            if (prqi->ring[idx].status == RTP_QUEUE_PKT_RETRANSMITTING){
                prqi->ring[idx].status = RTP_QUEUE_PKT_INIT;
                wheel_timer_remove(prqi->ring[idx].thdl);
            }
            prqi->next_seq++;
        }
    }

    if (slot->status == RTP_QUEUE_PKT_RETRANSMITTING){
        slot->status = RTP_QUEUE_PKT_INIT;
        wheel_timer_remove(slot->thdl);
    }
    //prepare完成后如果当前包就是下一个包，则释放内存，直接输出
    if (phdr->seq == prqi->next_seq && RTP_QUEUE_ASAP_POP == prqi->strategy){
        tlsf_free(prqi->pool, ptr);
        prqi->alloc_size -= len;
        __rtp_output(prqi, buf, len);
    }
    else{
        __queue_push(prqi, phdr->seq, ptr, buf, len);
    }

    return 0;
}

int rtp_queue_push(RTP_QUEUE_HANDLE handle, char* buf, int len)
{
    rtp_queue_info_s* prqi = __rtp_queue_info_from_handle(handle);
    if (NULL == prqi){
        return -1;
    }

    rtp_header_t hdr = {0};
    int hdr_len = rtp_unpack_header(&hdr, buf, len);
    if (hdr_len <= 0){
        printf("parse header failed\n");
        return -1;
    }

    pthread_mutex_lock(&prqi->mutex);
    //当前包是按序的下一个包, 可以直接输出
    if (prqi->next_seq == hdr.seq && RTP_QUEUE_ASAP_POP == prqi->strategy){
        __rtp_output(prqi, buf, len);
        prqi->ack_seq = hdr.seq;
    }
    //当前包比按序的下一个包小
    else if ((uint16_t)(prqi->next_seq - hdr.seq) < (0xffffU >> 1)) {
        //ignore
    }
    //当前包比按序的下一个包大, 放进队列中
    else {
        if (RTP_QUEUE_ASAP_POP == prqi->strategy) {
            //将中间丢失的包设置一个定时器，超时后发送NACK包要求服务端重传
            printf("timer from %u - %u\n", prqi->ack_seq + 1, hdr.seq);
            uint16_t i;
            for(i = prqi->ack_seq + 1; i != hdr.seq; i++){
                rtp_queue_slot_s* slot = &prqi->ring[__slot_index(prqi, i)];
                if (slot->status != RTP_QUEUE_PKT_RETRANSMITTING && slot->status != RTP_QUEUE_PKT_RECEIVED) {
                    slot->status = RTP_QUEUE_PKT_RETRANSMITTING;
                    slot->seq = i;

                    slot->nack_data.pri_data = prqi->pri_data;
                    slot->nack_data.seq = i;
                    slot->nack_data.chn = prqi->chn + 1;
                    slot->thdl = wheel_timer_add(40, prqi->send_nack, &slot->nack_data);
                    printf("add timer: seq(%u) chn(%d)\n", slot->nack_data.seq, slot->nack_data.chn);
                }
            }
        }
        prqi->ack_seq = hdr.seq;
        __queue_prepare_and_push(prqi, &hdr, buf, len);
    }
    pthread_mutex_unlock(&prqi->mutex);

    return 0;
}

int rtp_queue_get(RTP_QUEUE_HANDLE handle, uint16_t seq, char* buf, int* plen)
{
    rtp_queue_info_s* prqi = __rtp_queue_info_from_handle(handle);

    int idx = __slot_index(prqi, seq);
    rtp_queue_slot_s* pslot = &prqi->ring[idx];
    int ret = 0;

    pthread_mutex_lock(&prqi->mutex);
    if (pslot->seq == seq && pslot->status == RTP_QUEUE_PKT_RECEIVED){
        if (*plen >= pslot->len){
            memcpy(buf, pslot->ptr, pslot->len);
            *plen = pslot->len;

            ret = 0;
        }
        else{
            ret = -1;
        }
    }
    else{
        ret = -1;
    }
    pthread_mutex_unlock(&prqi->mutex);

    return ret;
}