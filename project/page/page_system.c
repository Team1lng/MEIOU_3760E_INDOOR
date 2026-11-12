#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "user_network.h"
#include "call_api.h"
#include "page_password_input.h"
#include "live/testRTSPClient.h"
#include "../pwm.h"
#include "Amy_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************** 布局常量 (EMOS-Indoor参考) **************************/
#define ADMIN_SUPER_PASSWORD "9149"
#define ROW_X 199
#define ROW_W 700
#define ROW_H 52
#define ROW1_Y 23
#define ROW_SPACING 52
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 250
#define ARROW_L_X 460
#define ARROW_R_X 867
#define ARROW_W 52
#define ARROW_H 52
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 50

/************************** 行索引 **************************/
typedef enum
{
    SYS_ROW_DEVICE_ID = 0,
    SYS_ROW_TIME,
    SYS_ROW_DATE_FORMAT,
    SYS_ROW_CLOCK,
    SYS_ROW_LANGUAGE,
    SYS_ROW_KEYTONE,
    SYS_ROW_DETECTION,
    SYS_ROW_UNLOCK_DELAY,
    SYS_ROW_UNLOCK_VOICE,
    SYS_ROW_ADMIN,
    SYS_ROW_NUM
} sys_row_t;

/************************** 页面上下文 **************************/
static struct
{
    int cur_device_id;
    lv_timer_t *time_task;
    lv_timer_t *pwd_tip_timer;
} s_ctx;

/************************** 函数前置声明 **************************/
static void sys_time_task_cb(lv_timer_t *task);

/************************** 辅助函数 **************************/

static int sys_row_y(sys_row_t row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static void sys_btn_value_update(sys_row_t row, const char *text)
{
    lv_obj_t *btn = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_ROW1_CONT_ID + row * 3), SYSTEM_ROW1_LABEL2_ID + row * 3);
    if (btn && text)
        lv_label_set_text(btn, text);
}

static const char *sys_date_format_str(void)
{
    static const char *formats[] = {"YY-MM-DD", "MM-DD-YY", "DD-MM-YY"};
    return formats[user_data_get()->private_cfg.date_format];
}

static const char *sys_language_str(void)
{
    return lang_str_get(STR_LANGUAGE_TYPE);
}

static const char *sys_on_off_str(bool on)
{
    return lang_str_get(on ? STR_ON : STR_OFF);
}

static void sys_time_format(char *buf, int len)
{
    time_t seconds = time(NULL);
    struct tm tm;
    localtime_r(&seconds, &tm);
    int fmt = user_data_get()->private_cfg.date_format;
    if (fmt == 0)
        snprintf(buf, len, "%04d-%02d-%02d  %02d:%02d",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
    else if (fmt == 1)
        snprintf(buf, len, "%02d-%02d-%04d  %02d:%02d",
                 tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
    else
        snprintf(buf, len, "%02d-%02d-%04d  %02d:%02d",
                 tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

/************************** 行创建辅助 **************************/

static void sys_row_separator_create(sys_row_t row)
{
    int y = sys_row_y(row);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static lv_obj_t *sys_row_btn_create(sys_row_t row, const char *label_text,
                                    const char *value_text, lv_event_cb_t cb)
{
    int y = sys_row_y(row);
    int cont_id = SYSTEM_ROW1_CONT_ID + row * 3;
    int label1_id = SYSTEM_ROW1_LABEL1_ID + row * 3;
    int label2_id = SYSTEM_ROW1_LABEL2_ID + row * 3;

    lv_obj_t *cont = lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        label_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 380, ROW_H,
        value_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    sys_row_separator_create(row);
    return cont;
}

static void sys_arrow_left_create(sys_row_t row, lv_event_cb_t cb)
{
    int y = sys_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_L_X, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_ROW1_CONT_ID + row * 3), SYSTEM_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_ROW1_CONT_ID + row * 3), SYSTEM_ROW1_LABEL2_ID + row * 3));
}

static void sys_arrow_right_create(sys_row_t row, lv_event_cb_t cb)
{
    int y = sys_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_ROW1_CONT_ID + row * 3), SYSTEM_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_ROW1_CONT_ID + row * 3), SYSTEM_ROW1_LABEL2_ID + row * 3));
}

/************************** 各回调函数 **************************/

static int sys_device_id_get(void)
{
    return user_data_get()->private_cfg.device.attr.device_type;
}

static void sys_device_id_set(int id)
{
    user_data_get()->private_cfg.device.attr.device_type = id;
}

/* Device ID 左右箭头 */
static void sys_device_id_left_cb(lv_event_t *ev)
{
    int id = s_ctx.cur_device_id;
    id = (id <= 1) ? 6 : id - 1;
    s_ctx.cur_device_id = id;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", id);
    sys_btn_value_update(SYS_ROW_DEVICE_ID, buf);
}

static void sys_device_id_right_cb(lv_event_t *ev)
{
    int id = s_ctx.cur_device_id;
    id = (id >= 6) ? 1 : id + 1;
    s_ctx.cur_device_id = id;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", id);
    sys_btn_value_update(SYS_ROW_DEVICE_ID, buf);
}

/* Time 弹窗 */
static struct tm s_time_popup_tm;

static void sys_time_popup_roller_cb(lv_event_t *ev)
{
    lv_obj_t *roller = lv_event_get_target(ev);
    int id = lv_obj_get_id(roller);
    int idx = lv_roller_get_selected(roller);

    if (id == 0)
        s_time_popup_tm.tm_year = 2021 + idx;
    else if (id == 1)
        s_time_popup_tm.tm_mon = idx;
    else if (id == 2)
        s_time_popup_tm.tm_mday = 1 + idx;
    else if (id == 3)
        s_time_popup_tm.tm_hour = idx;
    else if (id == 4)
        s_time_popup_tm.tm_min = idx;
}

static void sys_time_popup_ok_cb(lv_event_t *ev)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_TIME_POPUP_OVERLAY_ID);
    if (overlay)
        lv_obj_del(overlay);

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "date -s \"%04d-%02d-%02d %02d:%02d:00\"",
             s_time_popup_tm.tm_year, s_time_popup_tm.tm_mon + 1, s_time_popup_tm.tm_mday,
             s_time_popup_tm.tm_hour, s_time_popup_tm.tm_min);
    system(cmd);
    system("hwclock -w");
    sys_time_task_cb(NULL);
}

static void sys_time_popup_cancel_cb(lv_event_t *ev)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), SYSTEM_TIME_POPUP_OVERLAY_ID);
    if (overlay)
        lv_obj_del(overlay);
}

static void sys_time_popup_create(void)
{
    time_t seconds = time(NULL);
    localtime_r(&seconds, &s_time_popup_tm);
    s_time_popup_tm.tm_year += 1900;

    lv_obj_t *overlay = lv_common_img_btn_create(lv_scr_act_current(), SYSTEM_TIME_POPUP_OVERLAY_ID,
                                                      0, 0, SCREEN_W, SCREEN_H,
                                                      NULL, false, LV_OPA_80, 0x000000, LV_OPA_80, 0x000000,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = lv_common_setting_cont_create(overlay, -1,
                                                    253, 103, 598, 441,
                                                    NULL, LV_OPA_COVER, 0X161E26, LV_OPA_COVER, 0X161E26,
                                                    18, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    18, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(panel, true, LV_PART_MAIN);
    lv_common_text_create(panel, -1,
                          0, 0, 598, 46,
                          NULL, LV_OPA_COVER, LV_COMMON_BG_COLOR, LV_OPA_COVER, LV_COMMON_BG_COLOR,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_TIME), 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    int roller_w = 80;
    int roller_h = 180;
    int roller_y = 100;
    int start_x = 53;
    int gap = 15;

    lv_resource_init(icon_arrow_up, ICON_COMMON_ARROW_UP_PNG_OFFSET, ICON_COMMON_ARROW_UP_PNG_SIZE);
    lv_resource_init(icon_arrow_down, ICON_COMMON_ARROW_DOWN_PNG_OFFSET, ICON_COMMON_ARROW_DOWN_PNG_SIZE);

    for (int i = 0; i < 5; i++)
    {
        char opts[512] = {0};
        int count = 0;
        int def_idx = 0;

        if (i == 0)
        { /* Year */
            for (int y = 2021; y <= 2036; y++)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%04d%s", y, y == 2036 ? "" : "\n");
                strcat(opts, buf);
                if (y == s_time_popup_tm.tm_year)
                    def_idx = count;
                count++;
            }
        }
        else if (i == 1)
        { /* Month */
            for (int m = 1; m <= 12; m++)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02d%s", m, m == 12 ? "" : "\n");
                strcat(opts, buf);
                if (m == s_time_popup_tm.tm_mon + 1)
                    def_idx = count;
                count++;
            }
        }
        else if (i == 2)
        { /* Day */
            for (int d = 1; d <= 31; d++)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02d%s", d, d == 31 ? "" : "\n");
                strcat(opts, buf);
                if (d == s_time_popup_tm.tm_mday)
                    def_idx = count;
                count++;
            }
        }
        else if (i == 3)
        { /* Hour */
            for (int h = 0; h <= 23; h++)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02d%s", h, h == 23 ? "" : "\n");
                strcat(opts, buf);
                if (h == s_time_popup_tm.tm_hour)
                    def_idx = count;
                count++;
            }
        }
        else
        { /* Minute */
            for (int m = 0; m <= 59; m++)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02d%s", m, m == 59 ? "" : "\n");
                strcat(opts, buf);
                if (m == s_time_popup_tm.tm_min)
                    def_idx = count;
                count++;
            }
        }

        int rx = start_x + i * (roller_w + gap) + (i > 2 ? (60 - gap) : 0);
        lv_obj_t *roller = lv_roller_create(panel);
        lv_obj_set_child_id(roller, i);
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
        lv_obj_add_event_cb(roller, sys_time_popup_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

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
                                                          -1, 0, btn_y, btn_w, btn_h,
                                                          sys_time_popup_ok_cb,
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
                                                         -1, btn_w, btn_y, btn_w, btn_h,
                                                         sys_time_popup_cancel_cb,
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

static void sys_time_row_click(lv_event_t *ev)
{
    sys_time_popup_create();
}

/* Date Format 左右箭头 */
static void sys_date_format_left_cb(lv_event_t *ev)
{
    uint8_t *fmt = &user_data_get()->private_cfg.date_format;
    *fmt = (*fmt == 0) ? 2 : *fmt - 1;
    sys_btn_value_update(SYS_ROW_DATE_FORMAT, sys_date_format_str());
    sys_time_task_cb(NULL);
}

static void sys_date_format_right_cb(lv_event_t *ev)
{
    uint8_t *fmt = &user_data_get()->private_cfg.date_format;
    *fmt = (*fmt == 2) ? 0 : *fmt + 1;
    sys_btn_value_update(SYS_ROW_DATE_FORMAT, sys_date_format_str());
    sys_time_task_cb(NULL);
}

/* Clock 左右箭头 */
static void sys_clock_left_cb(lv_event_t *ev)
{
    screen_saver_type_def *en = &user_data_get()->private_cfg.screen_saver_en;
    *en = (*en == SCREEN_SAVER_ENABLE) ? SCREEN_SAVER_DISABLE : SCREEN_SAVER_ENABLE;
    sys_btn_value_update(SYS_ROW_CLOCK, sys_on_off_str(*en == SCREEN_SAVER_ENABLE));
}

static void sys_clock_right_cb(lv_event_t *ev)
{
    screen_saver_type_def *en = &user_data_get()->private_cfg.screen_saver_en;
    *en = (*en == SCREEN_SAVER_ENABLE) ? SCREEN_SAVER_DISABLE : SCREEN_SAVER_ENABLE;
    sys_btn_value_update(SYS_ROW_CLOCK, sys_on_off_str(*en == SCREEN_SAVER_ENABLE));
}

/* Language 左右箭头 */
static void sys_language_left_cb(lv_event_t *ev)
{
    int total = language_total_get();
    int cur = (int)language_id_get();
    int new_idx = (cur <= 0) ? total - 1 : cur - 1;
    language_id_set((LANGUAGE_ID)new_idx);
    user_data_get()->private_cfg.lang = new_idx;
    Amy_lv_page_goto(lv_page(system), LV_SCR_LOAD_ANIM_NONE, );
}

static void sys_language_right_cb(lv_event_t *ev)
{
    int total = language_total_get();
    int cur = (int)language_id_get();
    int new_idx = (cur + 1 >= total) ? 0 : cur + 1;
    language_id_set((LANGUAGE_ID)new_idx);
    user_data_get()->private_cfg.lang = new_idx;
    Amy_lv_page_goto(lv_page(system), LV_SCR_LOAD_ANIM_NONE, );
}

/* Keytone 左右箭头 */
static void sys_keytone_left_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.keytone_enable = !user_data_get()->private_cfg.keytone_enable;
    sys_btn_value_update(SYS_ROW_KEYTONE, sys_on_off_str(user_data_get()->private_cfg.keytone_enable));
}

static void sys_keytone_right_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.keytone_enable = !user_data_get()->private_cfg.keytone_enable;
    sys_btn_value_update(SYS_ROW_KEYTONE, sys_on_off_str(user_data_get()->private_cfg.keytone_enable));
}

/* Detection Preview 左右箭头 */
static void sys_detection_left_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.detection_preview_enable = !user_data_get()->private_cfg.detection_preview_enable;
    sys_btn_value_update(SYS_ROW_DETECTION, sys_on_off_str(user_data_get()->private_cfg.detection_preview_enable));
}

static void sys_detection_right_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.detection_preview_enable = !user_data_get()->private_cfg.detection_preview_enable;
    sys_btn_value_update(SYS_ROW_DETECTION, sys_on_off_str(user_data_get()->private_cfg.detection_preview_enable));
}

/* Unlock Delay 左右箭头 */
static void sys_unlock_delay_left_cb(lv_event_t *ev)
{
    uint8_t *delay = &user_data_get()->private_cfg.unlock_delay;
    *delay = (*delay <= 1) ? 10 : *delay - 1;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d S", *delay);
    sys_btn_value_update(SYS_ROW_UNLOCK_DELAY, buf);
}

static void sys_unlock_delay_right_cb(lv_event_t *ev)
{
    uint8_t *delay = &user_data_get()->private_cfg.unlock_delay;
    *delay = (*delay >= 10) ? 1 : *delay + 1;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d S", *delay);
    sys_btn_value_update(SYS_ROW_UNLOCK_DELAY, buf);
}

/* Unlocking Voice 左右箭头 */
static void sys_unlock_voice_left_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.unlock_voice_enable = !user_data_get()->private_cfg.unlock_voice_enable;
    sys_btn_value_update(SYS_ROW_UNLOCK_VOICE, sys_on_off_str(user_data_get()->private_cfg.unlock_voice_enable));
}

static void sys_unlock_voice_right_cb(lv_event_t *ev)
{
    user_data_get()->private_cfg.unlock_voice_enable = !user_data_get()->private_cfg.unlock_voice_enable;
    sys_btn_value_update(SYS_ROW_UNLOCK_VOICE, sys_on_off_str(user_data_get()->private_cfg.unlock_voice_enable));
}

/* Admin Setting — 密码验证通过后才进入 */
static void sys_pwd_tip_close_cb(lv_timer_t *t)
{
    s_ctx.pwd_tip_timer = NULL;
    lv_timer_del(t);
    lv_common_msgbox_del();
}

static void sys_pwd_tip_show(const char *msg)
{
    if (s_ctx.pwd_tip_timer != NULL)
    {
        lv_timer_del(s_ctx.pwd_tip_timer);
        s_ctx.pwd_tip_timer = NULL;
    }
    lv_common_msgbox_del();
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42, msg);
    s_ctx.pwd_tip_timer = lv_timer_create(sys_pwd_tip_close_cb, 2000, NULL);
}

static void sys_admin_pwd_back_cb(lv_event_t *e)
{
    Amy_lv_page_goto(lv_page(system), LV_SCR_LOAD_ANIM_NONE, );
}

static void sys_admin_pwd_confirm_cb(lv_event_t *e)
{
    const char *input = page_password_input_get_text();
    const char *pwd = user_data_get()->private_cfg.admin.password;

    if (strcmp(input, ADMIN_SUPER_PASSWORD) == 0)
    {
        Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
        return;
    }
    if (strlen(input) < ADMIN_PASSWORD_LEN)
    {
        sys_pwd_tip_show(lang_str_get(STR_PASSW_SHOT));
        return;
    }
    if (strcmp(input, pwd) == 0)
    {
        Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
        return;
    }
    sys_pwd_tip_show(lang_str_get(STR_PASSW_ERROR));
}

static void sys_admin_click_cb(lv_event_t *ev)
{
    page_password_input_set_placeholder(lang_str_get(STR_PLEASE_INPUT_PASSWORD));
    page_password_input_set_title_text(lang_str_get(STR_PASSWORD_INPUT));
    page_password_input_set_max_length(ADMIN_PASSWORD_LEN);
    page_password_input_set_back_cb(sys_admin_pwd_back_cb);
    page_password_input_set_confirm_cb(sys_admin_pwd_confirm_cb);
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

/* 返回，设备号变更时：保存变更并直接应用新 IP（重启 RTSP 服务器），不重启设备 */
static void system_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 时间更新任务 **************************/

static void sys_time_task_cb(lv_timer_t *task)
{
    char buf[64];
    sys_time_format(buf, sizeof(buf));
    sys_btn_value_update(SYS_ROW_TIME, buf);
}

/************************** 行创建 **************************/

static void sys_device_id_row_create(void)
{
    sys_row_t row = SYS_ROW_DEVICE_ID;
    int id = s_ctx.cur_device_id;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", id);
    sys_row_btn_create(row, lang_str_get(STR_DEVICE_ID), buf, NULL);
    sys_arrow_left_create(row, sys_device_id_left_cb);
    sys_arrow_right_create(row, sys_device_id_right_cb);
}

static void sys_time_row_create(void)
{
    sys_row_t row = SYS_ROW_TIME;
    char buf[64];
    sys_time_format(buf, sizeof(buf));
    sys_row_btn_create(row, lang_str_get(STR_TIME), buf, sys_time_row_click);
    sys_arrow_right_create(row, sys_time_row_click);
}

static void sys_date_format_row_create(void)
{
    sys_row_t row = SYS_ROW_DATE_FORMAT;
    sys_row_btn_create(row, lang_str_get(STR_DATE_FORMAT),
                       sys_date_format_str(), NULL);
    sys_arrow_left_create(row, sys_date_format_left_cb);
    sys_arrow_right_create(row, sys_date_format_right_cb);
}

static void sys_clock_row_create(void)
{
    sys_row_t row = SYS_ROW_CLOCK;
    bool on = (user_data_get()->private_cfg.screen_saver_en == SCREEN_SAVER_ENABLE);
    sys_row_btn_create(row, lang_str_get(STR_CLOCK),
                       sys_on_off_str(on), NULL);
    sys_arrow_left_create(row, sys_clock_left_cb);
    sys_arrow_right_create(row, sys_clock_right_cb);
}

static void sys_language_row_create(void)
{
    sys_row_t row = SYS_ROW_LANGUAGE;
    sys_row_btn_create(row, lang_str_get(STR_LANGUAGE), sys_language_str(), NULL);
    sys_arrow_left_create(row, sys_language_left_cb);
    sys_arrow_right_create(row, sys_language_right_cb);
}

static void sys_keytone_row_create(void)
{
    sys_row_t row = SYS_ROW_KEYTONE;
    sys_row_btn_create(row, lang_str_get(STR_KEYTONE),
                       sys_on_off_str(user_data_get()->private_cfg.keytone_enable), NULL);
    sys_arrow_left_create(row, sys_keytone_left_cb);
    sys_arrow_right_create(row, sys_keytone_right_cb);
}

static void sys_detection_row_create(void)
{
    sys_row_t row = SYS_ROW_DETECTION;
    sys_row_btn_create(row, lang_str_get(STR_DETECTION_PREVIEW),
                       sys_on_off_str(user_data_get()->private_cfg.detection_preview_enable), NULL);
    sys_arrow_left_create(row, sys_detection_left_cb);
    sys_arrow_right_create(row, sys_detection_right_cb);
}

static void sys_unlock_delay_row_create(void)
{
    sys_row_t row = SYS_ROW_UNLOCK_DELAY;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d S", user_data_get()->private_cfg.unlock_delay);
    sys_row_btn_create(row, lang_str_get(STR_INDOOR_UNLOCK_TIME),
                       buf, NULL);
    sys_arrow_left_create(row, sys_unlock_delay_left_cb);
    sys_arrow_right_create(row, sys_unlock_delay_right_cb);
}

static void sys_unlock_voice_row_create(void)
{
    sys_row_t row = SYS_ROW_UNLOCK_VOICE;
    sys_row_btn_create(row, lang_str_get(STR_UNLOCK_HINT),
                       sys_on_off_str(user_data_get()->private_cfg.unlock_voice_enable), NULL);
    sys_arrow_left_create(row, sys_unlock_voice_left_cb);
    sys_arrow_right_create(row, sys_unlock_voice_right_cb);
}

static void sys_admin_row_create(void)
{
    sys_row_t row = SYS_ROW_ADMIN;
    sys_row_btn_create(row, lang_str_get(STR_ADMIN_SETTING),
                       "******", sys_admin_click_cb);
    sys_arrow_right_create(row, sys_admin_click_cb);
}

/************************** 返回按钮 **************************/

static void system_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *system_back = lv_common_img_btn_create(
        lv_scr_act_current(), SYSTEM_BACK_BTN_ID, 929, 505, 67, 67,
        system_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(system_back, 21);
}

/************************** 侧栏标题 **************************/

static void system_title_create(void)
{
    lv_resource_init(title_icon, ICON_SETTING_SYSTEM_PNG_OFFSET, ICON_SETTING_SYSTEM_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SYSTEM_FOCUS_PNG_OFFSET, ICON_SETTING_SYSTEM_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), SYSTEM_TITLE_ICON_ID,
                             30, 218, 130, 130,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), SYSTEM_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_SYSTEM_SET),
                          0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(system)
{
    lv_common_bg_create(lv_scr_act_current());

    s_ctx.cur_device_id = sys_device_id_get();

    system_title_create();

    sys_device_id_row_create();
    sys_time_row_create();
    sys_date_format_row_create();
    sys_clock_row_create();
    sys_language_row_create();
    sys_keytone_row_create();
    sys_detection_row_create();
    sys_unlock_delay_row_create();
    sys_unlock_voice_row_create();
    sys_admin_row_create();

    system_back_create();

    s_ctx.time_task = lv_timer_create(sys_time_task_cb, 3000, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(system)
{
    if (s_ctx.time_task != NULL)
    {
        lv_timer_del(s_ctx.time_task);
        s_ctx.time_task = NULL;
    }
    if (s_ctx.pwd_tip_timer != NULL)
    {
        lv_timer_del(s_ctx.pwd_tip_timer);
        s_ctx.pwd_tip_timer = NULL;
    }
    if (sys_device_id_get() != s_ctx.cur_device_id)
    {
        sys_device_id_set(s_ctx.cur_device_id);    /* 保存变更后的设备号 */
        user_network_apply_device_static_ip_now(); /* 重新应用静态 IP */
        rtsp_server_request_restart();             /* 重启 RTSP 服务器以绑定新 IP */
        device_info_attr local_info = {
            .building = user_data_get()->private_cfg.device.attr.building,
            .room = user_data_get()->private_cfg.device.attr.room,
            .device_type = user_data_get()->private_cfg.device.attr.device_type,
        };
        sat_call_update_local_info(&local_info); /* 更新会话框架本地设备信息 */
    }
    user_data_save();

    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(system);
