#include "Amy_sdk.h"
#include "video_play.h"
#include "Amy_log.h"
#include "queues/queues.h"
#include "ringbuffer/ringbuffer.h"
#include "mempool/mempool.h"
#include "avilib.h"
#include "video_decode.h"
#include "audio_output.h"
#include "ak_common_graphics.h"
#include "ak_mem.h"
#include "lvgl/device/lv_disp_dev.h"
#include <stdlib.h>
#include <string.h>

/* =========================== 宏定义 =========================== */
#define VQ_NUM_MAX (25)                   // 视频队列最大元素数
#define AQ_NUM_MAX (25)                   // 音频队列最大元素数
#define SYNC_THREASHOLD (100)             // 音视频同步阈值(ms)
#define VQ_FEED_THRESH (3)                // 视频队列剩余空位触发读取的阈值
#define AQ_FEED_THRESH (2)                // 音频队列剩余空位触发读取的阈值
#define MIN(A, B) ((A) < (B) ? (A) : (B)) // 最小值宏
#define MAX(A, B) ((A) > (B) ? (A) : (B)) // 最大值宏

/* =========================== 类型定义 =========================== */
/**
 * @brief AVI播放器上下文结构体
 */
typedef struct avi_player_context
{
    int run;           // 播放器主运行标志
    Amy_mutex_t mutex; // 全局互斥锁
    Amy_cond_t cond;   // 播放状态条件变量（用于 PAUSE/STOP 时阻塞线程）
    Amy_thread_t tid;  // 主播放线程ID
    avi_t *avi_dev;    // AVI文件句柄

    mp_pool *mp;         // 内存池句柄
    queues_conetext *aq; // 音频队列句柄
    queues_conetext *vq; // 视频队列句柄

    unsigned long frame_duration; // 视频帧时长(ms)，= 1000 / v_rate

    double v_rate;               // 视频帧率
    unsigned long v_frame_idx;   // 当前已送解码的视频帧索引
    unsigned long v_frame_total; // 视频总帧数
    unsigned int v_width;        // 视频宽度
    unsigned int v_height;       // 视频高度
    int v_enc_type;              // 视频编码类型(MJPEG/H264)

    int has_audio;             // 是否包含音频(0-无,1-有)，播放中途音频流结束会被清零
    int file_has_audio;        // 文件本身是否含音频，重播时据此恢复，不受中途清零影响
    int a_channels;            // 音频通道数
    int a_samplerate;          // 音频采样率
    int a_bits;                // 音频位深
    int a_frame_len;           // 每次读取的音频帧字节数
    unsigned long a_frame_idx; // 当前已输出的音频帧索引

    int ao_run;                   // 音频输出线程运行标志
    Amy_thread_t ao_tid;          // 音频输出线程ID
    audio_output_context *ao_dev; // 音频输出设备句柄

    int vdec_run;                   // 视频解码线程运行标志
    Amy_thread_t vdec_tid;          // 视频解码线程ID
    video_decode_context *vdec_dev; // 视频解码设备句柄

    /*
     * 音视频时钟（单位：ms）
     *   a_clock = a_frame_idx × 每帧实际音频时长
     *             = a_frame_idx × a_frame_len × 1000
     *               / (a_channels × a_samplerate × a_bits / 8)
     *   v_clock = v_frame_idx × frame_duration
     */
    unsigned long long a_clock; // 音频时钟(ms)
    unsigned long long v_clock; // 视频时钟(ms)

    /*
     * 无音频限速基准：每次被命令进入 PLAY 时记录当时的墙钟与已送解码帧数。
     * 播放中可能多次进入 PLAY(暂停恢复/重播)，必须据此重新对表，
     * 否则墙钟持续流逝而帧号不回退，限速会因目标时刻全部落在过去而失效(快进)。
     */
    unsigned long long play_base_wall; // 进入 PLAY 的墙钟(ms)
    unsigned long long play_base_vidx; // 进入 PLAY 时已送解码的帧数

    unsigned int status;       // 播放器状态(PLAY/PAUSE/STOP/IDLE)
    unsigned long v_frame_pos; // 当前读取的视频帧位置

    int has_sps;
    int has_pps;
    int has_idr;
} avi_player_context;

/* =========================== 静态函数声明 =========================== */
static void avi_player_aq_release_cb(void *user_data, void *data);
static void avi_player_vq_release_cb(void *user_data, void *data);
static void a_clock_update(avi_player_context *ctx);
static void v_clock_update(avi_player_context *ctx);
static int video_player_refresh_cb(video_mode_attr *attr, void *user_data);
static void *avi_vdec_thread(void *arg);
static void *avi_ao_thread(void *arg);
static void *avi_player_thread(void *arg);

/* =========================== 静态函数实现 =========================== */

/**
 * @brief 音频队列元素释放回调
 * @param user_data 内存池句柄
 * @param data 待释放的音频数据
 */
static void avi_player_aq_release_cb(void *user_data, void *data)
{
    if (user_data && data)
    {
        Amy_mempool_free(user_data, data);
    }
}

/**
 * @brief 视频队列元素释放回调
 * @param user_data 视频解码设备句柄
 * @param data 待释放的视频流数据
 */
static void avi_player_vq_release_cb(void *user_data, void *data)
{
    if (user_data && data)
    {
        video_decodec_buf_put(user_data, data);
    }
}

/**
 * @brief 更新音频时钟
 *
 * 音频时钟基于实际音频帧字节数与采样参数推算，而非复用视频帧时长，
 * 以避免两路时钟因帧时长差异产生累积漂移。
 *
 * 计算公式（近似播放时钟）：
 *   written_ms  = a_frame_idx × a_frame_len × 1000 / bytes_per_sec
 *   buffered_ms = ao_used_bytes × 1000 / bytes_per_sec
 *   a_clock     = max(0, written_ms - buffered_ms)
 *
 * 说明：
 *   仅用“已写入字节数”会让启动阶段音频时钟虚高（设备缓冲尚未真正播放），
 *   从而导致视频被误判为落后并产生开头快放/跳帧。
 *
 * @param ctx 播放器上下文
 */
static void a_clock_update(avi_player_context *ctx)
{
    if (NULL == ctx)
    {
        AMY_LOG_ERROR("a_clock_update: ctx is NULL\n");
        return;
    }

    unsigned long long bytes_per_sec = (unsigned long long)ctx->a_channels * ctx->a_samplerate * (ctx->a_bits / 8);
    if (bytes_per_sec == 0)
    {
        return;
    }

    unsigned int afree = 0, aused = 0, atotal = 0;
    if (ctx->ao_dev)
    {
        audio_output_query(ctx->ao_dev, &afree, &aused, &atotal);
    }

    Amy_mutex_lock(&ctx->mutex);
    unsigned long long written_ms =
        (unsigned long long)ctx->a_frame_idx * ctx->a_frame_len * 1000ULL / bytes_per_sec;
    unsigned long long buffered_ms =
        (unsigned long long)aused * 1000ULL / bytes_per_sec;
    ctx->a_clock = (written_ms > buffered_ms) ? (written_ms - buffered_ms) : 0;
    Amy_mutex_unlock(&ctx->mutex);
}

/**
 * @brief 更新视频时钟
 * @param ctx 播放器上下文
 */
static void v_clock_update(avi_player_context *ctx)
{
    if (NULL == ctx)
    {
        AMY_LOG_ERROR("v_clock_update: ctx is NULL\n");
        return;
    }

    Amy_mutex_lock(&ctx->mutex);
    ctx->v_clock = (unsigned long long)ctx->v_frame_idx * ctx->frame_duration;
    Amy_mutex_unlock(&ctx->mutex);
}

/**
 * @brief 视频刷新回调（LVGL视频模式）
 * @param attr 视频模式属性
 * @param user_data 播放器上下文
 * @return  0-正常, 1-暂停, -1-失败/结束(跳帧)
 */
static int video_player_refresh_cb(video_mode_attr *attr, void *user_data)
{
    if (NULL == attr || NULL == user_data)
    {
        AMY_LOG_ERROR("video_player_refresh_cb: invalid param\n");
        return -1;
    }

    avi_player_context *ctx = (avi_player_context *)user_data;
    if (!ctx->vdec_dev)
    {
        AMY_LOG_ERROR("video_player_refresh_cb: vdec_dev is NULL\n");
        return -1;
    }

    int isnew = 0;
    struct ak_vdec_frame frame = {0};
    int ret = video_decodec_get_frame(ctx->vdec_dev, &frame, &isnew);
    if (ret != 0)
    {
        // 暂无可显示帧，保持上一帧画面（输出空属性）
        memset(attr, 0, sizeof(video_mode_attr));
        attr->dpx = 0;
        attr->dpy = 0;
        attr->dpw = 1024;
        attr->dph = 600;
        return 0;
    }

    // 设置视频输出属性
    attr->sw = frame.frame_obj.data.pitch_width;
    attr->sh = frame.frame_obj.data.pitch_height;
    attr->spx = 0;
    attr->spy = 0;
    attr->spw = frame.width;
    attr->sph = frame.height;
    attr->sfmt = GP_FORMAT_YUV420SP;
    ak_mem_dma_vaddr2paddr((void *)frame.frame_obj.data.data,
                           (unsigned long *)&attr->sphy);

    attr->dpx = 0;
    attr->dpy = 0;
    attr->dpw = 1024;
    attr->dph = 600;

    // 音视频同步处理（仅对新帧判断）
    if (isnew)
    {
        long long diff = 0;
        Amy_mutex_lock(&ctx->mutex);
        diff = (long long)ctx->a_clock - (long long)ctx->v_clock;
        Amy_mutex_unlock(&ctx->mutex);

        if (diff > SYNC_THREASHOLD)
        {
            // 视频落后音频，跳过当前帧令视频尽快追上
            return -1;
        }
        // 视频超前时不主动延迟：由 avi_vdec_thread 的同步检测负责限速
    }
    return 0;
}

/**
 * @brief 视频解码线程（从视频队列取流 → 送解码器）
 * @param arg 播放器上下文
 * @return NULL
 */
static void *avi_vdec_thread(void *arg)
{
    if (NULL == arg)
    {
        AMY_LOG_ERROR("avi_vdec_thread: arg is NULL\n");
        return NULL;
    }

    avi_player_context *ctx = (avi_player_context *)arg;
    stream_data *vst = NULL;
    unsigned int free_slots, used, total;
    unsigned long long play_base_wall = 0;
    unsigned long long play_base_vidx = 0;

    while (ctx->vdec_run)
    {
        // PAUSE / STOP / IDLE 状态：条件变量等待，避免忙轮询
        Amy_mutex_lock(&ctx->mutex);
        while (ctx->vdec_run &&
               ((ctx->status == VIDEO_PLAYER_STATUS_PAUSE) ||
                (ctx->status == VIDEO_PLAYER_STATUS_STOP) ||
                (ctx->status == VIDEO_PLAYER_STATUS_IDLE)))
        {
            Amy_cond_wait(&ctx->cond, &ctx->mutex);
        }

        Amy_mutex_unlock(&ctx->mutex);

        if (!ctx->vdec_run)
        {
            break;
        }

        // 视频超前音频：限速等待
        long long diff = 0;
        int status = VIDEO_PLAYER_STATUS_STOP;
        int has_audio = 0;
        unsigned long a_frame_idx = 0;
        unsigned long v_frame_idx = 0;
        Amy_mutex_lock(&ctx->mutex);
        diff = (long long)ctx->v_clock - (long long)ctx->a_clock;
        status = ctx->status;
        has_audio = ctx->has_audio;
        a_frame_idx = ctx->a_frame_idx;
        v_frame_idx = ctx->v_frame_idx;
        play_base_wall = ctx->play_base_wall;
        play_base_vidx = ctx->play_base_vidx;
        Amy_mutex_unlock(&ctx->mutex);

        /*
         * 无音频(或无有效音频时钟)时，视频没有外部节奏来源，解码线程会以
         * 硬件允许的最高速度送流，导致播放过快。此处按帧率用墙钟自行限速：
         * 限速基准在每次进入 PLAY 时由 avi_player_toggle_play 更新，
         * 此后第 N 帧约在 基准 + N*frame_duration 送解码。
         * 有音频时钟的文件仍走下方音频同步限速，不受影响。
         */
        if ((status == VIDEO_PLAYER_STATUS_PLAY) && !has_audio)
        {
            unsigned long long now_wall;
            Amy_timestamp(now_wall);
            unsigned long long target_wall =
                play_base_wall + (unsigned long long)(v_frame_idx - play_base_vidx) *
                                     ctx->frame_duration;
            if (now_wall < target_wall)
            {
                Amy_msleep(target_wall - now_wall);
                continue;
            }
        }

        /*
         * STOP->PLAY 切换初期，音频线程尚未产出首帧时钟（a_frame_idx==0），
         * 若视频线程连续送流会造成前几帧突进。此处先按帧时长限速。
         */
        if ((status == VIDEO_PLAYER_STATUS_PLAY) && has_audio &&
            (a_frame_idx == 0) && (v_frame_idx > 0))
        {
            Amy_msleep(ctx->frame_duration);
            continue;
        }

        if ((diff > SYNC_THREASHOLD) && has_audio && (status == VIDEO_PLAYER_STATUS_PLAY))
        {
            Amy_msleep(ctx->frame_duration);
            continue;
        }

        // 查询解码缓冲区，有空位则从队列取帧送解码
        video_decodec_stream_query(ctx->vdec_dev, &free_slots, &used, &total);
        if (used < (total - 1))
        {
            if (Amy_queues_get(ctx->vq, (void **)&vst, NULL, NULL) == 0)
            {
                if (video_decodec_send_stream(ctx->vdec_dev, vst) != 0)
                {
                    avi_player_vq_release_cb(ctx->vdec_dev, vst);
                    AMY_LOG_ERROR("avi_vdec_thread: send video stream failed\n");
                }
                else
                {
                    // 首帧解码完成后切换为 PAUSE，等待业务层显式 play
                    Amy_mutex_lock(&ctx->mutex);
                    ctx->v_frame_idx++;
                    Amy_mutex_unlock(&ctx->mutex);
                    v_clock_update(ctx);
                }
            }
            else
            {
                Amy_mutex_lock(&ctx->mutex);
                if (ctx->status == VIDEO_PLAYER_STATUS_INIT_LOADED)
                {
                    ctx->status = VIDEO_PLAYER_STATUS_PAUSE;
                }
                Amy_mutex_unlock(&ctx->mutex);
            }
        }

        Amy_msleep(10);
    }

    return NULL;
}

/**
 * @brief 音频输出线程（从音频队列取帧 → 写音频设备）
 * @param arg 播放器上下文
 * @return NULL
 */
static void *avi_ao_thread(void *arg)
{
    if (NULL == arg)
    {
        AMY_LOG_ERROR("avi_ao_thread: arg is NULL\n");
        return NULL;
    }

    avi_player_context *ctx = (avi_player_context *)arg;
    stream_data *ast = NULL;
    unsigned int used = 0, total = 0, afree = 0;

    while (ctx->ao_run)
    {
        // STOP / IDLE / INIT_LOADED / PAUSE 状态：条件变量等待，仅 PLAY 时输出音频
        Amy_mutex_lock(&ctx->mutex);
        while (ctx->ao_run &&
               ((ctx->status == VIDEO_PLAYER_STATUS_STOP) ||
                ctx->status == VIDEO_PLAYER_STATUS_PAUSE ||
                (ctx->status == VIDEO_PLAYER_STATUS_IDLE) ||
                (ctx->status == VIDEO_PLAYER_STATUS_INIT_LOADED)))
        {
            Amy_cond_wait(&ctx->cond, &ctx->mutex);
        }
        Amy_mutex_unlock(&ctx->mutex);
        if (!ctx->ao_run)
        {
            break;
        }
        // 查询音频输出缓冲区，有空位则从队列取帧写入
        audio_output_query(ctx->ao_dev, &afree, &used, &total);
        if (afree > (unsigned int)ctx->a_frame_len)
        {
            if (Amy_queues_get(ctx->aq, (void **)&ast, NULL, NULL) == 0)
            {
                if (ast && ast->size > 0)
                {
                    audio_output_write(ctx->ao_dev, (char *)ast->data, ast->size);

                    Amy_mutex_lock(&ctx->mutex);
                    ctx->a_frame_idx++;
                    Amy_mutex_unlock(&ctx->mutex);
                }
                // 无论写入与否均更新时钟，保证时钟单调递进
                a_clock_update(ctx);
                avi_player_aq_release_cb(ctx->mp, ast);
            }
        }

        Amy_msleep(10);
    }

    return NULL;
}

/**
 * @brief 遍历解析H264聚合帧，仅判断帧数据是否同时包含SPS(0x07)、PPS(0x08)和IDR帧(0x05)
 * @param h264_buf H264原始码流缓冲区（支持普通帧/聚合帧）
 * @param buf_size 码流缓冲区大小
 * @return 1：同时包含SPS+PPS+IDR三种NALU，0：缺少至少一种NALU，-1：无效参数/无效码流
 * @note 1. 仅做存在性判断，不提取任何NALU的具体内容，不依赖全局上下文
 *       2. 使用局部变量标记三种NALU的存在状态，逻辑轻量化
 *       3. 兼容H264 3字节/4字节起始码，支持聚合帧解析
 */
int is_h264_contains_sps_pps_idr(unsigned char *h264_buf, int buf_size, avi_player_context *ctx)
{
    // 1. 参数合法性校验
    if (h264_buf == NULL || buf_size < 3)
    {
        return -1; // 无效参数或码流长度不足
    }

    int offset = 0; // 遍历码流的偏移量

    // 3. 循环遍历，解析所有NALU单元（仅判断存在性，不提取内容）
    while (offset <= buf_size - 3)
    {
        int start_code_len = 0;

        // 查找NALU起始码：优先4字节(0x00 0x00 0x00 0x01)，再3字节(0x00 0x00 0x01)
        if (offset + 3 < buf_size &&
            h264_buf[offset] == 0 &&
            h264_buf[offset + 1] == 0 &&
            h264_buf[offset + 2] == 0 &&
            h264_buf[offset + 3] == 1)
        {
            start_code_len = 4;
        }
        else if (h264_buf[offset] == 0 &&
                 h264_buf[offset + 1] == 0 &&
                 h264_buf[offset + 2] == 1)
        {
            start_code_len = 3;
        }
        else
        {
            // 无起始码，偏移1字节继续查找
            offset++;
            continue;
        }

        // 计算NALU头部偏移（起始码后1字节即为NALU Header）
        int nalu_header_offset = offset + start_code_len;
        if (nalu_header_offset >= buf_size)
        {
            break; // 头部偏移超出码流长度，终止遍历
        }

        // 提取NALU类型（NALU Header低5位）
        unsigned char nalu_header = h264_buf[nalu_header_offset];
        int nalu_type = nalu_header & 0x1F;

        // 4. 标记对应NALU的存在状态（仅判断，不提取内容）
        if (nalu_type == 0x07)
        {
            ctx->has_sps = 1; // 标记包含SPS
            printf("标记包含SPS\r\n");
        }
        else if (nalu_type == 0x08)
        {
            ctx->has_pps = 1; // 标记包含PPS
            printf("标记包含PPS\r\n");
        }
        else if (nalu_type == 0x05)
        {
            ctx->has_idr = 1; // 标记包含IDR帧
            printf("标记包含IDR帧\r\n");
        }

        // 可选优化：三种NALU都已找到，可提前退出循环，提升性能
        if (ctx->has_sps && ctx->has_pps && ctx->has_idr)
        {
            break;
        }

        // 偏移到当前NALU末尾，继续查找下一个NALU
        offset = nalu_header_offset + 1;
    }

    // 5. 最终判断：是否同时包含SPS+PPS+IDR三种NALU
    if (ctx->has_sps && ctx->has_pps && ctx->has_idr)
    {
        return 1; // 三种NALU都存在
    }
    else
    {
        return 0; // 缺少至少一种NALU
    }
}

/**
 * @brief 播放器主线程（音视频帧读取 → 分发入队）
 * @param arg 播放器上下文
 * @return NULL
 */
static void *avi_player_thread(void *arg)
{
    if (NULL == arg)
    {
        AMY_LOG_ERROR("avi_player_thread: arg is NULL\n");
        return NULL;
    }

    avi_player_context *ctx = (avi_player_context *)arg;
    int key_frame = 0;
    ctx->v_frame_pos = 0;

    // --- 初始化视频队列 ---
    ctx->vq = Amy_queues_open(VQ_NUM_MAX, 0, 0, NULL, 0);
    if (!ctx->vq)
    {
        AMY_LOG_ERROR("avi_player_thread: open video queue failed\n");
        goto thread_exit;
    }

    // --- 启动视频解码线程 ---
    ctx->vdec_run = 1;
    ctx->vdec_dev = video_decodec_open(ctx->v_width, ctx->v_height, ctx->v_enc_type);
    if (!ctx->vdec_dev)
    {
        AMY_LOG_ERROR("avi_player_thread: open video decoder failed\n");
        goto thread_exit;
    }
    Amy_pthread_create(&ctx->vdec_tid, avi_vdec_thread, ctx, 32 * 1024, 12);

    // --- 初始化音频（如有音频流）---
    if (ctx->has_audio)
    {
        ctx->aq = Amy_queues_open(AQ_NUM_MAX, 0, 0, NULL, 0);
        if (!ctx->aq)
        {
            AMY_LOG_ERROR("avi_player_thread: open audio queue failed\n");
            goto thread_exit;
        }

        // 如采样率/通道数与当前设备不符则重新打开
        int chn = 0, samplerate = 0;
        if (ctx->ao_dev == NULL)
        {
            ctx->ao_dev = audio_output_get_handle();
        }
        audio_output_param_get(&chn, &samplerate, NULL);
        if ((!ctx->ao_dev) ||
            (ctx->a_samplerate != samplerate) ||
            (ctx->a_channels != chn))
        {
            if (ctx->ao_dev)
            {
                audio_output_close(ctx->ao_dev);
            }
            ctx->ao_dev = audio_output_open(ctx->a_channels, ctx->a_samplerate,
                                            80, 8 * 1024);
            if (!ctx->ao_dev)
            {
                AMY_LOG_ERROR("avi_player_thread: open audio output failed\n");
                goto thread_exit;
            }
        }

        ctx->ao_run = 1;
        Amy_pthread_create(&ctx->ao_tid, avi_ao_thread, ctx, 32 * 1024, 12);
    }

    // ====================== 音视频数据读取主循环 ======================
    while (ctx->run)
    {
        Amy_mutex_lock(&ctx->mutex);
        int status = ctx->status;
        Amy_mutex_unlock(&ctx->mutex);

        // PAUSE / STOP：等待状态变更信号（条件变量）
        if ((status == VIDEO_PLAYER_STATUS_PAUSE) ||
            (status == VIDEO_PLAYER_STATUS_STOP))
        {
            Amy_mutex_lock(&ctx->mutex);
            while (ctx->run &&
                   ((ctx->status == VIDEO_PLAYER_STATUS_PAUSE) ||
                    (ctx->status == VIDEO_PLAYER_STATUS_STOP)))
            {
                Amy_cond_wait(&ctx->cond, &ctx->mutex);
            }
            Amy_mutex_unlock(&ctx->mutex);
            continue;
        }

        if ((status == VIDEO_PLAYER_STATUS_PLAY) ||
            (status == VIDEO_PLAYER_STATUS_IDLE))
        {
            // 视频帧全部读取完毕 → 停止
            if (ctx->v_frame_pos >= ctx->v_frame_total)
            {
                Amy_mutex_lock(&ctx->mutex);
                ctx->status = VIDEO_PLAYER_STATUS_STOP;
                Amy_play_event_put(AMY_PLAY_STATE_EVENT_ID_FINISH);
                Amy_mutex_unlock(&ctx->mutex);
                continue;
            }

            // --- 视频帧入队 ---
            int vqs = Amy_queues_new_els(ctx->vq);
            if (vqs < VQ_FEED_THRESH)
            {
                AVI_set_video_position(ctx->avi_dev, ctx->v_frame_pos);
                int size = AVI_frame_size(ctx->avi_dev, ctx->v_frame_pos);

                stream_data *vst = video_decodec_buf_get(ctx->vdec_dev, size);
                if (vst)
                {
                    vst->size = AVI_read_frame(ctx->avi_dev,
                                               (char *)vst->data, &key_frame);
                    if (vst->size < 0)
                    {
                        avi_player_vq_release_cb(ctx->vdec_dev, vst);
                        AMY_LOG_ERROR("avi_player_thread: read video frame failed"
                                      "(pos=%lu, size=%d)\n",
                                      ctx->v_frame_pos, vst->size);
                        ctx->v_frame_pos++;
                    }
                    else if (ctx->status == VIDEO_PLAYER_STATUS_INIT_LOADED)
                    {
                        // INIT_LOADED 阶段静默丢帧，等待解码线程切换状态
                        avi_player_vq_release_cb(ctx->vdec_dev, vst);
                    }
                    else if (ctx->status == VIDEO_PLAYER_STATUS_IDLE)
                    {
                        // IDLE 阶段扫描关键帧：所有帧入队保留 SPS/PPS，vdec 线程在 IDLE 期间阻塞
                        if (ctx->v_enc_type == H264_ENC_TYPE)
                        {
                            key_frame = is_h264_contains_sps_pps_idr(
                                vst->data, vst->size, ctx);
                        }
                        else
                        {
                            key_frame = 1;
                        }

                        if (Amy_queues_put(ctx->vq, vst) != 0)
                        {
                            avi_player_vq_release_cb(ctx->vdec_dev, vst);
                            AMY_LOG_ERROR("avi_player_thread: put video frame "
                                          "to queue failed\n");
                        }
                        else
                        {
                            if (key_frame == 1)
                            {
                                Amy_mutex_lock(&ctx->mutex);
                                ctx->status = VIDEO_PLAYER_STATUS_INIT_LOADED;
                                Amy_cond_broadcast(&ctx->cond);
                                Amy_mutex_unlock(&ctx->mutex);
                                AMY_LOG_INFO("avi_player_thread: initial key frame "
                                             "found at pos=%lu\n",
                                             ctx->v_frame_pos);
                            }
                            ctx->v_frame_pos++;
                        }
                    }
                    else
                    {
                        if (Amy_queues_put(ctx->vq, vst) != 0)
                        {
                            avi_player_vq_release_cb(ctx->vdec_dev, vst);
                            AMY_LOG_ERROR("avi_player_thread: put video frame "
                                          "to queue failed\n");
                        }
                        else
                        {
                            ctx->v_frame_pos++;
                        }
                    }
                }
            }

            // --- 音频帧入队（如有音频流）---
            if (ctx->has_audio && ctx->ao_dev)
            {
                int aqs = Amy_queues_new_els(ctx->aq);
                if (aqs < AQ_FEED_THRESH)
                {
                    stream_data *ast = Amy_mempool_alloc(
                        ctx->mp, sizeof(stream_data) + ctx->a_frame_len);
                    if (ast)
                    {
                        ast->data = (unsigned char *)ast + sizeof(stream_data);
                        ast->size = AVI_read_audio(ctx->avi_dev,
                                                   (char *)ast->data,
                                                   ctx->a_frame_len);
                        if (ast->size <= 0)
                        {
                            avi_player_aq_release_cb(ctx->mp, ast);
                            if (ast->size == 0)
                            {
                                AMY_LOG_INFO("avi_player_thread: audio stream ended"
                                             ", disable audio sync\n");
                                Amy_mutex_lock(&ctx->mutex);
                                ctx->has_audio = 0;
                                Amy_mutex_unlock(&ctx->mutex);
                            }
                            else
                            {
                                AMY_LOG_ERROR("avi_player_thread: read audio frame "
                                              "failed(size=%d)\n",
                                              ast->size);
                            }
                        }
                        else if (Amy_queues_put(ctx->aq, ast) != 0)
                        {
                            avi_player_aq_release_cb(ctx->mp, ast);
                            AMY_LOG_ERROR("avi_player_thread: put audio frame "
                                          "to queue failed\n");
                        }
                    }
                }
            }
        }

        Amy_msleep(10);
    }
    // =================== 主循环结束 ===================

thread_exit:
    // 停止音频输出线程并释放资源
    if (ctx->ao_run)
    {
        ctx->ao_run = 0;
        Amy_cond_broadcast(&ctx->cond); // 唤醒可能阻塞的音频线程
        Amy_pthread_join(ctx->ao_tid, NULL);
        if (ctx->ao_dev)
        {
            audio_output_close(ctx->ao_dev);
            ctx->ao_dev = NULL;
        }
    }

    if (ctx->aq)
    {
        Amy_queues_close(ctx->aq, avi_player_aq_release_cb, ctx->mp);
        ctx->aq = NULL;
    }

    // 停止视频解码线程并释放资源
    if (ctx->vdec_run)
    {
        ctx->vdec_run = 0;
        Amy_cond_broadcast(&ctx->cond); // 唤醒可能阻塞的解码线程
        Amy_pthread_join(ctx->vdec_tid, NULL);
    }

    if (ctx->vq)
    {
        Amy_queues_close(ctx->vq, avi_player_vq_release_cb, ctx->vdec_dev);
        ctx->vq = NULL;
    }

    if (ctx->vdec_dev)
    {
        video_decodec_close(ctx->vdec_dev);
        ctx->vdec_dev = NULL;
    }

    return NULL;
}

/* =========================== 对外接口实现 =========================== */

/**
 * @brief 打开AVI播放器并启动播放
 *
 * 动态分配播放器上下文，支持多实例并发（每次调用返回独立的上下文指针）。
 *
 * @param file AVI文件路径
 * @return 播放器上下文指针，失败返回 NULL
 */
avi_player_context *avi_player_open(const char *file)
{
    if (NULL == file || strlen(file) == 0)
    {
        AMY_LOG_ERROR("avi_player_open: invalid file path\n");
        return NULL;
    }

    // 动态分配上下文，支持多实例
    avi_player_context *ctx = (avi_player_context *)malloc(
        sizeof(avi_player_context));
    if (!ctx)
    {
        AMY_LOG_ERROR("avi_player_open: alloc context failed\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(avi_player_context));
    Amy_mutex_init(&ctx->mutex, NULL);
    Amy_cond_init(&ctx->cond, NULL);

    Amy_mutex_lock(&ctx->mutex);

    // 打开AVI文件
    ctx->avi_dev = AVI_open_input_file(file, 1);
    if (!ctx->avi_dev)
    {
        AMY_LOG_ERROR("avi_player_open: open AVI file [%s] failed\n", file);
        goto open_fail;
    }

    // 初始化内存池
    ctx->mp = Amy_mempool_init(512 * 1024, 256 * 1024);
    if (!ctx->mp)
    {
        AMY_LOG_ERROR("avi_player_open: init mempool failed\n");
        goto open_fail;
    }

    // 初始化视频参数
    ctx->v_rate = AVI_frame_rate(ctx->avi_dev);
    if (ctx->v_rate <= 0)
    {
        AMY_LOG_ERROR("avi_player_open: invalid video frame rate(%.2lf)\n",
                      ctx->v_rate);
        goto open_fail;
    }

    ctx->v_frame_pos = 0;
    ctx->a_clock = 0;
    ctx->v_clock = 0;
    ctx->v_frame_idx = 0;
    ctx->a_frame_idx = 0;
    ctx->v_width = AVI_video_width(ctx->avi_dev);
    ctx->v_height = AVI_video_height(ctx->avi_dev);
    ctx->v_frame_total = AVI_video_frames(ctx->avi_dev);
    ctx->frame_duration = (unsigned long)(1000.0 / ctx->v_rate);
    ctx->has_sps = 0;
    ctx->has_pps = 0;
    ctx->has_idr = 0;

    // 判断视频编码类型
    char *enc = AVI_video_compressor(ctx->avi_dev);
    ctx->v_enc_type = (enc && strcmp(enc, "MJPG") == 0)
                          ? MJPEG_ENC_TYPE
                          : H264_ENC_TYPE;

    // 初始化音频参数
    ctx->a_channels = AVI_audio_channels(ctx->avi_dev);
    ctx->a_samplerate = AVI_audio_rate(ctx->avi_dev);
    ctx->a_bits = AVI_audio_bits(ctx->avi_dev);
    ctx->has_audio = 0;

    /*
     * 仅头里声明音频格式不可靠：无音频的录像(如部分 CCTV 文件)也会带 auds 头，
     * 但索引中没有音频块(audio_chunks==0)，audio_index 为空，AVI_read_audio 恒返回 -1。
     * 若误判为有音频，音频时钟永远不前进，会让解码线程因音视频同步而卡死。
     * 因此必须同时确认容器内真实存在音频块。
     */
    if ((ctx->a_bits == 16) &&
        ((ctx->a_samplerate == 8000) || (ctx->a_samplerate == 16000)) &&
        (AVI_audio_chunks(ctx->avi_dev) > 0))
    {
        ctx->has_audio = 1;
        ctx->a_frame_len = ctx->a_channels * ctx->a_samplerate * (ctx->a_bits / 8) * (int)ctx->frame_duration / 1000;
        AMY_LOG_INFO("avi_player_open: audio ch=%d rate=%d bits=%d "
                     "frame_len=%d\n",
                     ctx->a_channels, ctx->a_samplerate,
                     ctx->a_bits, ctx->a_frame_len);
    }
    ctx->file_has_audio = ctx->has_audio;

    // 初始化播放器状态并启动主线程
    ctx->ao_run = 0;
    ctx->vdec_dev = NULL;
    ctx->ao_dev = NULL;
    ctx->status = VIDEO_PLAYER_STATUS_IDLE;
    ctx->run = 1;

    lv_video_mode_enable(1);
    Amy_video_mode_register_device(video_player_refresh_cb, ctx);
    Amy_pthread_create(&ctx->tid, avi_player_thread, ctx, 64 * 1024, 12);

    Amy_mutex_unlock(&ctx->mutex);
    return ctx;

open_fail:
    Amy_mutex_unlock(&ctx->mutex);
    if (ctx->mp)
    {
        Amy_mempool_destory(ctx->mp);
    }
    if (ctx->avi_dev)
    {
        AVI_close(ctx->avi_dev);
    }
    Amy_cond_destroy(&ctx->cond);
    Amy_mutex_destroy(&ctx->mutex);
    free(ctx);
    return NULL;
}

/**
 * @brief 播放/暂停状态统一切换接口
 *
 * 内部根据当前状态自动执行 resume / pause 逻辑，切换后广播条件变量
 * 唤醒处于等待中的子线程。
 *
 * @param ctx 播放器上下文
 * @return  0: 成功
 *         -1: 参数错误
 *         -2: 当前状态无法切换（如 IDLE 状态尚未完成首帧加载）
 */
int avi_player_toggle_play(avi_player_context *ctx)
{
    if (NULL == ctx)
    {
        AMY_LOG_ERROR("avi_player_toggle_play: ctx is NULL\n");
        return -1;
    }

    int ret = 0;
    Amy_mutex_lock(&ctx->mutex);
    switch (ctx->status)
    {
    case VIDEO_PLAYER_STATUS_PAUSE:
        ctx->status = VIDEO_PLAYER_STATUS_PLAY;
        Amy_timestamp(ctx->play_base_wall);
        ctx->play_base_vidx = ctx->v_frame_idx;
        Amy_play_event_put(AMY_PLAY_STATE_EVENT_ID_PLAY);
        Amy_cond_broadcast(&ctx->cond); // 唤醒阻塞中的子线程
        break;

    case VIDEO_PLAYER_STATUS_PLAY:
        ctx->status = VIDEO_PLAYER_STATUS_PAUSE;
        Amy_play_event_put(AMY_PLAY_STATE_EVENT_ID_PAUSE);
        // 子线程将在下一次轮询时进入条件变量等待，无需主动通知
        break;

    case VIDEO_PLAYER_STATUS_STOP:
        // 停止 → 重播：重置帧位置并恢复播放
        if (ctx->vq)
        {
            Amy_queues_flush(ctx->vq, avi_player_vq_release_cb, ctx->vdec_dev);
        }
        if (ctx->aq)
        {
            Amy_queues_flush(ctx->aq, avi_player_aq_release_cb, ctx->mp);
        }
        if (ctx->ao_dev)
        {
            /* 清空声卡侧残留缓冲，避免重播初期音频时钟/听感异常 */
            audio_output_flush(ctx->ao_dev);
        }
        ctx->v_frame_pos = 0;
        ctx->v_frame_idx = 0;
        ctx->a_frame_idx = 0;
        ctx->a_clock = 0;
        ctx->v_clock = 0;
        ctx->has_sps = 0;
        ctx->has_pps = 0;
        ctx->has_idr = 0;
        ctx->has_audio = ctx->file_has_audio;
        AVI_set_video_position(ctx->avi_dev, 0);
        AVI_set_audio_position(ctx->avi_dev, 0);
        ctx->status = VIDEO_PLAYER_STATUS_PLAY;
        Amy_timestamp(ctx->play_base_wall);
        ctx->play_base_vidx = ctx->v_frame_idx;
        Amy_play_event_put(AMY_PLAY_STATE_EVENT_ID_PLAY);
        Amy_cond_broadcast(&ctx->cond);
        break;

    case VIDEO_PLAYER_STATUS_INIT_LOADED:
        ctx->status = VIDEO_PLAYER_STATUS_PLAY;
        Amy_timestamp(ctx->play_base_wall);
        ctx->play_base_vidx = ctx->v_frame_idx;
        Amy_play_event_put(AMY_PLAY_STATE_EVENT_ID_PLAY);
        Amy_cond_broadcast(&ctx->cond);
        break;

    case VIDEO_PLAYER_STATUS_IDLE:
        // 首帧尚未加载完成，无法切换
        AMY_LOG_WARNING("avi_player_toggle_play: cannot toggle in IDLE status\n");
        ret = -2;
        break;

    default:
        AMY_LOG_ERROR("avi_player_toggle_play: unknown status(%u)\n", ctx->status);
        ret = -2;
        break;
    }

    Amy_mutex_unlock(&ctx->mutex);
    return ret;
}

/**
 * @brief 获取剩余播放时长
 * @param ctx             播放器上下文
 * @param remain_duration [out] 剩余播放时长(ms)
 * @return 0: 成功, -1: 参数错误
 */
int video_play_param_get(avi_player_context *ctx, int *remain_duration)
{
    if (NULL == ctx || NULL == remain_duration)
    {
        AMY_LOG_ERROR("video_play_param_get: invalid param\n");
        return -1;
    }

    Amy_mutex_lock(&ctx->mutex);
    *remain_duration = (int)(ctx->frame_duration *
                             (ctx->v_frame_total - ctx->v_frame_pos));
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

/**
 * @brief 关闭播放器并释放全部资源
 *
 * 停止主线程（主线程退出时会级联停止子线程），然后释放 AVI 设备、
 * 内存池、互斥锁和上下文内存。
 *
 * @param ctx 播放器上下文（调用成功后指针指向的内存已被释放，调用方应置 NULL）
 * @return 0: 成功, -1: 参数错误
 */
int avi_player_close(avi_player_context *ctx)
{
    if (NULL == ctx)
    {
        AMY_LOG_ERROR("avi_player_close: ctx is NULL\n");
        return -1;
    }

    Amy_mutex_lock(&ctx->mutex);
    if (!ctx->run)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return 0;
    }

    lv_video_mode_enable(0);

    // 通知主线程退出，并广播唤醒所有阻塞的子线程
    ctx->run = 0;
    Amy_cond_broadcast(&ctx->cond);
    Amy_mutex_unlock(&ctx->mutex);

    // 等待主线程退出（主线程内部负责清理子线程）
    Amy_pthread_join(ctx->tid, NULL);

    // 释放顶层资源
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->mp)
    {
        Amy_mempool_destory(ctx->mp);
        ctx->mp = NULL;
    }
    if (ctx->avi_dev)
    {
        AVI_close(ctx->avi_dev);
        ctx->avi_dev = NULL;
    }
    Amy_mutex_unlock(&ctx->mutex);

    Amy_cond_destroy(&ctx->cond);
    Amy_mutex_destroy(&ctx->mutex);
    free(ctx);

    return 0;
}