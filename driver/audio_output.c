#include "Amy_sdk.h"
#include "Amy_log.h"
#include "audio_output.h"
#include "ak_ao.h"
#include <fcntl.h>
#include <stdint.h>
#include "ringbuffer/ringbuffer.h"
#include "speex_aec_wapper.h"
#include "ak_log.h"

/* ==================== 常量 ==================== */
#define AO_FRAME_MS 20           /* 每 tick 混音时长 */
#define AO_MAX_SAMPLE_RATE 48000 /* 支持的最高采样率 */
#define AO_MAX_CHANNELS 2        /* 支持的最高声道数 */
#define AO_MAX_FRAME_SAMPLES (AO_MAX_SAMPLE_RATE * AO_MAX_CHANNELS * AO_FRAME_MS / 1000)
#define AUDIO_OUTPUT_HW_VOLUME 0 // 硬件模拟音量固定值(dB)
#define AUDIO_OUTPUT_HW_GAIN 4   // 硬件数字增益固定值，对齐 EMOS-Indoor 的 user_ao_gain

/* 普通通道漂移吸收：对端送帧速率与本地 DAC 采样率来自两片独立晶振，
   必然存在固定比例差。不补偿时 ring 会被缓慢排空，排空后混音只能补零，
   表现为通话时间越长越断续。这里按 ring 水位微调重采样消费速率来吸收该差异，
   修正上限 ±2%：实测失配约 1.0%，留一倍余量，避免贴着上限失去调节能力。 */
#define AO_DRIFT_PPM_PER_BYTE 8 /* 水位误差(字节) -> 速率修正(ppm) */
#define AO_DRIFT_MAX_PPM 20000  /* 速率修正上限(±2%) */
/* ==================== 线性重采样器（铃声对齐 + 普通通道漂移吸收） ==================== */
typedef struct
{
    int src_rate;     /* 源采样率（铃声），0 表示未初始化 */
    int dst_rate;     /* 目标采样率（主通道/AO 采样率） */
    uint32_t step;    /* 定点步进（低 16 位为小数）：src_rate<<16 / dst_rate */
    uint64_t pos;     /* 定点位置（低 16 位为小数）：高 48 位为源样本整数索引 */
    int64_t next_idx; /* 下一个将要读入的输入样本绝对索引 */
    int have_s0, have_s1;
    int16_t s0, s1; /* 插值对：s0=in[floor(pos)], s1=in[floor(pos)+1] */
    int64_t s1_idx; /* s1 的绝对输入索引 */

    /* 输入缓冲：批量读入、逐个消费，跨调用保留未消费样本，
       避免“输出缓冲区满时仍消费输入导致丢帧/静音” */
    int16_t in[256];
    int in_len;
    int in_pos;
} audio_resampler_t;

/* ==================== 单通道状态（对外 opaque） ==================== */
typedef struct audio_output_context
{
    int role;   /* audio_output_role_t */
    int active; /* ref_count > 0，通道参与混音 */
    int ref_count;

    ringbuffer_context *ring; /* pcm 缓存 */
    int drift_target;         /* 漂移吸收的目标水位(字节)，普通通道使用 */

    int volume;
    uint32_t gain_q16; /* 软件音量增益（Q16，1.0 = 65536） */
    int channel;       /* 声道数（mono=1/stereo=2） */
    int sample;        /* 采样率 */
    int sample_bits;   /* 位宽 */

    audio_resampler_t resampler; /* 仅铃声通道使用 */
} audio_output_context;

/* ==================== 混音管理器 ==================== */
typedef struct
{
    Amy_mutex_t mutex; /* 保护全部状态 */
    int hw_open;       /* AO 硬件是否打开 */
    int running;       /* 混音线程运行标志 */
    int keep_open;     /* 设备常开：无活跃通道时保持 AO 与混音线程运行 */
    Amy_thread_t tid;  /* 混音线程 */

    int ao_handle_id;   /* AO 硬件句柄，-1 无效 */
    int ao_channel;     /* 当前 AO 声道数 */
    int ao_sample;      /* 当前 AO 采样率 */
    int ao_sample_bits; /* 当前 AO 位宽 */

    audio_output_context ch[AUDIO_OUTPUT_ROLE_MAX];
} audio_output_manager_t;

static audio_output_manager_t g_ao = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .ao_handle_id = -1,
};

/* ==================== 内部工具 ==================== */
static int active_count(audio_output_manager_t *m)
{
    int n = 0;
    for (int i = 0; i < AUDIO_OUTPUT_ROLE_MAX; i++)
        if (m->ch[i].active)
            n++;
    return n;
}

/* 混音基准通道：普通播放优先，否则铃声 */
static audio_output_context *base_channel(audio_output_manager_t *m)
{
    if (m->ch[AUDIO_OUTPUT_ROLE_NORMAL].active)
        return &m->ch[AUDIO_OUTPUT_ROLE_NORMAL];
    if (m->ch[AUDIO_OUTPUT_ROLE_RING].active)
        return &m->ch[AUDIO_OUTPUT_ROLE_RING];
    return NULL;
}

/* 音量值 -> Q16 线性增益（1.0 = 65536）。
   音量 0~100 平滑映射到 -36~0 dB（每单位音量 0.36 dB，无 10dB 档跳变）。
   低音量区不再骤降至 -60dB 以下导致不可闻；高档 0dB 满幅、不额外 +10dB 削顶。
   增益 = 10^((vol-100)*0.36/20)，离线计算展开为 101 档查表，避免软浮点。 */
static uint32_t volume_to_gain_q16(int volume)
{
    static const uint32_t gain_table[101] = {
        1039,
        1083,
        1128,
        1176,
        1226,
        1278,
        1332,
        1388,
        1447,
        1508,
        1572,
        1639,
        1708,
        1780,
        1856,
        1934,
        2016,
        2101,
        2190,
        2283,
        2379,
        2480,
        2585,
        2695,
        2809,
        2927,
        3051,
        3180,
        3315,
        3455,
        3601,
        3754,
        3913,
        4078,
        4251,
        4431,
        4618,
        4814,
        5017,
        5230,
        5451,
        5682,
        5922,
        6173,
        6434,
        6706,
        6990,
        7286,
        7594,
        7916,
        8250,
        8600,
        8964,
        9343,
        9738,
        10150,
        10580,
        11028,
        11494,
        11981,
        12488,
        13016,
        13567,
        14141,
        14739,
        15363,
        16013,
        16691,
        17397,
        18133,
        18901,
        19701,
        20534,
        21403,
        22309,
        23253,
        24237,
        25263,
        26332,
        27446,
        28608,
        29818,
        31080,
        32395,
        33766,
        35195,
        36684,
        38237,
        39855,
        41541,
        43299,
        45132,
        47041,
        49032,
        51107,
        53270,
        55524,
        57874,
        60323,
        62875,
        65536,
    };
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;
    return gain_table[volume];
}

static void resampler_init(audio_resampler_t *r, int src, int dst)
{
    memset(r, 0, sizeof(*r));
    r->src_rate = src;
    r->dst_rate = dst;
    r->step = (uint32_t)(((uint64_t)src << 16) / (uint64_t)dst);
}

/* 从 ring 读取至多 max_samples 个 16bit 样本到 out（不足补零）。
   返回实际读到的样本数。 */
static int read_channel_pcm(ringbuffer_context *ring, int16_t *out, int max_samples)
{
    unsigned int used = Amy_ringbuffer_used_byte(ring);
    int avail = (int)(used / 2);
    if (avail > max_samples)
        avail = max_samples;
    int got = avail;
    if (avail > 0)
        Amy_ringbuffer_read(ring, (char *)out, avail * 2);
    if (avail < max_samples)
        memset(out + avail, 0, (size_t)(max_samples - avail) * sizeof(int16_t));
    return got;
}

/* 铃声通道重采样读取：从 ring buffer 拉取输入，产出 max_samples 个主通道采样率样本到 out。
   返回实际产出的样本数（输入不足时补零）。 */
static int resample_ring_read(audio_output_context *ring, int16_t *out, int max_samples)
{
    audio_resampler_t *r = &ring->resampler;
    int produced = 0;

    while (produced < max_samples)
    {
        /* 1. 产出优先：只要插值位置仍落在当前区间内，就继续产出，不消耗输入 */
        if (r->have_s0 && r->have_s1 && (int64_t)(r->pos >> 16) == r->s1_idx - 1)
        {
            uint32_t frac = (uint32_t)(r->pos & 0xFFFF); /* 低 16 位为小数 */
            int32_t diff = (int32_t)r->s1 - (int32_t)r->s0;
            int32_t v = (int32_t)r->s0 + (int32_t)(((int64_t)diff * (int64_t)frac) >> 16);
            if (v > 32767)
                v = 32767;
            else if (v < -32768)
                v = -32768;
            out[produced++] = (int16_t)v;
            r->pos += r->step;
            continue;
        }

        /* 2. 需要新输入：内部缓冲耗尽则批量读入 */
        if (r->in_pos >= r->in_len)
        {
            unsigned int used = Amy_ringbuffer_used_byte(ring->ring);
            if (used == 0)
                break;
            int bytes = (int)used;
            if (bytes > (int)sizeof(r->in))
                bytes = (int)sizeof(r->in);
            int n = bytes / 2;
            if (n <= 0)
                break;
            Amy_ringbuffer_read(ring->ring, (char *)r->in, n * 2);
            r->in_len = n;
            r->in_pos = 0;
        }

        /* 3. 逐个消费输入样本，维护插值对 */
        int16_t cur = r->in[r->in_pos++];
        if (r->next_idx == 0)
        {
            r->s1 = cur;
            r->s1_idx = 0;
            r->have_s1 = 1;
            r->next_idx = 1;
            continue;
        }
        r->s0 = r->s1;
        r->have_s0 = r->have_s1;
        r->s1 = cur;
        r->s1_idx = r->next_idx;
        r->have_s1 = 1;
        r->next_idx++;
    }
    if (produced < max_samples)
        memset(out + produced, 0, (size_t)(max_samples - produced) * sizeof(int16_t));
    return produced;
}

/* 普通通道带漂移吸收的重采样读取：以 ring 水位为反馈微调源采样率，
   使长期消费速率收敛到对端送帧速率，避免 ring 被缓慢排空后补零。
   水位高于目标说明消费偏慢，提高源速率加快消费；反之降低。 */
static int resample_drift_read(audio_output_context *ch, int16_t *out, int max_samples, int ao_rate)
{
    audio_resampler_t *r = &ch->resampler;
    int base = ch->sample > 0 ? ch->sample : ao_rate;

    int used = (int)Amy_ringbuffer_used_byte(ch->ring);
    int err = used - ch->drift_target;
    int ppm = err * AO_DRIFT_PPM_PER_BYTE;
    if (ppm > AO_DRIFT_MAX_PPM)
        ppm = AO_DRIFT_MAX_PPM;
    else if (ppm < -AO_DRIFT_MAX_PPM)
        ppm = -AO_DRIFT_MAX_PPM;

    int src = base + (int)((int64_t)base * ppm / 1000000);
    if (src < 1)
        src = 1;

    if (r->dst_rate != ao_rate)
    {
        /* 采样率变化：重置插值状态，按新速率重新起步 */
        resampler_init(r, src, ao_rate);
    }
    else if (r->src_rate != src)
    {
        /* 仅调整消费比例：保留插值位置，避免产生波形断点 */
        r->src_rate = src;
        r->step = (uint32_t)(((uint64_t)src << 16) / (uint64_t)ao_rate);
    }
    return resample_ring_read(ch, out, max_samples);
}

/* ====================== AO 发送 ====================== */
static void audio_output_send(int ao_handle_id, char *data, int len, int use_aec)
{
    if (use_aec)
        speex_aec_render(speex_aec_global_handle(), (int16_t *)data, len / 2);

    int total_sent = 0;
    while (total_sent < len)
    {
        while (1)
        {
            struct ak_dev_buf_status status;
            if (ak_ao_get_buf_status(ao_handle_id, &status) != 0)
                continue;
            if ((status.buf_remain_len + len - total_sent) <= status.buf_total_len)
                break;
        }

        int play_len = 0;
        ak_ao_send_frame(ao_handle_id, (unsigned char *)(data + total_sent), len - total_sent, &play_len);
        if (play_len <= 0)
            break;
        total_sent += play_len;
    }
}

/* ====================== 混音线程 ====================== */
static int audio_mix_step(audio_output_manager_t *m, int32_t *acc, int16_t *tmp)
{
    int has_data = 0;
    int ao_rate, ao_ch;
    int frame_samples, total_samples;

    Amy_mutex_lock(&m->mutex);
    if (!m->running || !m->hw_open || m->ao_handle_id < 0)
    {
        Amy_mutex_unlock(&m->mutex);
        return -1;
    }

    ao_rate = m->ao_sample;
    ao_ch = m->ao_channel;
    frame_samples = ao_rate * AO_FRAME_MS / 1000;
    if (frame_samples < AUDIO_OUTPUT_MIN_SIZE / 2)
        frame_samples = AUDIO_OUTPUT_MIN_SIZE / 2;
    total_samples = frame_samples * ao_ch;

    memset(acc, 0, (size_t)total_samples * sizeof(int32_t));

    /* 1. 普通播放通道（单声道走漂移吸收重采样，多声道保持直读） */
    audio_output_context *normal = &m->ch[AUDIO_OUTPUT_ROLE_NORMAL];
    if (normal->active && normal->ring)
    {
        int got;
        if (normal->channel == 1 && ao_ch == 1)
        {
            got = resample_drift_read(normal, tmp, total_samples, ao_rate);
        }
        else
        {
            got = read_channel_pcm(normal->ring, tmp, total_samples);
        }
        if (got > 0)
            has_data = 1;
        for (int i = 0; i < total_samples; i++)
            acc[i] += (int32_t)(((int64_t)tmp[i] * normal->gain_q16) >> 16);
    }

    /* 2. 铃声通道（采样率不一致时重采样对齐主通道） */
    audio_output_context *ring = &m->ch[AUDIO_OUTPUT_ROLE_RING];
    if (ring->active && ring->ring)
    {
        int got;
        if (ring->sample != ao_rate)
        {
            audio_resampler_t *r = &ring->resampler;
            if (r->src_rate != ring->sample || r->dst_rate != ao_rate)
            {
                resampler_init(r, ring->sample, ao_rate);
                AMY_LOG_DEBUG("[ao][resample] ring resample trigger %d -> %d\n",
                              ring->sample, ao_rate);
            }
            got = resample_ring_read(ring, tmp, frame_samples);
        }
        else
        {
            got = read_channel_pcm(ring->ring, tmp, frame_samples);
        }
        if (got > 0)
            has_data = 1;

        /* 铃声为单声道，上混到 ao_ch 声道后叠加（软件增益） */
        for (int i = 0; i < frame_samples; i++)
        {
            int32_t s = (int32_t)(((int64_t)tmp[i] * ring->gain_q16) >> 16);
            if (ao_ch == 1)
            {
                acc[i] += s;
            }
            else
            {
                acc[2 * i] += s;
                acc[2 * i + 1] += s;
            }
        }
    }

    Amy_mutex_unlock(&m->mutex);

    /* 峰值限制器：大增益（如音量100%的+10dB）下整帧按比例压低到满幅，
       保留波形形状，避免硬削顶成方波产生爆音 */
    int32_t peak = 0;
    for (int i = 0; i < total_samples; i++)
    {
        int32_t a = acc[i] < 0 ? -acc[i] : acc[i];
        if (a > peak)
            peak = a;
    }
    if (peak > 32767)
    {
        for (int i = 0; i < total_samples; i++)
            acc[i] = (int32_t)(((int64_t)acc[i] * 32767) / peak);
    }

    /* 硬限幅兜底（峰值限制器后仅剩整数舍入余量） */
    for (int i = 0; i < total_samples; i++)
    {
        int32_t v = acc[i];
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        tmp[i] = (int16_t)v;
    }

    audio_output_send(m->ao_handle_id, (char *)tmp, total_samples * 2, has_data);
    return 0;
}

static void *audio_mix_thread(void *arg)
{
    audio_output_manager_t *m = (audio_output_manager_t *)arg;
    int max_samples = AO_MAX_FRAME_SAMPLES;
    int32_t *acc = (int32_t *)Amy_malloc(sizeof(int32_t) * max_samples);
    int16_t *tmp = (int16_t *)Amy_malloc(sizeof(int16_t) * max_samples);
    if (!acc || !tmp)
    {
        AMY_LOG_ERROR("[ao][mix] thread malloc failed\n");
        if (acc)
            Amy_free(acc);
        if (tmp)
            Amy_free(tmp);
        return NULL;
    }

    AMY_LOG_INFO("[ao][mix] mixer thread start\n");
    while (audio_mix_step(m, acc, tmp) >= 0)
    {
        /* 循环由 audio_output_send 的硬件缓冲水位自限速 */
    }

    Amy_free(acc);
    Amy_free(tmp);
    AMY_LOG_INFO("[ao][mix] mixer thread exit\n");
    return NULL;
}

/* ====================== AO 硬件打开/重置 ====================== */

/*
    设置各种音频处理参数结构体为默认值，默认值是从ak_audio_config.h抄的。
    参数仅作参考，实际参数要根据不同产品独立设定。
    1：NEAR(ai), ak_audio_nr_attr
    2：NEAR(ai), ak_audio_agc_attr
    3：NEAR(ai), ak_audio_aec_attr
    4：NEAR(ai), ak_audio_aslc_attr
    5：FAR(ao),  ak_audio_nr_attr
    6：FAR(ao),  ak_audio_aslc_attr
    7：NEAR(ai), ak_ai_set_eq_attr
    8：FAR(ao),  ak_ai_set_eq_attr
*/
static void setup_default_audio_argument(void *audio_args, char args_type)
{
    struct ak_audio_nr_attr default_ai_nr_attr = {-40, 0, 1};
    struct ak_audio_agc_attr default_ai_agc_attr = {24576, 4, 0, 80, 0, 1};
    struct ak_audio_aec_attr default_ai_aec_attr = {0, 1024, 1024, 0, 512, 1, 0};
    struct ak_audio_aslc_attr default_ai_aslc_attr = {32768, 0, 0};
    struct ak_audio_eq_attr default_ai_eq_attr = {
        0,
        10,
        {50, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {717, 717, 717, 717, 717, 717, 717, 717, 717, 717},
        {TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    struct ak_audio_nr_attr default_ao_nr_attr = {-25, 0, 1};
    struct ak_audio_aslc_attr default_ao_aslc_attr = {29491, 10, 1};
    struct ak_audio_eq_attr default_ao_eq_attr = {
        0,
        10,
        {50, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {717, 717, 717, 717, 717, 717, 717, 717, 717, 717},
        {TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    switch (args_type)
    {
    case 1:
        *(struct ak_audio_nr_attr *)audio_args = default_ai_nr_attr;
        break;
    case 2:
        *(struct ak_audio_agc_attr *)audio_args = default_ai_agc_attr;
        break;
    case 3:
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

    return;
}

static int ao_open_hw(int channel, int sample)
{
    struct ak_audio_out_param param;
    memset(&param, 0, sizeof(param));
    param.pcm_data_attr.channel_num = channel;
    param.pcm_data_attr.sample_bits = 16;
    param.pcm_data_attr.sample_rate = sample;
    param.dev_id = DEV_DAC;

    int ao_handle_id = -1;
    if (ak_ao_open(&param, &ao_handle_id))
    {
        Amy_msleep(10);
        if (ak_ao_open(&param, &ao_handle_id))
        {
            AMY_LOG_ERROR("ao open failed\n");
            return -1;
        }
    }
    /* 固定数字/模拟增益：音量改由软件按通道调整，硬件仅满幅输出 */
    ak_ao_set_volume(ao_handle_id, AUDIO_OUTPUT_HW_VOLUME);
    ak_ao_set_gain(ao_handle_id, AUDIO_OUTPUT_HW_GAIN);
    if ((DEV_DAC == param.dev_id) && (AUDIO_CHANNEL_MONO == channel))
    {
        struct ak_audio_nr_attr nr_attr;
        setup_default_audio_argument(&nr_attr, 5);

        /* enable_nr, nr only support 8000 or 16000 sample rate */
        if (sample != AK_AUDIO_SAMPLE_RATE_8000 && sample != AK_AUDIO_SAMPLE_RATE_16000)
        {
            ak_print_warning_ex(MODULE_ID_APP, "nr only support sample rate 8000 or 16000\n");
        }
        else
        {
            if (ak_ao_set_nr_attr(ao_handle_id, &nr_attr))
            {
                ak_print_error_ex(MODULE_ID_APP, "ak_ao_set_nr_attr failed\n");
                goto err;
            }
        }
    }

    struct ak_audio_aslc_attr ao_aslc_attr;
    setup_default_audio_argument(&ao_aslc_attr, 6);
    if (ak_ao_set_aslc_attr(ao_handle_id, &ao_aslc_attr))
    {
        ak_print_error_ex(MODULE_ID_APP, "ak_ao_set_aslc_attr failed\n");
        goto err;
    }

    struct ak_audio_eq_attr ao_eq_attr;
    setup_default_audio_argument(&ao_eq_attr, 8);
    if (ak_ao_set_eq_attr(ao_handle_id, &ao_eq_attr))
    {
        ak_print_error_ex(MODULE_ID_APP, "ak_ao_set_eq_attr failed\n");
        goto err;
    }

    /* 啸叫抑制：针对两机近距离互耦形成的反馈环，AEC 无法消除这类环路 */
    if (ak_ao_enable_hs(ao_handle_id, AUDIO_FUNC_ENABLE))
    {
        ak_print_error_ex(MODULE_ID_APP, "ak_ao_enable_hs failed\n");
        goto err;
    }

    return ao_handle_id;

err:
    ak_ao_close(ao_handle_id);
    return -1;
}

/* 重置 AO 硬件参数：停止输出 -> 重置参数 -> 重启输出。
   失败时降级重启旧参数，不直接崩溃。 */
static int do_ao_reset(int handle, int channel, int sample)
{
    struct ak_audio_out_param param;
    memset(&param, 0, sizeof(param));
    param.pcm_data_attr.channel_num = channel;
    param.pcm_data_attr.sample_bits = 16;
    param.pcm_data_attr.sample_rate = sample;
    param.dev_id = DEV_DAC;

    AMY_LOG_INFO("[ao][reset] stop -> set -> restart chn=%d sample=%d\n", channel, sample);

    int ret = ak_ao_reset_params(handle, &param);
    if (ret != 0)
    {
        AMY_LOG_ERROR("[ao][reset] reset_params failed ret=%d, fallback restart\n", ret);
        ak_ao_restart(handle);
        return -1;
    }

    ret = ak_ao_cancel(handle);
    if (ret != 0)
        AMY_LOG_WARNING("[ao][reset] cancel ret=%d\n", ret);

    ret = ak_ao_restart(handle);
    if (ret != 0)
    {
        AMY_LOG_ERROR("[ao][reset] restart failed ret=%d\n", ret);
        return -1;
    }
    return 0;
}

/* ====================== 打开（内部） ====================== */
static audio_output_context *audio_output_open_internal(int role, int channel, int sample, int volume, int buffer_size)
{
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || channel <= 0 || sample <= 0)
        return NULL;

    Amy_mutex_lock(&g_ao.mutex);
    audio_output_context *ch = &g_ao.ch[role];

    /* 已打开：引用计数 +1（防重复初始化） */
    if (ch->ref_count > 0)
    {
        ch->ref_count++;
        AMY_LOG_DEBUG("[ao] role=%d already open ref=%d\n", role, ch->ref_count);
        Amy_mutex_unlock(&g_ao.mutex);
        return ch;
    }

    /* 分配 pcm 缓存 */
    ringbuffer_context *ring = Amy_ringbuffer_open(buffer_size);
    if (!ring)
    {
        AMY_LOG_ERROR("[ao] role=%d ringbuffer open failed\n", role);
        Amy_mutex_unlock(&g_ao.mutex);
        return NULL;
    }

    ch->role = role;
    ch->ring = ring;
    ch->channel = channel;
    ch->sample = sample;
    ch->sample_bits = 16;
    ch->volume = volume;
    ch->gain_q16 = volume_to_gain_q16(volume);
    ch->ref_count = 1;
    ch->active = 1;
    ch->drift_target = buffer_size / 2;
    memset(&ch->resampler, 0, sizeof(ch->resampler));

    if (!g_ao.hw_open)
    {
        /* AO 未打开（无活跃通道且设备未常开）：打开 AO 硬件 + 启动混音线程 */
        int h = ao_open_hw(channel, sample);
        if (h < 0)
        {
            Amy_ringbuffer_close(ring);
            memset(ch, 0, sizeof(*ch));
            Amy_mutex_unlock(&g_ao.mutex);
            return NULL;
        }
        g_ao.ao_handle_id = h;
        g_ao.ao_channel = channel;
        g_ao.ao_sample = sample;
        g_ao.ao_sample_bits = 16;
        g_ao.hw_open = 1;
        g_ao.running = 1;
        Amy_pthread_create(&g_ao.tid, audio_mix_thread, &g_ao, 32 * 1024, 12);
        AMY_LOG_INFO("[ao] first channel role=%d, AO open chn=%d sample=%d\n", role, channel, sample);
    }
    else if (role == AUDIO_OUTPUT_ROLE_NORMAL)
    {
        /* 普通通道成为混音基准：AO 对齐普通通道采样率 */
        if (g_ao.ao_sample != sample || g_ao.ao_channel != channel)
        {
            AMY_LOG_INFO("[ao][reset] normal base open, sync AO chn=%d sample=%d\n", channel, sample);
            if (do_ao_reset(g_ao.ao_handle_id, channel, sample) == 0)
            {
                g_ao.ao_sample = sample;
                g_ao.ao_channel = channel;
            }
            else
            {
                AMY_LOG_ERROR("[ao][reset] normal base sync failed, keep chn=%d sample=%d\n",
                              g_ao.ao_channel, g_ao.ao_sample);
            }
        }
        else
        {
            AMY_LOG_INFO("[ao] ring active, normal open same rate, no reset\n");
        }
    }
    else
    {
        /* 铃声：普通通道不活跃时铃声即为基准，AO 对齐铃声采样率；否则重采样对齐普通通道 */
        if (!g_ao.ch[AUDIO_OUTPUT_ROLE_NORMAL].active)
        {
            if (g_ao.ao_sample != sample || g_ao.ao_channel != channel)
            {
                AMY_LOG_INFO("[ao][reset] ring sole base open, sync AO chn=%d sample=%d\n",
                             channel, sample);
                if (do_ao_reset(g_ao.ao_handle_id, channel, sample) == 0)
                {
                    g_ao.ao_sample = sample;
                    g_ao.ao_channel = channel;
                }
                else
                {
                    AMY_LOG_ERROR("[ao][reset] ring base sync failed, keep chn=%d sample=%d\n",
                                  g_ao.ao_channel, g_ao.ao_sample);
                }
            }
        }
        else
        {
            AMY_LOG_INFO("[ao] role=%d open, AO keep chn=%d sample=%d (ring will resample)\n",
                         role, g_ao.ao_channel, g_ao.ao_sample);
        }
    }
    Amy_mutex_unlock(&g_ao.mutex);
    return ch;
}

/* ====================== 对外：普通播放通道 ====================== */
audio_output_context *audio_output_open(int channel, int sample, int volume, int buffer)
{
    return audio_output_open_internal(AUDIO_OUTPUT_ROLE_NORMAL, channel, sample, volume, buffer);
}

audio_output_context *audio_output_get_handle(void)
{
    Amy_mutex_lock(&g_ao.mutex);
    audio_output_context *ch = &g_ao.ch[AUDIO_OUTPUT_ROLE_NORMAL];
    if (ch->ref_count <= 0 || !ch->active || !ch->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return NULL;
    }
    ch->ref_count++;
    Amy_mutex_unlock(&g_ao.mutex);
    return ch;
}

/* ====================== 对外：铃声通道 ====================== */
audio_output_context *audio_output_open_ring(int channel, int sample, int volume, int buffer)
{
    return audio_output_open_internal(AUDIO_OUTPUT_ROLE_RING, channel, sample, volume, buffer);
}

audio_output_context *audio_output_get_ring_handle(void)
{
    Amy_mutex_lock(&g_ao.mutex);
    audio_output_context *ch = &g_ao.ch[AUDIO_OUTPUT_ROLE_RING];
    if (ch->ref_count <= 0 || !ch->active || !ch->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return NULL;
    }
    ch->ref_count++;
    Amy_mutex_unlock(&g_ao.mutex);
    return ch;
}

/* ====================== 设备常开 ====================== */
int audio_output_keep_open(int channel, int sample)
{
    if (channel <= 0 || sample <= 0)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    g_ao.keep_open = 1;

    if (!g_ao.hw_open)
    {
        int h = ao_open_hw(channel, sample);
        if (h < 0)
        {
            g_ao.keep_open = 0;
            Amy_mutex_unlock(&g_ao.mutex);
            return -1;
        }
        g_ao.ao_handle_id = h;
        g_ao.ao_channel = channel;
        g_ao.ao_sample = sample;
        g_ao.ao_sample_bits = 16;
        g_ao.hw_open = 1;
        g_ao.running = 1;
        Amy_pthread_create(&g_ao.tid, audio_mix_thread, &g_ao, 32 * 1024, 12);
        AMY_LOG_INFO("[ao] keep open, AO open chn=%d sample=%d\n", channel, sample);
    }

    Amy_mutex_unlock(&g_ao.mutex);
    return 0;
}

/* ====================== 安全关闭 ====================== */
int audio_output_close(audio_output_context *ctx)
{
    if (!ctx)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }

    audio_output_context *ch = &g_ao.ch[role];
    if (ch->ref_count <= 0)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    if (ch->ref_count > 1)
    {
        ch->ref_count--;
        Amy_mutex_unlock(&g_ao.mutex);
        return 0;
    }

    /* ref_count 1 -> 0，释放该通道私有资源 */
    ch->ref_count = 0;
    ch->active = 0;
    ringbuffer_context *old_ring = ch->ring;
    ch->ring = NULL;
    memset(&ch->resampler, 0, sizeof(ch->resampler));

    int active = active_count(&g_ao);
    int stop_thread = 0;
    int old_ao_handle = g_ao.ao_handle_id;

    if (active == 0)
    {
        /* 最后一通道关闭：若设备常开则保持 AO 与混音线程（输出静音），否则停止并关闭 */
        if (!g_ao.keep_open)
        {
            g_ao.running = 0;
            stop_thread = 1;
        }
    }
    else if (active == 1 && 0) // 不需要提前同步，保持当前AO
    {
        /* 两路降到一路：同步 AO 到剩余基准通道 */
        audio_output_context *base = base_channel(&g_ao);
        if (base && (base->sample != g_ao.ao_sample || base->channel != g_ao.ao_channel))
        {
            AMY_LOG_INFO("[ao][reset] single channel remain, sync AO chn=%d sample=%d\n",
                         base->channel, base->sample);
            if (do_ao_reset(g_ao.ao_handle_id, base->channel, base->sample) == 0)
            {
                g_ao.ao_sample = base->sample;
                g_ao.ao_channel = base->channel;
            }
            else
            {
                AMY_LOG_ERROR("[ao][reset] close sync failed, keep chn=%d sample=%d\n",
                              g_ao.ao_channel, g_ao.ao_sample);
            }
        }
    }
    Amy_mutex_unlock(&g_ao.mutex);

    /* 释放本通道 pcm 缓存（已标记 inactive，锁外安全） */
    if (old_ring)
        Amy_ringbuffer_close(old_ring);

    if (stop_thread)
    {
        Amy_pthread_join(g_ao.tid, NULL);
        Amy_mutex_lock(&g_ao.mutex);
        if (g_ao.ao_handle_id == old_ao_handle)
        {
            ak_ao_close(old_ao_handle);
            g_ao.ao_handle_id = -1;
            g_ao.hw_open = 0;
        }
        Amy_mutex_unlock(&g_ao.mutex);
        AMY_LOG_INFO("[ao] all channels closed, AO destroyed\n");
    }

    return 0;
}

/* ====================== 写数据 ====================== */
int audio_output_write(audio_output_context *ctx, char *data, int size)
{
    if (!ctx || !data || size <= 0)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->active || !ctx->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    int ret = Amy_ringbuffer_write(ctx->ring, data, size);
    Amy_mutex_unlock(&g_ao.mutex);
    return ret == size ? 0 : -1;
}

/* ====================== 刷新 ====================== */
int audio_output_flush(audio_output_context *ctx)
{
    if (!ctx)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    int ret = Amy_ringbuffer_flush(ctx->ring);
    Amy_mutex_unlock(&g_ao.mutex);
    return ret;
}

int audio_output_clear(audio_output_context *ctx)
{
    if (!ctx)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    int ret = Amy_ringbuffer_flush(ctx->ring);
    Amy_mutex_unlock(&g_ao.mutex);
    return ret;
}

/* ====================== 设置音量 ====================== */
int audio_output_set_volume(audio_output_context *ctx, int vol)
{
    if (!ctx)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->active)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    ctx->volume = vol;
    ctx->gain_q16 = volume_to_gain_q16(vol);
    Amy_mutex_unlock(&g_ao.mutex);
    return 0;
}

/* ====================== 重置通道参数（采样率/声道） ====================== */
int audio_output_reset_params(audio_output_context *ctx, int channel, int sample)
{
    if (!ctx || channel <= 0 || sample <= 0)
        return -1;
    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->active)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }

    int active = active_count(&g_ao);

    if (role == AUDIO_OUTPUT_ROLE_NORMAL)
    {
        /* 普通通道是混音基准：无论其他通道是否 active，都允许重置 AO。
           铃声会在混音时检测 ao_rate 变化并重新重采样对齐主通道。 */
        if (do_ao_reset(g_ao.ao_handle_id, channel, sample) == 0)
        {
            ctx->channel = channel;
            ctx->sample = sample;
            g_ao.ao_channel = channel;
            g_ao.ao_sample = sample;
            AMY_LOG_INFO("[ao][reset] normal ok chn=%d sample=%d\n", channel, sample);
            Amy_mutex_unlock(&g_ao.mutex);
            return 0;
        }
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    else
    {
        if (active > 1)
        {
            /* 多通道：铃声仅重采样对齐，不重置 AO */
            ctx->channel = channel;
            ctx->sample = sample;
            AMY_LOG_INFO("[ao][reset] ring rate change to %d, resample align, no AO reset\n", sample);
            Amy_mutex_unlock(&g_ao.mutex);
            return 0;
        }

        if (do_ao_reset(g_ao.ao_handle_id, channel, sample) == 0)
        {
            ctx->channel = channel;
            ctx->sample = sample;
            g_ao.ao_channel = channel;
            g_ao.ao_sample = sample;
            AMY_LOG_INFO("[ao][reset] ring ok chn=%d sample=%d\n", channel, sample);
            Amy_mutex_unlock(&g_ao.mutex);
            return 0;
        }
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
}

/* ====================== 查询 ====================== */
int audio_output_query(audio_output_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total)
{
    if (!ctx)
        return -1;

    Amy_mutex_lock(&g_ao.mutex);
    int role = ctx->role;
    if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->ring)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    if (free)
        *free = Amy_ringbuffer_free_byte(ctx->ring);
    if (used)
        *used = Amy_ringbuffer_used_byte(ctx->ring);
    if (total)
        *total = Amy_ringbuffer_max_byte(ctx->ring);
    Amy_mutex_unlock(&g_ao.mutex);
    return 0;
}

/* ====================== 参数查询（普通通道基准/AO 硬件参数） ====================== */
int audio_output_param_get(int *out_chn, int *out_sample, int *out_vol)
{
    Amy_mutex_lock(&g_ao.mutex);
    if (!g_ao.hw_open)
    {
        Amy_mutex_unlock(&g_ao.mutex);
        return -1;
    }
    if (out_chn)
        *out_chn = g_ao.ao_channel;
    if (out_sample)
        *out_sample = g_ao.ao_sample;
    if (out_vol)
        *out_vol = g_ao.ch[AUDIO_OUTPUT_ROLE_NORMAL].volume;
    Amy_mutex_unlock(&g_ao.mutex);
    return 0;
}

/* ====================== 等待播放空 ====================== */
int audio_output_wait_buffer_empty(audio_output_context *ctx)
{
    if (!ctx)
        return -1;

    int timeout = 3000;
    int elapsed = 0;

    while (elapsed < timeout)
    {
        Amy_mutex_lock(&g_ao.mutex);
        int role = ctx->role;
        if (role < 0 || role >= AUDIO_OUTPUT_ROLE_MAX || &g_ao.ch[role] != ctx || !ctx->ring)
        {
            Amy_mutex_unlock(&g_ao.mutex);
            break;
        }
        int used = Amy_ringbuffer_used_byte(ctx->ring);
        Amy_mutex_unlock(&g_ao.mutex);

        if (used == 0)
            break;

        Amy_msleep(10);
        elapsed += 10;
    }
    return 0;
}
