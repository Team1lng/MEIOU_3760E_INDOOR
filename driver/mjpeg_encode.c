#include "mjpeg_encode.h"
#include "Amy_sdk.h"
#include "Amy_log.h"
#include "queues/queues.h"
#include "ringbuffer/ringbuffer.h"
#include "mempool/mempool.h"
#include "ak_common.h"
#include "ak_venc.h"
#define MEM_POOL_ONE_MAX (128*1024)
#define ONEMEM (sizeof(stream_data) + MEM_POOL_ONE_MAX)

typedef struct mjpeg_encodec_context {
    Amy_mutex_t mutex;
    int encode_handle_id;
    queues_conetext* q;
    mp_pool* mp;
    int enc_wdith;
    int enc_height;
}mjpeg_encodec_context;

static int mjpeg_encode_device_open(int w, int h) {
    int handle_id = -1;
    struct venc_param ve_param;
    ve_param.width = w;
    ve_param.height = h;
    ve_param.fps = 25;
    ve_param.goplen = ve_param.fps * 2;
    ve_param.target_kbps = 1024;
    ve_param.max_kbps = 2048;
    ve_param.br_mode = BR_MODE_CBR;
    ve_param.minqp = 25;          //qp set
    ve_param.maxqp = 50;          //qp max value
    ve_param.initqp = (ve_param.minqp + ve_param.maxqp) / 2;    //qp value
    ve_param.jpeg_qlevel = JPEG_QLEVEL_DEFAULT;     //jpeg qlevel
    ve_param.chroma_mode = CHROMA_4_2_0;            //chroma mode
    ve_param.max_picture_size = 0;                  //0 means default
    ve_param.enc_level = 30;                 //enc level
    ve_param.smart_mode = 0;                  //smart mode set
    ve_param.smart_goplen = 100;                //smart mode value
    ve_param.smart_quality = 50;                 //quality
    ve_param.smart_static_value = 0;                //value
    ve_param.enc_out_type = MJPEG_ENC_TYPE;           //enc type
    ve_param.profile = PROFILE_JPEG;
    if (ak_venc_open(&ve_param, &handle_id)) {
        handle_id = -1;
        AMY_LOG_ERROR("open video encode failed\n");
    }
    return handle_id;
}


mjpeg_encodec_context* mjpeg_encode_open(int width, int height) {
    mjpeg_encodec_context* dev = (mjpeg_encodec_context*)Amy_malloc(sizeof(mjpeg_encodec_context));
    if (!dev) {
        AMY_LOG_ERROR("aloc mjpeg encode context failed\n");
        return NULL;
    }
    dev->q = Amy_queues_open(10, 0, 0, NULL, 0);
    if (!dev->q) {
        AMY_LOG_ERROR("open queue failed\n");
        Amy_free(dev);
        return NULL;
    }
    int size = ONEMEM;
    dev->mp = Amy_mempool_init(size * 5, size * 3);
    dev->enc_wdith = width;
    dev->enc_height = height;

    dev->encode_handle_id = mjpeg_encode_device_open(width, height);
    Amy_mutex_init(&dev->mutex, NULL);
    return dev;
}
static void mjepg_encode_data_release_cb(void* user_data, void* data) {
    Amy_mempool_free(user_data, data);
}

int mjpeg_encode_close(mjpeg_encodec_context* ctx) {
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->encode_handle_id >= 0) {
        ak_venc_close(ctx->encode_handle_id);
        ctx->encode_handle_id = -1;
    }
    Amy_queues_close(ctx->q, mjepg_encode_data_release_cb, ctx->mp);
    Amy_mempool_destory(ctx->mp);
    Amy_mutex_unlock(&ctx->mutex);
    Amy_mutex_destroy(&ctx->mutex);
    Amy_free(ctx);
    return 0;
}
int mjpeg_encode_send_frame(mjpeg_encodec_context* ctx, const unsigned char* data, int size, int w, int h) {
    struct video_stream stream;
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->encode_handle_id < 0) {
        AMY_LOG_ERROR("encode id is nullptr \n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    if ((w != ctx->enc_wdith) || (h != ctx->enc_height)) {
        AMY_LOG_ERROR("encode pixel is error \n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    memset(&stream, 0, sizeof(struct video_stream));
    if (ak_venc_encode_frame(ctx->encode_handle_id, data, size, NULL, &stream) == 0) {
        stream_data* d = (stream_data*)Amy_mempool_alloc(ctx->mp, sizeof(stream_data) + stream.len);
        if (d) {
            d->data = (unsigned char*)d + sizeof(stream_data);
            d->size = stream.len;
            Amy_timestamp(d->timestamp);
            memcpy(d->data, stream.data, stream.len);
            if (Amy_queues_put(ctx->q, d)) {
                mjepg_encode_data_release_cb(ctx->mp, d);
            }
        }
        ak_venc_release_stream(ctx->encode_handle_id, &stream);
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int mjpeg_encode_get_stream(mjpeg_encodec_context* ctx, stream_data** data) {
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->encode_handle_id < 0) {
        AMY_LOG_ERROR("encode id is nullptr \n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    if (Amy_queues_get(ctx->q, (void**)data, NULL, NULL)) {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int mjpeg_encode_release_stream(mjpeg_encodec_context* ctx, stream_data* data) {
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->encode_handle_id < 0) {
        AMY_LOG_ERROR("encode id is nullptr \n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    mjepg_encode_data_release_cb(ctx->mp, data);
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

