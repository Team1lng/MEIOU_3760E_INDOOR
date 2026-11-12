#include "Amy_sdk.h"
#include "Amy_log.h"
#include "audio_input.h"
#include "ak_ai.h"
#include "ringbuffer/ringbuffer.h"
#include "mempool/mempool.h"
#include "speex_aec_wapper.h"

typedef struct audio_input_context
{
    Amy_mutex_t mutex;
    int ref_count;

    int run;
    Amy_thread_t tid;

    ringbuffer_context *aq;
    ringbuffer_context *aq_live;
    mp_pool *mp;

    int ai_handle_id;
    int frame_len;
    int volume;
    int channel;
    int sample; // 修复拼写错误 smaple -> sample
} audio_input_context;

static audio_input_context audio_dev;
static Amy_mutex_t g_open_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_aec_mode = AEC_MODE_ANYCLOUD; /* 默认安凯内置 AEC，AI 常开 */

/* AEC 尾长（样本数）：决定可消除的回声路径延迟上限，16k 下 2048 = 128ms。
   原值 512（32ms），近距离回声疑似超出该尾长 */
#define AI_AEC_TAIL_SAMPLES 2048

static void audio_frame_release_cb(void *user_data, void *data)
{
    Amy_mempool_free(user_data, data);
}

static void *audio_input_thread(void *arg)
{
    int ret = 0;
    struct frame frame = {0};
    audio_input_context *ctx = (audio_input_context *)arg;
    int ai_handle_id = ctx->ai_handle_id;
    ak_ai_start_capture(ai_handle_id);
    while (1)
    {
        // 原子检查退出标志
        Amy_mutex_lock(&ctx->mutex);
        if (!ctx->run)
        {
            Amy_mutex_unlock(&ctx->mutex);
            break;
        }
        Amy_mutex_unlock(&ctx->mutex);

        ret = ak_ai_get_frame(ai_handle_id, &frame, 0);
        if (ret)
        {
            Amy_msleep(10);
            continue;
        }

        speex_aec_capture(speex_aec_global_handle(), (int16_t *)frame.data, frame.len / 2);

        Amy_mutex_lock(&ctx->mutex);
        // 仅在资源有效时写入
        if (ctx->aq && ctx->aq_live)
        {
            if (Amy_ringbuffer_free_byte(ctx->aq) < frame.len)
                Amy_ringbuffer_flush(ctx->aq);
            Amy_ringbuffer_write(ctx->aq, (char *)frame.data, frame.len);

            if (Amy_ringbuffer_free_byte(ctx->aq_live) < frame.len)
                Amy_ringbuffer_flush(ctx->aq_live);
            Amy_ringbuffer_write(ctx->aq_live, (char *)frame.data, frame.len);
        }
        Amy_mutex_unlock(&ctx->mutex);

        ak_ai_release_frame(ai_handle_id, &frame);
    }
    ak_ai_stop_capture(ai_handle_id);
    AMY_LOG_INFO("audio input thread finish\n");
    return NULL;
}

static void setup_default_audio_argument(void *audio_args, char args_type)
{
    struct ak_audio_nr_attr default_ai_nr_attr = {-30, 0, 1};
    struct ak_audio_agc_attr default_ai_agc_attr = {24576, 4, 0, 80, 0, 1};
    struct ak_audio_aec_attr default_ai_aec_attr = {0, 1024, 1024, 0, AI_AEC_TAIL_SAMPLES, 0, 9830};
    struct ak_audio_aslc_attr default_ai_aslc_attr = {32768, 6, 1};
    struct ak_audio_eq_attr default_ai_eq_attr = {
        0, 1, {5000, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000}, {-6144, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {716, 717, 717, 717, 717, 717, 717, 717, 717, 717}, {TYPE_HSF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1}, 0, 0, 0, 0, 0, 0, 1, {1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    struct ak_audio_nr_attr default_ao_nr_attr = {0, 0, 0};
    struct ak_audio_aslc_attr default_ao_aslc_attr = {32768, 0, 0};
    struct ak_audio_eq_attr default_ao_eq_attr = {
        0, 10, {50, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {717, 717, 717, 717, 717, 717, 717, 717, 717, 717}, {TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1}, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    switch (args_type)
    {
    case 1:
        *(struct ak_audio_nr_attr *)audio_args = default_ai_nr_attr;
        break;
    case 2:
        *(struct ak_audio_agc_attr *)audio_args = default_ai_agc_attr;
        break;
    case 3:
        /* 安凯 AEC 按模式启用：仅 AEC_MODE_ANYCLOUD 下开启 */
        default_ai_aec_attr.enable = (g_aec_mode == AEC_MODE_ANYCLOUD) ? 1 : 0;
        *(struct ak_audio_aec_attr *)audio_args = default_ai_aec_attr;
        break;
    case 4:
        *(struct ak_audio_aslc_attr *)audio_args = default_ai_aslc_attr;
        break;
    case 5:
        *(struct ak_audio_nr_attr *)audio_args = default_ao_nr_attr;
        break;
    case 6:
        *(struct ak_audio_aslc_attr *)audio_args = default_ao_aslc_attr;
        break;
    case 7:
        *(struct ak_audio_eq_attr *)audio_args = default_ai_eq_attr;
        break;
    case 8:
        *(struct ak_audio_eq_attr *)audio_args = default_ao_eq_attr;
        break;
    default:
        break;
    }
}

static int audio_device_open(int channel, int sample, int volume)
{
    struct ak_audio_in_param param;
    int ai_handle_id = -1;

    memset(&param, 0, sizeof(struct ak_audio_in_param));
    param.pcm_data_attr.channel_num = channel;
    param.pcm_data_attr.sample_bits = 16;
    param.pcm_data_attr.sample_rate = sample;
    param.dev_id = DEV_ADC;

    if (ak_ai_open(&param, &ai_handle_id))
    {
        AMY_LOG_ERROR("ak audio input open failed\n");
        return -1;
    }
    if (ak_ai_set_source(ai_handle_id, AI_SOURCE_MIC))
    {
        AMY_LOG_ERROR("ai set source failed\n");
        ak_ai_close(ai_handle_id);
        return -1;
    }

    struct ak_audio_nr_attr nr_attr;
    setup_default_audio_argument(&nr_attr, 1);
    if ((sample == 8000 || sample == 16000) && ak_ai_set_nr_attr(ai_handle_id, &nr_attr))
        AMY_LOG_ERROR("ak_ai_set_nr_attr fail\n");
    ak_ai_enable_nr(ai_handle_id, AUDIO_FUNC_ENABLE);

    struct ak_audio_agc_attr agc_attr;
    setup_default_audio_argument(&agc_attr, 2);
    if ((sample == 8000 || sample == 16000) && ak_ai_set_agc_attr(ai_handle_id, &agc_attr))
        AMY_LOG_ERROR("ak_ai_set_agc_attr failed\n");
    ak_ai_enable_agc(ai_handle_id, AUDIO_FUNC_ENABLE);

    struct ak_audio_aec_attr aec_attr;
    setup_default_audio_argument(&aec_attr, 3);
    if ((sample == 8000 || sample == 16000) && ak_ai_set_aec_attr(ai_handle_id, &aec_attr))
        AMY_LOG_ERROR("ak_ai_set_aec_attr failed\n");
    if (g_aec_mode == AEC_MODE_ANYCLOUD)
        ak_ai_enable_aec(ai_handle_id, AUDIO_FUNC_ENABLE);

    if (ak_ai_set_gain(ai_handle_id, 4))
        AMY_LOG_ERROR("ak_ai_set_gain failed\n");

    struct ak_audio_aslc_attr ai_aslc_attr;
    setup_default_audio_argument(&ai_aslc_attr, 4);
    if (ak_ai_set_aslc_attr(ai_handle_id, &ai_aslc_attr))
        AMY_LOG_ERROR("ak_ai_set_aslc_attr failed\n");

    struct ak_audio_eq_attr ai_eq_attr;
    setup_default_audio_argument(&ai_eq_attr, 7);
    if (ak_ai_set_eq_attr(ai_handle_id, &ai_eq_attr))
        AMY_LOG_ERROR("ak_ai_set_eq_attr failed\n");

    return ai_handle_id;
}

// ====================== 稳定版：打开 ======================
audio_input_context *audio_input_open(int channel, int sample, int volume)
{
    Amy_mutex_lock(&g_open_mutex);
    audio_input_context *d = &audio_dev;

    /* 复用条件：已有使用者，或 ANYCLOUD 模式下设备处于常开残留态（ref=0 但仍在运行） */
    if (d->ref_count > 0 || (g_aec_mode == AEC_MODE_ANYCLOUD && d->run))
    {
        d->ref_count++;
        AMY_LOG_INFO("audio input reused, ref_count=%d\n", d->ref_count);
        Amy_mutex_unlock(&g_open_mutex);
        return d;
    }

    int ai_handle_id = audio_device_open(channel, sample, volume);
    if (ai_handle_id < 0)
    {
        usleep(100 * 1000);
        ai_handle_id = audio_device_open(channel, sample, volume);
        if (ai_handle_id < 0)
        {
            AMY_LOG_ERROR("audio device open failed\n");
            Amy_mutex_unlock(&g_open_mutex);
            return NULL;
        }
    }

    ak_ai_set_volume(ai_handle_id, 0);

    memset(d, 0, sizeof(audio_input_context));
    Amy_mutex_init(&d->mutex, NULL);

    if (ak_ai_get_frame_length(ai_handle_id, &d->frame_len))
        goto err_close_ai;

    d->mp = Amy_mempool_init(10 * (d->frame_len + sizeof(stream_data)), 10 * (d->frame_len + sizeof(stream_data)));
    if (!d->mp)
        goto err_free_mp;

    d->aq = Amy_ringbuffer_open(d->frame_len * 10);
    if (!d->aq)
        goto err_close_aq;

    d->aq_live = Amy_ringbuffer_open(d->frame_len * 10);
    if (!d->aq_live)
        goto err_close_aq_live;

    d->ai_handle_id = ai_handle_id;
    d->channel = channel;
    d->sample = sample;
    d->volume = volume;
    d->ref_count = 1;
    d->run = 1;

    Amy_pthread_create(&d->tid, audio_input_thread, d, 32 * 1024, 12);
    /* 仅 SPEEX 模式创建外置 AEC；ANYCLOUD 模式下全局句柄置空，采集线程空转 */
    if (g_aec_mode == AEC_MODE_SPEEX)
    {
        void *aec = speex_aec_create(sample);
        speex_aec_set_global_handle(aec);
    }
    else
    {
        speex_aec_set_global_handle(NULL);
    }
    AMY_LOG_INFO("audio input opened: channel=%d rate=%d aec_mode=%d\n", channel, sample, g_aec_mode);
    Amy_mutex_unlock(&g_open_mutex);
    return d;

err_close_aq_live:
    Amy_ringbuffer_close(d->aq_live);
err_close_aq:
    Amy_ringbuffer_close(d->aq);
err_free_mp:
    Amy_mempool_destory(d->mp);
err_close_ai:
    ak_ai_close(ai_handle_id);
    Amy_mutex_destroy(&d->mutex);
    memset(d, 0, sizeof(audio_input_context)); // 完全清空
    Amy_mutex_unlock(&g_open_mutex);
    return NULL;
}

// ====================== 稳定版：关闭（绝对安全） ======================
int audio_input_close(audio_input_context *ctx)
{
    if (ctx == NULL || ctx != &audio_dev)
    {
        AMY_LOG_ERROR("invalid ctx\n");
        return -1;
    }

    Amy_mutex_lock(&g_open_mutex);
    if (ctx->ref_count <= 0)
    {
        AMY_LOG_ERROR("already closed\n");
        Amy_mutex_unlock(&g_open_mutex);
        return -1;
    }

    if (ctx->ref_count > 1)
    {
        ctx->ref_count--;
        AMY_LOG_INFO("audio close ref=%d\n", ctx->ref_count);
        Amy_mutex_unlock(&g_open_mutex);
        return 0;
    }

    if (g_aec_mode == AEC_MODE_ANYCLOUD)
    {
        /* 模式B：AI 永不关闭。仅释放使用计数，设备/采集线程保持运行。
           若此前以 SPEEX 模式打开，这里把设备切到安凯 AEC 并销毁 speex。 */
        struct ak_audio_aec_attr aec_attr;
        setup_default_audio_argument(&aec_attr, 3);
        if (ctx->ai_handle_id >= 0)
            ak_ai_set_aec_attr(ctx->ai_handle_id, &aec_attr);
        void *old_aec = speex_aec_global_handle();
        if (old_aec)
        {
            speex_aec_destroy(old_aec);
            speex_aec_set_global_handle(NULL);
        }
        ctx->ref_count = 0;
        AMY_LOG_INFO("audio keep-alive: AI stays open (anycloud aec mode)\n");
        Amy_mutex_unlock(&g_open_mutex);
        return 0;
    }

    // 先保存所有旧资源句柄——join 期间 open() 可能 memset 覆盖 ctx，
    // 导致旧句柄丢失无法释放，甚至误销毁新 session 的资源
    Amy_mutex_lock(&ctx->mutex);
    ctx->run = 0;
    int old_ai_handle = ctx->ai_handle_id;
    ringbuffer_context *old_aq = ctx->aq;
    ringbuffer_context *old_aq_live = ctx->aq_live;
    mp_pool *old_mp = ctx->mp;
    void *old_aec = speex_aec_global_handle();

    Amy_mutex_unlock(&ctx->mutex);
    Amy_mutex_unlock(&g_open_mutex);

    Amy_pthread_join(ctx->tid, NULL);
    ctx->ai_handle_id = -1;
    ctx->aq = NULL;
    ctx->aq_live = NULL;
    ctx->mp = NULL;
    // 无论如何都释放旧资源
    ak_ai_close(old_ai_handle);
    if (old_aq)
        Amy_ringbuffer_close(old_aq);
    if (old_aq_live)
        Amy_ringbuffer_close(old_aq_live);
    if (old_mp)
        Amy_mempool_destory(old_mp);
    if (old_aec)
        speex_aec_destroy(old_aec);

    Amy_mutex_lock(&g_open_mutex);
    ctx->ref_count--;
    AMY_LOG_INFO("audio close ref=%d\n", ctx->ref_count);
    if (ctx->ref_count > 0 || ctx->run)
    {
        AMY_LOG_INFO("ai re-opened during close, old resources released\n");
        Amy_mutex_unlock(&g_open_mutex);
        return 0;
    }

    speex_aec_set_global_handle(NULL);
    Amy_mutex_destroy(&ctx->mutex);
    memset(ctx, 0, sizeof(audio_input_context));
    AMY_LOG_INFO("audio input destroyed\n");
    Amy_mutex_unlock(&g_open_mutex);
    return 0;
}

// ====================== 全部接口：锁内安全检查 ======================
stream_data *audio_input_buffer_get(audio_input_context *ctx, int size)
{
    if (ctx == NULL || ctx != &audio_dev)
        return NULL;

    stream_data *ast = NULL;
    Amy_mutex_lock(&ctx->mutex);

    // 锁内判断，绝对安全
    if (ctx->ref_count <= 0 || !ctx->run || !ctx->mp)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return NULL;
    }

    ast = Amy_mempool_alloc(ctx->mp, sizeof(stream_data) + size);
    if (ast)
    {
        ast->data = (unsigned char *)ast + sizeof(stream_data);
        ast->size = size;
    }

    Amy_mutex_unlock(&ctx->mutex);
    return ast;
}

int audio_input_frame_get(audio_input_context *ctx, stream_data *ast)
{
    if (!ctx || !ast || ctx != &audio_dev)
        return -1;

    int ret = -1;
    Amy_mutex_lock(&ctx->mutex);

    if (ctx->ref_count <= 0 || !ctx->run || !ctx->aq)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    ret = Amy_ringbuffer_read(ctx->aq, (char *)ast->data, ast->size);
    Amy_timestamp(ast->timestamp);
    ast->keyframe = 0;

    Amy_mutex_unlock(&ctx->mutex);
    return ret;
}

int audio_input_frame_get_live(audio_input_context *ctx, stream_data *ast)
{
    if (!ctx || !ast || ctx != &audio_dev)
        return -1;

    int ret = -1;
    Amy_mutex_lock(&ctx->mutex);

    if (ctx->ref_count <= 0 || !ctx->run || !ctx->aq_live)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    ret = Amy_ringbuffer_read(ctx->aq_live, (char *)ast->data, ast->size);
    Amy_timestamp(ast->timestamp);
    ast->keyframe = 0;

    Amy_mutex_unlock(&ctx->mutex);
    return ret;
}

int audio_input_frame_release(audio_input_context *ctx, stream_data *frame)
{
    if (!ctx || !frame || ctx != &audio_dev)
        return -1;

    Amy_mutex_lock(&ctx->mutex);

    if (!ctx->mp)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    audio_frame_release_cb(ctx->mp, frame);
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int audio_input_set_volume(audio_input_context *ctx, int volume)
{
    if (!ctx || ctx != &audio_dev)
        return -1;

    Amy_mutex_lock(&ctx->mutex);

    if (ctx->ai_handle_id < 0 || !ctx->run)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    ak_ai_set_volume(ctx->ai_handle_id, volume);
    ctx->volume = volume;
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int audio_input_query(audio_input_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total)
{
    if (!ctx || ctx != &audio_dev)
        return -1;

    Amy_mutex_lock(&ctx->mutex);

    if (!ctx->aq)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    if (free)
        *free = Amy_ringbuffer_free_byte(ctx->aq);
    if (used)
        *used = Amy_ringbuffer_used_byte(ctx->aq);
    if (total)
        *total = Amy_ringbuffer_max_byte(ctx->aq);

    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int audio_input_query_live(audio_input_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total)
{
    if (!ctx || ctx != &audio_dev)
        return -1;

    Amy_mutex_lock(&ctx->mutex);

    if (!ctx->aq_live)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    if (free)
        *free = Amy_ringbuffer_free_byte(ctx->aq_live);
    if (used)
        *used = Amy_ringbuffer_used_byte(ctx->aq_live);
    if (total)
        *total = Amy_ringbuffer_max_byte(ctx->aq_live);

    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

// ====================== 安全获取句柄 + 参数 + 引用计数（不读数据） ======================
// 功能：
// 1. 直接返回有效音频句柄（ctx 非空 = 100% 可用）
// 2. 自动输出通道、采样率、音量
// 3. 自动引用计数 +1，使用期间设备不会被其他线程关闭
// 4. 不读取音频数据，纯安全获取句柄
// 返回：有效句柄 或 NULL
audio_input_context *audio_input_param_get(int *out_chn, int *out_sample, int *out_vol)
{
    // 加全局锁，原子操作
    Amy_mutex_lock(&g_open_mutex);

    // 严格校验：只有完全打开、资源完整、运行中的设备才返回。
    // ANYCLOUD 模式下允许常开残留态（ref=0 但设备仍在运行）。
    if ((audio_dev.ref_count <= 0 && !(g_aec_mode == AEC_MODE_ANYCLOUD && audio_dev.run)) ||
        !audio_dev.run ||
        audio_dev.mp == NULL ||
        audio_dev.aq == NULL)
    {
        Amy_mutex_unlock(&g_open_mutex);
        return NULL;
    }

    // 【关键】引用计数 +1，保证持有期间不会被关闭销毁
    audio_dev.ref_count++;
    AMY_LOG_INFO("get handle success, ref=%d\n", audio_dev.ref_count);

    // 输出参数（线程安全）
    if (out_chn)
        *out_chn = audio_dev.channel;
    if (out_sample)
        *out_sample = audio_dev.sample;
    if (out_vol)
        *out_vol = audio_dev.volume;

    // 解锁并返回有效句柄
    Amy_mutex_unlock(&g_open_mutex);
    return &audio_dev;
}

// ====================== AEC 工作模式切换（测试用） ======================
int audio_input_aec_mode_get(void)
{
    return g_aec_mode;
}

int audio_input_aec_mode_set(int mode)
{
    if (mode != AEC_MODE_SPEEX && mode != AEC_MODE_ANYCLOUD)
        return -1;

    Amy_mutex_lock(&g_open_mutex);
    g_aec_mode = mode;

    /* 从"AI 永不关闭"切回"即用即关"时，若设备处于空闲常开残留态（无使用者），
       真实关闭一次——这也正是验证"安凯 AEC 下关闭 AI 卡死"的时机。 */
    if (mode == AEC_MODE_SPEEX && audio_dev.run && audio_dev.ref_count <= 0)
    {
        audio_dev.ref_count = 1;
        Amy_mutex_unlock(&g_open_mutex);
        audio_input_close(&audio_dev);
        return 0;
    }

    Amy_mutex_unlock(&g_open_mutex);
    AMY_LOG_INFO("AEC mode set to %s\n", mode == AEC_MODE_ANYCLOUD ? "ANYCLOUD" : "SPEEX");
    return 0;
}