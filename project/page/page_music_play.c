#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_tfcard.h"
#include "Amy_file_manager.h"
#include "Amy_music_player.h"
#include "Amy_sdk.h"
#include "user_standby.h"
#include "user_data.h"

/************************** 布局常量 (1024x600) **************************/
#define BG_COLOR 0x222431

/* 标题 */
#define TITLE_Y 26

/* 黑胶唱片 */
#define VINYL_X 362
#define VINYL_Y 138
#define VINYL_W ICON_MEDIA_ALBUM_ART_PNG_WIDTH
#define VINYL_H ICON_MEDIA_ALBUM_ART_PNG_HEIGHT
#define VINYL_SIZE 301

/* 进度条 */
#define CUR_TIME_X 19
#define CUR_TIME_Y 537
#define CUR_TIME_W 60
#define CUR_TIME_H 28

#define PROG_BAR_X 85
#define PROG_BAR_Y 541
#define PROG_BAR_W 300
#define PROG_BAR_H 20

#define TOTAL_TIME_X 401
#define TOTAL_TIME_Y 537
#define TOTAL_TIME_W 100
#define TOTAL_TIME_H 28

/* 控制按钮 */
#define CTRL_BTN_Y 533
#define CTRL_BTN_W 35
#define CTRL_BTN_H 35
#define PREV_BTN_X 516
#define PLAY_BTN_X 586
#define NEXT_BTN_X 656

/* 音量 */
#define VOL_MINUS_X 735
#define VOL_ICON_X 779
#define VOL_ICON_Y 537
#define VOL_ICON_W 28
#define VOL_ICON_H 28
#define VOL_LABEL_X 810
#define VOL_LABEL_W 30
#define VOL_PLUS_X 848

/* 返回按钮 */
#define BACK_X 929
#define BACK_Y 505
#define BACK_W 67
#define BACK_H 67

#define PROGRESS_MAX 100
#define TIMER_PERIOD 500

/************************** 循环模式 **************************/
#define LOOP_OFF 0
#define LOOP_LIST 1
#define LOOP_SINGLE 2

/************************** 播放状态 **************************/
static struct
{
    int loop_mode;

    lv_obj_t *vinyl_cont;
    lv_anim_t vinyl_anim;
    bool vinyl_anim_running;

    lv_obj_t *play_btn;
    firamware_resource play_res;
    firamware_resource pause_res;
    firamware_resource play_focus_res;
    firamware_resource pause_focus_res;

    lv_obj_t *progress_slider;
    lv_obj_t *cur_time_label;
    lv_obj_t *total_time_label;
    lv_obj_t *vol_label;
    lv_obj_t *title_label;

    lv_timer_t *progress_timer;
    bool progress_dragging;
} player_ctx = {
    .loop_mode = LOOP_OFF,
};

/************************** 函数声明 **************************/
static void music_play_back_obj_click(lv_event_t *ev);
static void music_play_prev_obj_click(lv_event_t *ev);
static void music_play_next_obj_click(lv_event_t *ev);
static void music_play_play_obj_click(lv_event_t *ev);
static void music_play_progress_obj_change(lv_event_t *ev);
static void music_play_vol_minus_obj_click(lv_event_t *ev);
static void music_play_vol_plus_obj_click(lv_event_t *ev);
static void music_play_vol_icon_obj_click(lv_event_t *ev);

static void music_play_title_create(void);
static void music_play_vinyl_create(void);
static void music_play_progress_create(void);
static void music_play_prev_create(void);
static void music_play_play_create(void);
static void music_play_next_create(void);
static void music_play_volume_create(void);
static void music_play_back_create(void);

/************************** 辅助函数 **************************/
static void format_time(int seconds, char *buf, int buf_size)
{
    int m = seconds / 60;
    int s = seconds % 60;
    snprintf(buf, buf_size, "%02d:%02d", m, s);
}

static void format_duration(float dur, char *buf, int buf_size)
{
    int total_sec = (int)(dur + 0.5f);
    format_time(total_sec, buf, buf_size);
}

static void update_time_labels(void)
{
    if (!player_ctx.cur_time_label || !player_ctx.total_time_label)
        return;

    int pos = Amy_music_player_get_position();
    char buf[16];
    format_time(pos, buf, sizeof(buf));
    lv_label_set_text(player_ctx.cur_time_label, buf);

    float dur = Amy_music_player_get_duration();
    format_duration(dur, buf, sizeof(buf));
    lv_label_set_text(player_ctx.total_time_label, buf);
}

static void update_progress(void)
{
    if (!player_ctx.progress_slider || player_ctx.progress_dragging)
        return;

    int val = Amy_music_player_get_progress();
    lv_bar_set_value(player_ctx.progress_slider, val, LV_ANIM_OFF);
}

static void update_vol_label(void)
{
    if (!player_ctx.vol_label)
        return;
    char buf[8];
    /* UI 显示 1-10，实际设置值 = 1-100 */
    snprintf(buf, sizeof(buf), "%d", user_data_get()->private_cfg.scene.bg_music_vol / 10);
    lv_label_set_text(player_ctx.vol_label, buf);
}

static void update_play_icon(void)
{
    if (!player_ctx.play_btn)
        return;
    void *src = Amy_music_player_is_playing()
                    ? (void *)&player_ctx.pause_res
                    : (void *)&player_ctx.play_res;
    void *focus = Amy_music_player_is_playing()
                      ? (void *)&player_ctx.pause_focus_res
                      : (void *)&player_ctx.play_focus_res;
    lv_obj_set_style_bg_img_src(player_ctx.play_btn, src, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(player_ctx.play_btn, focus, LV_STATE_PRESSED);
}

static void load_track_metadata(void)
{
    const char *name = Amy_music_player_get_current_name();
    if (player_ctx.title_label)
        lv_label_set_text(player_ctx.title_label, name ? name : "");

    update_time_labels();
    update_progress();
}

/************************** 黑胶旋转动画 **************************/
static void vinyl_rotate_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_angle((lv_obj_t *)var, v, LV_PART_MAIN);
}

static void vinyl_rotate_start(void)
{
    if (!player_ctx.vinyl_cont)
        return;
    lv_anim_del(player_ctx.vinyl_cont, vinyl_rotate_cb);
    int32_t current = lv_obj_get_style_transform_angle(player_ctx.vinyl_cont, LV_PART_MAIN);
    lv_anim_init(&player_ctx.vinyl_anim);
    lv_anim_set_var(&player_ctx.vinyl_anim, player_ctx.vinyl_cont);
    lv_anim_set_exec_cb(&player_ctx.vinyl_anim, vinyl_rotate_cb);
    lv_anim_set_values(&player_ctx.vinyl_anim, current, current + 3600);
    lv_anim_set_time(&player_ctx.vinyl_anim, 2000);
    lv_anim_set_repeat_count(&player_ctx.vinyl_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&player_ctx.vinyl_anim, lv_anim_path_linear);
    lv_anim_start(&player_ctx.vinyl_anim);
    player_ctx.vinyl_anim_running = true;
}

static void vinyl_rotate_stop(void)
{
    if (!player_ctx.vinyl_cont)
        return;
    lv_anim_del(player_ctx.vinyl_cont, vinyl_rotate_cb);
    player_ctx.vinyl_anim_running = false;
}

/************************** 播放结束回调 **************************/

/* 播放线程调用：仅入队事件，UI 在 LVGL 线程处理 */
static void on_track_finished(void)
{
    Amy_music_finish_event_put();
}

/* LVGL 线程事件回调：自动切下一首 */
static void music_play_finish_event_cb(void *arg)
{
    (void)arg;

    int total = Amy_music_player_get_track_count();
    if (total <= 0)
    {
        vinyl_rotate_stop();
        update_play_icon();
        return;
    }

    int next = Amy_music_player_get_current_index() + 1;
    if (next >= total)
        next = 0;

    Amy_music_player_play(next);
    load_track_metadata();
    vinyl_rotate_start();
    update_play_icon();
}

/************************** 播放控制 **************************/

static void stop_playback(void)
{
    Amy_music_player_set_finish_cb(NULL);
    Amy_music_player_stop();
    vinyl_rotate_stop();
}

/************************** 定时器回调 **************************/
static void progress_timer_cb(lv_timer_t *t)
{
    if (!Amy_music_player_is_playing())
        return;

    update_time_labels();
    update_progress();
}

/************************** 回调实现 **************************/

static void music_play_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(music), LV_SCR_LOAD_ANIM_NONE, );
}

static void music_play_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
    page_common_tfcard_change_cb(arg);
}

static void music_play_prev_obj_click(lv_event_t *ev)
{
    Amy_music_player_prev();
    load_track_metadata();
    if (Amy_music_player_is_playing())
        vinyl_rotate_start();
    update_play_icon();
}

static void music_play_next_obj_click(lv_event_t *ev)
{
    Amy_music_player_next();
    load_track_metadata();
    if (Amy_music_player_is_playing())
        vinyl_rotate_start();
    update_play_icon();
}

static void music_play_play_obj_click(lv_event_t *ev)
{
    int was_playing = Amy_music_player_is_playing();

    if (!was_playing)
    {
        Amy_music_player_set_finish_cb(on_track_finished);
        Amy_music_player_set_volume(user_data_get()->private_cfg.scene.bg_music_vol);
    }

    Amy_music_player_toggle();

    if (Amy_music_player_is_playing())
    {
        standby_timer_close();
        vinyl_rotate_start();
    }
    else
    {
        standby_timer_restart(true, true);
        vinyl_rotate_stop();
    }
    update_play_icon();
}

static void music_play_progress_obj_change(lv_event_t *ev)
{
    lv_event_code_t code = lv_event_get_code(ev);
    lv_obj_t *slider = lv_event_get_target(ev);

    if (code == LV_EVENT_PRESSING || code == LV_EVENT_PRESSED)
        player_ctx.progress_dragging = true;
    else if (code == LV_EVENT_RELEASED)
    {
        player_ctx.progress_dragging = false;
        int val = lv_bar_get_value(slider);
        Amy_music_player_seek_percent(val);
    }
}

static void music_play_vol_minus_obj_click(lv_event_t *ev)
{
    int vol = user_data_get()->private_cfg.scene.bg_music_vol;
    if (vol > 10)
    {
        vol -= 10;
        user_data_get()->private_cfg.scene.bg_music_vol = vol;
        Amy_music_player_set_volume(vol);
        update_vol_label();
    }
}

static void music_play_vol_plus_obj_click(lv_event_t *ev)
{
    int vol = user_data_get()->private_cfg.scene.bg_music_vol;
    if (vol < 100)
    {
        vol += 10;
        user_data_get()->private_cfg.scene.bg_music_vol = vol;
        Amy_music_player_set_volume(vol);
        update_vol_label();
    }
}

static void music_play_vol_icon_obj_click(lv_event_t *ev)
{
    int vol = user_data_get()->private_cfg.scene.bg_music_vol;
    if (vol >= 100)
        vol = 10;
    else
        vol += 10;
    user_data_get()->private_cfg.scene.bg_music_vol = vol;
    Amy_music_player_set_volume(vol);
    update_vol_label();
}

/************************** 创建函数 **************************/

static void music_play_title_create(void)
{
    const char *name = Amy_music_player_get_current_name();
    player_ctx.title_label = lv_common_text_create(lv_scr_act_current(), MUSIC_PLAY_TITLE_ID,
                                                   0, TITLE_Y, SCREEN_W, 28,
                                                   NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   name ? name : "", 0xffffff, 0x02A9FF,
                                                   LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void music_play_vinyl_create(void)
{
    lv_resource_init(album_icon, ICON_MEDIA_ALBUM_ART_PNG_OFFSET, ICON_MEDIA_ALBUM_ART_PNG_SIZE);
    player_ctx.vinyl_cont = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_VINYL_CONT_ID,
                                                     VINYL_X, VINYL_Y, VINYL_SIZE, VINYL_SIZE,
                                                     NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     &album_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    lv_obj_set_style_radius(player_ctx.vinyl_cont, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(player_ctx.vinyl_cont, true, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(player_ctx.vinyl_cont, VINYL_SIZE / 2, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(player_ctx.vinyl_cont, VINYL_SIZE / 2, LV_PART_MAIN);
}

static void music_play_progress_create(void)
{
    char cur_buf[16], total_buf[16];

    format_time(0, cur_buf, sizeof(cur_buf));
    player_ctx.cur_time_label = lv_common_text_create(lv_scr_act_current(), MUSIC_PLAY_CUR_TIME_ID,
                                                      CUR_TIME_X, CUR_TIME_Y, CUR_TIME_W, CUR_TIME_H,
                                                      NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      cur_buf, 0xffffff, 0x02A9FF,
                                                      LV_TEXT_ALIGN_RIGHT, lv_font_normal);

    format_duration(0.0f, total_buf, sizeof(total_buf));
    player_ctx.total_time_label = lv_common_text_create(lv_scr_act_current(), MUSIC_PLAY_TOTAL_TIME_ID,
                                                        TOTAL_TIME_X, TOTAL_TIME_Y, TOTAL_TIME_W, TOTAL_TIME_H,
                                                        NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                        total_buf, 0xffffff, 0x02A9FF,
                                                        LV_TEXT_ALIGN_LEFT, lv_font_normal);

    lv_obj_t *prog_cont = lv_common_slider_create(lv_scr_act_current(), MUSIC_PLAY_PROGRESS_CONT_ID,
                                                  PROG_BAR_X, PROG_BAR_Y, PROG_BAR_W, PROG_BAR_H,
                                                  NULL,
                                                  LV_OPA_TRANSP, 0x00,
                                                  10, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  10, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  -1, 0, 0, 0, -1,
                                                  NULL, 0, LV_TEXT_ALIGN_CENTER, NULL,
                                                  0, 0, PROG_BAR_W, PROG_BAR_H,
                                                  MUSIC_PLAY_PROGRESS_SLIDER_ID,
                                                  LV_OPA_COVER, 0xC4C4C4, LV_OPA_COVER, 0x0CBC48,
                                                  music_play_progress_obj_change,
                                                  -1, 0, 0, 0, -1, NULL, 0, 0, LV_ALIGN_CENTER, NULL,
                                                  -1, 0, 0, 0, -1, NULL, 0, 0, LV_ALIGN_CENTER, NULL,
                                                  0, 0, 0x0CBC48, LV_OPA_TRANSP, NULL,
                                                  0, PROGRESS_MAX, 0);
    player_ctx.progress_slider = lv_obj_get_child_by_id(prog_cont, MUSIC_PLAY_PROGRESS_SLIDER_ID);
    /* 进度条只做显示，不支持拖动/点击跳转 */
    lv_obj_clear_flag(player_ctx.progress_slider, LV_OBJ_FLAG_CLICKABLE);
}

static void music_play_prev_create(void)
{
    lv_resource_init(prev_icon, ICON_MEDIA_PREV_PNG_OFFSET, ICON_MEDIA_PREV_PNG_SIZE);
    lv_resource_init(prev_focus, ICON_MEDIA_PREV_FOCUS_PNG_OFFSET, ICON_MEDIA_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *prev_btn = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_PREV_BTN_ID,
                                                  PREV_BTN_X, CTRL_BTN_Y, CTRL_BTN_W, CTRL_BTN_H,
                                                  music_play_prev_obj_click, true,
                                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &prev_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &prev_focus);
    lv_obj_set_ext_click_area(prev_btn, 21);
}

static void music_play_play_create(void)
{
    lv_resource_init(play_icon, ICON_MEDIA_PLAY_PNG_OFFSET, ICON_MEDIA_PLAY_PNG_SIZE);
    lv_resource_init(play_focus, ICON_MEDIA_PLAY_FOCUS_PNG_OFFSET, ICON_MEDIA_PLAY_FOCUS_PNG_SIZE);
    lv_resource_init(pause_icon, ICON_MEDIA_PAUSE_PNG_OFFSET, ICON_MEDIA_PAUSE_PNG_SIZE);
    lv_resource_init(pause_focus, ICON_MEDIA_PAUSE_FOCUS_PNG_OFFSET, ICON_MEDIA_PAUSE_FOCUS_PNG_SIZE);
    memcpy(&player_ctx.play_res, &play_icon, sizeof(firamware_resource));
    memcpy(&player_ctx.pause_res, &pause_icon, sizeof(firamware_resource));
    memcpy(&player_ctx.play_focus_res, &play_focus, sizeof(firamware_resource));
    memcpy(&player_ctx.pause_focus_res, &pause_focus, sizeof(firamware_resource));

    player_ctx.play_btn = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_PLAY_BTN_ID,
                                                   PLAY_BTN_X, CTRL_BTN_Y, CTRL_BTN_W, CTRL_BTN_H,
                                                   music_play_play_obj_click, true,
                                                   LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &player_ctx.play_res, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &pause_focus);
    lv_obj_set_ext_click_area(player_ctx.play_btn, 21);
}

static void music_play_next_create(void)
{
    lv_resource_init(next_icon, ICON_MEDIA_NEXT_PNG_OFFSET, ICON_MEDIA_NEXT_PNG_SIZE);
    lv_resource_init(next_focus, ICON_MEDIA_NEXT_FOCUS_PNG_OFFSET, ICON_MEDIA_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *next_btn = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_NEXT_BTN_ID,
                                                  NEXT_BTN_X, CTRL_BTN_Y, CTRL_BTN_W, CTRL_BTN_H,
                                                  music_play_next_obj_click, true,
                                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &next_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &next_focus);
    lv_obj_set_ext_click_area(next_btn, 21);
}

static void music_play_volume_create(void)
{
    lv_resource_init(del_icon, ICON_MEDIA_DEL_PNG_OFFSET, ICON_MEDIA_DEL_PNG_SIZE);
    lv_resource_init(del_icon_focus, ICON_MEDIA_DEL_FOCUS_PNG_OFFSET, ICON_MEDIA_DEL_FOCUS_PNG_SIZE);
    lv_obj_t *minus = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_VOL_MINUS_BTN_ID,
                                               VOL_MINUS_X, CTRL_BTN_Y, CTRL_BTN_W, CTRL_BTN_H,
                                               music_play_vol_minus_obj_click, true,
                                               LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &del_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &del_icon_focus);
    lv_obj_set_ext_click_area(minus, 21);

    lv_resource_init(speaker_icon, ICON_MEDIA_VOLUME_PNG_OFFSET, ICON_MEDIA_VOLUME_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_VOL_CONT_ID,
                             VOL_ICON_X, VOL_ICON_Y, VOL_ICON_W, VOL_ICON_H,
                             music_play_vol_icon_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &speaker_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);

    player_ctx.vol_label = lv_common_text_create(lv_scr_act_current(), MUSIC_PLAY_VOL_LABEL_ID,
                                                 VOL_LABEL_X, CTRL_BTN_Y, VOL_LABEL_W, CTRL_BTN_H,
                                                 NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 NULL, 0xffffff, 0x02A9FF,
                                                 LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
    update_vol_label();

    lv_resource_init(add_icon, ICON_MEDIA_ADD_PNG_OFFSET, ICON_MEDIA_ADD_PNG_SIZE);
    lv_resource_init(add_icon_focus, ICON_MEDIA_ADD_FOCUS_PNG_OFFSET, ICON_MEDIA_ADD_FOCUS_PNG_SIZE);
    lv_obj_t *plus = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_VOL_PLUS_BTN_ID,
                                              VOL_PLUS_X, CTRL_BTN_Y, CTRL_BTN_W, CTRL_BTN_H,
                                              music_play_vol_plus_obj_click, true,
                                              LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &add_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &add_icon_focus);
    lv_obj_set_ext_click_area(plus, 21);
}

static void music_play_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *music_play_back = lv_common_img_btn_create(lv_scr_act_current(), MUSIC_PLAY_BACK_BTN_ID,
                                                         BACK_X, BACK_Y, BACK_W, BACK_H,
                                                         music_play_back_obj_click, true,
                                                         LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(music_play_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(music_play)
{
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, BG_COLOR);

    player_ctx.progress_dragging = false;
    player_ctx.vinyl_anim_running = false;
    Amy_music_player_set_finish_cb(on_track_finished);
    if (Amy_music_player_is_playing())
        standby_timer_close();
    music_play_title_create();
    music_play_vinyl_create();
    music_play_progress_create();
    music_play_prev_create();
    music_play_play_create();
    music_play_next_create();
    music_play_volume_create();
    music_play_back_create();

    load_track_metadata();

    if (Amy_music_player_is_playing())
    {
        update_play_icon();
        vinyl_rotate_start();
    }

    player_ctx.progress_timer = lv_timer_create(progress_timer_cb, TIMER_PERIOD, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, music_play_sd_change_cb);
    Amy_event_cb_register(AMY_EVENT_ID_MUSIC_FINISH, music_play_finish_event_cb);
}

static void Amy_lv_page_quit(music_play)
{
    user_data_save();
    stop_playback();

    if (player_ctx.progress_timer)
    {
        lv_timer_del(player_ctx.progress_timer);
        player_ctx.progress_timer = NULL;
    }

    standby_timer_restart(true, true);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_MUSIC_FINISH, NULL);
}

Amy_lv_page_create(music_play);
