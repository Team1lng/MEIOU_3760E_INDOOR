#include "Amy_audio_play.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Amy_log.h"
#include "Amy_sdk.h"
#include "audio_output.h"
#include "res.h"
#include "queues/queues.h"
#include "mempool/mempool.h"
#include "mad.h"
#include "audio_output.h"

#define MP3_BUF_MAX (1 * 1024)

#define RING_PLAY_STATUS_STOP 0
#define RING_PLAY_STATUS_PLAY 1

typedef struct
{
    int fd;
    int pos;
    unsigned long offset;
    int file_size;

    char *data;
    int data_size;
} mad_buffer;

typedef struct ring_play_request
{
    unsigned int request_id;
    int interrupt_enable;
    int play_index;
    int volume;
    void (*play_start_cb)(int);
    int (*play_finish_cb)(int);
} ring_play_request;

typedef struct ring_play_param
{
    int run;
    Amy_thread_t tid;
    Amy_mutex_t mutex;
    Amy_cond_t cond;
    int status;

    char *mtd_name;

    int sample_rate;
    int audio_dev_cleared;

    mad_buffer buffer;
    audio_output_context *audio_dev;

    const audio_ring_resource *resource;
    int resource_cnt;
    unsigned int next_request_id;
    unsigned int stop_request_id;
    ring_play_request pending;
    ring_play_request active;
} ring_play_param;

static ring_play_param ring_play_ctx;

static int audio_request_cancelled(ring_play_param *ctx)
{
    int cancelled;

    Amy_mutex_lock(&ctx->mutex);
    cancelled = (ctx->active.request_id != ctx->next_request_id) ||
                (ctx->stop_request_id == ctx->active.request_id);
    Amy_mutex_unlock(&ctx->mutex);

    return cancelled;
}

static enum mad_flow input(void *data, struct mad_stream *stream)
{
    ring_play_param *ctx = (ring_play_param *)data;
    mad_buffer *buffer = (mad_buffer *)&ctx->buffer;

    if (audio_request_cancelled(ctx))
    {
        return MAD_FLOW_STOP;
    }

    // 1. 计算未处理数据大小（确保非负）
    size_t unproc_size = (stream->bufend > stream->next_frame) ? (size_t)(stream->bufend - stream->next_frame) : 0;
    // 2. 检查缓冲区剩余空间是否足够
    if (unproc_size > MP3_BUF_MAX)
    {
        AMY_LOG_ERROR("Error: Unprocessed data exceeds buffer size!\n");
        return MAD_FLOW_BREAK;
    }
    else if (unproc_size <= 0)
    {
        if (buffer->pos >= buffer->file_size)
        {
            AMY_LOG_INFO("file read eof\n");
            return MAD_FLOW_STOP;
        }
    }
    // 3. 移动未处理数据到缓冲区头部（如果有）
    if (unproc_size > 0)
    {
        memmove(buffer->data, stream->next_frame, unproc_size); // 用 memmove 避免重叠拷贝
    }
    // 4. 从文件读取新数据
    size_t read_size = MP3_BUF_MAX - unproc_size;
    if (buffer->pos + read_size > buffer->file_size)
    {
        read_size = buffer->file_size - buffer->pos;
    }

    if (read_size > 0)
    {
        ssize_t len = read(buffer->fd, buffer->data + unproc_size, read_size);
        if (len <= 0)
        { // 处理读取错误或 EOF
            if (len < 0)
            {
                AMY_LOG_ERROR("Read error: %d\n", len);
            }
            return MAD_FLOW_STOP;
        }
        buffer->data_size = unproc_size + (size_t)len;
        buffer->pos += len;
    }
    else
    {
        // AMY_LOG_INFO("file unproc size:%d\n", unproc_size);
        //   buffer->data_size = unproc_size; // 无新数据，仅保留未处理部分
        return MAD_FLOW_STOP;
    }

    // 5. 提交数据给 libmad
    if (buffer->data_size > 0)
    {
        mad_stream_buffer(stream, (unsigned char *)buffer->data, buffer->data_size);
        return MAD_FLOW_CONTINUE;
    }
    return MAD_FLOW_STOP; // 无数据可处理
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline signed int scale(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));
    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
    unsigned int nsamples; // nchannels,
    mad_fixed_t const *left_ch;
    //* right_ch;
    ring_play_param *ctx = (ring_play_param *)data;
    //  mad_buffer* buffer = (mad_buffer*)&ctx->buffer;
    /* pcm->samplerate contains the sampling frequency */
    // nchannels = pcm->channels;
    nsamples = pcm->length;
    left_ch = pcm->samples[0];
    // right_ch = pcm->samples[1];
    short int sample[1152] = {0};
    int size = 0;
    while (nsamples--)
    {
        /* output sample(s) in 16-bit signed little-endian PCM */
        sample[size++] = scale(*left_ch++);
    }
    if (audio_request_cancelled(ctx))
    {
        return MAD_FLOW_STOP;
    }
    if (ctx->audio_dev == NULL)
    {
        ctx->audio_dev = audio_output_open_ring(1, pcm->samplerate, ctx->active.volume, 8 * 1024);
        ctx->sample_rate = pcm->samplerate;
        audio_output_set_volume(ctx->audio_dev, ctx->active.volume);
    }
    else
    {
        int chn, rate;
        audio_output_param_get(&chn, &rate, NULL);
        if ((chn != 1) || (rate != pcm->samplerate))
        {
            audio_output_reset_params(ctx->audio_dev, 1, pcm->samplerate);
            audio_output_set_volume(ctx->audio_dev, ctx->active.volume);
            ctx->sample_rate = pcm->samplerate;
        }
    }

    if (!ctx->audio_dev)
    {
        return MAD_FLOW_CONTINUE;
    }
    if (!ctx->audio_dev_cleared)
    {
        audio_output_clear(ctx->audio_dev);
        ctx->audio_dev_cleared = 1;
    }
    unsigned int used = 0;
    unsigned int total = 0;
    while (1)
    {
        if (audio_output_query(ctx->audio_dev, NULL, &used, &total) != 0)
        {
            return MAD_FLOW_STOP;
        }
        if (audio_request_cancelled(ctx))
        {
            return MAD_FLOW_STOP;
        }
        if ((used + (size * 2)) <= (total))
        {
            break;
        }
        Amy_msleep(10);
    }
    audio_output_write(ctx->audio_dev, (char *)sample, size * 2);
    return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
    // mad_buffer* buffer = data;
    // AMY_LOG_ERROR("decoding error 0x%04x (%s)\n", stream->error, mad_stream_errorstr(stream));
    /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
    return MAD_FLOW_CONTINUE;
}

static int audio_decode(ring_play_param *ctx)
{
    struct mad_decoder decoder;
    int result = 0;

    /* configure input, output, and error functions */
    ctx->buffer.fd = open(ctx->mtd_name, O_RDONLY, 0);
    if (ctx->buffer.fd < 0)
    {
        AMY_LOG_ERROR("open %s failed\n", ctx->mtd_name);
        return -1;
    }
    // AMY_LOG_INFO("play ring %d.mp3\n", ctx->active.play_index);

    ctx->buffer.pos = 0;
    ctx->buffer.data_size = 0;
    ctx->audio_dev_cleared = 0;
    ctx->buffer.offset = ctx->resource[ctx->active.play_index].offset;
    ctx->buffer.file_size = ctx->resource[ctx->active.play_index].size;
    memset(ctx->buffer.data, 0, ctx->buffer.data_size);
    lseek(ctx->buffer.fd, (unsigned long)ctx->buffer.offset, SEEK_SET);
    mad_decoder_init(&decoder, ctx, input, 0 /* header */, 0 /* filter */, output, error, 0 /* message */);

    /* start decoding */
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    /* release the decoder */
    mad_decoder_finish(&decoder);
    close(ctx->buffer.fd);

    // 等待buffer输出完毕
    if (ctx->audio_dev)
    {
        int used = 0;
        while (!audio_request_cancelled(ctx))
        {
            if (audio_output_query(ctx->audio_dev, NULL, (unsigned int *)&used, NULL) != 0)
            {
                break;
            }
            if (used <= 0)
            {
                break;
            }

            Amy_msleep(10);
        }
        if (audio_request_cancelled(ctx))
        {
            audio_output_flush(ctx->audio_dev);
        }
    }
    // AMY_LOG_INFO("mp3 play finish\n");
    return result;
}

static void *audio_play_thread(void *arg)
{
    ring_play_param *ctx = (ring_play_param *)arg;
    while (ctx->run)
    {
        Amy_mutex_lock(&ctx->mutex);
        ctx->status = RING_PLAY_STATUS_STOP;
        Amy_cond_broadcast(&ctx->cond);
        while (ctx->active.request_id == ctx->next_request_id && ctx->run)
        {
            Amy_cond_wait(&ctx->cond, &ctx->mutex);
        }
        if (!ctx->run)
        {
            Amy_mutex_unlock(&ctx->mutex);
            break;
        }
        ctx->active = ctx->pending;
        if (ctx->stop_request_id == ctx->active.request_id)
        {
            ctx->stop_request_id = 0;
        }
        ctx->status = RING_PLAY_STATUS_PLAY;
        Amy_mutex_unlock(&ctx->mutex);

        do
        {
            if (ctx->active.play_start_cb)
            {
                ctx->active.play_start_cb(ctx->active.play_index);
            }
            audio_decode(ctx);
            if (audio_request_cancelled(ctx) || !ctx->active.play_finish_cb || !ctx->active.play_finish_cb(ctx->active.play_index))
            {
                break;
            }
        } while (!audio_request_cancelled(ctx));

        /* 一次播放请求（可能多轮循环）结束后才关闭铃声通道，避免每轮循环反复开关 */
        if (ctx->audio_dev)
        {
            audio_output_close(ctx->audio_dev);
            ctx->audio_dev = NULL;
        }
    }
    return NULL;
}

int Amy_audio_play_init(const char *mtd_name, const audio_ring_resource *resource, int cnt)
{
    memset(&ring_play_ctx, 0, sizeof(ring_play_ctx));
    Amy_mutex_init(&ring_play_ctx.mutex, NULL);
    Amy_cond_init(&ring_play_ctx.cond, NULL);
    ring_play_ctx.status = RING_PLAY_STATUS_STOP;
    ring_play_ctx.run = 1;

    ring_play_ctx.mtd_name = (char *)Amy_malloc(strlen(mtd_name) + 1);
    memset(ring_play_ctx.mtd_name, 0, strlen(mtd_name) + 1);
    strcpy(ring_play_ctx.mtd_name, mtd_name);

    ring_play_ctx.resource = resource;
    ring_play_ctx.resource_cnt = cnt;
    ring_play_ctx.buffer.pos = 0;
    ring_play_ctx.buffer.data_size = 0;
    ring_play_ctx.buffer.data = (char *)Amy_malloc(MP3_BUF_MAX);
    Amy_pthread_create(&ring_play_ctx.tid, audio_play_thread, &ring_play_ctx, 32 * 1024, 12);
    return 0;
}

static void audio_play_wait_finish(ring_play_param *ctx, unsigned int request_id)
{
    Amy_mutex_lock(&ctx->mutex);
    while (ctx->status == RING_PLAY_STATUS_PLAY && ctx->active.request_id == request_id)
    {
        Amy_cond_wait(&ctx->cond, &ctx->mutex);
    }
    Amy_mutex_unlock(&ctx->mutex);
}

int Amy_audio_play(int index, int volume, int interrupt_enable, void (*start_cb)(int), int (*finish_cb)(int))
{
    if (index >= ring_play_ctx.resource_cnt)
    {
        AMY_LOG_ERROR("play ring index error\n");
        return -1;
    }

    Amy_mutex_lock(&ring_play_ctx.mutex);
    ring_play_ctx.next_request_id++;
    ring_play_ctx.pending.request_id = ring_play_ctx.next_request_id;
    ring_play_ctx.pending.interrupt_enable = interrupt_enable;
    ring_play_ctx.pending.play_index = index;
    ring_play_ctx.pending.volume = volume;
    ring_play_ctx.pending.play_start_cb = start_cb;
    ring_play_ctx.pending.play_finish_cb = finish_cb;
    Amy_cond_signal(&ring_play_ctx.cond);
    Amy_mutex_unlock(&ring_play_ctx.mutex);
    return 0;
}

int Amy_audio_play_stop(void)
{
    unsigned int request_id;

    Amy_mutex_lock(&ring_play_ctx.mutex);
    if (ring_play_ctx.status == RING_PLAY_STATUS_STOP)
    {
        if (ring_play_ctx.active.request_id == ring_play_ctx.next_request_id)
        {
            Amy_mutex_unlock(&ring_play_ctx.mutex);
            return 0;
        }
        if (ring_play_ctx.pending.interrupt_enable == false)
        {
            Amy_mutex_unlock(&ring_play_ctx.mutex);
            return -1;
        }
        ring_play_ctx.active.request_id = ring_play_ctx.next_request_id;
        Amy_cond_signal(&ring_play_ctx.cond);
        Amy_mutex_unlock(&ring_play_ctx.mutex);
        return 0;
    }
    if (ring_play_ctx.active.interrupt_enable == false)
    {
        Amy_mutex_unlock(&ring_play_ctx.mutex);
        return -1;
    }
    request_id = ring_play_ctx.active.request_id;
    ring_play_ctx.stop_request_id = request_id;
    Amy_cond_signal(&ring_play_ctx.cond);
    Amy_mutex_unlock(&ring_play_ctx.mutex);

    audio_play_wait_finish(&ring_play_ctx, request_id);
    return 0;
}