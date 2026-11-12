#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"

/************************** 全局上下文 **************************/
int g_ring_door = 0;

/************************** 布局常量 **************************/
#define PANEL_W 556
#define DOOR_ICON_X 30
#define DOOR1_ICON_Y 151
#define DOOR2_ICON_Y 310
#define DOOR_ICON_W 130
#define DOOR_ICON_H 130
#define DOOR1_LABEL_Y 271
#define DOOR2_LABEL_Y 430
#define DOOR_SEL_COLOR 0xff0303
#define PANEL_TITLE_X 300
#define PANEL_TITLE_Y 29
#define ROW_X 199
#define ROW_W 700
#define LABEL_W 260
#define LABEL_OFS_X 0
#define LABEL_OFS_Y 6
#define ROW_H 52
#define ROW1_Y 76
#define ROW_SPACING 52
#define SEP_OFFSET_Y 50
#define SEP_X 472
#define SEP_W 422
#define ARROW_L_X 478
#define ARROW_R_X 869

/************************** 行内容 **************************/
static const struct
{
    xls_str_id_t label_id;
    bool left_arrow;
} s_door_rows[] = {
    {STR_STATE, true},
    {STR_UNLOCK_TIME, true},
    {STR_UNGATE_TIME, true},
    {STR_RECORD_MODE, true},
    {STR_MOTION_DETECTION, true},
    {STR_MOTION_DETECT_RECORD_MODE, true},
    {STR_MOTION_DETECTION_DURATION, true},
    {STR_MESSAGE, true},
    {STR_MESSAGE_TIME, true},
    {STR_RING_SETTING, false},
};
#define DOOR_ROW_NUM (int)(sizeof(s_door_rows) / sizeof(s_door_rows[0]))

/************************** 移动侦测时长选项（Row 6） **************************/
/* 10~20s 步进 1，20s 之后步进 10，最大 300s */
#define MD_DURATION_MIN 10
#define MD_DURATION_STEP_END 20
#define MD_DURATION_MAX 300
#define MD_DURATION_STEP_BASE (MD_DURATION_STEP_END - MD_DURATION_MIN) /* 步进1段的末索引: 10 */
#define MD_DURATION_COUNT (MD_DURATION_STEP_BASE + 1 + (MD_DURATION_MAX - MD_DURATION_STEP_END) / 10) /* 11 + 28 = 39 */

static int door_md_duration_secs(int idx)
{
    if (idx <= MD_DURATION_STEP_BASE)
        return MD_DURATION_MIN + idx;
    if (idx >= MD_DURATION_COUNT - 1)
        return MD_DURATION_MAX;
    return MD_DURATION_STEP_END + (idx - MD_DURATION_STEP_BASE) * 10;
}

static int door_md_duration_to_idx(uint32_t secs)
{
    if (secs <= MD_DURATION_MIN)
        return 0;
    if (secs <= MD_DURATION_STEP_END)
        return (int)secs - MD_DURATION_MIN;
    if (secs >= MD_DURATION_MAX)
        return MD_DURATION_COUNT - 1;

    int step = (int)((secs - MD_DURATION_STEP_END + 5) / 10);
    if (step < 1)
        step = 1;
    return MD_DURATION_STEP_BASE + step;
}

/************************** 行值选项 **************************/
static const int s_door_option_counts[] = {2, 10, 10, 2, 4, 2, MD_DURATION_COUNT, 2, 3};

static int s_door_value_idx[DOOR_ROW_NUM];

static int door_option_count(int row)
{
    return s_door_option_counts[row];
}

static const char *door_value_get_text(int row, int idx)
{
    static char buf[16];

    switch (row)
    {
    case 0:
        return lang_str_get(idx == 0 ? STR_ON : STR_OFF);
    case 1:
    case 2:
        snprintf(buf, sizeof(buf), "%d%s", idx + 1, lang_str_get(STR_S));
        return buf;
    case 3:
        return lang_str_get(idx == 0 ? STR_VIDEO : STR_SNAP);
    case 4:
        return lang_str_get((int[]){STR_OFF, STR_LOW, STR_MEDIUM, STR_HIGH}[idx]);
    case 5:
        return lang_str_get(idx == 0 ? STR_SNAP : STR_VIDEO);
    case 6:
        snprintf(buf, sizeof(buf), "%d%s", door_md_duration_secs(idx), lang_str_get(STR_S));
        return buf;
    case 7:
        return lang_str_get(idx == 0 ? STR_ON : STR_OFF);
    case 8:
        snprintf(buf, sizeof(buf), "%d%s", (int[]){30, 60, 120}[idx], lang_str_get(STR_S));
        return buf;
    default:
        return "";
    }
}

/************************** 配置映射：索引 ↔ user_data 字段值 **************************/

/* Row 0: Status → device_valid */
static int door_status_to_idx(bool valid) { return valid ? 0 : 1; }
static bool door_idx_to_status(int idx) { return idx == 0; }

/* Row 1/2: Lock/Gate unlock time → gate_lock_close_time / garage_lock_close_time (1-10) */
static int door_lock_time_to_idx(uint16_t t)
{
    if (t < 1)
        return 0;
    if (t > 10)
        return 9;
    return t - 1;
}
static uint16_t door_idx_to_lock_time(int idx) { return (uint16_t)(idx + 1); }

/* Row 3: Record mode → global_record_mode */
static int door_record_mode_to_idx(record_mode_type_def m)
{
    return m == RECORD_MODE_VIDEO ? 0 : 1;
}
static record_mode_type_def door_idx_to_record_mode(int idx)
{
    return idx == 0 ? RECORD_MODE_VIDEO : RECORD_MODE_PHOTO;
}

/* Row 4: Motion detection → human_detect.detect_level (低/中/高/关) */

/* Row 5: MD Record mode → human_detect.record_mode */
static int door_md_record_to_idx(record_mode_type_def m)
{
    return m == RECORD_MODE_PHOTO ? 0 : 1;
}
static record_mode_type_def door_idx_to_md_record(int idx)
{
    return idx == 0 ? RECORD_MODE_PHOTO : RECORD_MODE_VIDEO;
}

/* Row 6: Motion detection duration → human_detect.alarm_interval (秒) */

/* Row 7: Message → message_sw */
static int door_message_sw_to_idx(bool v) { return v ? 0 : 1; }
static bool door_idx_to_message_sw(int idx) { return idx == 0; }

/* Row 8: Message time → message_time (30/60/120) */
static int door_message_time_to_idx(uint8_t t)
{
    if (t <= 30)
        return 0;
    if (t <= 60)
        return 1;
    return 2;
}
static uint8_t door_idx_to_message_time(int idx)
{
    switch (idx)
    {
    case 0:
        return 30;
    case 1:
        return 60;
    case 2:
        return 120;
    default:
        return 30;
    }
}

/************************** 辅助：label 查找（行都含 CONT，先找CONT再找label） **************************/
static lv_obj_t *door_row_label_get(int label_base_id, int row)
{
    return lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), DOOR_ROW1_CONT_ID + row * 3), label_base_id + row * 3);
}

/************************** 行创建辅助 **************************/

static int door_row_y(int row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static void door_row_separator_create(int row)
{
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, door_row_y(row) + SEP_OFFSET_Y, SEP_W);
}

static lv_obj_t *door_row_btn_create(int row, const char *label_text,
                                     const char *value_text, lv_event_cb_t cb)
{
    int y = door_row_y(row);
    int cont_id = DOOR_ROW1_CONT_ID + row * 3;
    int label1_id = DOOR_ROW1_LABEL1_ID + row * 3;
    int label2_id = DOOR_ROW1_LABEL2_ID + row * 3;

    lv_obj_t *cont = lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        label_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 380, ROW_H,
        value_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    door_row_separator_create(row);
    return cont;
}

static void door_btn_value_update(int row, const char *text)
{
    lv_obj_t *btn = door_row_label_get(DOOR_ROW1_LABEL2_ID, row);
    if (btn && text)
        lv_label_set_text(btn, text);
}

static void door_arrow_left_create(int row, lv_event_cb_t cb)
{
    int y = door_row_y(row) + 2;
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_L_X, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_user_data(arrow, (void *)(intptr_t)row);
    lv_common_arrow_link_press(arrow,
                               door_row_label_get(DOOR_ROW1_LABEL1_ID, row),
                               door_row_label_get(DOOR_ROW1_LABEL2_ID, row));
}

static void door_arrow_right_create(int row, lv_event_cb_t cb)
{
    int y = door_row_y(row) + 2;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_user_data(arrow, (void *)(intptr_t)row);
    lv_common_arrow_link_press(arrow,
                               door_row_label_get(DOOR_ROW1_LABEL1_ID, row),
                               door_row_label_get(DOOR_ROW1_LABEL2_ID, row));
}

/************************** 页面上下文 **************************/
static struct
{
    int cur_door; /* 1 或 2 */
} s_door_ctx;

/************************** 配置加载/保存 **************************/

static device_single_cfg *door_cur_device(void)
{
    return &user_data_get()->public_cfg.door_device[s_door_ctx.cur_door - 1];
}

static void door_load_config(void)
{
    device_single_cfg *dev = door_cur_device();
    door_config_type_def *cfg = &dev->door_cfg;

    s_door_value_idx[0] = door_status_to_idx(dev->device_valid);
    s_door_value_idx[1] = door_lock_time_to_idx(cfg->gate_lock_close_time);
    s_door_value_idx[2] = door_lock_time_to_idx(cfg->garage_lock_close_time);
    s_door_value_idx[3] = door_record_mode_to_idx(cfg->global_record_mode);
    s_door_value_idx[4] = cfg->human_detect.detect_level;
    s_door_value_idx[5] = door_md_record_to_idx(cfg->human_detect.record_mode);
    s_door_value_idx[6] = door_md_duration_to_idx(cfg->human_detect.alarm_interval);
    s_door_value_idx[7] = door_message_sw_to_idx(cfg->message_sw);
    s_door_value_idx[8] = door_message_time_to_idx(cfg->message_time);

    for (int i = 0; i < DOOR_ROW_NUM - 1; i++)
        door_btn_value_update(i, door_value_get_text(i, s_door_value_idx[i]));
}

static void door_unlock_param_sync(int lock_type)
{
    struct ipcamera_info *dev = &user_data_get()->public_cfg.net_door_device[s_door_ctx.cur_door - 1];
    if (dev->ipaddr[0] == '\0')
        return;

    door_config_type_def *cfg = &door_cur_device()->door_cfg;
    int lock_time = (lock_type == 0) ? cfg->gate_lock_close_time : cfg->garage_lock_close_time;
    sat_ipcamera_device_unlock_param(dev->ipaddr, dev->port, dev->username, dev->password,
                                     lock_time, lock_type, 1000, false);
}

static void door_human_detect_sync(detect_level_type_def level)
{
    struct ipcamera_info *dev = &user_data_get()->public_cfg.net_door_device[s_door_ctx.cur_door - 1];
    if (dev->ipaddr[0] == '\0')
        return;

    /* 门口机当前仅识别 0=关 / 非0=开，灵敏度档位后续在门口机侧配置 */
    sat_ipcamera_device_human_detect_ctrl(dev->ipaddr, dev->port, dev->username, dev->password,
                                          level, 1000, false);
}

static void door_save_row(int row)
{
    device_single_cfg *dev = door_cur_device();
    door_config_type_def *cfg = &dev->door_cfg;
    int idx = s_door_value_idx[row];

    switch (row)
    {
    case 0:
        dev->device_valid = door_idx_to_status(idx);
        break;
    case 1:
        cfg->gate_lock_close_time = door_idx_to_lock_time(idx);
        door_unlock_param_sync(0);
        break;
    case 2:
        cfg->garage_lock_close_time = door_idx_to_lock_time(idx);
        door_unlock_param_sync(1);
        break;
    case 3:
        cfg->global_record_mode = door_idx_to_record_mode(idx);
        if (cfg->global_record_mode == RECORD_MODE_PHOTO)
        {
            cfg->message_sw = false;
            s_door_value_idx[7] = door_message_sw_to_idx(false);
            door_btn_value_update(7, door_value_get_text(7, s_door_value_idx[7]));
        }
        break;
    case 4:
        cfg->human_detect.detect_level = (detect_level_type_def)idx;
        door_human_detect_sync(cfg->human_detect.detect_level);
        break;
    case 5:
        cfg->human_detect.record_mode = door_idx_to_md_record(idx);
        break;
    case 6:
        cfg->human_detect.alarm_interval = (uint32_t)door_md_duration_secs(idx);
        break;
    case 7:
        cfg->message_sw = door_idx_to_message_sw(idx);
        if (cfg->message_sw)
        {
            cfg->global_record_mode = RECORD_MODE_VIDEO;
            s_door_value_idx[3] = door_record_mode_to_idx(cfg->global_record_mode);
            door_btn_value_update(3, door_value_get_text(3, s_door_value_idx[3]));
        }
        break;
    case 8:
        cfg->message_time = door_idx_to_message_time(idx);
        break;
    default:
        break;
    }
}

/************************** 函数声明 **************************/
static void door_door1_obj_click(lv_event_t *ev);
static void door_door2_obj_click(lv_event_t *ev);
static void door_ring_setting_click(lv_event_t *ev);
static void door_back_obj_click(lv_event_t *ev);

static void door_select(int door);
static void door_selector_create(void);
static void door_rows_create(void);
static void door_back_create(void);

/************************** 回调实现 **************************/

static void door_door1_obj_click(lv_event_t *ev)
{
    door_select(1);
}

static void door_door2_obj_click(lv_event_t *ev)
{
    door_select(2);
}

static void door_ring_setting_click(lv_event_t *ev)
{
    g_ring_door = s_door_ctx.cur_door - 1;
    Amy_lv_page_goto(lv_page(ring), LV_SCR_LOAD_ANIM_NONE, );
}

static void door_row_left_cb(lv_event_t *ev)
{
    lv_obj_t *btn = lv_event_get_target(ev);
    int row = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (row == 4)
    {
        if (user_data_get()->private_cfg.scene.digital_photo_frame_sw ||
            user_data_get()->private_cfg.scene.bg_music_sw)
        {
            lv_common_msgbox_create(251, 106, 586, 388, 62,
                                    lang_str_get(STR_CLOSE_SCENE_MODE), NULL, NULL);
            return;
        }
    }
    if (row == 3 && user_data_get()->public_cfg.global_func.scene_mode == SCENE_MODE_AWAY)
    {
        lv_common_msgbox_create(251, 106, 586, 388, 62,
                                lang_str_get(STR_IN_NOT_AT_HOME), NULL, NULL);
        return;
    }
    int count = door_option_count(row);
    if (count <= 1)
        return;
    s_door_value_idx[row] = (s_door_value_idx[row] <= 0) ? count - 1 : s_door_value_idx[row] - 1;
    door_btn_value_update(row, door_value_get_text(row, s_door_value_idx[row]));
    door_save_row(row);
}

static void door_row_right_cb(lv_event_t *ev)
{
    lv_obj_t *btn = lv_event_get_target(ev);
    int row = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (row == 4)
    {
        if (user_data_get()->private_cfg.scene.digital_photo_frame_sw ||
            user_data_get()->private_cfg.scene.bg_music_sw)
        {
            lv_common_msgbox_create(251, 106, 586, 388, 62,
                                    lang_str_get(STR_CLOSE_SCENE_MODE), NULL, NULL);
            return;
        }
    }
    if (row == 3 && user_data_get()->public_cfg.global_func.scene_mode == SCENE_MODE_AWAY)
    {
        lv_common_msgbox_create(251, 106, 586, 388, 62,
                                lang_str_get(STR_IN_NOT_AT_HOME), NULL, NULL);
        return;
    }
    int count = door_option_count(row);
    if (count <= 1)
        return;
    s_door_value_idx[row] = (s_door_value_idx[row] >= count - 1) ? 0 : s_door_value_idx[row] + 1;
    door_btn_value_update(row, door_value_get_text(row, s_door_value_idx[row]));
    door_save_row(row);
}

static void door_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 创建函数 **************************/

static void door_select(int door)
{
    if (door == s_door_ctx.cur_door)
    {
        return;
    }
    s_door_ctx.cur_door = door;
    lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), DOOR_DOOR1_LABEL_ID), lv_color_hex(door == 1 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), DOOR_DOOR2_LABEL_ID), lv_color_hex(door == 2 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
    lv_label_set_text(lv_obj_get_child_by_id(lv_scr_act_current(), DOOR_PANEL_TITLE_ID), door == 1 ? lang_str_get(STR_DOOR1) : lang_str_get(STR_DOOR2));
    door_load_config();
}

static void door_selector_create(void)
{
    lv_resource_init(door_icon, ICON_SETTING_DOOR_PNG_OFFSET, ICON_SETTING_DOOR_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_DOOR_FOCUS_PNG_OFFSET, ICON_SETTING_DOOR_FOCUS_PNG_SIZE);
    lv_obj_t *door1 = lv_common_img_btn_create(lv_scr_act_current(), DOOR_DOOR1_BTN_ID, DOOR_ICON_X, DOOR1_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
                                               door_door1_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(door1, 21);
    lv_common_text_create(lv_scr_act_current(), DOOR_DOOR1_LABEL_ID, 0, DOOR1_LABEL_Y, SIDEBAR_W, ROW_H,
                          door_door1_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_DOOR1), DOOR_SEL_COLOR, 0x02A9FF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    lv_obj_t *door2 = lv_common_img_btn_create(lv_scr_act_current(), DOOR_DOOR2_BTN_ID, DOOR_ICON_X, DOOR2_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
                                               door_door2_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(door2, 21);
    lv_common_text_create(lv_scr_act_current(), DOOR_DOOR2_LABEL_ID, 0, DOOR2_LABEL_Y, SIDEBAR_W, ROW_H,
                          door_door2_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_DOOR2), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

static void door_rows_create(void)
{
    for (int i = 0; i < DOOR_ROW_NUM - 1; i++)
    {
        door_row_btn_create(i, lang_str_get(s_door_rows[i].label_id), door_value_get_text(i, 0), NULL);
        if (s_door_rows[i].left_arrow)
            door_arrow_left_create(i, door_row_left_cb);
        door_arrow_right_create(i, door_row_right_cb);
    }

    /* 最后一行："Ring setting" 可点击跳转 */
    {
        int i = DOOR_ROW_NUM - 1;
        door_row_btn_create(i, lang_str_get(s_door_rows[i].label_id),
                            lang_str_get(s_door_rows[i].label_id), door_ring_setting_click);
        door_arrow_right_create(i, door_ring_setting_click);
    }
}

static void door_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *door_back = lv_common_img_btn_create(lv_scr_act_current(), DOOR_BACK_BTN_ID, 929, 505, 67, 67,
                                                   door_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(door_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(door)
{
    s_door_ctx.cur_door = 1;

    lv_common_bg_create(lv_scr_act_current());

    door_selector_create();
    lv_common_text_create(lv_scr_act_current(), DOOR_PANEL_TITLE_ID, PANEL_TITLE_X, PANEL_TITLE_Y, 424, ROW_H,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_DOOR1), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
    door_rows_create();
    door_back_create();
    door_load_config();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(door)
{
    user_data_save();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(door);
