#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "Amy_tfcard.h"
#include "tuya/tuya_uuid_and_key.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************** 布局常量 (EMOS-Indoor参考) **************************/
#define ROW_X 199
#define ROW_W 700
#define ROW_H 52
#define ROW1_Y 75
#define ROW_SPACING 52
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 250
#define VALUE_X 280
#define VALUE_W 380
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 50

/************************** 行内容 **************************/
static const struct
{
    xls_str_id_t label_id;
    const char *value;
} s_info_rows[] = {
    {STR_HARDWARE_VERSION, "Ver 1.2"},
    {STR_FIRMWARE_VERSION, "ver 1.0"},
    {STR_SOFTWARE_VERSION, SOFT_VERSION},
    {STR_DOOR_1_VERSION, NULL},
    {STR_DOOR_2_VERSION, NULL},
    {STR_RELEASE_DATE, NULL},
    {STR_SD_SIZE, NULL},
    {STR_DEVICE_ID, NULL},
};
#define INFO_ROW_NUM (int)(sizeof(s_info_rows) / sizeof(s_info_rows[0]))

static const char *system_info_release_date(void)
{
    static char date[16];
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char temp[] = __DATE__; /* "Mmm dd yyyy" */
    int year = atoi(temp + 7);
    temp[6] = '\0';
    int day = atoi(temp + 4);
    temp[3] = '\0';
    int month = 1;
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(temp, months[i]) == 0)
        {
            month = i + 1;
            break;
        }
    }
    snprintf(date, sizeof(date), "%d-%02d-%02d", year, month, day);
    return date;
}

static const char *system_info_sd_size(void)
{
    static char size[32];
    unsigned long total_kb = 0, used_kb = 0, free_kb = 0;
    if (tfcard_capacity_get(&total_kb, &used_kb, &free_kb) == 0)
    {
        snprintf(size, sizeof(size), "%.2fG/%.2fG",
                 free_kb / (1024.0 * 1024.0), total_kb / (1024.0 * 1024.0));
    }
    else
    {
        snprintf(size, sizeof(size), "0.00G/0.00G");
    }
    return size;
}

static const char *system_info_device_id(void)
{
    static char id[64];
    unsigned char uuid[64] = {0};
    unsigned char key[64] = {0};
    id[0] = '\0';
    if (tuya_uuid_and_key_read(uuid, key))
    {
        snprintf(id, sizeof(id), "%s", (char *)uuid);
    }
    return id;
}

static const char *system_info_row_value(int i)
{
    switch (s_info_rows[i].label_id)
    {
    case STR_DOOR_1_VERSION:
        return page_common_door_version_get(0);
    case STR_DOOR_2_VERSION:
        return page_common_door_version_get(1);
    case STR_RELEASE_DATE:
        return system_info_release_date();
    case STR_SD_SIZE:
        return system_info_sd_size();
    case STR_DEVICE_ID:
        return system_info_device_id();
    default:
        return s_info_rows[i].value;
    }
}

/************************** 函数声明 **************************/
static void system_info_back_obj_click(lv_event_t *ev);

/************************** 回调实现 **************************/

static void system_info_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 行创建 **************************/

static void system_info_rows_create(void)
{
    int row = 0;
    for (int i = 0; i < INFO_ROW_NUM; i++)
    {
        /* 门口机不在线时隐藏对应版本行，其余行紧凑上移 */
        if ((s_info_rows[i].label_id == STR_DOOR_1_VERSION && !page_common_door_online_get(0)) ||
            (s_info_rows[i].label_id == STR_DOOR_2_VERSION && !page_common_door_online_get(1)))
            continue;

        int y = ROW1_Y + ROW_SPACING * row;
        int cont_id = SYSTEM_INFO_ROW1_CONT_ID + i * 3;
        int label1_id = SYSTEM_INFO_ROW1_LABEL1_ID + i * 3;
        int label2_id = SYSTEM_INFO_ROW1_LABEL2_ID + i * 3;

        lv_common_setting_btn_double_label_create(
            lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
            NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
            label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
            lang_str_get(s_info_rows[i].label_id), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
            label2_id, VALUE_X, 0, VALUE_W, ROW_H,
            system_info_row_value(i), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

        lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
        row++;
    }
}

/************************** 返回按钮 **************************/

static void system_info_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *system_info_back = lv_common_img_btn_create(
        lv_scr_act_current(), SYSTEM_INFO_BACK_BTN_ID, 929, 505, 67, 67,
        system_info_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(system_info_back, 21);
}

/************************** 侧栏标题 **************************/

static void system_info_title_create(void)
{
    lv_resource_init(title_icon, ICON_SETTING_SYSTEM_INFO_PNG_OFFSET, ICON_SETTING_SYSTEM_INFO_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SYSTEM_INFO_FOCUS_PNG_OFFSET, ICON_SETTING_SYSTEM_INFO_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), SYSTEM_INFO_TITLE_ICON_ID,
                             30, 218, 130, 130,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), SYSTEM_INFO_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_SYSTEM_INFO),
                          0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(system_info)
{
    lv_common_bg_create(lv_scr_act_current());

    system_info_title_create();
    system_info_rows_create();
    system_info_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(system_info)
{
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(system_info);
