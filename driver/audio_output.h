#ifndef _AMY_AUDIO_OUTPUT_H_
#define _AMY_AUDIO_OUTPUT_H_
#define AUDIO_OUTPUT_MIN_SIZE 640
typedef struct audio_output_context audio_output_context;

/* 音频通道角色：用于区分参与混音的不同逻辑通道 */
typedef enum
{
    AUDIO_OUTPUT_ROLE_NORMAL = 0, /* 普通播放通道（音乐/CCTV），作为混音基准采样率 */
    AUDIO_OUTPUT_ROLE_RING = 1,   /* 铃声通道，采样率不一致时被重采样对齐主通道 */
    AUDIO_OUTPUT_ROLE_MAX,        /* 通道角色总数（内部使用） */
} audio_output_role_t;

/* ---- 普通播放通道（原有接口，语义不变） ---- */
audio_output_context *audio_output_open(int channel, int sample, int volume, int buffer);
audio_output_context *audio_output_get_handle(void);

/* ---- 铃声通道（新增） ---- */
audio_output_context *audio_output_open_ring(int channel, int sample, int volume, int buffer);
audio_output_context *audio_output_get_ring_handle(void);

/* ---- 设备常开（不占用任何通道的引用计数/活跃标志） ---- */
int audio_output_keep_open(int channel, int sample);

/* ---- 通用接口（ctx 携带角色，内部自动路由到对应通道） ---- */
int audio_output_write(audio_output_context *ctx, char *data, int size);
int audio_output_close(audio_output_context *ctx);
int audio_output_set_volume(audio_output_context *ctx, int volume);
int audio_output_reset_params(audio_output_context *ctx, int channel, int sample);
int audio_output_wait_buffer_empty(audio_output_context *ctx);
int audio_output_flush(audio_output_context *ctx);
int audio_output_clear(audio_output_context *ctx);
int audio_output_query(audio_output_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total);

int audio_output_param_get(int *chn, int *smaple, int *vol);

#endif
