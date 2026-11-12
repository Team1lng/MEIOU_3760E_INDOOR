#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "Amy_tfcard.h"
#include "../pwm.h"
#include "Amy_sdk.h"
#include "user_upgrade.h"
#include "Amy_lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* UI 编排的重试次数（最多 2 次） */
static int s_upgrade_attempts = 0;

/* 状态提示框：显示消息，一秒后自动关闭 */
static lv_timer_t *s_upgrade_status_timer = NULL;

/* 进入页面后自动开始门口机升级（由 OTA 下载完成触发） */
static bool s_auto_door_upgrade = false;

/************************** 布局常量 (EMOS-Indoor参考) **************************/
#define ROW_X 199
#define ROW_W 700
#define ROW_H 52
#define ROW1_Y 75
#define ROW_SPACING 52
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 250
#define ARROW_R_X 867
#define ARROW_W 52
#define ARROW_H 52
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 50

/************************** 行索引 **************************/
typedef enum
{
    ADV_ROW_FORMAT_SD = 0,
    ADV_ROW_FACTORY_RESET,
    ADV_ROW_FACTORY_OUTDOOR,
    ADV_ROW_FACTORY_ADMIN,
    ADV_ROW_SOFTWARE_UPDATE,
    ADV_ROW_RESTART,
    ADV_ROW_NUM
} adv_row_t;

/************************** 行标签 ID **************************/
static const xls_str_id_t s_adv_label_ids[] = {
    STR_FORMAT_SD,
    STR_FACTORY_RESET,
    STR_FACTORY_OUTDOOR,
    STR_RESTOR,
    STR_SOFTWARE_UPDATE,
    STR_RESTART_SYSTEM,
};

/************************** 函数声明 **************************/
static void advanced_back_obj_click(lv_event_t *ev);
static void advanced_row_separator_create(adv_row_t row);
static void advanced_arrow_right_create(adv_row_t row, lv_event_cb_t cb);

/************************** 辅助 **************************/

static int adv_row_y(adv_row_t row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static void advanced_row_separator_create(adv_row_t row)
{
    int y = adv_row_y(row);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static void advanced_arrow_right_create(adv_row_t row, lv_event_cb_t cb)
{
    int y = adv_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADVANCED_ROW1_CONT_ID + row * 3), ADVANCED_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADVANCED_ROW1_CONT_ID + row * 3), ADVANCED_ROW1_LABEL2_ID + row * 3));
}

/************************** 弹窗辅助 **************************/

/* 通用 No 回调：关闭当前弹窗 */
static void adv_msgbox_cancel_cb(lv_event_t *ev)
{
    lv_common_msgbox_del();
}

static void adv_upgrade_status_close_cb(lv_timer_t *t)
{
    s_upgrade_status_timer = NULL;
    lv_timer_del(t);
    lv_common_msgbox_del();
}

static void adv_upgrade_status_show(const char *msg)
{
    if (s_upgrade_status_timer != NULL)
    {
        lv_timer_del(s_upgrade_status_timer);
        s_upgrade_status_timer = NULL;
    }
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42, msg);
    s_upgrade_status_timer = lv_timer_create(adv_upgrade_status_close_cb, 1000, NULL);
}

/************************** 格式化 SD 卡 **************************/

static void adv_format_sd_confirm_cb(lv_event_t *ev)
{
    if (Amy_tfcard_status() & SD_STATE_INSERT)
    {
        Amy_tfcard_format();
        lv_common_msgbox_notify_create(329, 172, 366, 256, 42,
                                       lang_str_get(STR_FORMATING));
    }
    else
    {
        adv_upgrade_status_show(lang_str_get(STR_NO_SD_CARD));
    }
}

static void adv_format_sd_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_WANT_FORMAT_SD),
                            adv_format_sd_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 恢复出厂设置 **************************/

static void adv_factory_reset_confirm_cb(lv_event_t *ev)
{
    user_data_reset();
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42,
                                   lang_str_get(STR_FACTORY_RESET_ING));
    backlight_enable(0);
    Amy_msleep(300);
    system("reboot");
}

static void adv_factory_reset_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_WANT_FACTORY_RESET),
                            adv_factory_reset_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 门口机复位 **************************/

static void adv_factory_outdoor_confirm_cb(lv_event_t *ev)
{
    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        const char *ip = user_data_get()->public_cfg.net_door_device[i].ipaddr;
        if (ip[0] == '\0')
            continue;
        sat_ipcamera_device_factory_reset(ip, 80,
                                          USER_NETWORK_AUTH_USER,
                                          USER_NETWORK_AUTH_PASSWORD,
                                          3000, false);
    }
    adv_upgrade_status_show(lang_str_get(STR_FACTORY_RESET_ING));
}

static void adv_factory_outdoor_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_WANT_FACTORY_OUTDOOR),
                            adv_factory_outdoor_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 恢复管理员 **************************/

static void adv_factory_admin_confirm_cb(lv_event_t *ev)
{
    strncpy(user_data_get()->private_cfg.admin.password,
            ADMIN_DEFAULT_PASSWORD, ADMIN_PASSWORD_LEN);
    user_data_get()->private_cfg.device.attr.room = 0;
    user_data_save();
    adv_upgrade_status_show(lang_str_get(STR_FACTORY_RESET_ING));
}

static void adv_factory_admin_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_RESTORE_ADMIN),
                            adv_factory_admin_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 软件升级 **************************/

/* 创建或更新升级进度弹窗（含进度条 + 百分比） */
static void adv_upgrade_progress_display(int percent)
{
    char buf[64] = {0};
    lv_obj_t *bg = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_MSGBOX_BG);
    if (bg == NULL)
    {
        snprintf(buf, sizeof(buf), "%s",
                 lang_str_get(STR_SOFTWARE_UPDATE_ING));
        lv_common_msgbox_progress_create(251, 106, 586, 388, 0, buf);
        return;
    }

    lv_obj_t *msgbox = lv_obj_get_child_by_id(bg, LV_COMMON_MSGBOX_CONT);
    if (msgbox == NULL)
        return;

    lv_obj_t *title = lv_obj_get_child_by_id(msgbox, LV_COMMON_MSGBOX_TITLE);
    if (title != NULL)
    {
        snprintf(buf, sizeof(buf), "%s",
                 lang_str_get(STR_SOFTWARE_UPDATE_ING));
        lv_label_set_text(title, buf);
    }

    lv_obj_t *bar = lv_obj_get_child_by_id(msgbox, LV_COMMON_MSGBOX_PROGRESS_BAR);
    if (bar != NULL)
        lv_bar_set_value(bar, percent, LV_ANIM_OFF);

    lv_obj_t *pct = lv_obj_get_child_by_id(msgbox, LV_COMMON_MSGBOX_PROGRESS_LABEL);
    if (pct != NULL)
    {
        snprintf(buf, sizeof(buf), "%d%%", percent);
        lv_label_set_text(pct, buf);
    }
}

/* 升级状态事件回调（由消息队列派发到主线程） */
static void adv_upgrade_state_callback(void *arg)
{
    door_upgrade_event_t *ev = (door_upgrade_event_t *)arg;

    switch (ev->state)
    {
    case DOOR_UPGRADE_ONLINE:
        /* 关闭检查状态提示框，再创建进度条提示框（检查框无进度条，不能原地更新） */
        lv_common_msgbox_del();
        adv_upgrade_progress_display(0);
        break;
    case DOOR_UPGRADE_PROGRESS:
        adv_upgrade_progress_display(ev->progress);
        break;
    case DOOR_UPGRADE_SUCCESS:
        adv_upgrade_status_show(lang_str_get(STR_UPDATE_SUCCEE));
        break;
    case DOOR_UPGRADE_OFFLINE:
        /* 门机离线：一秒后自动关闭 */
        adv_upgrade_status_show(lang_str_get(STR_DEVICE_OFFLINE));
        break;
    case DOOR_UPGRADE_FAILED:
        if (s_upgrade_attempts < 2)
        {
            /* 重新建立升级流程（最多 2 次） */
            s_upgrade_attempts++;
            if (!outdoor_network_upgrade())
            {
                adv_upgrade_status_show(lang_str_get(STR_DEVICE_OFFLINE));
                return;
            }
            lv_common_msgbox_notify_create(329, 172, 366, 256, 42,
                                           lang_str_get(STR_CHECK_OUTDOOR_STATUS));
        }
        else
        {
            adv_upgrade_status_show(lang_str_get(STR_DEVICE_OFFLINE));
        }
        break;
    default:
        break;
    }
}

/* 启动门口机升级流程（软件升级行确认 / 涂鸦 OTA 下载完成后自动进入共用） */
static void adv_software_update_start(void)
{
    if (outdoor_network_upgrade_is_running())
        return;

    /* 1. 先提示检查门口机状态 */
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42,
                                   lang_str_get(STR_CHECK_OUTDOOR_STATUS));

    /* 2. 无升级文件：显示后一秒自动关闭 */
    const char *fw_path = TFCARD_MOUNT_PATH "/SAT_ANYKA3918OS";
    if (access(fw_path, F_OK) != 0)
    {
        adv_upgrade_status_show(lang_str_get(STR_NO_UPGRAD_FW));
        return;
    }

    /* 3. 启动升级任务，门机离线由事件回调提示 */
    if (!outdoor_network_upgrade())
    {
        adv_upgrade_status_show(lang_str_get(STR_DEVICE_OFFLINE));
        return;
    }

    s_upgrade_attempts = 1;
}

static void adv_software_update_confirm_cb(lv_event_t *ev)
{
    adv_software_update_start();
}

/* 由 page_ota 设置：门口机固件 OTA 下载完成后，进入本页自动开始升级 */
void page_advanced_set_auto_door_upgrade(bool enable)
{
    s_auto_door_upgrade = enable;
}

static void adv_software_update_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_WANT_SOFTWARE_UPDATE),
                            adv_software_update_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 重启系统 **************************/

static void adv_restart_confirm_cb(lv_event_t *ev)
{
    backlight_enable(0);
    Amy_msleep(100);
    system("reboot");
}

static void adv_restart_cb(lv_event_t *ev)
{
    lv_common_msgbox_create(251, 106, 586, 388, 62,
                            lang_str_get(STR_WANT_RESTART_SYSTEM),
                            adv_restart_confirm_cb, adv_msgbox_cancel_cb);
}

/************************** 返回 **************************/

static void advanced_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 行创建 **************************/

static void adv_row_create(adv_row_t row, lv_event_cb_t cb)
{
    int y = adv_row_y(row);
    int cont_id = ADVANCED_ROW1_CONT_ID + row * 3;
    int label1_id = ADVANCED_ROW1_LABEL1_ID + row * 3;
    int label2_id = ADVANCED_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(s_adv_label_ids[row]), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 380, ROW_H,
        lang_str_get(STR_CONFIRM), 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    advanced_row_separator_create(row);
    advanced_arrow_right_create(row, cb);
}

/************************** 返回按钮 **************************/

static void advanced_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *advanced_back = lv_common_img_btn_create(
        lv_scr_act_current(), ADVANCED_BACK_BTN_ID, 929, 505, 67, 67,
        advanced_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(advanced_back, 21);
}

/************************** 侧栏标题 **************************/

static void advanced_title_create(void)
{
    lv_resource_init(title_icon, ICON_SETTING_ADVANCED_PNG_OFFSET, ICON_SETTING_ADVANCED_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_ADVANCED_FOCUS_PNG_OFFSET, ICON_SETTING_ADVANCED_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), ADVANCED_TITLE_ICON_ID,
                             30, 218, 130, 130,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), ADVANCED_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_SENIOR_SET),
                          0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(advanced)
{
    lv_common_bg_create(lv_scr_act_current());

    advanced_title_create();

    adv_row_create(ADV_ROW_FORMAT_SD, adv_format_sd_cb);
    adv_row_create(ADV_ROW_FACTORY_RESET, adv_factory_reset_cb);
    adv_row_create(ADV_ROW_FACTORY_OUTDOOR, adv_factory_outdoor_cb);
    adv_row_create(ADV_ROW_FACTORY_ADMIN, adv_factory_admin_cb);
    adv_row_create(ADV_ROW_SOFTWARE_UPDATE, adv_software_update_cb);
    adv_row_create(ADV_ROW_RESTART, adv_restart_cb);

    advanced_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
    Amy_event_cb_register(AMY_EVENT_ID_UPDATE_DOOR, adv_upgrade_state_callback);

    /* 门口机固件 OTA 下载完成后自动进入升级流程 */
    if (s_auto_door_upgrade)
    {
        s_auto_door_upgrade = false;
        adv_software_update_start();
    }
}

static void Amy_lv_page_quit(advanced)
{
    if (s_upgrade_status_timer != NULL)
    {
        lv_timer_del(s_upgrade_status_timer);
        s_upgrade_status_timer = NULL;
    }
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_UPDATE_DOOR, NULL);
}

Amy_lv_page_create(advanced);
