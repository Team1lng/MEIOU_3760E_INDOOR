#include "Amy_music_player.h"
#include "Amy_sdk.h"
#include "audio_output.h"
#include "Amy_log.h"
#include "Amy_file_manager.h"
#include "Amy_tfcard.h"
#include "mad.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/************************** 常量 **************************/
#define MP3_BUF_MAX (128 * 1024)
#define MP3_READ_CHUNK (4 * 1024)
#define PCM_BUF_SAMPLES 1152
#define PCM_BUF_MAX (PCM_BUF_SAMPLES * 2)

/************************** 播放状态 **************************/
#define STATE_IDLE 0
#define STATE_PLAY 1
#define STATE_PAUSE 2

/************************** 上下文 **************************/
typedef struct
{
    int run;
    Amy_thread_t tid;
    Amy_mutex_t mutex;
    Amy_cond_t cond;

    /* 播放状态: 0=IDLE, 1=PLAYING, 2=PAUSED */
    int state;
    int stop_requested;
    int seek_requested;
    int seek_target_ms;

    /* 曲目信息 */
    int track_current;
    int track_total;
    char track_name[256];
    int duration_ms;
    int file_size;
    char file_path[384];
    int volume;

    /* 曲目切换标记 */
    int new_track;

    /* 倍速 */
    float speed;
    float speed_phase;

    /* 解码状态 */
    int fd;
    int bytes_read;
    int resume_byte;
    mad_timer_t current_position;
    int position_offset_ms;
    int resume_position_ms;
    audio_output_context *audio_dev;
    int audio_dev_cleared;
    int sample_rate;

    /* 缓冲区 */
    char *mp3_buf;
    int mp3_buf_fill;

    /* 回调 */
    void (*finish_cb)(void);
} music_player_ctx;

static music_player_ctx g_player = {
    .volume = 50,
    .speed = 1.0f,
};

/************************** libmad 回调 **************************/

static inline signed int scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow input(void *data, struct mad_stream *stream)
{
    music_player_ctx *ctx = (music_player_ctx *)data;

    if (ctx->stop_requested)
        return MAD_FLOW_STOP;

    /* 将已消费的数据移出缓冲区，保留未处理尾部，然后从文件读新数据追加。
     * 当 libmad 扫描完整块都无法找到同步字时（如 ID3V2 标签），
     * 数据被自然丢弃，文件偏移推进到下一块，最终到达音频数据。 */
    if (stream->next_frame != NULL)
    {
        ctx->mp3_buf_fill = (ctx->mp3_buf + ctx->mp3_buf_fill) - (const char *)stream->next_frame;
        memmove(ctx->mp3_buf, stream->next_frame, ctx->mp3_buf_fill);
    }

    int read_max = MP3_READ_CHUNK - ctx->mp3_buf_fill;
    if (read_max <= 0)
    {
        ctx->mp3_buf_fill = 0;
        read_max = MP3_READ_CHUNK;
    }

    ssize_t len = read(ctx->fd, ctx->mp3_buf + ctx->mp3_buf_fill, read_max);
    if (len <= 0)
    {
        if (len < 0)
            AMY_LOG_ERROR("music read error: %d\n", (int)len);
        return MAD_FLOW_STOP;
    }

    ctx->bytes_read += len;
    ctx->mp3_buf_fill += len;
    mad_stream_buffer(stream, (unsigned char *)ctx->mp3_buf, ctx->mp3_buf_fill);
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow header_cb(void *data, struct mad_header const *header)
{
    music_player_ctx *ctx = (music_player_ctx *)data;
    mad_timer_add(&ctx->current_position, header->duration);
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
    music_player_ctx *ctx = (music_player_ctx *)data;

    if (ctx->stop_requested)
        return MAD_FLOW_STOP;

    unsigned int nsamples = pcm->length;
    mad_fixed_t const *left_ch = pcm->samples[0];
    short int sample[PCM_BUF_MAX];
    int size = 0;

    if (ctx->speed == 1.0f)
    {
        while (nsamples--)
            sample[size++] = scale(*left_ch++);
    }
    else
    {
        float phase = ctx->speed_phase;
        while (nsamples--)
        {
            short s = scale(*left_ch++);
            phase += 1.0f;
            if (phase >= ctx->speed)
            {
                phase -= ctx->speed;
                if (size < PCM_BUF_MAX)
                    sample[size++] = s;
            }
        }
        ctx->speed_phase = phase;
    }

    if (!ctx->audio_dev)
    {
        ctx->audio_dev = audio_output_open(1, pcm->samplerate, ctx->volume, 8 * 1024);
        audio_output_set_volume(ctx->audio_dev, ctx->volume);
        ctx->sample_rate = pcm->samplerate;
        ctx->audio_dev_cleared = 0;
    }
    else
    {
        int chn, rate;
        audio_output_param_get(&chn, &rate, NULL);
        if ((chn != 1) || (rate != pcm->samplerate))
        {
            audio_output_reset_params(ctx->audio_dev, 1, pcm->samplerate);
            audio_output_set_volume(ctx->audio_dev, ctx->volume);
            ctx->sample_rate = pcm->samplerate;
            ctx->audio_dev_cleared = 0;
        }
    }

    if (!ctx->audio_dev)
        return MAD_FLOW_CONTINUE;

    if (!ctx->audio_dev_cleared)
    {
        audio_output_clear(ctx->audio_dev);
        ctx->audio_dev_cleared = 1;
    }

    unsigned int used = 0, total = 0;
    while (1)
    {
        if (audio_output_query(ctx->audio_dev, NULL, &used, &total) != 0)
            return MAD_FLOW_STOP;
        if (ctx->stop_requested)
            return MAD_FLOW_STOP;
        if ((used + (unsigned int)(size * 2)) <= total)
            break;
        Amy_msleep(10);
    }

    audio_output_write(ctx->audio_dev, (char *)sample, size * 2);
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
    return MAD_FLOW_CONTINUE;
}

/************************** 解码 **************************/

static int music_decode(music_player_ctx *ctx, int start_byte)
{
    struct mad_decoder decoder;
    int result;

    ctx->fd = open(ctx->file_path, O_RDONLY);
    if (ctx->fd < 0)
    {
        AMY_LOG_ERROR("music open %s failed\n", ctx->file_path);
        return -1;
    }
    ctx->bytes_read = start_byte;
    if (start_byte > 0)
    {
        lseek(ctx->fd, start_byte, SEEK_SET);
        ctx->position_offset_ms = ctx->resume_position_ms;
    }
    else
    {
        ctx->position_offset_ms = 0;
        /* 跳过 ID3V2 标签，避免大标签撑满 libmad 缓冲区导致解码失败 */
        unsigned char head[10];
        if (read(ctx->fd, head, 10) == 10)
        {
            if (head[0] == 'I' && head[1] == 'D' && head[2] == '3')
            {
                int tag_size = ((head[6] & 0x7F) << 21) | ((head[7] & 0x7F) << 14) |
                               ((head[8] & 0x7F) << 7) | (head[9] & 0x7F);
                ctx->bytes_read = tag_size + 10;
                lseek(ctx->fd, ctx->bytes_read, SEEK_SET);
            }
            else
            {
                lseek(ctx->fd, 0, SEEK_SET);
            }
        }
    }
    ctx->audio_dev_cleared = 0;
    ctx->mp3_buf_fill = 0;
    mad_timer_reset(&ctx->current_position);
    mad_decoder_init(&decoder, ctx, input, header_cb, NULL, output, error_cb, NULL);
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&decoder);
    if (ctx->audio_dev && !ctx->stop_requested)
    {
        unsigned int used = 0;
        while (1)
        {
            if (audio_output_query(ctx->audio_dev, NULL, &used, NULL) != 0)
                break;
            if (used <= 0)
                break;
            Amy_msleep(10);
        }
    }
    if (ctx->stop_requested && ctx->audio_dev)
        audio_output_flush(ctx->audio_dev);

    if (ctx->audio_dev)
    {
        audio_output_close(ctx->audio_dev);
        ctx->audio_dev = NULL;
    }
    close(ctx->fd);
    ctx->fd = -1;
    return result;
}

/************************** 【修复版】后台播放线程（核心改动） **************************/
static void *music_player_thread(void *arg)
{
    music_player_ctx *ctx = (music_player_ctx *)arg;

    while (ctx->run)
    {
        Amy_mutex_lock(&ctx->mutex);
        // 阻塞等待播放状态
        while (ctx->run && ctx->state != STATE_PLAY)
            Amy_cond_wait(&ctx->cond, &ctx->mutex);

        if (!ctx->run)
        {
            Amy_mutex_unlock(&ctx->mutex);
            break;
        }

        // 一次性读取所有请求标记并立刻清零，防止重复处理
        int start_byte = ctx->resume_byte;
        int need_new_track = ctx->new_track;
        int need_seek = ctx->seek_requested;

        ctx->resume_byte = 0;
        ctx->new_track = 0;
        ctx->seek_requested = 0;
        ctx->stop_requested = 0;

        Amy_mutex_unlock(&ctx->mutex);

        // 一轮循环只执行一次解码，根据标记选择入口，无嵌套递归
        if (need_seek)
        {
            music_decode(ctx, start_byte);
        }
        else if (need_new_track)
        {
            music_decode(ctx, 0);
        }
        else
        {
            music_decode(ctx, start_byte);
        }

        // ============ 解码结束后统一状态处理 ============
        Amy_mutex_lock(&ctx->mutex);

        // 暂停状态：记录断点，直接进入下一轮等待
        if (ctx->state == STATE_PAUSE)
        {
            unsigned long cur = mad_timer_count(ctx->current_position, MAD_UNITS_MILLISECONDS);
            ctx->resume_position_ms = ctx->position_offset_ms + (int)cur;
            ctx->resume_byte = ctx->bytes_read;
            Amy_mutex_unlock(&ctx->mutex);
            continue;
        }

        // 被 stop / 切歌 / 跳转中断，或外部已置 IDLE：不触发播放完成回调，
        // 否则“切下一首”会同时被 finish 回调再自动切一首，导致跳两首
        if (ctx->state == STATE_IDLE || ctx->stop_requested)
        {
            Amy_mutex_unlock(&ctx->mutex);
            continue;
        }

        // 文件自然播放完毕，自动切为空闲状态
        ctx->state = STATE_IDLE;
        void (*cb)(void) = ctx->finish_cb;
        Amy_mutex_unlock(&ctx->mutex);
        if (cb)
            cb();
    }

    return NULL;
}

/************************** 时长计算  **************************/

/* MPEG 比特率表 (kbps): [MPEG1][layer], [MPEG2/2.5][layer] */
static const int bitrate_table[5][15] = {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}, /* MPEG1 layer1 */
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},    /* MPEG1 layer2 */
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},     /* MPEG1 layer3 */
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},    /* MPEG2/2.5 layer1 */
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}          /* MPEG2/2.5 layer2&3 */
};

/* 每帧采样数: [layer][MPEG1/MPEG2/MPEG2.5] */
static const int sample_num_table[3][3] = {
    {384, 384, 384},    /* layer1 */
    {1152, 1152, 1152}, /* layer2 */
    {1152, 576, 576}    /* layer3 */
};

/* 采样率: [MPEG-version][index] */
static const int sample_rate_table[3][3] = {
    {44100, 48000, 32000}, /* MPEG1 */
    {22050, 24000, 16000}, /* MPEG2 */
    {11025, 12000, 8000}   /* MPEG2.5 */
};

static int scan_frame_header(const unsigned char *buf, int *out_bitrate,
                             int *out_sample_rate, int *out_sample_num,
                             int *out_frame_size, int *out_side_info_size,
                             int *out_padding)
{
    if (buf[0] != 0xFF || (buf[1] & 0xE0) != 0xE0)
        return -1;

    int version_id = (buf[1] & 0x18) >> 3;   /* BB: 3=MPEG1, 2=MPEG2, 0=MPEG2.5 */
    int layer_num = (buf[1] & 0x06) >> 1;    /* CC: 3=layer1, 2=layer2, 1=layer3 */
    int bitrate_idx = (buf[2] & 0xF0) >> 4;  /* EEEE */
    int sample_idx = (buf[2] & 0x0C) >> 2;   /* FF */
    int padding = (buf[2] & 0x02) >> 1;      /* G */
    int channel_mode = (buf[3] & 0xC0) >> 6; /* II */

    if (bitrate_idx == 0xF || layer_num == 0x0 || sample_idx >= 3)
        return -1;

    /* 查表获取比特率 */
    int table_row;
    if (version_id == 3)
        table_row = 3 - layer_num;
    else
        table_row = (layer_num == 3) ? 3 : 4;

    int bitrate = bitrate_table[table_row][bitrate_idx];
    if (bitrate == 0)
        return -1;

    /* 查表获取采样率和每帧采样数 */
    int version_col = (version_id == 3) ? 0 : ((version_id == 2) ? 1 : 2);
    int sample_rate = sample_rate_table[version_col][sample_idx];
    int sample_num = sample_num_table[3 - layer_num][version_col];

    /* 帧大小 */
    int frame_size;
    if (layer_num != 3) /* layer1 */
        frame_size = (sample_num / 8 * bitrate * 1000) / sample_rate + padding * 4;
    else /* layer2/3 */
        frame_size = (sample_num / 8 * bitrate * 1000) / sample_rate + padding;

    /* 边信息大小 (用于定位 Xing/Info 头) */
    int side_info_size;
    if (version_id == 3)
        side_info_size = (channel_mode == 3) ? 17 : 32;
    else
        side_info_size = (channel_mode == 3) ? 9 : 17;

    *out_bitrate = bitrate;
    *out_sample_rate = sample_rate;
    *out_sample_num = sample_num;
    *out_frame_size = frame_size;
    *out_side_info_size = side_info_size;
    *out_padding = padding;
    return 0;
}

static int scan_duration(const char *file_path)
{
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        AMY_LOG_ERROR("scan_duration open %s failed\n", file_path);
        return 0;
    }

    int file_size = (int)lseek(fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        close(fd);
        return 0;
    }

    /* 检查 ID3V2 标签，计算音频数据起始偏移 */
    int start_pos = 0;
    unsigned char head[10];
    lseek(fd, 0, SEEK_SET);
    if (read(fd, head, 10) == 10)
    {
        if (head[0] == 'I' && head[1] == 'D' && head[2] == '3')
        {
            /* ID3V2 大小: 4字节 synchsafe 整数 (每字节低7位) */
            int tag_size = ((head[6] & 0x7F) << 21) | ((head[7] & 0x7F) << 14) |
                           ((head[8] & 0x7F) << 7) | (head[9] & 0x7F);
            start_pos = tag_size + 10;
        }
    }

    /* 检查 ID3V1 标签 (文件末尾 128 字节) */
    int id3v1_size = 0;
    lseek(fd, -128, SEEK_END);
    unsigned char tail[128];
    if (read(fd, tail, 128) == 128)
    {
        if (tail[0] == 'T' && tail[1] == 'A' && tail[2] == 'G')
            id3v1_size = 128;
    }

    /* 搜索第一个有效帧头，并解析 Xing/Info/VBRI */
    lseek(fd, start_pos, SEEK_SET);
    unsigned char buf[1024];
    int total_data_size = file_size - id3v1_size - start_pos;
    int duration_ms = 0;
    int found = 0;

    while (lseek(fd, 0, SEEK_CUR) + 4 < file_size - id3v1_size)
    {
        int cur_pos = lseek(fd, 0, SEEK_CUR);
        int read_size = (cur_pos + 1024 <= file_size - id3v1_size)
                            ? 1024
                            : (file_size - id3v1_size - cur_pos);

        int n = read(fd, buf, read_size);
        if (n < 4)
            break;

        for (int i = 0; i < n - 4; i++)
        {
            if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0)
                continue;

            int bitrate, sample_rate, sample_num, frame_size, side_info_size, padding;
            if (scan_frame_header(buf + i, &bitrate, &sample_rate, &sample_num,
                                  &frame_size, &side_info_size, &padding) != 0)
                continue;

            /* Xing/Info 头位于: 帧同步字 + 4字节帧头 + side_info_size 之后 */
            int xing_pos = cur_pos + i + 4 + side_info_size;
            unsigned char xing_buf[32] = {0};
            lseek(fd, xing_pos, SEEK_SET);
            read(fd, xing_buf, 32);

            int total_frames = 0;

            if (xing_buf[0] == 'X' && xing_buf[1] == 'i' && xing_buf[2] == 'n' && xing_buf[3] == 'g')
            {
                /* VBR: Xing 头 */
                if (xing_buf[7] & 0x01)
                {
                    total_frames = ((int)xing_buf[8] << 24) | ((int)xing_buf[9] << 16) |
                                   ((int)xing_buf[10] << 8) | (int)xing_buf[11];
                }
                else
                {
                    total_frames = total_data_size / frame_size;
                }
            }
            else if (xing_buf[0] == 'I' && xing_buf[1] == 'n' && xing_buf[2] == 'f' && xing_buf[3] == 'o')
            {
                /* CBR: Info 头 */
                if (xing_buf[7] & 0x01)
                {
                    total_frames = ((int)xing_buf[8] << 24) | ((int)xing_buf[9] << 16) |
                                   ((int)xing_buf[10] << 8) | (int)xing_buf[11];
                }
                else
                {
                    total_frames = total_data_size / frame_size;
                }
            }
            else if (xing_buf[0] == 'V' && xing_buf[1] == 'B' && xing_buf[2] == 'R' && xing_buf[3] == 'I')
            {
                /* VBR: VBRI 头 (Fraunhofer) */
                total_frames = ((int)xing_buf[14] << 24) | ((int)xing_buf[15] << 16) |
                               ((int)xing_buf[16] << 8) | (int)xing_buf[17];
            }
            else
            {
                /* CBR: 无元数据头，按文件大小估算 */
                total_frames = total_data_size / frame_size;
            }

            if (total_frames > 0 && sample_rate > 0)
            {
                /* duration = 帧数 * 每帧采样数 / 采样率 */
                duration_ms = (int)((long long)total_frames * sample_num * 1000 / sample_rate);
            }
            found = 1;
            break;
        }
        if (found)
            break;
    }

    close(fd);

    if (duration_ms <= 0)
        AMY_LOG_ERROR("scan_duration %s returned %dms (file_size=%d, total_data=%d)\n",
                      file_path, duration_ms, file_size, total_data_size);
    return duration_ms;
}

/************************** 内部辅助 **************************/

static void ensure_init(void)
{
    if (g_player.run)
        return;

    g_player.run = 1;
    g_player.mp3_buf = (char *)Amy_malloc(MP3_BUF_MAX);
    Amy_mutex_init(&g_player.mutex, NULL);
    Amy_cond_init(&g_player.cond, NULL);
    Amy_pthread_create(&g_player.tid, music_player_thread, &g_player, 32 * 1024, 12);
}

static void build_file_path(music_player_ctx *ctx)
{
    snprintf(ctx->file_path, sizeof(ctx->file_path),
             TFCARD_MOUNT_PATH "/music/%s", ctx->track_name);
}

static void load_track_info(int index)
{
    g_player.track_current = index;
    g_player.track_total = Amy_fm_get_count(FM_DIR_MUSIC);

    fm_file_info_t info;
    if (Amy_fm_get_file(FM_DIR_MUSIC, index, &info) == 0)
    {
        strncpy(g_player.track_name, info.name, sizeof(g_player.track_name) - 1);
        g_player.track_name[sizeof(g_player.track_name) - 1] = '\0';
    }
    else
    {
        g_player.track_name[0] = '\0';
        AMY_LOG_ERROR("music Amy_fm_get_file(%d) failed\n", index);
    }

    if (g_player.track_name[0] == '\0')
    {
        g_player.file_path[0] = '\0';
        g_player.file_size = 0;
        g_player.duration_ms = 0;
        return;
    }

    build_file_path(&g_player);
    AMY_LOG_DEBUG("music file_path=%s\n", g_player.file_path);

    struct stat st;
    if (stat(g_player.file_path, &st) == 0)
        g_player.file_size = (int)st.st_size;
    else
    {
        g_player.file_size = 0;
        AMY_LOG_ERROR("music stat %s failed\n", g_player.file_path);
    }

    g_player.duration_ms = scan_duration(g_player.file_path);
    AMY_LOG_DEBUG("music load track[%d]: name=%s, duration=%dms, size=%d\n",
                  index, g_player.track_name, g_player.duration_ms, g_player.file_size);
}

static void prepare_play(int index)
{
    load_track_info(index);
    g_player.resume_byte = 0;
    g_player.resume_position_ms = 0;
    g_player.position_offset_ms = 0;
    g_player.seek_requested = 0;
    g_player.stop_requested = 0;
    mad_timer_reset(&g_player.current_position);
}

/************************** 播放控制 **************************/

int Amy_music_player_play(int index)
{
    ensure_init();

    Amy_mutex_lock(&g_player.mutex);

    /* 先更新曲目信息再请求停止，保证解码线程中止旧解码后
     * 重新开始时会读取到新 file_path；两条路径统一，避免
     * 从 IDLE 调用时漏置 new_track 导致“日志已更新却不播”。 */
    load_track_info(index);
    g_player.resume_byte = 0;
    g_player.resume_position_ms = 0;
    g_player.position_offset_ms = 0;
    g_player.seek_requested = 0;
    mad_timer_reset(&g_player.current_position);

    g_player.stop_requested = 1;
    g_player.new_track = 1;

    g_player.state = STATE_PLAY;
    Amy_cond_signal(&g_player.cond);
    Amy_mutex_unlock(&g_player.mutex);

    return 0;
}

int Amy_music_player_pause(void)
{
    Amy_mutex_lock(&g_player.mutex);
    if (g_player.state == STATE_PLAY)
    {
        g_player.stop_requested = 1;
        g_player.state = STATE_PAUSE;
    }
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

int Amy_music_player_resume(void)
{
    Amy_mutex_lock(&g_player.mutex);
    if (g_player.state == STATE_PAUSE || g_player.state == STATE_IDLE)
        g_player.state = STATE_PLAY;
    Amy_cond_signal(&g_player.cond);
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

int Amy_music_player_toggle(void)
{
    Amy_mutex_lock(&g_player.mutex);
    if (g_player.state == STATE_PLAY)
    {
        g_player.stop_requested = 1;
        g_player.state = STATE_PAUSE;
    }
    else
    {
        g_player.state = STATE_PLAY;
        Amy_cond_signal(&g_player.cond);
    }
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

int Amy_music_player_stop(void)
{
    if (!g_player.run)
        return 0;

    Amy_mutex_lock(&g_player.mutex);
    g_player.stop_requested = 1;
    g_player.state = STATE_IDLE;
    g_player.finish_cb = NULL;
    g_player.track_name[0] = '\0';
    g_player.position_offset_ms = 0;
    mad_timer_reset(&g_player.current_position);
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

int Amy_music_player_is_playing(void)
{
    return g_player.state == STATE_PLAY;
}

/************************** 曲目导航（增加防抖） **************************/

int Amy_music_player_next(void)
{
    Amy_mutex_lock(&g_player.mutex);
    int state = g_player.state;
    int total = Amy_fm_get_count(FM_DIR_MUSIC);
    int cur = g_player.track_current;
    // 已有切歌/跳转请求直接拦截防抖
    if (g_player.new_track || g_player.seek_requested)
    {
        Amy_mutex_unlock(&g_player.mutex);
        return 0;
    }
    Amy_mutex_unlock(&g_player.mutex);

    if (total <= 0)
        return -1;

    int next = (cur + 1 < total) ? cur + 1 : 0;

    if (state == STATE_IDLE || state == STATE_PAUSE)
    {
        Amy_music_player_select(next);
    }
    else
    {
        Amy_music_player_play(next);
    }

    return 0;
}

int Amy_music_player_prev(void)
{
    Amy_mutex_lock(&g_player.mutex);
    int state = g_player.state;
    int total = Amy_fm_get_count(FM_DIR_MUSIC);
    int cur = g_player.track_current;
    // 防抖拦截
    if (g_player.new_track || g_player.seek_requested)
    {
        Amy_mutex_unlock(&g_player.mutex);
        return 0;
    }
    Amy_mutex_unlock(&g_player.mutex);

    if (total <= 0)
        return -1;

    int prev = (cur > 0) ? cur - 1 : total - 1;

    if (state == STATE_IDLE || state == STATE_PAUSE)
    {
        Amy_music_player_select(prev);
    }
    else
    {
        Amy_music_player_play(prev);
    }

    return 0;
}

int Amy_music_player_select(int index)
{
    ensure_init();

    Amy_mutex_lock(&g_player.mutex);
    if (g_player.state == STATE_PLAY)
    {
        g_player.stop_requested = 1;
        Amy_mutex_unlock(&g_player.mutex);
        Amy_msleep(50);
        Amy_mutex_lock(&g_player.mutex);
    }

    prepare_play(index);
    g_player.state = STATE_IDLE;
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

/************************** 当前曲目信息 **************************/

int Amy_music_player_get_current_index(void)
{
    return g_player.track_current;
}

const char *Amy_music_player_get_current_name(void)
{
    return g_player.track_name;
}

float Amy_music_player_get_duration(void)
{
    return g_player.duration_ms / 1000.0f;
}

int Amy_music_player_get_duration_ms(void)
{
    return g_player.duration_ms;
}

int Amy_music_player_get_progress(void)
{
    if (g_player.duration_ms <= 0)
        return 0;

    Amy_mutex_lock(&g_player.mutex);
    unsigned long cur = mad_timer_count(g_player.current_position, MAD_UNITS_MILLISECONDS);
    int pos_ms = g_player.position_offset_ms + (int)cur;
    Amy_mutex_unlock(&g_player.mutex);

    int pct = (pos_ms * 100) / g_player.duration_ms;
    return pct > 100 ? 100 : pct;
}

int Amy_music_player_get_position(void)
{
    if (!g_player.run)
        return 0;

    Amy_mutex_lock(&g_player.mutex);
    unsigned long cur = mad_timer_count(g_player.current_position, MAD_UNITS_MILLISECONDS);
    int pos_ms = g_player.position_offset_ms + (int)cur;
    Amy_mutex_unlock(&g_player.mutex);

    return (pos_ms + 500) / 1000;
}

int Amy_music_player_get_position_ms(void)
{
    if (!g_player.run)
        return 0;

    Amy_mutex_lock(&g_player.mutex);
    unsigned long cur = mad_timer_count(g_player.current_position, MAD_UNITS_MILLISECONDS);
    int pos_ms = g_player.position_offset_ms + (int)cur;
    Amy_mutex_unlock(&g_player.mutex);

    return pos_ms;
}

/************************** 跳转 **************************/

int Amy_music_player_seek_percent(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    int ms = (g_player.duration_ms * percent) / 100;
    return Amy_music_player_seek_ms(ms);
}

int Amy_music_player_seek_ms(int ms)
{
    if (!g_player.run)
        return -1;

    Amy_mutex_lock(&g_player.mutex);
    if (g_player.state != STATE_PLAY && g_player.state != STATE_PAUSE)
    {
        Amy_mutex_unlock(&g_player.mutex);
        return -1;
    }

    if (ms < 0)
        ms = 0;
    if (ms > g_player.duration_ms)
        ms = g_player.duration_ms;

    g_player.seek_target_ms = ms;
    g_player.seek_requested = 1;
    g_player.stop_requested = 1;

    /* 估算字节偏移 */
    if (g_player.duration_ms > 0 && g_player.file_size > 0)
        g_player.resume_byte = (int)((long long)ms * g_player.file_size / g_player.duration_ms);
    g_player.resume_position_ms = ms;

    if (g_player.state == STATE_PAUSE)
        g_player.state = STATE_PLAY;

    Amy_cond_signal(&g_player.cond);
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

int Amy_music_player_seek_sec(int sec)
{
    return Amy_music_player_seek_ms(sec * 1000);
}

/************************** 倍速播放 **************************/

int Amy_music_player_set_speed(float speed)
{
    if (speed != 0.5f && speed != 1.0f && speed != 2.0f && speed != 3.0f)
        return -1;

    ensure_init();

    Amy_mutex_lock(&g_player.mutex);
    g_player.speed = speed;
    g_player.speed_phase = 0.0f;
    Amy_mutex_unlock(&g_player.mutex);
    return 0;
}

float Amy_music_player_get_speed(void)
{
    return g_player.speed;
}

/************************** 音量 **************************/

void Amy_music_player_set_volume(int volume)
{
    ensure_init();
    Amy_mutex_lock(&g_player.mutex);
    g_player.volume = volume;
    if (g_player.audio_dev)
    {
        audio_output_set_volume(g_player.audio_dev, volume);
    }

    Amy_mutex_unlock(&g_player.mutex);
}

int Amy_music_player_get_volume(void)
{
    return g_player.volume;
}

/************************** 回调 **************************/

void Amy_music_player_set_finish_cb(void (*cb)(void))
{
    if (!g_player.run)
        return;

    Amy_mutex_lock(&g_player.mutex);
    g_player.finish_cb = cb;
    Amy_mutex_unlock(&g_player.mutex);
}

/************************** 工具 **************************/

int Amy_music_player_get_track_count(void)
{
    return Amy_fm_get_count(FM_DIR_MUSIC);
}