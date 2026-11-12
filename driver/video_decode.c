#include "video_decode.h"
#include "Amy_log.h"
#include "ak_common.h"
#include "ak_vdec.h"
#include "queues/queues.h"
#include "ringbuffer/ringbuffer.h"
#include "mempool/mempool.h"
#include "device/lv_disp_dev.h"

#define MEM_POOL_ONE_MAX (2048 * 1024)
#define ONEMEM (sizeof(stream_data) + MEM_POOL_ONE_MAX)

typedef struct video_decode_context
{
    int codec_dev;
    queues_conetext *q;
    mp_pool *mp;

    int w, h;
    struct ak_vdec_frame lastframe;
    unsigned long long last_ts;
    Amy_mutex_t mutex;
    int run;
    Amy_thread_t tid;
} video_decode_context;

static void video_stream_release_cb(void *user_data, void *data)
{
    mp_pool *mp = (mp_pool *)user_data;
    Amy_mempool_free(mp, data);
}

static void video_send_frame(int handle_id, unsigned char *data, int read_len)
{
    int send_len = 0;
    int dec_len = 0;
    int ret = 0;

    while (read_len > 0)
    {
        ret = ak_vdec_send_stream(handle_id, &data[send_len], read_len, 0, &dec_len);
        if (ret != 0)
        {
            AMY_LOG_ERROR("write video data failed!:%#X %#X %#X %#X\n", data[0], data[1], data[2], data[3]);
            break;
        }
        read_len -= dec_len;
        send_len += dec_len;
    }
    ak_vdec_frame_decode(handle_id);
}
static void *video_decodec_thread(void *arg)
{
    video_decode_context *ctx = (video_decode_context *)arg;
    stream_data *vst = NULL;
    struct ak_vdec_frame frame;
    while ((ctx->run))
    {
        int remain = Amy_queues_new_els(ctx->q);
        if (remain <= 0)
        {
            continue;
        }
        // Amy_mutex_lock(&ctx->mutex);
        // if (ctx->last_ts != ctx->lastframe.ts)
        // {
        //     Amy_mutex_unlock(&ctx->mutex);
        //     continue;
        // }
        // Amy_mutex_unlock(&ctx->mutex);
        if (Amy_queues_get(ctx->q, (void **)&vst, NULL, NULL) == 0)
        {
            if (vst->size > 0)
            {
                video_send_frame(ctx->codec_dev, vst->data, vst->size);
            }
            video_stream_release_cb(ctx->mp, vst);
        }

        memset(&frame, 0, sizeof(struct ak_vdec_frame));
        if (ak_vdec_get_frame(ctx->codec_dev, &frame) == 0)
        {
            Amy_timestamp(frame.ts);
            Amy_mutex_lock(&ctx->mutex);
            memcpy(&ctx->lastframe, &frame, sizeof(struct ak_vdec_frame));
            Amy_mutex_unlock(&ctx->mutex);
            Amy_video_frame_notify(); // 解出新帧，唤醒显示线程立即渲染
            ak_vdec_release_frame(ctx->codec_dev, &frame);
        }
    }
    ak_vdec_end_stream(ctx->codec_dev);

    int status = 0;
    while (1)
    {
        memset(&frame, 0, sizeof(struct ak_vdec_frame));
        if (ak_vdec_get_frame(ctx->codec_dev, &frame) == 0)
        {
            ak_vdec_release_frame(ctx->codec_dev, &frame);
        }
        else
        {
            ak_vdec_get_decode_finish(ctx->codec_dev, &status);
            if (status)
            {
                break;
            }
        }
    }
    //  AMY_LOG_ERROR("video decodec queus:%d\n", Amy_queues_new_els(ctx->q));
    AMY_LOG_INFO("video decode finish\n");
    return NULL;
}

video_decode_context *video_decodec_open(int max_w, int max_h, int enc_type)
{
    int size = ONEMEM * 2 + sizeof(video_decode_context);
    mp_pool *mp = Amy_mempool_init(size, size);
    if (!mp)
    {
        AMY_LOG_ERROR("open queue failed\n");
        return NULL;
    }

    video_decode_context *d = (video_decode_context *)Amy_mempool_alloc(mp, sizeof(video_decode_context));
    if (!d)
    {
        AMY_LOG_ERROR("alloc video decodec failed\n");
        return NULL;
    }
    memset(d, 0, sizeof(struct video_decode_context));
    d->q = Amy_queues_open(10, 0, 0, NULL, 1);
    if (!d->q)
    {
        AMY_LOG_ERROR("open queue failed\n");
        video_stream_release_cb(mp, d);
        Amy_mempool_destory(mp);
        return NULL;
    }
    d->mp = mp;
    Amy_mutex_init(&d->mutex, NULL);
    memset(&(d->lastframe), 0, sizeof(struct ak_vdec_frame));

    struct ak_vdec_param param = {0};
    param.vdec_type = enc_type;
    param.sc_width = max_w;
    param.sc_height = max_h;
    param.output_type = AK_YUV420SP;
    param.frame_buf_num = 3;
    param.stream_buf_size = 3 * 1024 * 1024;
    if (ak_vdec_open(&param, &d->codec_dev))
    {
        AMY_LOG_ERROR("open vdec open failed\n");
        Amy_queues_close(d->q, video_stream_release_cb, d->mp);
        Amy_mempool_destory(mp);
        return NULL;
    }
    d->w = max_w;
    d->h = max_h;
    d->last_ts = 0;
    d->run = 1;
    Amy_pthread_create(&d->tid, video_decodec_thread, d, 32 * 1024, 12);
    AMY_LOG_INFO("open video decodec:%dx%d type:%d\n", max_w, max_h, enc_type);
    return d;
}

stream_data *video_decodec_buf_get(video_decode_context *ctx, int size)
{
    stream_data *d = (stream_data *)Amy_mempool_alloc(ctx->mp, sizeof(stream_data) + size);
    if (!d)
    {
        AMY_LOG_ERROR("mempool alloc failed\n");
        return NULL;
    }
    d->data = (unsigned char *)d + sizeof(stream_data);
    d->size = size;
    return d;
}

int video_decodec_buf_put(video_decode_context *ctx, stream_data *data)
{
    video_stream_release_cb(ctx->mp, data);
    return 0;
}

int video_decodec_send_stream(video_decode_context *ctx, stream_data *data)
{
    return Amy_queues_put(ctx->q, data);
}

int video_decodec_stream_query(video_decode_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total)
{
    if (total)
    {
        *total = Amy_queues_max_els(ctx->q);
    }
    if (used)
    {
        *used = Amy_queues_new_els(ctx->q);
    }
    if (free)
    {
        *free = Amy_queues_max_els(ctx->q) - Amy_queues_new_els(ctx->q);
    }
    return 0;
}

int video_decodec_close(video_decode_context *ctx)
{
    ctx->run = 0;
    Amy_queues_enable(ctx->q, 0);
    Amy_pthread_join(ctx->tid, NULL);
    Amy_queues_close(ctx->q, video_stream_release_cb, ctx->mp);
    ak_vdec_close(ctx->codec_dev);
    Amy_mutex_destroy(&ctx->mutex);
    Amy_mempool_destory(ctx->mp);
    return 0;
}

int video_decodec_get_frame(video_decode_context *ctx, struct ak_vdec_frame *frame, int *is_new)
{
    int ret = -1;
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->lastframe.frame_obj.data.data_size)
    {
        memcpy(frame, &ctx->lastframe, sizeof(struct ak_vdec_frame));
        if (ctx->last_ts != ctx->lastframe.ts)
        {
            ctx->last_ts = ctx->lastframe.ts;
            if (is_new)
            {
                *is_new = 1;
            }
        }
        else if (is_new)
        {
            *is_new = 0;
        }
        ret = 0;
    }
    Amy_mutex_unlock(&ctx->mutex);
    return ret;
}
