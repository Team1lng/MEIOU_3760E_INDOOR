#ifndef _AMY_MUSIC_PLAYER_H_
#define _AMY_MUSIC_PLAYER_H_

/* 播放控制 */
int Amy_music_player_play(int index);
int Amy_music_player_pause(void);
int Amy_music_player_resume(void);
int Amy_music_player_toggle(void);
int Amy_music_player_stop(void);
int Amy_music_player_is_playing(void);

/* 曲目导航 */
int Amy_music_player_next(void);
int Amy_music_player_prev(void);
int Amy_music_player_select(int index);

/* 当前曲目信息 */
int Amy_music_player_get_current_index(void);
const char *Amy_music_player_get_current_name(void);
float Amy_music_player_get_duration(void);
int Amy_music_player_get_duration_ms(void);
int Amy_music_player_get_progress(void);
int Amy_music_player_get_position(void);
int Amy_music_player_get_position_ms(void);

/* 跳转 */
int Amy_music_player_seek_percent(int percent);
int Amy_music_player_seek_ms(int ms);
int Amy_music_player_seek_sec(int sec);

/* 倍速播放 (0.5, 1.0, 2.0, 3.0) */
int Amy_music_player_set_speed(float speed);
float Amy_music_player_get_speed(void);

/* 音量 */
void Amy_music_player_set_volume(int volume);
int Amy_music_player_get_volume(void);

/* 回调 */
void Amy_music_player_set_finish_cb(void (*cb)(void));

/* 工具 */
int Amy_music_player_get_track_count(void);

#endif
