#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include <stdio.h>

/************************** 布局常量  **************************/
#define ROW_X 199
#define ROW_W 700
#define ROW_H 60
#define ROW1_Y 75
#define ROW_SPACING 60
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 260
#define VALUE_X 290
#define VALUE_W 370
#define ARROW_L_X 460
#define ARROW_R_X 867
#define ARROW_W 52
#define ARROW_H 60
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 58

/************************** 行索引 ) **************************/
typedef enum
{
    SCENE_ROW_PHOTO_SW = 0,
    SCENE_ROW_PHOTO_TIME,
    SCENE_ROW_MUSIC_SW,
    SCENE_ROW_MUSIC_VOL,
    SCENE_ROW_NUM
} scene_row_t;

#define SCENE_CFG (user_data_get()->private_cfg.scene)

/************************** 函数声明 **************************/
static void scene_back_obj_click(lv_event_t *ev);
static void scene_arrow_left_create(scene_row_t row, lv_event_cb_t cb);
static void scene_arrow_right_create(scene_row_t row, lv_event_cb_t cb);

/************************** 辅助 **************************/

static int scene_row_y(scene_row_t row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static const char *scene_on_off_str(bool on)
{
    return lang_str_get(on ? STR_ON : STR_OFF);
}

static void scene_value_update(scene_row_t row, const char *text)
{
    lv_obj_t *label = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SCENE_ROW1_CONT_ID + row * 3), SCENE_ROW1_LABEL2_ID + row * 3);
    if (label && text)
        lv_label_set_text(label, text);
}

static void scene_arrow_left_create(scene_row_t row, lv_event_cb_t cb)
{
    int y = scene_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_L_X, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SCENE_ROW1_CONT_ID + row * 3), SCENE_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SCENE_ROW1_CONT_ID + row * 3), SCENE_ROW1_LABEL2_ID + row * 3));
}

static void scene_arrow_right_create(scene_row_t row, lv_event_cb_t cb)
{
    int y = scene_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SCENE_ROW1_CONT_ID + row * 3), SCENE_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), SCENE_ROW1_CONT_ID + row * 3), SCENE_ROW1_LABEL2_ID + row * 3));
}

/************************** 数码相框开关  **************************/

static void scene_photo_sw_toggle_cb(lv_event_t *ev)
{
    if (user_data_get()->public_cfg.door_device[0].door_cfg.human_detect.detect_level != DETECT_LEVEL_OFF ||
        user_data_get()->public_cfg.door_device[1].door_cfg.human_detect.detect_level != DETECT_LEVEL_OFF)
    {
        lv_common_msgbox_create(251, 106, 586, 388, 62,
                                lang_str_get(STR_MOTION_DETECTION_CLOSE), NULL, NULL);
        return;
    }
    SCENE_CFG.digital_photo_frame_sw = !SCENE_CFG.digital_photo_frame_sw;
    scene_value_update(SCENE_ROW_PHOTO_SW, scene_on_off_str(SCENE_CFG.digital_photo_frame_sw));
}

/************************** 数码相框切换时间 **************************/

static void scene_photo_time_left_cb(lv_event_t *ev)
{
    if (--SCENE_CFG.digital_photo_sw_time < 1)
        SCENE_CFG.digital_photo_sw_time = 30;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d %s", SCENE_CFG.digital_photo_sw_time, lang_str_get(STR_S));
    scene_value_update(SCENE_ROW_PHOTO_TIME, buf);
}

static void scene_photo_time_right_cb(lv_event_t *ev)
{
    if (++SCENE_CFG.digital_photo_sw_time > 30)
        SCENE_CFG.digital_photo_sw_time = 1;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d %s", SCENE_CFG.digital_photo_sw_time, lang_str_get(STR_S));
    scene_value_update(SCENE_ROW_PHOTO_TIME, buf);
}

static const char *scene_photo_time_value(void)
{
    static char buf[16];
    snprintf(buf, sizeof(buf), "%d %s", SCENE_CFG.digital_photo_sw_time, lang_str_get(STR_S));
    return buf;
}

/************************** 背景音乐开关 **************************/

static void scene_music_sw_toggle_cb(lv_event_t *ev)
{
    if (user_data_get()->public_cfg.door_device[0].door_cfg.human_detect.detect_level != DETECT_LEVEL_OFF ||
        user_data_get()->public_cfg.door_device[1].door_cfg.human_detect.detect_level != DETECT_LEVEL_OFF)
    {
        lv_common_msgbox_create(251, 106, 586, 388, 62,
                                lang_str_get(STR_MOTION_DETECTION_CLOSE), NULL, NULL);
        return;
    }
    SCENE_CFG.bg_music_sw = !SCENE_CFG.bg_music_sw;
    scene_value_update(SCENE_ROW_MUSIC_SW, scene_on_off_str(SCENE_CFG.bg_music_sw));
}

/************************** 背景音乐音量 **************************/

static void scene_music_vol_left_cb(lv_event_t *ev)
{
    SCENE_CFG.bg_music_vol -= 10;
    if (SCENE_CFG.bg_music_vol < 10)
        SCENE_CFG.bg_music_vol = 100;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", SCENE_CFG.bg_music_vol / 10);
    scene_value_update(SCENE_ROW_MUSIC_VOL, buf);
}

static void scene_music_vol_right_cb(lv_event_t *ev)
{
    SCENE_CFG.bg_music_vol += 10;
    if (SCENE_CFG.bg_music_vol > 100)
        SCENE_CFG.bg_music_vol = 10;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", SCENE_CFG.bg_music_vol / 10);
    scene_value_update(SCENE_ROW_MUSIC_VOL, buf);
}

static const char *scene_music_vol_value(void)
{
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", SCENE_CFG.bg_music_vol / 10);
    return buf;
}

/************************** 行创建 **************************/

static void scene_photo_sw_row_create(void)
{
    scene_row_t row = SCENE_ROW_PHOTO_SW;
    int y = scene_row_y(row);
    int cont_id = SCENE_ROW1_CONT_ID + row * 3;
    int label1_id = SCENE_ROW1_LABEL1_ID + row * 3;
    int label2_id = SCENE_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(STR_DIGITAL_PHOTO), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, VALUE_X, 0, VALUE_W, ROW_H,
        scene_on_off_str(SCENE_CFG.digital_photo_frame_sw), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    scene_arrow_left_create(SCENE_ROW_PHOTO_SW, scene_photo_sw_toggle_cb);
    scene_arrow_right_create(SCENE_ROW_PHOTO_SW, scene_photo_sw_toggle_cb);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static void scene_photo_time_row_create(void)
{
    scene_row_t row = SCENE_ROW_PHOTO_TIME;
    int y = scene_row_y(row);
    int cont_id = SCENE_ROW1_CONT_ID + row * 3;
    int label1_id = SCENE_ROW1_LABEL1_ID + row * 3;
    int label2_id = SCENE_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(STR_DIGITAL_PHOTO_SWITCH_TIME), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, VALUE_X, 0, VALUE_W, ROW_H,
        scene_photo_time_value(), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    scene_arrow_left_create(SCENE_ROW_PHOTO_TIME, scene_photo_time_left_cb);
    scene_arrow_right_create(SCENE_ROW_PHOTO_TIME, scene_photo_time_right_cb);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static void scene_music_sw_row_create(void)
{
    scene_row_t row = SCENE_ROW_MUSIC_SW;
    int y = scene_row_y(row);
    int cont_id = SCENE_ROW1_CONT_ID + row * 3;
    int label1_id = SCENE_ROW1_LABEL1_ID + row * 3;
    int label2_id = SCENE_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(STR_BACKGROUND_MUSIC), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, VALUE_X, 0, VALUE_W, ROW_H,
        scene_on_off_str(SCENE_CFG.bg_music_sw), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    scene_arrow_left_create(SCENE_ROW_MUSIC_SW, scene_music_sw_toggle_cb);
    scene_arrow_right_create(SCENE_ROW_MUSIC_SW, scene_music_sw_toggle_cb);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static void scene_music_vol_row_create(void)
{
    scene_row_t row = SCENE_ROW_MUSIC_VOL;
    int y = scene_row_y(row);
    int cont_id = SCENE_ROW1_CONT_ID + row * 3;
    int label1_id = SCENE_ROW1_LABEL1_ID + row * 3;
    int label2_id = SCENE_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(STR_BACKGROUND_MUSIC_VOLUME), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, VALUE_X, 0, VALUE_W, ROW_H,
        scene_music_vol_value(), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    scene_arrow_left_create(SCENE_ROW_MUSIC_VOL, scene_music_vol_left_cb);
    scene_arrow_right_create(SCENE_ROW_MUSIC_VOL, scene_music_vol_right_cb);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

/************************** 返回按钮 **************************/

static void scene_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

static void scene_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *scene_back = lv_common_img_btn_create(
        lv_scr_act_current(), SCENE_BACK_BTN_ID, 929, 505, 67, 67,
        scene_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(scene_back, 21);
}

/************************** 侧栏标题 **************************/

static void scene_title_create(void)
{
    lv_resource_init(title_icon, ICON_SETTING_SCENE_PNG_OFFSET, ICON_SETTING_SCENE_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SCENE_FOCUS_PNG_OFFSET, ICON_SETTING_SCENE_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), SCENE_TITLE_ICON_ID,
                             30, 218, 130, 130,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), SCENE_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_SCENE_SET), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(scene)
{
    lv_common_bg_create(lv_scr_act_current());

    scene_title_create();
    scene_photo_sw_row_create();
    scene_photo_time_row_create();
    scene_music_sw_row_create();
    scene_music_vol_row_create();
    scene_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(scene)
{
    user_data_save();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(scene);
