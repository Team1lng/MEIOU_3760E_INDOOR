#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "Amy_tfcard.h"
#include "Amy_file_manager.h"
#include "Amy_audio_play.h"
#include "Amy_music_player.h"
#include <stdio.h>
#include <stdlib.h>

/************************** 布局常量 **************************/
#define RING_ICON_X 62
#define RING1_ICON_Y 151
#define RING2_ICON_Y 310
#define RING3_ICON_Y 469
#define RING_ICON_W 66
#define RING_ICON_H 66
#define RING1_LABEL_Y 225
#define RING2_LABEL_Y 384
#define RING3_LABEL_Y 543
#define RING_SEL_COLOR 0x02A9FF

#define ROW_X 199
#define ROW_W 700
#define ROW_H 52
#define ROW1_Y 75
#define ROW_SPACING 52
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 250
#define ARROW_L_X 478
#define ARROW_R_X 869
#define ARROW_W 20
#define ARROW_H 30
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 50

/************************** 行索引 **************************/
typedef enum
{
    RING_ROW_SCHEDULE = 0,
    RING_ROW_TIME,
    RING_ROW_MODE,
    RING_ROW_SELECT,
    RING_ROW_VOLUME,
    RING_ROW_NUM
} ring_row_t;

/************************** 页面上下文 **************************/
static struct
{
    int door;
    int ring_index;
    int schedule_start[3];
    int schedule_end[3];
    int ring_time[3];
    int ring_mode[3];
    int ring_type[3];
    int ring_select[3];
    int ring_volume[3];
} s_ctx = {
    .schedule_start = {600, 1200, 1800},
    .schedule_end = {1200, 1800, 600},
    .ring_time = {RING_DURATION_30S, RING_DURATION_30S, RING_DURATION_30S},
    .ring_type = {1, 1, 1},
    .ring_select = {1, 1, 1},
    .ring_volume = {3, 3, 3},
};

/************************** 函数声明 **************************/
static void ring_sel_ring1_click(lv_event_t *ev);
static void ring_sel_ring2_click(lv_event_t *ev);
static void ring_sel_ring3_click(lv_event_t *ev);
static void ring_back_click(lv_event_t *ev);

static void ring_schedule_click(lv_event_t *ev);
static void ring_time_left_cb(lv_event_t *ev);
static void ring_time_right_cb(lv_event_t *ev);
static void ring_mode_left_cb(lv_event_t *ev);
static void ring_mode_right_cb(lv_event_t *ev);
static void ring_select_left_cb(lv_event_t *ev);
static void ring_select_right_cb(lv_event_t *ev);
static void ring_volume_left_cb(lv_event_t *ev);
static void ring_volume_right_cb(lv_event_t *ev);

static void ring_schedule_popup_create(void);
static void ring_schedule_popup_ok_cb(lv_event_t *ev);
static void ring_schedule_popup_cancel_cb(lv_event_t *ev);
static void ring_schedule_popup_roller_cb(lv_event_t *ev);

static int ring_row_y(ring_row_t row);
static void ring_row_value_update(ring_row_t row, const char *text);
static void ring_select(int ring_idx);
static void ring_ui_refresh(void);
static void ring_preview_play(void);

static void ring_sidebar_create(void);
static void ring_rows_create(void);
static void ring_back_create(void);

/************************** 辅助函数 **************************/

static int ring_row_y(ring_row_t row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static void ring_row_value_update(ring_row_t row, const char *text)
{
    lv_obj_t *label = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), RING_ROW1_CONT_ID + row * 3), RING_ROW1_LABEL2_ID + row * 3);
    if (label && text)
        lv_label_set_text(label, text);
}

/* 自定义铃声是否可用：SD 已挂载且 /mnt/tf/music 下存在 .mp3（与数码相框共用该目录） */
static bool ring_custom_available(bool need_scan)
{
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
        return false;
    Amy_fm_refresh(FM_DIR_MUSIC);
    return Amy_fm_get_count(FM_DIR_MUSIC) > 0;
}

/* 自定义铃声不可用时，把三个 Ring 的 Customized 全部回退为 Standard（同步写回 user_data） */
static bool ring_mode_sanitize(bool need_scan)
{
    if (ring_custom_available(need_scan))
        return false;

    bool changed = false;
    for (int i = 0; i < 3; i++)
    {
        if (s_ctx.ring_mode[i])
        {
            s_ctx.ring_mode[i] = 0;
            user_data_get()->public_cfg.door_device[s_ctx.door].door_cfg.ringtone[i].mode = 0;
            changed = true;
        }
    }
    return changed;
}

/* 把当前 Ring 的 duration/volume/mode/type/select 写回 user_data 并持久化 */
static void ring_persist_current(void)
{
    ringtone_config_type_def *rt = &user_data_get()->public_cfg.door_device[s_ctx.door].door_cfg.ringtone[s_ctx.ring_index];
    rt->duration = s_ctx.ring_time[s_ctx.ring_index];
    rt->volume = s_ctx.ring_volume[s_ctx.ring_index];
    rt->mode = s_ctx.ring_mode[s_ctx.ring_index];
    rt->type = s_ctx.ring_type[s_ctx.ring_index];
    rt->select = s_ctx.ring_select[s_ctx.ring_index];
    user_data_save();
}

/* SD 卡插拔全局处理：无自定义铃声时回退为标准铃声。
 * 挂在 page_common_tfcard_change_cb 上，任何页面拔卡都会生效；仅操作内存态，无需界面存在 */
void ring_mode_on_sd_changed(void)
{
    if (ring_mode_sanitize(false))
        user_data_save();
}

/* SD 卡插拔：先回退不可用的自定义铃声，再走通用弹窗逻辑 */
static void ring_sd_change_cb(void *arg)
{
    if (ring_mode_sanitize(true))
    {
        ring_ui_refresh();
        user_data_save();
    }
    page_common_tfcard_change_cb(arg);
}

static const char *ring_schedule_str(void)
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d - %02d:%02d",
             s_ctx.schedule_start[s_ctx.ring_index] / 100, s_ctx.schedule_start[s_ctx.ring_index] % 100,
             s_ctx.schedule_end[s_ctx.ring_index] / 100, s_ctx.schedule_end[s_ctx.ring_index] % 100);
    return buf;
}

static const char *ring_time_str(void)
{
    static char buf[16];
    static const int duration_sec[3] = {20, 30, 60};
    snprintf(buf, sizeof(buf), "%d S", duration_sec[s_ctx.ring_time[s_ctx.ring_index]]);
    return buf;
}

static const char *ring_mode_str(void)
{
    return s_ctx.ring_mode[s_ctx.ring_index] ? "Customized" : "Standard";
}

/* 取 select(1 基准) 对应音乐的真实文件名；数量不足返回空串。超长截断，避免溢出右侧 */
static const char *ring_music_name(int sel)
{
    static char buf[48];
    buf[0] = '\0';

    int count = Amy_fm_get_count(FM_DIR_MUSIC);
    if (count <= 0 || sel < 1 || sel > count)
        return buf;

    fm_file_info_t info;
    if (Amy_fm_get_file(FM_DIR_MUSIC, sel - 1, &info) != 0)
        return buf;

    strncpy(buf, info.name, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    int len = (int)strlen(buf);
    if (len > 24)
    {
        int cut = 24;
        while (cut > 0 && (buf[cut] & 0xC0) == 0x80) /* 不在多字节 UTF-8 字符中间截断 */
            cut--;
        buf[cut] = '\0';
        strcat(buf, "...");
    }
    return buf;
}

static const char *ring_select_str(void)
{
    static char buf[32];
    if (s_ctx.ring_mode[s_ctx.ring_index])
    {
        return ring_music_name(s_ctx.ring_select[s_ctx.ring_index]);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d", s_ctx.ring_type[s_ctx.ring_index]);
    }
    return buf;
}

static const char *ring_volume_str(void)
{
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", s_ctx.ring_volume[s_ctx.ring_index]);
    return buf;
}

static const char *ring_title_str(void)
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "Ring %d / Door %d",
             s_ctx.ring_index + 1, g_ring_door + 1);
    return buf;
}

static void ring_separator_create(ring_row_t row)
{
    int y = ring_row_y(row);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

/************************** Row 创建辅助 **************************/

static lv_obj_t *ring_row_btn_create(ring_row_t row, const char *label_text,
                                     const char *value_text, lv_event_cb_t cb)
{
    int y = ring_row_y(row);
    int cont_id = RING_ROW1_CONT_ID + row * 3;
    int label1_id = RING_ROW1_LABEL1_ID + row * 3;
    int label2_id = RING_ROW1_LABEL2_ID + row * 3;

    lv_obj_t *cont = lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        label_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 380, ROW_H,
        value_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    ring_separator_create(row);
    return cont;
}

static void ring_arrow_left_create(ring_row_t row, lv_event_cb_t cb)
{
    int y = ring_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_L_X, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), RING_ROW1_CONT_ID + row * 3), RING_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), RING_ROW1_CONT_ID + row * 3), RING_ROW1_LABEL2_ID + row * 3));
}

static void ring_arrow_right_create(ring_row_t row, lv_event_cb_t cb)
{
    int y = ring_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), RING_ROW1_CONT_ID + row * 3), RING_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), RING_ROW1_CONT_ID + row * 3), RING_ROW1_LABEL2_ID + row * 3));
}

/************************** 回调实现 **************************/

static void ring_sel_ring1_click(lv_event_t *ev)
{
    ring_select(0);
}

static void ring_sel_ring2_click(lv_event_t *ev)
{
    ring_select(1);
}

static void ring_sel_ring3_click(lv_event_t *ev)
{
    ring_select(2);
}

static void ring_select(int ring_idx)
{
    if (ring_idx == s_ctx.ring_index)
        return;

    /* 切换 Ring 时停掉上一个 Ring 正在播放的预览（内置铃声或自定义音乐） */
    Amy_audio_play_stop();
    Amy_music_player_stop();

    s_ctx.ring_index = ring_idx;

    lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), RING_SEL_RING1_LABEL_ID), lv_color_hex(ring_idx == 0 ? RING_SEL_COLOR : 0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), RING_SEL_RING2_LABEL_ID), lv_color_hex(ring_idx == 1 ? RING_SEL_COLOR : 0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), RING_SEL_RING3_LABEL_ID), lv_color_hex(ring_idx == 2 ? RING_SEL_COLOR : 0xffffff), LV_PART_MAIN);
    lv_label_set_text(lv_obj_get_child_by_id(lv_scr_act_current(), RING_PANEL_TITLE_ID), ring_title_str());
    ring_ui_refresh();
}

static void ring_ui_refresh(void)
{
    ring_row_value_update(RING_ROW_SCHEDULE, ring_schedule_str());
    ring_row_value_update(RING_ROW_TIME, ring_time_str());
    ring_row_value_update(RING_ROW_MODE, ring_mode_str());
    ring_row_value_update(RING_ROW_SELECT, ring_select_str());
    ring_row_value_update(RING_ROW_VOLUME, ring_volume_str());
}

/* 铃声预览：按当前 Ring 的 mode/select(或 type)/volume 播报一次，播完自然停止。
 * 音量 0 视为静音，停止并返回；音量/音量映射与 page_live 门铃播放保持一致 */
static void ring_preview_play(void)
{
    Amy_audio_play_stop();
    Amy_music_player_stop();

    if (s_ctx.ring_volume[s_ctx.ring_index] <= 0)
        return;

    int vol_percent = (s_ctx.ring_volume[s_ctx.ring_index] * 100) / 6;
    if (vol_percent < 1)
        vol_percent = 1;
    if (vol_percent > 100)
        vol_percent = 100;

    if (s_ctx.ring_mode[s_ctx.ring_index])
    {
        Amy_fm_refresh(FM_DIR_MUSIC);
        int music_count = Amy_fm_get_count(FM_DIR_MUSIC);
        if (music_count <= 0 || s_ctx.ring_select[s_ctx.ring_index] < 1 ||
            s_ctx.ring_select[s_ctx.ring_index] > music_count)
            return;
        Amy_music_player_set_volume(vol_percent);
        Amy_music_player_play(s_ctx.ring_select[s_ctx.ring_index] - 1);
    }
    else
    {
        Amy_audio_play(s_ctx.ring_type[s_ctx.ring_index], vol_percent, true, NULL, NULL);
    }
}

/* Schedule: 点击打开时间选择弹窗 */
static void ring_schedule_click(lv_event_t *ev)
{
    ring_schedule_popup_create();
}

/* Ring Time: 三档 20/30/60 秒循环（对应 duration 枚举 0/1/2），改动后写回 user_data */
static void ring_time_left_cb(lv_event_t *ev)
{
    s_ctx.ring_time[s_ctx.ring_index] = (s_ctx.ring_time[s_ctx.ring_index] <= RING_DURATION_20S)
                                            ? RING_DURATION_60S
                                            : s_ctx.ring_time[s_ctx.ring_index] - 1;
    ring_persist_current();
    ring_row_value_update(RING_ROW_TIME, ring_time_str());
}

static void ring_time_right_cb(lv_event_t *ev)
{
    s_ctx.ring_time[s_ctx.ring_index] = (s_ctx.ring_time[s_ctx.ring_index] >= RING_DURATION_60S)
                                            ? RING_DURATION_20S
                                            : s_ctx.ring_time[s_ctx.ring_index] + 1;
    ring_persist_current();
    ring_row_value_update(RING_ROW_TIME, ring_time_str());
}

/* Ring Mode: 左右箭头切换 Standard / Customized，切换后写回 user_data */
static void ring_mode_left_cb(lv_event_t *ev)
{
    if (!s_ctx.ring_mode[s_ctx.ring_index] && !ring_custom_available(true))
        return; /* 无SD或无铃声，不允许切到自定义 */
    s_ctx.ring_mode[s_ctx.ring_index] = !s_ctx.ring_mode[s_ctx.ring_index];
    ring_persist_current();
    ring_row_value_update(RING_ROW_MODE, ring_mode_str());
    ring_row_value_update(RING_ROW_SELECT, ring_select_str());
    ring_preview_play();
}

static void ring_mode_right_cb(lv_event_t *ev)
{
    if (!s_ctx.ring_mode[s_ctx.ring_index] && !ring_custom_available(true))
        return; /* 无SD或无铃声，不允许切到自定义 */
    s_ctx.ring_mode[s_ctx.ring_index] = !s_ctx.ring_mode[s_ctx.ring_index];
    ring_persist_current();
    ring_row_value_update(RING_ROW_MODE, ring_mode_str());
    ring_row_value_update(RING_ROW_SELECT, ring_select_str());
    ring_preview_play();
}

/* Ring Select: Customized 按实际音乐数量切换，Standard 切内置铃声 1..6；改动后写回 user_data */
static void ring_select_left_cb(lv_event_t *ev)
{
    if (s_ctx.ring_mode[s_ctx.ring_index])
    {
        int count = Amy_fm_get_count(FM_DIR_MUSIC);
        if (count > 0)
        {
            s_ctx.ring_select[s_ctx.ring_index] = (s_ctx.ring_select[s_ctx.ring_index] <= 1)
                                                      ? count
                                                      : s_ctx.ring_select[s_ctx.ring_index] - 1;
        }
    }
    else
    {
        s_ctx.ring_type[s_ctx.ring_index] = (s_ctx.ring_type[s_ctx.ring_index] <= 1) ? 5 : s_ctx.ring_type[s_ctx.ring_index] - 1;
    }
    ring_persist_current();
    ring_row_value_update(RING_ROW_SELECT, ring_select_str());
    ring_preview_play();
}

static void ring_select_right_cb(lv_event_t *ev)
{
    if (s_ctx.ring_mode[s_ctx.ring_index])
    {
        int count = Amy_fm_get_count(FM_DIR_MUSIC);
        if (count > 0)
        {
            s_ctx.ring_select[s_ctx.ring_index] = (s_ctx.ring_select[s_ctx.ring_index] >= count)
                                                      ? 1
                                                      : s_ctx.ring_select[s_ctx.ring_index] + 1;
        }
    }
    else
    {
        s_ctx.ring_type[s_ctx.ring_index] = (s_ctx.ring_type[s_ctx.ring_index] >= 5) ? 1 : s_ctx.ring_type[s_ctx.ring_index] + 1;
    }
    ring_persist_current();
    ring_row_value_update(RING_ROW_SELECT, ring_select_str());
    ring_preview_play();
}

/* Ring Volume: 0-6，对应 rt->volume；改动后写回 user_data */
static void ring_volume_left_cb(lv_event_t *ev)
{
    s_ctx.ring_volume[s_ctx.ring_index] = (s_ctx.ring_volume[s_ctx.ring_index] <= 0) ? 6 : s_ctx.ring_volume[s_ctx.ring_index] - 1;
    ring_persist_current();
    ring_row_value_update(RING_ROW_VOLUME, ring_volume_str());
    ring_preview_play();
}

static void ring_volume_right_cb(lv_event_t *ev)
{
    s_ctx.ring_volume[s_ctx.ring_index] = (s_ctx.ring_volume[s_ctx.ring_index] >= 6) ? 0 : s_ctx.ring_volume[s_ctx.ring_index] + 1;
    ring_persist_current();
    ring_row_value_update(RING_ROW_VOLUME, ring_volume_str());
    ring_preview_play();
}

/* 返回 */
static void ring_back_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(door), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** Schedule 弹窗 **************************/

static int s_popup_start_hour, s_popup_start_min;
static int s_popup_end_hour, s_popup_end_min;

static void ring_schedule_popup_roller_cb(lv_event_t *ev)
{
    lv_obj_t *roller = lv_event_get_target(ev);
    int id = lv_obj_get_id(roller);
    int val = lv_roller_get_selected(roller);

    if (id == 0)
        s_popup_start_hour = val;
    else if (id == 1)
        s_popup_start_min = val;
    else if (id == 2)
        s_popup_end_hour = val;
    else if (id == 3)
        s_popup_end_min = val;
}

static void ring_schedule_popup_ok_cb(lv_event_t *ev)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), RING_POPUP_OVERLAY_ID);
    if (overlay)
        lv_obj_del(overlay);

    s_ctx.schedule_start[s_ctx.ring_index] = s_popup_start_hour * 100 + s_popup_start_min;
    s_ctx.schedule_end[s_ctx.ring_index] = s_popup_end_hour * 100 + s_popup_end_min;
    ring_row_value_update(RING_ROW_SCHEDULE, ring_schedule_str());

    /* 写回 user_data 当前门口机对应 Ring 的时段并持久化 */
    ringtone_config_type_def *rt = &user_data_get()->public_cfg.door_device[g_ring_door].door_cfg.ringtone[s_ctx.ring_index];
    rt->start_h = s_popup_start_hour;
    rt->start_m = s_popup_start_min;
    rt->end_h = s_popup_end_hour;
    rt->end_m = s_popup_end_min;
    user_data_save();
}

static void ring_schedule_popup_cancel_cb(lv_event_t *ev)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), RING_POPUP_OVERLAY_ID);
    if (overlay)
        lv_obj_del(overlay);
}

static void ring_schedule_popup_create(void)
{
    s_popup_start_hour = s_ctx.schedule_start[s_ctx.ring_index] / 100;
    s_popup_start_min = s_ctx.schedule_start[s_ctx.ring_index] % 100;
    s_popup_end_hour = s_ctx.schedule_end[s_ctx.ring_index] / 100;
    s_popup_end_min = s_ctx.schedule_end[s_ctx.ring_index] % 100;

    lv_obj_t *overlay = lv_common_img_btn_create(lv_scr_act_current(), RING_POPUP_OVERLAY_ID,
                                                 0, 0, SCREEN_W, SCREEN_H,
                                                 NULL, false, LV_OPA_50, 0x000000, LV_OPA_50, 0x000000,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = lv_common_setting_cont_create(overlay, RING_POPUP_PANEL_ID,
                                                    253, 103, 598, 441,
                                                    NULL, LV_OPA_COVER, 0X161E26, LV_OPA_COVER, 0X161E26,
                                                    12, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    12, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    lv_common_text_create(panel, -1,
                          0, 0, 598, 46,
                          NULL, LV_OPA_COVER, LV_COMMON_BG_COLOR, LV_OPA_COVER, LV_COMMON_BG_COLOR,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_SCHEDULE), 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    int roller_w = 80;
    int roller_h = 180;
    int roller_y = 100;
    int start_x = 88;
    int gap = 15;

    lv_resource_init(icon_arrow_up, ICON_COMMON_ARROW_UP_PNG_OFFSET, ICON_COMMON_ARROW_UP_PNG_SIZE);
    lv_resource_init(icon_arrow_down, ICON_COMMON_ARROW_DOWN_PNG_OFFSET, ICON_COMMON_ARROW_DOWN_PNG_SIZE);

    /* 4 个滚筒: start_hour, start_min, end_hour, end_min */
    for (int i = 0; i < 4; i++)
    {
        char opts[512] = {0};
        int count = 0;
        int def_idx = 0;
        int max_val = (i == 0 || i == 2) ? 23 : 59;

        for (int v = 0; v <= max_val; v++)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%02d%s", v, v == max_val ? "" : "\n");
            strcat(opts, buf);
            if (i == 0 && v == s_popup_start_hour)
                def_idx = count;
            else if (i == 1 && v == s_popup_start_min)
                def_idx = count;
            else if (i == 2 && v == s_popup_end_hour)
                def_idx = count;
            else if (i == 3 && v == s_popup_end_min)
                def_idx = count;
            count++;
        }

        int rx = start_x + i * (roller_w + gap) + (i > 1 ? (60 - gap) : 0);
        lv_obj_t *roller = lv_roller_create(panel);
        lv_obj_set_id(roller, i);
        lv_obj_set_pos(roller, rx, roller_y);
        lv_obj_set_size(roller, roller_w, roller_h);
        lv_obj_set_style_text_font(roller, lv_font_normal, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(roller, 38, LV_PART_MAIN);
        lv_roller_set_options(roller, opts, LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(roller, 3);
        lv_roller_set_selected(roller, def_idx, LV_ANIM_OFF);
        lv_obj_set_style_text_color(roller, lv_color_hex(0x808080), LV_PART_MAIN);
        lv_obj_set_style_text_color(roller, lv_color_hex(0xD7C197), LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(roller, LV_OPA_TRANSP, LV_PART_SELECTED);
        lv_obj_add_event_cb(roller, ring_schedule_popup_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

        /* Arrows above and below roller */
        lv_common_img_btn_create(panel, -1,
                                 rx + (roller_w - ICON_COMMON_ARROW_UP_PNG_WIDTH) / 2,
                                 roller_y - ICON_COMMON_ARROW_UP_PNG_HEIGHT - 2,
                                 ICON_COMMON_ARROW_UP_PNG_WIDTH, ICON_COMMON_ARROW_UP_PNG_HEIGHT,
                                 NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 &icon_arrow_up, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

        lv_common_img_btn_create(panel, -1,
                                 rx + (roller_w - ICON_COMMON_ARROW_DOWN_PNG_WIDTH) / 2,
                                 roller_y + roller_h + 2,
                                 ICON_COMMON_ARROW_DOWN_PNG_WIDTH, ICON_COMMON_ARROW_DOWN_PNG_HEIGHT,
                                 NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 &icon_arrow_down, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);
    }

    /* 底部按钮 (msgbox风格) */
    lv_resource_init(icon_cancel, ICON_COMMON_MSGBOX_CANCEL_PNG_OFFSET, ICON_COMMON_MSGBOX_CANCEL_PNG_SIZE);
    lv_resource_init(icon_confirm, ICON_COMMON_MSGBOX_CONFIRM_PNG_OFFSET, ICON_COMMON_MSGBOX_CONFIRM_PNG_SIZE);

    int btn_w = 598 / 2;
    int btn_h = 69;
    int btn_y = 441 - btn_h;

    /* 确认按钮 (左) */
    lv_obj_t *confirm_btn = lv_common_img_text_btn_create(panel,
                                                          RING_POPUP_OK_BTN_ID, 0, btn_y, btn_w, btn_h,
                                                          ring_schedule_popup_ok_cb,
                                                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, btn_w, btn_h, 0,
                                                          lang_str_get(STR_CONFIRM), 0xffffff, 0xff0000,
                                                          LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
                                                          0, 0, btn_w, btn_h, 1,
                                                          &icon_confirm, LV_OPA_TRANSP, 0xffffff, LV_ALIGN_CENTER, NULL);
    lv_obj_set_style_radius(confirm_btn, 0, LV_PART_MAIN);

    /* 取消按钮 (右) */
    lv_obj_t *cancel_btn = lv_common_img_text_btn_create(panel,
                                                         RING_POPUP_CANCEL_BTN_ID, btn_w, btn_y, btn_w, btn_h,
                                                         ring_schedule_popup_cancel_cb,
                                                         LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, btn_w, btn_h, 0,
                                                         lang_str_get(STR_CANCEL), 0xffffff, 0xff0000,
                                                         LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
                                                         0, 0, btn_w, btn_h, 1,
                                                         &icon_cancel, LV_OPA_TRANSP, 0xffffff, LV_ALIGN_CENTER, NULL);
    lv_obj_set_style_radius(cancel_btn, 0, LV_PART_MAIN);
}

/************************** 创建函数 **************************/

static void ring_sidebar_create(void)
{
    /* Ring 1 */
    lv_resource_init(ring1_icon, ICON_RING_RING1_PNG_OFFSET, ICON_RING_RING1_PNG_SIZE);
    lv_resource_init(ring1_icon_focus, ICON_RING_RING1_FOCUS_PNG_OFFSET, ICON_RING_RING1_FOCUS_PNG_SIZE);
    lv_obj_t *ring1_btn = lv_common_img_btn_create(lv_scr_act_current(), RING_SEL_RING1_BTN_ID,
                                                   RING_ICON_X, RING1_ICON_Y, RING_ICON_W, RING_ICON_H,
                                                   ring_sel_ring1_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &ring1_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &ring1_icon_focus);
    lv_obj_set_ext_click_area(ring1_btn, 21);
    lv_common_text_create(lv_scr_act_current(), RING_SEL_RING1_LABEL_ID,
                          0, RING1_LABEL_Y, SIDEBAR_W, 28,
                          ring_sel_ring1_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_RING1), RING_SEL_COLOR, 0xff0000, LV_TEXT_ALIGN_CENTER, lv_font_normal);

    /* Ring 2 */
    lv_resource_init(ring2_icon, ICON_RING_RING2_PNG_OFFSET, ICON_RING_RING2_PNG_SIZE);
    lv_resource_init(ring2_icon_focus, ICON_RING_RING2_FOCUS_PNG_OFFSET, ICON_RING_RING2_FOCUS_PNG_SIZE);
    lv_obj_t *ring2_btn = lv_common_img_btn_create(lv_scr_act_current(), RING_SEL_RING2_BTN_ID,
                                                   RING_ICON_X, RING2_ICON_Y, RING_ICON_W, RING_ICON_H,
                                                   ring_sel_ring2_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &ring2_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &ring2_icon_focus);
    lv_obj_set_ext_click_area(ring2_btn, 21);
    lv_common_text_create(lv_scr_act_current(), RING_SEL_RING2_LABEL_ID,
                          0, RING2_LABEL_Y, SIDEBAR_W, 28,
                          ring_sel_ring2_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_RING2), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER, lv_font_normal);

    /* Ring 3 */
    lv_resource_init(ring3_icon, ICON_RING_RING3_PNG_OFFSET, ICON_RING_RING3_PNG_SIZE);
    lv_resource_init(ring3_icon_focus, ICON_RING_RING3_FOCUS_PNG_OFFSET, ICON_RING_RING3_FOCUS_PNG_SIZE);
    lv_obj_t *ring3_btn = lv_common_img_btn_create(lv_scr_act_current(), RING_SEL_RING3_BTN_ID,
                                                   RING_ICON_X, RING3_ICON_Y, RING_ICON_W, RING_ICON_H,
                                                   ring_sel_ring3_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &ring3_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &ring3_icon_focus);
    lv_obj_set_ext_click_area(ring3_btn, 21);
    lv_common_text_create(lv_scr_act_current(), RING_SEL_RING3_LABEL_ID,
                          0, RING3_LABEL_Y, SIDEBAR_W, 28,
                          ring_sel_ring3_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_RING3), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void ring_rows_create(void)
{
    /* Schedule */
    ring_row_btn_create(RING_ROW_SCHEDULE, lang_str_get(STR_SCHEDULE), ring_schedule_str(), ring_schedule_click);
    ring_arrow_right_create(RING_ROW_SCHEDULE, ring_schedule_click);

    /* Ring Time */
    ring_row_btn_create(RING_ROW_TIME, lang_str_get(STR_RING_TIME), ring_time_str(), NULL);
    ring_arrow_left_create(RING_ROW_TIME, ring_time_left_cb);
    ring_arrow_right_create(RING_ROW_TIME, ring_time_right_cb);

    /* Ring Mode */
    ring_row_btn_create(RING_ROW_MODE, lang_str_get(STR_RING_MODE), ring_mode_str(), NULL);
    ring_arrow_left_create(RING_ROW_MODE, ring_mode_left_cb);
    ring_arrow_right_create(RING_ROW_MODE, ring_mode_right_cb);

    /* Ring Select */
    ring_row_btn_create(RING_ROW_SELECT, lang_str_get(STR_RING_SELECT), ring_select_str(), NULL);
    ring_arrow_left_create(RING_ROW_SELECT, ring_select_left_cb);
    ring_arrow_right_create(RING_ROW_SELECT, ring_select_right_cb);

    /* Ring Volume */
    ring_row_btn_create(RING_ROW_VOLUME, lang_str_get(STR_RING_VOLUME), ring_volume_str(), NULL);
    ring_arrow_left_create(RING_ROW_VOLUME, ring_volume_left_cb);
    ring_arrow_right_create(RING_ROW_VOLUME, ring_volume_right_cb);
}

static void ring_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *ring_back = lv_common_img_btn_create(lv_scr_act_current(), RING_BACK_BTN_ID, 929, 505, 67, 67,
                                                   ring_back_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(ring_back, 21);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(ring)
{
    lv_common_bg_create(lv_scr_act_current());

    /* 从 user_data 加载当前门口机的三个 Ring 时段/mode/type/select */
    s_ctx.door = g_ring_door;
    /* 每次进入固定从 Ring1 开始，与侧边栏高亮(硬编码 Ring1)保持一致，
       避免残留上一次访问的 ring_index 导致内容/编辑落到错误时段 */
    s_ctx.ring_index = 0;
    door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[s_ctx.door].door_cfg;
    for (int i = 0; i < 3; i++)
    {
        s_ctx.schedule_start[i] = cfg->ringtone[i].start_h * 100 + cfg->ringtone[i].start_m;
        s_ctx.schedule_end[i] = cfg->ringtone[i].end_h * 100 + cfg->ringtone[i].end_m;
        s_ctx.ring_time[i] = cfg->ringtone[i].duration;
        s_ctx.ring_volume[i] = cfg->ringtone[i].volume;
        s_ctx.ring_mode[i] = cfg->ringtone[i].mode;
        s_ctx.ring_type[i] = cfg->ringtone[i].type;
        s_ctx.ring_select[i] = cfg->ringtone[i].select;
    }

    /* 若自定义铃声已不可用（SD 拔出等），进入页面时自动回退为标准铃声并持久化 */
    if (ring_mode_sanitize(true))
        user_data_save();

    /* Customized 且保存的 select 超出当前音乐数量时，收敛到数量上限 */
    int music_cnt = Amy_fm_get_count(FM_DIR_MUSIC);
    for (int i = 0; i < 3; i++)
    {
        if (s_ctx.ring_mode[i] && music_cnt > 0 && s_ctx.ring_select[i] > music_cnt)
            s_ctx.ring_select[i] = music_cnt;
    }

    ring_sidebar_create();

    lv_common_text_create(lv_scr_act_current(), RING_PANEL_TITLE_ID,
                          ROW_X, 29, 200, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          ring_title_str(), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal);

    ring_rows_create();
    ring_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, ring_sd_change_cb);
}

static void Amy_lv_page_quit(ring)
{
    /* 离开页面时停止铃声预览，避免铃声在后续页面继续播放 */
    Amy_audio_play_stop();
    Amy_music_player_stop();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(ring);
