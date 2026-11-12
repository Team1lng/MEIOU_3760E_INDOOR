#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "device_types.h"
#include "call_api.h"
#include "page_call.h"
#include "page_common_call.h"
#include "user_standby.h"
#include "user_data.h"
#include "sat_user_common.h"
#include <stdio.h>

/************************** 布局常量 **************************/
#define BTN_W 160
#define BTN_H 160
#define BTN_Y 220
#define BTN_GAP 40

/************************** 选中设备（transfer 页面设置） **************************/
int g_call_selected_dev = 0;

void call_set_selected_dev(int dev)
{
    g_call_selected_dev = dev;
}

/************************** 页面上下文 **************************/
static struct
{
    intercom_status status;
    lv_timer_t *no_interact_task;
} s_ctx = {.status = INTERCOM_UNKNOW};

void intercom_status_set(intercom_status status)
{
    s_ctx.status = status;
}

intercom_status intercom_status_get(void)
{
    return s_ctx.status;
}

/************************** 函数声明 **************************/
static void dial_btn_click(lv_event_t *ev);
static void hangup_btn_click(lv_event_t *ev);
static void back_btn_click(lv_event_t *ev);

static void buttons_create(void);
static void callee_label_create(void);
static void back_btn_create(void);
static void call_ui_update(void);

static void call_event_callback(void *arg);
static void incoming_handle(device_info_attr device_info);
static void outgoing_ringing_handle(device_info_attr device_info);
static void connected_handle(device_info_attr device_info);
static void end_handle(call_cb_attr *attr);

static void audio_play_start_cb(int arg);
static int audio_play_finish_cb(int arg);

static void no_interact_timer_start(void);
static void no_interact_timer_stop(void);

/************************** 按钮事件 **************************/

static void dial_btn_click(lv_event_t *ev)
{
    if (s_ctx.status == INTERCOM_INCOMING)
    {
        no_interact_timer_stop();
        sat_call_answer_current();
        return;
    }

    if (g_call_selected_dev == 0 || g_call_selected_dev == DEVICE_TYPE_ALL)
        return;

    device_info_attr dev_info = SAT_DEVICE_INFO_DEFAULT;
    dev_info.building = user_data_get()->private_cfg.device.attr.building;
    dev_info.room = user_data_get()->private_cfg.device.attr.room;
    dev_info.device_type = g_call_selected_dev;

    sat_call_dial(&dev_info);
    s_ctx.status = INTERCOM_OUTGOING;
    intercom_device_info_set(&dev_info);
    call_ui_update();
    no_interact_timer_start();
}

static void hangup_btn_click(lv_event_t *ev)
{
    sat_call_hangup_current();
    common_call_session_complete_handle();
}

static void back_btn_click(lv_event_t *ev)
{
    s_ctx.status = INTERCOM_UNKNOW;
    sat_call_hangup_current();
    common_call_session_complete_handle();
}

/************************** UI 更新 **************************/

static void call_ui_update(void)
{
    lv_obj_t *dial = lv_obj_get_child_by_id(lv_scr_act_current(), CALL_DIAL_BTN_ID);
    lv_obj_t *hangup = lv_obj_get_child_by_id(lv_scr_act_current(), CALL_HANGUP_BTN_ID);

    if (dial)
        lv_obj_add_flag(dial, LV_OBJ_FLAG_HIDDEN);
    if (hangup)
        lv_obj_add_flag(hangup, LV_OBJ_FLAG_HIDDEN);

    int left_x = (SCREEN_W - BTN_W * 2 - BTN_GAP) / 2;
    int right_x = left_x + BTN_W + BTN_GAP;
    int center_x = (SCREEN_W - BTN_W) / 2;

    switch (s_ctx.status)
    {
    case INTERCOM_INCOMING:
        if (dial)
        {
            lv_obj_set_style_x(dial, left_x, LV_PART_MAIN);
            lv_obj_clear_flag(dial, LV_OBJ_FLAG_HIDDEN);
        }
        if (hangup)
        {
            lv_obj_set_style_x(hangup, right_x, LV_PART_MAIN);
            lv_obj_clear_flag(hangup, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case INTERCOM_OUTGOING:
    case INTERCOM_TALKING:
        if (hangup)
        {
            lv_obj_set_style_x(hangup, center_x, LV_PART_MAIN);
            lv_obj_clear_flag(hangup, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    default:
        if (dial)
        {
            lv_obj_set_style_x(dial, center_x, LV_PART_MAIN);
            lv_obj_clear_flag(dial, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    }
}

/************************** 会话事件处理 **************************/

static void incoming_handle(device_info_attr device_info)
{
    device_info_attr current_device;

    if (sat_call_get_current_call(&current_device) && memcmp(&device_info, &current_device, sizeof(device_info_attr)) == 0)
    {
        common_call_incoming_handle(device_info);
    }
}

static void outgoing_ringing_handle(device_info_attr device_info)
{
    if (user_data_get()->private_cfg.mute_enable)
        return;
    Amy_audio_play(1, 80, true, audio_play_start_cb, audio_play_finish_cb);
}

static void connected_handle(device_info_attr device_info)
{
    s_ctx.status = INTERCOM_TALKING;
    no_interact_timer_stop();
    intercom_device_info_set(&device_info);
    callee_label_create();
    intercom_audio_open();
    call_ui_update();
    Amy_audio_play_stop();
}

static void call_end_notify_timer_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    common_call_session_complete_handle();
}

/* 内线未接通（呼入振铃/呼出等待）超时：挂断并提示等待超时 */
static void no_interact_timeout_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    s_ctx.no_interact_task = NULL;

    sat_call_hangup_current();
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42, lang_str_get(STR_WAIT_TIMEOUT));
    lv_timer_create_recycle(call_end_notify_timer_cb, 800, NULL);
}

static void no_interact_timer_start(void)
{
    no_interact_timer_stop();
    s_ctx.no_interact_task = lv_timer_create(no_interact_timeout_cb, CALL_DEFAULT_NO_INTERACT_TIMEOUT_MS, NULL);
}

static void no_interact_timer_stop(void)
{
    if (s_ctx.no_interact_task != NULL)
    {
        lv_timer_del(s_ctx.no_interact_task);
        s_ctx.no_interact_task = NULL;
    }
}

static void end_handle(call_cb_attr *attr)
{
    device_info_attr cur_dev;
    bool is_my_session = (intercom_get_current_device(&cur_dev) == 0 &&
                          memcmp(&attr->device_info, &cur_dev, sizeof(device_info_attr)) == 0);
    if (is_my_session)
    {
        no_interact_timer_stop();
        Amy_audio_play_stop();
        switch (attr->reason)
        {
        case REASON_BUSY:
            lv_common_msgbox_notify_create(329, 172, 366, 256, 42, lang_str_get(STR_DEVICE_BUSY));
            lv_timer_create_recycle(call_end_notify_timer_cb, 800, NULL);
            break;
        case REASON_NO_RESPONSE:
            lv_common_msgbox_notify_create(329, 172, 366, 256, 42, lang_str_get(STR_CALL_TIMEOUT));
            lv_timer_create_recycle(call_end_notify_timer_cb, 800, NULL);
            break;
        case REASON_NO_INTERACT_TIMEOUT:
            lv_common_msgbox_notify_create(329, 172, 366, 256, 42, lang_str_get(STR_WAIT_TIMEOUT));
            lv_timer_create_recycle(call_end_notify_timer_cb, 800, NULL);
            break;
        case REASON_MAX_DURATION_TIMEOUT:
            lv_common_msgbox_notify_create(329, 172, 366, 256, 42, lang_str_get(STR_CALL_TIMED_OUT));
            lv_timer_create_recycle(call_end_notify_timer_cb, 800, NULL);
            break;
        case REASON_HANGUP:
            common_call_session_complete_handle();
            break;
        default:
            break;
        }
    }
}

static void call_event_callback(void *arg)
{
    call_cb_attr *attr = (call_cb_attr *)arg;
    switch (attr->call_event)
    {
    case AMY_CALL_EVENT_ID_CALL_INCOMING:
        incoming_handle(attr->device_info);
        break;
    case AMY_CALL_EVENT_ID_CALL_OUTGOING_RINGING:
        outgoing_ringing_handle(attr->device_info);
        break;
    case AMY_CALL_EVENT_ID_CALL_CONNECTED:
        connected_handle(attr->device_info);
        break;
    case AMY_CALL_EVENT_ID_CALL_END:
        end_handle(attr);
        break;
    default:
        break;
    }
}

/************************** 音频回调 **************************/

static void audio_play_start_cb(int arg)
{
}

static int audio_play_finish_cb(int arg)
{
    return 1;
}

/************************** UI 创建 **************************/

static void buttons_create(void)
{
    lv_resource_init(icon_dial, ICON_TRANSFER_DIAL_CALL_PNG_OFFSET, ICON_TRANSFER_DIAL_CALL_PNG_SIZE);
    lv_resource_init(icon_dial_focus, ICON_TRANSFER_DIAL_CALL_FOCUS_PNG_OFFSET, ICON_TRANSFER_DIAL_CALL_FOCUS_PNG_SIZE);
    lv_obj_t *dial = lv_common_img_btn_create(lv_scr_act_current(), CALL_DIAL_BTN_ID,
                                              432, BTN_Y, BTN_W, BTN_H,
                                              dial_btn_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &icon_dial, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_dial_focus);
    lv_obj_set_ext_click_area(dial, 21);

    lv_resource_init(icon_hangup, ICON_TRANSFER_DIAL_HANGU_PNG_OFFSET, ICON_TRANSFER_DIAL_HANGU_PNG_SIZE);
    lv_resource_init(icon_hangup_focus, ICON_TRANSFER_DIAL_HANGUP_FOCUS_PNG_OFFSET, ICON_TRANSFER_DIAL_HANGUP_FOCUS_PNG_SIZE);
    lv_obj_t *hangup = lv_common_img_btn_create(lv_scr_act_current(), CALL_HANGUP_BTN_ID,
                                                422, BTN_Y, BTN_W, BTN_H,
                                                hangup_btn_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                &icon_hangup, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_hangup_focus);
    lv_obj_set_ext_click_area(hangup, 21);
    lv_obj_add_flag(hangup, LV_OBJ_FLAG_HIDDEN);
}

static void callee_label_create(void)
{
    lv_obj_t *old_remote = lv_obj_get_child_by_id(lv_scr_act_current(), CALL_REMOTE_ID_LABEL_ID);
    if (old_remote)
        lv_obj_del(old_remote);
    lv_obj_t *old_local = lv_obj_get_child_by_id(lv_scr_act_current(), CALL_LOCAL_ID_LABEL_ID);
    if (old_local)
        lv_obj_del(old_local);

    char buf[64];
    if (s_ctx.status == INTERCOM_INCOMING || s_ctx.status == INTERCOM_TALKING)
    {
        device_info_attr device_info = SAT_DEVICE_INFO_DEFAULT;
        if (intercom_get_current_device(&device_info) == 0)
        {
            int id = device_info.device_type - DEVICE_TYPE_INDOOR_ID1 + 1;
            snprintf(buf, sizeof(buf), "%s: %d", lang_str_get(STR_CALLED_DEVICE_ID), id);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%s: --", lang_str_get(STR_CALLED_DEVICE_ID));
        }
    }
    else
    {
        if (g_call_selected_dev == DEVICE_TYPE_INDOOR_FAMILY)
            snprintf(buf, sizeof(buf), "%s: ALL", lang_str_get(STR_CALLED_DEVICE_ID));
        else if (g_call_selected_dev >= DEVICE_TYPE_INDOOR_ID1 && g_call_selected_dev <= DEVICE_TYPE_INDOOR_ID6)
            snprintf(buf, sizeof(buf), "%s: %d", lang_str_get(STR_CALLED_DEVICE_ID), g_call_selected_dev);
        else
            snprintf(buf, sizeof(buf), "%s: --", lang_str_get(STR_CALLED_DEVICE_ID));
    }

    lv_common_text_create(lv_scr_act_current(), CALL_REMOTE_ID_LABEL_ID,
                          42, 42, 200, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          buf, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

    char local_buf[64];
    snprintf(local_buf, sizeof(local_buf), "%s: %d", lang_str_get(STR_LOCAL_DEVICE_ID),
             user_data_get()->private_cfg.device.attr.device_type);
    lv_common_text_create(lv_scr_act_current(), CALL_LOCAL_ID_LABEL_ID,
                          782, 42, 200, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          local_buf, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_RIGHT, lv_font_normal);
}

static void back_btn_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), CALL_BACK_BTN_ID, 929, 505, 67, 67,
                                              back_btn_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(back, 21);
}

/************************** OTA 事件 **************************/

static void ota_event_cb(void *arg)
{
    ota_event_t *ev = (ota_event_t *)arg;
    if (ev->state != OTA_STATE_DOWNLOAD_START)
        return;
    sat_call_hangup_all();
    intercom_audio_close();
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(call)
{
    standby_timer_close();
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);

    if (s_ctx.status != INTERCOM_INCOMING)
        s_ctx.status = INTERCOM_UNKNOW;

    callee_label_create();
    buttons_create();
    back_btn_create();
    call_ui_update();

    if (s_ctx.status == INTERCOM_INCOMING)
        no_interact_timer_start();

    if (s_ctx.status == INTERCOM_INCOMING &&
        user_data_get()->public_cfg.global_func.scene_mode != SCENE_MODE_SLEEP)
        Amy_audio_play(1, 80, true, audio_play_start_cb, audio_play_finish_cb);

    Amy_event_cb_register(AMY_EVENT_ID_CALL, call_event_callback);
    Amy_event_cb_register(AMY_EVENT_ID_OTA, ota_event_cb);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
    lv_obj_event_pressed_callback_register(NULL);
}

static void Amy_lv_page_quit(call)
{
    standby_timer_restart(true, true);
    no_interact_timer_stop();
    intercom_audio_close();
    Amy_audio_play_stop();
    Amy_event_cb_register(AMY_EVENT_ID_CALL, common_call_extern_call_callback);
    Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
    lv_obj_event_pressed_callback_register(page_default_obj_press_event_cb);
}

Amy_lv_page_create(call);
