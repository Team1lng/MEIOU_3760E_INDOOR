#include "Amy_log.h"
#include "Amy_sdk.h"
#include "ak_common.h"
#include "ak_vi.h"
#include "mjpeg_encode.h"
#include "queues/queues.h"
#include "ak_common_graphics.h"
#include "ak_common.h"
#include "ak_mem.h"
#include "ak_tde.h"
typedef struct snapshot_context_t
{
    // int run;
    char *data;
    ;
    int pixel_w;
    int pixel_h;
    int valid_w;
    int valid_h;
    Amy_mutex_t mutex;
    Amy_thread_t tid;
    mjpeg_encodec_context *venc_dev;
    int (*finish_cb)(const char *data, int size);
} snapshot_context_t;

static void *snapshot_thread(void *arg)
{
    stream_data *vst = NULL;
    char *dst_data = NULL;
    snapshot_context_t *ctx = (snapshot_context_t *)arg;
    bool completed = false;
    struct ak_tde_layer src, dst;

    tde_layer_init(src, ctx->pixel_w, ctx->pixel_h, 0, 0, ctx->valid_w, ctx->valid_h, GP_FORMAT_YUV420SP, ctx->data);
    ctx->valid_w = 1024;
    ctx->valid_h = 600;

    ctx->venc_dev = mjpeg_encode_open(ctx->valid_w, ctx->valid_h);
    if (!ctx->venc_dev)
    {
        AMY_LOG_ERROR("video frame encode failed\n");
        goto exit;
    }
    int dst_size = ctx->valid_w * ctx->valid_h * 3 / 2;
    dst_data = (char *)ak_mem_dma_alloc(MODULE_ID_TDE, dst_size);
    if (!dst_data)
    {
        AMY_LOG_ERROR("video frame alloc failed\n");
        goto exit;
    }

    tde_layer_init(dst, ctx->valid_w, ctx->valid_h, 0, 0, ctx->valid_w, ctx->valid_h, GP_FORMAT_YUV420P, dst_data);
    ak_tde_opt_scale(&src, &dst);
    if (mjpeg_encode_send_frame(ctx->venc_dev, (unsigned char *)dst_data, dst_size, ctx->valid_w, ctx->valid_h))
    {
        AMY_LOG_ERROR("video encode send frame failed\n");
        goto exit;
    }

    if (mjpeg_encode_get_stream(ctx->venc_dev, &vst))
    {
        AMY_LOG_ERROR("video encode get stream failed\n");
        goto exit;
    }
    completed = true;
    if (ctx->finish_cb)
    {
        ctx->finish_cb((const char *)vst->data, vst->size);
    }
    mjpeg_encode_release_stream(ctx->venc_dev, vst);
exit:
    if (!completed && ctx->finish_cb)
        ctx->finish_cb(NULL, 0);
    if (ctx->venc_dev)
    {
        mjpeg_encode_close(ctx->venc_dev);
    }
    if (dst_data)
    {
        ak_mem_dma_free(dst_data);
    }
    Amy_snapshort_event_put(0);
    Amy_free(ctx);
    return NULL;
}

int Amy_snapshot_start(char *data, int pixel_w, int pixel_h, int valid_w, int valid_h, int (*finish_cb)(const char *data, int size))
{
    snapshot_context_t *ctx = (snapshot_context_t *)Amy_malloc(sizeof(snapshot_context_t));
    if (!ctx)
    {
        AMY_LOG_ERROR("alloc snashot context failed\n");
        return -1;
    }
    memset(ctx, 0, sizeof(snapshot_context_t));
    ctx->valid_w = valid_w;
    ctx->valid_h = valid_h;
    ctx->pixel_w = pixel_w;
    ctx->pixel_h = pixel_h;
    //!!! 这里不分配数据，因为直接从解码器获取的，但是需要注意的在编码之前不能关闭解码器。
    ctx->data = data;
    ctx->finish_cb = finish_cb;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
    {
        Amy_free(ctx);
        return -1;
    }
    if (pthread_attr_setstacksize(&attr, 32 * 1024) != 0 ||
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        pthread_attr_destroy(&attr);
        Amy_free(ctx);
        return -1;
    }
    pthread_t tid;
    Amy_snapshort_event_put(1);
    int result = pthread_create(&tid, &attr, snapshot_thread, ctx);
    pthread_attr_destroy(&attr);
    if (result != 0)
    {
        Amy_snapshort_event_put(0);
        Amy_free(ctx);
        return -1; /* No worker owns this request; the caller may retry it. */
    }
    return 0;
}
