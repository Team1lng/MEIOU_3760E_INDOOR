#include "video_record.h"
#include "Amy_log.h"
#include "queues/queues.h"
#include "ringbuffer/ringbuffer.h"
#include "mempool/mempool.h"
#include "avilib.h"
#include "mjpeg_encode.h"
#include "audio_input.h"
#include "ak_common.h"
#include "ak_vi.h"
#include <stdatomic.h> /* [FIX] 引入原子操作头文件 */

typedef struct avi_record_context
{
    int run;
    Amy_thread_t tid;

    /* [FIX] vframe_cnt 改为原子类型，消除主线程读、录制线程写的竞态 */
    atomic_ullong vframe_cnt;

    unsigned long long start_timestamp;
    unsigned long long finish_timestamp;

    mp_pool *mp;
    queues_conetext *vq;
    queues_conetext *aq;
    avi_t *avi_dev;

    int sample_rate;
    int video_width;
    int video_height;
    char file_path[128];
    char compressor[8];
    int (*record_finish_cb)(const char *file, int);
} avi_record_context;

static void avi_mem_release_cb(void *user_data, void *data)
{
    Amy_mempool_free((mp_pool *)user_data, data);
}

static void avi_record_head_write(avi_record_context *ctx, stream_data *vst)
{
    if (!ctx->avi_dev)
    {
        sprintf(ctx->file_path, "%s.tmp", ctx->file_path);
        ctx->avi_dev = AVI_open_output_file(ctx->file_path);
    }

    if (ctx->avi_dev)
    {
        AVI_set_video(ctx->avi_dev, ctx->video_width, ctx->video_height, 0, ctx->compressor);
        if (ctx->aq)
        {
            AVI_set_audio(ctx->avi_dev, 1, ctx->sample_rate, 16, 1,
                          ctx->sample_rate == 8000 ? 128 : 256);
        }
        AVI_write_frame(ctx->avi_dev, (char *)vst->data, vst->size, 1);
    }

    AMY_LOG_INFO("-----------record %s head write success!\n", ctx->file_path);
}

static int avi_record_tail_write(avi_record_context *ctx)
{
    unsigned long long duration = ctx->finish_timestamp > ctx->start_timestamp
                                      ? ctx->finish_timestamp - ctx->start_timestamp : 0;
    /* [FIX] 读取原子变量用 atomic_load */
    unsigned long long vframe_cnt = atomic_load(&ctx->vframe_cnt);
    double fps = duration ? 1000.0 * vframe_cnt / duration : 1.0;
    ctx->avi_dev->fps = fps;
    AVI_close(ctx->avi_dev);
    ctx->avi_dev = NULL;
    AMY_LOG_INFO("record %s fps:%.2f, duration:%llums, frame cnt:%llu\n",
                 ctx->file_path,
                 fps,
                 duration,
                 vframe_cnt);
    return 0;
}

static void *avi_record_thread(void *arg)
{
    avi_record_context *ctx = (avi_record_context *)arg;
    stream_data *vst = NULL, *ast = NULL;

    while (ctx->run)
    {
        if (ctx->vq && (vst == NULL))
        {
            if (Amy_queues_get(ctx->vq, (void **)&vst, NULL, NULL))
            {
                vst = NULL;
            }
        }
        if (ctx->aq && (ast == NULL))
        {
            if (Amy_queues_get(ctx->aq, (void **)&ast, NULL, NULL))
            {
                ast = NULL;
            }
        }

        if ((ast == NULL) && (vst == NULL))
        {
            Amy_msleep(10);
            continue;
        }

        /* [FIX] 读取原子变量用 atomic_load */
        if (atomic_load(&ctx->vframe_cnt) == 0)
        {
            if (ast)
            {
                avi_mem_release_cb(ctx->mp, ast);
                ast = NULL;
            }
            if (vst == NULL)
            {
                Amy_msleep(10);
                continue;
            }
            if (!vst->keyframe)
            {
                avi_mem_release_cb(ctx->mp, vst);
                vst = NULL;
                continue;
            }
            avi_record_head_write(ctx, vst);
            ctx->start_timestamp = vst->timestamp;
            avi_mem_release_cb(ctx->mp, vst);
            vst = NULL;
            /* [FIX] 写原子变量用 atomic_fetch_add，保证对外可见 */
            atomic_fetch_add(&ctx->vframe_cnt, 1);
        }
        else if (ctx->avi_dev)
        {
            if ((vst) && ((ast == NULL) || (vst->timestamp <= ast->timestamp)))
            {
                AVI_write_frame(ctx->avi_dev, (char *)vst->data, vst->size,
                                vst->keyframe ? 1 : 0);
                ctx->finish_timestamp = vst->timestamp;
                avi_mem_release_cb(ctx->mp, vst);
                /* [FIX] 同上 */
                atomic_fetch_add(&ctx->vframe_cnt, 1);
                vst = NULL;
            }
            else if ((ast) && ((vst == NULL) || (ast->timestamp <= vst->timestamp)))
            {
                AVI_write_audio(ctx->avi_dev, (char *)ast->data, ast->size);
                avi_mem_release_cb(ctx->mp, ast);
                ast = NULL;
            }
        }
    }

    /* 线程退出时清理未消费的帧 */
    if (ast)
    {
        avi_mem_release_cb(ctx->mp, ast);
    }
    if (vst)
    {
        avi_mem_release_cb(ctx->mp, vst);
    }

    if (ctx->avi_dev)
    {
        avi_record_tail_write(ctx);
    }
    if (ctx->record_finish_cb)
    {
        unsigned long long duration = ctx->finish_timestamp > ctx->start_timestamp
                                          ? ctx->finish_timestamp - ctx->start_timestamp : 0;
        ctx->record_finish_cb(ctx->file_path, duration / 1000);
    }

    Amy_record_event_put(0);
    AMY_LOG_INFO("record thread finish\n");
    return NULL;
}

avi_record_context *avi_record_open(int w, int h, int sample_rate,
                                    const char *path, const char *compressor,
                                    int (*video_record_finish_func)(const char *file, int))
{
    int mem_size = sizeof(avi_record_context) + sizeof(stream_data) + 1024 * 1024;
    mp_pool *mp = Amy_mempool_init(mem_size, mem_size);
    if (!mp)
    {
        AMY_LOG_ERROR("mempool init failed\n");
        return NULL;
    }

    avi_record_context *ctx =
        (avi_record_context *)Amy_mempool_alloc(mp, sizeof(avi_record_context));
    if (!ctx)
    {
        /* [FIX] 原代码此处用了未初始化的 ctx->mp，改为直接用局部 mp */
        Amy_mempool_destory(mp);
        AMY_LOG_ERROR("mempool alloc failed\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(avi_record_context));
    ctx->mp = mp;

    ctx->vq = Amy_queues_open(64, 0, 0, NULL, 0);
    if (!ctx->vq)
    {
        Amy_mempool_destory(ctx->mp);
        AMY_LOG_ERROR("open video queues failed\n");
        return NULL;
    }

    if (sample_rate)
    {
        ctx->aq = Amy_queues_open(64, 0, 0, NULL, 0);
        if (!ctx->aq)
        {
            /* [FIX] 原代码忘记关闭已打开的 vq，补上 */
            Amy_queues_close(ctx->vq, avi_mem_release_cb, ctx->mp);
            Amy_mempool_destory(ctx->mp);
            AMY_LOG_ERROR("open audio queues failed\n");
            return NULL;
        }
    }

    ctx->sample_rate = sample_rate;
    ctx->video_width = w;
    ctx->video_height = h;
    /* [FIX] 原子变量显式初始化 */
    atomic_init(&ctx->vframe_cnt, 0);
    ctx->avi_dev = NULL;
    ctx->record_finish_cb = video_record_finish_func;
    memset(ctx->file_path, 0, sizeof(ctx->file_path));
    strncpy(ctx->file_path, path, sizeof(ctx->file_path) - 1);
    memset(ctx->compressor, 0, sizeof(ctx->compressor));
    strncpy(ctx->compressor, compressor, sizeof(ctx->compressor) - 1);
    ctx->run = 1;

    Amy_record_event_put(1);
    Amy_pthread_create(&ctx->tid, avi_record_thread, ctx, 128 * 1024, 12);
    return ctx;
}

int avi_record_close(avi_record_context *ctx)
{
    if (!ctx)
        return -1;

    ctx->run = 0;
    Amy_pthread_join(ctx->tid, NULL);

    if (ctx->aq)
    {
        Amy_queues_close(ctx->aq, avi_mem_release_cb, ctx->mp);
    }
    if (ctx->vq)
    {
        Amy_queues_close(ctx->vq, avi_mem_release_cb, ctx->mp);
    }

    Amy_mempool_destory(ctx->mp);
    return 0;
}

stream_data *avi_buffer_get(avi_record_context *ctx, int size)
{
    stream_data *d =
        (stream_data *)Amy_mempool_alloc(ctx->mp, sizeof(stream_data) + size);
    if (!d)
    {
        AMY_LOG_ERROR("mempool alloc failed(%d)\n", size);
        AMY_LOG_ERROR("used:%d, total=%d\n",
                      Amy_prog_memory_get(ctx->mp),
                      Amy_total_memory_total(ctx->mp));
        AMY_LOG_ERROR("video queues: used:%d, total:%d\n",
                      Amy_queues_new_els(ctx->vq),
                      Amy_queues_max_els(ctx->vq));
        AMY_LOG_ERROR("audio queues: used:%d, total:%d\n",
                      Amy_queues_new_els(ctx->aq),
                      Amy_queues_max_els(ctx->aq));
        return NULL;
    }
    d->data = (unsigned char *)d + sizeof(stream_data);
    d->size = size;
    d->keyframe = 0;
    return d;
}

int avi_buffer_put(avi_record_context *ctx, stream_data *data)
{
    avi_mem_release_cb(ctx->mp, data);
    return 0;
}

int avi_video_send(avi_record_context *ctx, stream_data *data)
{
    int res = Amy_queues_put(ctx->vq, data);
    if (res)
    {
        AMY_LOG_ERROR("send video queue failed\n");
    }
    return res;
}

int avi_audio_send(avi_record_context *ctx, stream_data *data)
{
    /* [FIX] 用 atomic_load 读取 vframe_cnt，避免与录制线程的写操作竞态 */
    if ((!ctx->aq) || (atomic_load(&ctx->vframe_cnt) < 1))
    {
        return -1;
    }
    int res = Amy_queues_put(ctx->aq, data);
    if (res)
    {
        AMY_LOG_ERROR("send audio queue failed\n");
    }
    return res;
}

int avi_has_audio(avi_record_context *ctx)
{
    /* sample_rate 和 aq 在 open 后只读，不需要锁 */
    return (ctx->sample_rate && ctx->aq);
}