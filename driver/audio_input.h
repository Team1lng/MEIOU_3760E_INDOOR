#ifndef _AMY_AUDIO_INPUT_H_
#define _AMY_AUDIO_INPUT_H_
#include "Amy_sdk.h"
typedef struct audio_input_context audio_input_context;

/* AEC 工作模式 */
#define AEC_MODE_SPEEX     0 /* 外置 speex AEC，AI 即用即关 */
#define AEC_MODE_ANYCLOUD  1 /* 安凯内置 AEC，AI 永不关闭 */

/* 运行时切换 AEC/AI 工作模式（仅影响后续 open/close，不打断当前会话） */
int audio_input_aec_mode_set(int mode);
int audio_input_aec_mode_get(void);

audio_input_context *audio_input_open(int channel, int sample, int volume);

stream_data *audio_input_buffer_get(audio_input_context *ctx, int size);

int audio_input_frame_get(audio_input_context *ctx, stream_data *ast);

int audio_input_frame_release(audio_input_context *ctx, stream_data *frame);

int audio_input_close(audio_input_context *ctx);

int audio_input_set_volume(audio_input_context *ctx, int volume);

int audio_input_query(audio_input_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total);

audio_input_context *audio_input_param_get(int *chn, int *smaple, int *vol);

int audio_input_frame_get_live(audio_input_context *ctx, stream_data *ast);

int audio_input_query_live(audio_input_context *ctx, unsigned int *free, unsigned int *used, unsigned int *total);
#endif