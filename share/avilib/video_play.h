#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_
typedef struct avi_player_context avi_player_context;


#define VIDEO_PLAYER_STATUS_IDLE        0X00    // 初始空闲：未查找/入队关键帧
#define VIDEO_PLAYER_STATUS_INIT_LOADED 0X01    // 中间状态：初始状态下关键帧已入队，未完成解码出队（禁止新帧入队，允许解码出队）
#define VIDEO_PLAYER_STATUS_PAUSE       0X02    // 暂停状态：关键帧已解码显示，静态预览
#define VIDEO_PLAYER_STATUS_PLAY        0X04    // 播放状态：正常读取并解码后续帧
#define VIDEO_PLAYER_STATUS_STOP        0X05    // 停止状态：播放结束/主动停止
avi_player_context* avi_player_open(const char* file);

int avi_player_close(avi_player_context* ctx);

int avi_player_toggle_play(avi_player_context *ctx);

int video_play_param_get(avi_player_context *ctx, int *curnt_duration);
#endif
