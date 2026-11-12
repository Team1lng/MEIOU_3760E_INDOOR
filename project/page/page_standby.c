#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "page_live.h"
#include "call_api.h"
#include "tuya/tuya_sdk.h"
#include "user_data.h"
#include "sat_user_common.h"
#include "sat_wifi.h"
#include "../monitor.h"
#include "standby_photo_frame.h"
#include "standby_bg_music.h"
#include "Amy_tfcard.h"
#include "user_standby.h"
#include "audio_input.h"
#include "../pwm.h"
#include <time.h>

/************************** 布局常量 **************************/

#define SEP_X 796
#define SEP_Y 50
#define SEP_W 178

/* 时间（左侧，大字体） */
#define TIME_X 196
#define TIME_Y 160
#define TIME_W 307
#define TIME_H 133

/* 日期（时间下方） */
#define DATE_X 226
#define DATE_Y 310
#define DATE_W 259
#define DATE_H 39

/* 天气分隔线（居中） */
#define WEATHER_LINE_X 537
#define WEATHER_LINE_Y 175
#define WEATHER_LINE_W 2
#define WEATHER_LINE_H 140

/* 天气模块（右侧，与时间左右对称） */
#define WEATHER_TEMP_X 568
#define WEATHER_TEMP_Y 165
#define WEATHER_TEMP_W 126
#define WEATHER_TEMP_H 63

#define WEATHER_COND_X 568
#define WEATHER_COND_Y 245
#define WEATHER_COND_W 380
#define WEATHER_COND_H 42

#define WEATHER_IMG_X 719
#define WEATHER_IMG_Y 145
#define WEATHER_IMG_W 120
#define WEATHER_IMG_H 120

#define WEATHER_RANGE_X 568
#define WEATHER_RANGE_Y 300
#define WEATHER_RANGE_W 259
#define WEATHER_RANGE_H 39

/* 底部通知面板 */
#define PANEL_Y 472
#define PANEL_W SCREEN_W
#define PANEL_H 128

/* 通知图标 (87x87) */
#define ICON_W 87
#define ICON_H 87
#define ICON1_X 291
#define ICON2_X 416
#define ICON3_X 541
#define ICON4_X 666
#define ICON_Y 500

/* 角标 */
#define BADGE_W 31
#define BADGE_H 26
#define BADGE_OFFSET_X 49
#define BADGE_OFFSET_Y -5
#define BADGE_COLOR 0xff0000

/* AEC 模式切换测试按钮（左上角） */
#define AEC_TEST_X 10
#define AEC_TEST_Y 4
#define AEC_TEST_W 200
#define AEC_TEST_H 28

/************************** 全局状态 **************************/

/* 普通待机熄屏：进入待机显示待机信息，SCREEN_OFF_DELAY_S 秒后熄灭背光，点击唤醒重新计时 */
#define SCREEN_OFF_DELAY_S 60
static bool s_screen_off;   /* 当前是否熄屏 */
static int s_off_countdown; /* 熄屏倒计时(秒) */

/* 移动侦测冷却：进入待机页、以及每次侦测处理结束后，MOTION_GUARD_MS 内的侦测事件一律忽略，
 * 留出点击待机页图标的时间，避免刚回到待机就被预览顶掉 */
#define MOTION_GUARD_MS 5000
static uint32_t s_motion_guard_until; /* 冷却截止 tick */

/************************** 函数声明 **************************/
static void standby_monitor_click(lv_event_t *ev);
static void standby_event_record_click(lv_event_t *ev);
static void standby_message_click(lv_event_t *ev);
static void standby_detection_click(lv_event_t *ev);

static void standby_bg_create(void);
static void standby_panel_create(void);
static void standby_notification_icons_create(void);
static void standby_clock_create(void);
static void standby_date_create(void);
static void standby_weather_create(void);
static void standby_aec_test_create(void);
static void standby_aec_test_label_update(void);
static void standby_aec_test_click(lv_event_t *ev);
static void standby_update_time_label(void);
static void standby_update_date_label(void);
static void standby_update_weather_display(void);
static void standby_badge_counts_update(void);
static void _set_weather_icon(lv_obj_t *img_obj, unsigned long offset, unsigned long size);
static void standby_idle_timer_cb(lv_timer_t *timer);
static void standby_screen_click_cb(lv_event_t *ev);
static void standby_screen_off(void);
static void standby_screen_wake(void);
static bool standby_click_wake_if_off(void);
static bool motion_preview_active(void);
static bool motion_guard_active(void);

/************************** 回调实现 **************************/

static void standby_screen_click_cb(lv_event_t *ev)
{
    if (standby_click_wake_if_off() && user_data_get()->private_cfg.screen_saver_en)
        return;
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 普通待机熄屏 **************************/
static void page_standby_tfcard_change_cb(void *arg)
{
    standby_badge_counts_update();
}

/* 待机页 SD 插拔：刷新未读角标，并保留通用插拔提示框 */
static void standby_tfcard_change_cb(void *arg)
{
    standby_badge_counts_update();
    page_common_tfcard_change_cb(arg);
}

static void standby_screen_off(void)
{
    if (s_screen_off)
        return;
    s_screen_off = true;
    s_off_countdown = 0;
    /* 熄屏前先关闭已有弹窗：其全屏模态遮罩在熄屏后不可见但仍会拦截点击，导致无法唤醒 */
    lv_common_msgbox_del();
    backlight_enable(0);
    /* 熄屏期间禁用 SD 插拔提示回调：避免不可见模态遮罩拦截点击唤醒 */
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_standby_tfcard_change_cb);
}

static void standby_screen_wake(void)
{
    if (!s_screen_off)
        return;
    backlight_enable(true);
    backlight_set(user_data_get()->private_cfg.display_brightness);
    s_screen_off = false;
    s_off_countdown = 0;
    /* 恢复 SD 插拔提示回调 */
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, standby_tfcard_change_cb);
}

/* 熄屏时点击仅唤醒并停留在待机页，不触发按钮/页面跳转；
 * 同时关闭待机跳转计时，避免 page_power 的全局按压回调重新使能后 60s 再次 enter 待机页 */
static bool standby_click_wake_if_off(void)
{
    if (!s_screen_off)
        return false;
    standby_screen_wake();
    standby_timer_close();
    return true;
}

static void standby_monitor_click(lv_event_t *ev)
{
    if (standby_click_wake_if_off())
        return;
    int channel = monitor_first_valid_channel_get();
    if (channel == MON_CHANNEL_UNKNOW)
    {
        /* 无有效监控通道，回退到监控通道选择页 */
        Amy_lv_page_goto(lv_page(monitor), LV_SCR_LOAD_ANIM_NONE, );
        return;
    }

    monitor_channel_set(channel);
    monitor_status_set(MONITOR_OUTGOING_CONNECTING);

    if (is_channel_ipc_camera(channel) == 0)
    {
        device_info_attr dev_info = SAT_DEVICE_INFO_DEFAULT;
        dev_info.building = user_data_get()->private_cfg.device.attr.building;
        dev_info.room = 0x00;
        dev_info.device_type = (channel == MON_CHANNEL_DOOR1)
                                   ? DEVICE_TYPE_DOOR_ID1
                                   : DEVICE_TYPE_DOOR_ID2;
        sat_call_dial(&dev_info);
        monitor_device_info_set(&dev_info);
    }

    Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}

static void standby_event_record_click(lv_event_t *ev)
{
    if (standby_click_wake_if_off())
        return;
    g_media_record_type = MEDIA_RECORD_TYPE_CALL;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void standby_message_click(lv_event_t *ev)
{
    if (standby_click_wake_if_off())
        return;
    g_media_record_type = MEDIA_RECORD_TYPE_MESSAGE;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void standby_detection_click(lv_event_t *ev)
{
    if (standby_click_wake_if_off())
        return;
    g_media_record_type = MEDIA_RECORD_TYPE_DETECTION;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 创建函数 **************************/

/* 获取待机全屏背景容器（所有待机控件都挂在此容器下） */
static lv_obj_t *standby_bg_get(void)
{
    return lv_obj_get_child_by_id(lv_scr_act_current(), STANDBY_BG_CONT_ID);
}

static void standby_bg_create(void)
{
    lv_obj_t *bg = lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    lv_obj_set_child_id(bg, STANDBY_BG_CONT_ID);
}

static void standby_panel_create(void)
{
    lv_common_rect_create(standby_bg_get(), 0, PANEL_Y, PANEL_W, PANEL_H, LV_COMMON_PANEL_COLOR);
}

static void standby_badge_create(int icon_x, int icon_y, int badge_id)
{
    int bx = icon_x + BADGE_OFFSET_X;
    int by = icon_y + BADGE_OFFSET_Y;

    lv_common_text_create(standby_bg_get(), badge_id,
                          bx, by, BADGE_W, BADGE_H,
                          NULL,
                          LV_OPA_COVER, BADGE_COLOR, LV_OPA_COVER, BADGE_COLOR,
                          4, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          4, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0xffffff,
                          LV_TEXT_ALIGN_CENTER_BUTTOM, lv_font_small);

    /* 默认隐藏，刷新角标时按未读个数显示（为 0 也显示） */
    lv_obj_add_flag(lv_obj_get_child_by_id(standby_bg_get(), badge_id), LV_OBJ_FLAG_HIDDEN);
}

static void standby_notification_icons_create(void)
{
    lv_resource_init(icon_mon, ICON_STANDBY_MONITOR_PNG_OFFSET, ICON_STANDBY_MONITOR_PNG_SIZE);
    lv_resource_init(icon_focus_mon, ICON_STANDBY_MONITOR_FOCUS_PNG_OFFSET, ICON_STANDBY_MONITOR_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(standby_bg_get(), STANDBY_MONITOR_ICON_ID,
                             ICON1_X, ICON_Y, ICON_W, ICON_H,
                             standby_monitor_click, true,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_mon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus_mon);

    lv_resource_init(icon_call, ICON_STANDBY_CALL_RECORD_PNG_OFFSET, ICON_STANDBY_CALL_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus_call, ICON_STANDBY_CALL_RECORD_FOCUS_PNG_OFFSET, ICON_STANDBY_CALL_RECORD_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(standby_bg_get(), STANDBY_EVENT_RECORD_ICON_ID,
                             ICON2_X, ICON_Y, ICON_W, ICON_H,
                             standby_event_record_click, true,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_call, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus_call);

    lv_resource_init(icon_msg, ICON_STANDBY_MESSAGE_RECORD_PNG_OFFSET, ICON_STANDBY_MESSAGE_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus_msg, ICON_STANDBY_MESSAGE_RECORD_FOCUS_PNG_OFFSET, ICON_STANDBY_MESSAGE_RECORD_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(standby_bg_get(), STANDBY_MESSAGE_ICON_ID,
                             ICON3_X, ICON_Y, ICON_W, ICON_H,
                             standby_message_click, true,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_msg, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus_msg);

    lv_resource_init(icon_det, ICON_STANDBY_DETECTION_RECORD_PNG_OFFSET, ICON_STANDBY_DETECTION_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus_det, ICON_STANDBY_DETECTION_RECORD_FOCUS_PNG_OFFSET, ICON_STANDBY_DETECTION_RECORD_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(standby_bg_get(), STANDBY_DETECTION_ICON_ID,
                             ICON4_X, ICON_Y, ICON_W, ICON_H,
                             standby_detection_click, true,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_det, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus_det);

    /* 创建角标 */
    standby_badge_create(ICON2_X, ICON_Y, STANDBY_BADGE1_ID); /* 通话记录 */
    standby_badge_create(ICON3_X, ICON_Y, STANDBY_BADGE2_ID); /* 消息记录 */
    standby_badge_create(ICON4_X, ICON_Y, STANDBY_BADGE3_ID); /* 侦测记录 */
    standby_badge_create(ICON1_X, ICON_Y, STANDBY_BADGE4_ID); /* 监控（预留） */
}

static void standby_clock_create(void)
{
    lv_common_text_create(standby_bg_get(), STANDBY_CLOCK_LABEL_ID,
                          TIME_X, TIME_Y, TIME_W, TIME_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_CENTER, lv_font_plus);
}

static void standby_date_create(void)
{
    lv_common_text_create(standby_bg_get(), STANDBY_DATE_LABEL_ID,
                          DATE_X, DATE_Y, DATE_W, DATE_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_CENTER, lv_font_large);
}

static void _set_weather_icon(lv_obj_t *img_obj, unsigned long offset, unsigned long size)
{
    if (img_obj == NULL)
        return;
    lv_resource_init(icon, offset, size);
    lv_obj_set_style_bg_img_src(img_obj, &icon, LV_PART_MAIN);
}

static void standby_update_time_label(void)
{
    lv_obj_t *time_label = lv_obj_get_child_by_id(standby_bg_get(), STANDBY_CLOCK_LABEL_ID);
    if (time_label == NULL)
        return;

    time_t now = time(NULL);
    struct tm local_tm;
    Amy_time2localtime(now, local_tm);
    lv_label_set_text_fmt(time_label, "%02d:%02d", local_tm.tm_hour, local_tm.tm_min);
}

static void standby_update_date_label(void)
{
    lv_obj_t *date_label = lv_obj_get_child_by_id(standby_bg_get(), STANDBY_DATE_LABEL_ID);
    if (date_label == NULL)
        return;

    time_t now = time(NULL);
    struct tm local_tm;
    Amy_time2localtime(now, local_tm);
    int fmt = user_data_get()->private_cfg.date_format;
    if (fmt == 0)
        lv_label_set_text_fmt(date_label, "%04d-%02d-%02d", local_tm.tm_year, local_tm.tm_mon, local_tm.tm_mday);
    else if (fmt == 1)
        lv_label_set_text_fmt(date_label, "%02d-%02d-%04d", local_tm.tm_mon, local_tm.tm_mday, local_tm.tm_year);
    else
        lv_label_set_text_fmt(date_label, "%02d-%02d-%04d", local_tm.tm_mday, local_tm.tm_mon, local_tm.tm_year);
}

static void standby_adjust_layout_by_tuya(void)
{
    tuya_api_weather *weather_info = (tuya_api_weather *)&user_data_get()->public_cfg.weather_info;
    lv_obj_t *bg = standby_bg_get();
    lv_obj_t *line = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_SEPARATOR_ID);
    lv_obj_t *date_label = lv_obj_get_child_by_id(bg, STANDBY_DATE_LABEL_ID);
    lv_obj_t *time_label = lv_obj_get_child_by_id(bg, STANDBY_CLOCK_LABEL_ID);
    lv_obj_t *temp_cur = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_TEMP_CUR_ID);
    lv_obj_t *temp_range = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_TEMP_RANGE_ID);
    lv_obj_t *cond_label = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_CONDITION_LABEL_ID);
    lv_obj_t *cond_img = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_CONDITION_IMG_ID);

    if (weather_info->is_valid)
    {
        if (line)
            lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
        if (temp_cur)
            lv_obj_clear_flag(temp_cur, LV_OBJ_FLAG_HIDDEN);
        if (temp_range)
            lv_obj_clear_flag(temp_range, LV_OBJ_FLAG_HIDDEN);
        if (cond_label)
            lv_obj_clear_flag(cond_label, LV_OBJ_FLAG_HIDDEN);
        if (cond_img)
            lv_obj_clear_flag(cond_img, LV_OBJ_FLAG_HIDDEN);
        if (date_label)
            lv_obj_set_pos(date_label, DATE_X, DATE_Y);
        if (time_label)
            lv_obj_set_pos(time_label, TIME_X, TIME_Y);
    }
    else
    {
        if (line)
            lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        if (temp_cur)
            lv_obj_add_flag(temp_cur, LV_OBJ_FLAG_HIDDEN);
        if (temp_range)
            lv_obj_add_flag(temp_range, LV_OBJ_FLAG_HIDDEN);
        if (cond_label)
            lv_obj_add_flag(cond_label, LV_OBJ_FLAG_HIDDEN);
        if (cond_img)
            lv_obj_add_flag(cond_img, LV_OBJ_FLAG_HIDDEN);
        if (date_label)
            lv_obj_set_pos(date_label, (SCREEN_W - DATE_W) / 2, DATE_Y);
        if (time_label)
            lv_obj_set_pos(time_label, (SCREEN_W - TIME_W) / 2, TIME_Y);
    }
}

static void standby_update_weather_display(void)
{
    // 使用用户数据保存涂鸦天气考虑到未来可能分机使用主机的天气信息，而不是通过云端获取
    tuya_api_weather weather_data;
    if (tuya_api_get_weather(&weather_data) == true)
    {
        if (memcmp(&user_data_get()->public_cfg.weather_info, &weather_data, sizeof(user_data_get()->public_cfg.weather_info)) != 0)
        {
            memcpy(&user_data_get()->public_cfg.weather_info, &weather_data, sizeof(user_data_get()->public_cfg.weather_info));
            user_data_save();
        }
    }
    // MQTT离线时天气必然抓不到，立即清空持久化数据，避免显示过期天气；
    // 在线后抓到新数据会重新写回（供未来分机同步主机天气使用）
    else if (!tuya_mqtt_is_online())
    {
        if (user_data_get()->public_cfg.weather_info.is_valid)
        {
            memset(&user_data_get()->public_cfg.weather_info, 0, sizeof(user_weather_t));
            user_data_save();
        }
    }

    tuya_api_weather *weather_info = (tuya_api_weather *)&user_data_get()->public_cfg.weather_info;
    if (weather_info->is_valid == false)
        return;

    lv_obj_t *bg = standby_bg_get();
    lv_obj_t *cur_temp_label = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_TEMP_CUR_ID);
    if (cur_temp_label)
        lv_label_set_text_fmt(cur_temp_label, "%d℃", weather_info->temp);

    lv_obj_t *temp_range_label = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_TEMP_RANGE_ID);
    if (temp_range_label)
        lv_label_set_text_fmt(temp_range_label, "%d℃ ~ %d℃", weather_info->tlow, weather_info->thigh);

    lv_obj_t *condition_label = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_CONDITION_LABEL_ID);
    lv_obj_t *condition_img = lv_obj_get_child_by_id(bg, STANDBY_WEATHER_CONDITION_IMG_ID);

    const char *cond_str = "";
    switch (weather_info->condition)
    {
    case 120:
        cond_str = lang_str_get(STR_SUNNY);
        _set_weather_icon(condition_img, ICON_WEATHER_SUNNY_PNG_OFFSET, ICON_WEATHER_SUNNY_PNG_SIZE);
        break;
    case 101:
        cond_str = lang_str_get(STR_HEAVY_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_RAIN_PNG_OFFSET, ICON_WEATHER_HEAVEY_RAIN_PNG_SIZE);
        break;
    case 102:
        cond_str = lang_str_get(STR_THUNDERSTORM);
        _set_weather_icon(condition_img, ICON_WEATHER_THUNDERSTORM_PNG_OFFSET, ICON_WEATHER_THUNDERSTORM_PNG_SIZE);
        break;
    case 103:
        cond_str = lang_str_get(STR_SANDSTORM);
        _set_weather_icon(condition_img, ICON_WEATHER_SANDSTORM_PNG_OFFSET, ICON_WEATHER_SANDSTORM_PNG_SIZE);
        break;
    case 104:
        cond_str = lang_str_get(STR_LIGHT_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_SNOW_PNG_OFFSET, ICON_WEATHER_LIGHT_SNOW_PNG_SIZE);
        break;
    case 105:
        cond_str = lang_str_get(STR_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_SNOW_PNG_OFFSET, ICON_WEATHER_HEAVEY_SNOW_PNG_SIZE);
        break;
    case 106:
        cond_str = lang_str_get(STR_POGONIP);
        _set_weather_icon(condition_img, ICON_WEATHER_HAZE_PNG_OFFSET, ICON_WEATHER_HAZE_PNG_SIZE);
        break;
    case 107:
        cond_str = lang_str_get(STR_RAINSTORM);
        _set_weather_icon(condition_img, ICON_WEATHER_TORRENTIAL_RAIN_PNG_OFFSET, ICON_WEATHER_TORRENTIAL_RAIN_PNG_SIZE);
        break;
    case 108:
        cond_str = lang_str_get(STR_ISOLATED_SHOWERS);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_RAIN_PNG_OFFSET, ICON_WEATHER_LIGHT_RAIN_PNG_SIZE);
        break;
    case 109:
        cond_str = lang_str_get(STR_FLOATING_DUST);
        _set_weather_icon(condition_img, ICON_WEATHER_HAZE_PNG_OFFSET, ICON_WEATHER_HAZE_PNG_SIZE);
        break;
    case 110:
        cond_str = lang_str_get(STR_LIGHTNING);
        _set_weather_icon(condition_img, ICON_WEATHER_THUNDERSTORM_PNG_OFFSET, ICON_WEATHER_THUNDERSTORM_PNG_SIZE);
        break;
    case 111:
        cond_str = lang_str_get(STR_LIGHT_SHOWERS);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_RAIN_PNG_OFFSET, ICON_WEATHER_LIGHT_RAIN_PNG_SIZE);
        break;
    case 112:
        cond_str = lang_str_get(STR_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_RAIN_PNG_OFFSET, ICON_WEATHER_LIGHT_RAIN_PNG_SIZE);
        break;
    case 113:
        cond_str = lang_str_get(STR_RAIN_AND_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_SLEET_PNG_OFFSET, ICON_WEATHER_SLEET_PNG_SIZE);
        break;
    case 114:
        cond_str = lang_str_get(STR_DUST_TORNADO);
        _set_weather_icon(condition_img, ICON_WEATHER_SANDSTORM_PNG_OFFSET, ICON_WEATHER_SANDSTORM_PNG_SIZE);
        break;
    case 115:
        cond_str = lang_str_get(STR_ICE_PELLET);
        _set_weather_icon(condition_img, ICON_WEATHER_HAILSTORM_PNG_OFFSET, ICON_WEATHER_HAILSTORM_PNG_SIZE);
        break;
    case 116:
        cond_str = lang_str_get(STR_STRONG_SANDSTORM);
        _set_weather_icon(condition_img, ICON_WEATHER_SANDSTORM_PNG_OFFSET, ICON_WEATHER_SANDSTORM_PNG_SIZE);
        break;
    case 117:
        cond_str = lang_str_get(STR_BLOWING_SAND);
        _set_weather_icon(condition_img, ICON_WEATHER_SANDSTORM_PNG_OFFSET, ICON_WEATHER_SANDSTORM_PNG_SIZE);
        break;
    case 118:
        cond_str = lang_str_get(STR_LIGHT_TO_MODERATE_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_RAIN_PNG_OFFSET, ICON_WEATHER_LIGHT_RAIN_PNG_SIZE);
        break;
    case 119:
        cond_str = lang_str_get(STR_SUNNY);
        _set_weather_icon(condition_img, ICON_WEATHER_SUNNY_PNG_OFFSET, ICON_WEATHER_SUNNY_PNG_SIZE);
        break;
    case 121:
        cond_str = lang_str_get(STR_FOG);
        _set_weather_icon(condition_img, ICON_WEATHER_HAZE_PNG_OFFSET, ICON_WEATHER_HAZE_PNG_SIZE);
        break;
    case 122:
        cond_str = lang_str_get(STR_SHOWER);
        _set_weather_icon(condition_img, ICON_WEATHER_MODERATE_RAIN_PNG_OFFSET, ICON_WEATHER_MODERATE_RAIN_PNG_SIZE);
        break;
    case 123:
        cond_str = lang_str_get(STR_HEAVY_SHOWERS);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_RAIN_PNG_OFFSET, ICON_WEATHER_HEAVEY_RAIN_PNG_SIZE);
        break;
    case 124:
        cond_str = lang_str_get(STR_HEAVY_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_SNOW_PNG_OFFSET, ICON_WEATHER_HEAVEY_SNOW_PNG_SIZE);
        break;
    case 125:
        cond_str = lang_str_get(STR_TORRENTIAL_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_TORRENTIAL_RAIN_PNG_OFFSET, ICON_WEATHER_TORRENTIAL_RAIN_PNG_SIZE);
        break;
    case 126:
        cond_str = lang_str_get(STR_BLIZZARD);
        _set_weather_icon(condition_img, ICON_WEATHER_BLIZZARD_PNG_OFFSET, ICON_WEATHER_BLIZZARD_PNG_SIZE);
        break;
    case 127:
        cond_str = lang_str_get(STR_HAIL);
        _set_weather_icon(condition_img, ICON_WEATHER_HAILSTORM_PNG_OFFSET, ICON_WEATHER_HAILSTORM_PNG_SIZE);
        break;
    case 128:
        cond_str = lang_str_get(STR_LIGHT_TO_MODERATE_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_SNOW_PNG_OFFSET, ICON_WEATHER_LIGHT_SNOW_PNG_SIZE);
        break;
    case 129:
        cond_str = lang_str_get(STR_PARTLY_CLOUDY);
        _set_weather_icon(condition_img, ICON_WEATHER_CLOUDY_PNG_OFFSET, ICON_WEATHER_CLOUDY_PNG_SIZE);
        break;
    case 130:
        cond_str = lang_str_get(STR_LIGHT_SNOW_SHOWERS);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_SNOW_PNG_OFFSET, ICON_WEATHER_LIGHT_SNOW_PNG_SIZE);
        break;
    case 131:
        cond_str = lang_str_get(STR_MODERATE_SNOW);
        _set_weather_icon(condition_img, ICON_WEATHER_MODERATE_SNOW_PNG_OFFSET, ICON_WEATHER_MODERATE_SNOW_PNG_SIZE);
        break;
    case 132:
        cond_str = lang_str_get(STR_OVERCAST);
        _set_weather_icon(condition_img, ICON_WEATHER_OVERCAST_PNG_OFFSET, ICON_WEATHER_OVERCAST_PNG_SIZE);
        break;
    case 133:
        cond_str = lang_str_get(STR_HOARFROST);
        _set_weather_icon(condition_img, ICON_WEATHER_OVERCAST_PNG_OFFSET, ICON_WEATHER_OVERCAST_PNG_SIZE);
        break;
    case 134:
        cond_str = lang_str_get(STR_HEAVY_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_RAIN_PNG_OFFSET, ICON_WEATHER_HEAVEY_RAIN_PNG_SIZE);
        break;
    case 136:
        cond_str = lang_str_get(STR_THUNDERSTORMS_WITH_HAIL);
        _set_weather_icon(condition_img, ICON_WEATHER_HAILSTORM_PNG_OFFSET, ICON_WEATHER_HAILSTORM_PNG_SIZE);
        break;
    case 137:
        cond_str = lang_str_get(STR_FREEZING_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_SLEET_PNG_OFFSET, ICON_WEATHER_SLEET_PNG_SIZE);
        break;
    case 138:
        cond_str = lang_str_get(STR_SNOW_SHOWERS);
        _set_weather_icon(condition_img, ICON_WEATHER_MODERATE_SNOW_PNG_OFFSET, ICON_WEATHER_MODERATE_SNOW_PNG_SIZE);
        break;
    case 139:
        cond_str = lang_str_get(STR_LIGHT_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_LIGHT_RAIN_PNG_OFFSET, ICON_WEATHER_LIGHT_RAIN_PNG_SIZE);
        break;
    case 140:
        cond_str = lang_str_get(STR_FOG);
        _set_weather_icon(condition_img, ICON_WEATHER_HAZE_PNG_OFFSET, ICON_WEATHER_HAZE_PNG_SIZE);
        break;
    case 141:
        cond_str = lang_str_get(STR_MODERATE_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_MODERATE_RAIN_PNG_OFFSET, ICON_WEATHER_MODERATE_RAIN_PNG_SIZE);
        break;
    case 142:
        cond_str = lang_str_get(STR_CLOUDY);
        _set_weather_icon(condition_img, ICON_WEATHER_CLOUDY_PNG_OFFSET, ICON_WEATHER_CLOUDY_PNG_SIZE);
        break;
    case 143:
        cond_str = lang_str_get(STR_THUNDER_SHOWER);
        _set_weather_icon(condition_img, ICON_WEATHER_THUNDERSTORM_PNG_OFFSET, ICON_WEATHER_THUNDERSTORM_PNG_SIZE);
        break;
    case 144:
        cond_str = lang_str_get(STR_MODERATE_TO_HEAVY_RAIN);
        _set_weather_icon(condition_img, ICON_WEATHER_HEAVEY_RAIN_PNG_OFFSET, ICON_WEATHER_HEAVEY_RAIN_PNG_SIZE);
        break;
    case 145:
        cond_str = lang_str_get(STR_HEAVY_TO_RAINSTORM);
        _set_weather_icon(condition_img, ICON_WEATHER_TORRENTIAL_RAIN_PNG_OFFSET, ICON_WEATHER_TORRENTIAL_RAIN_PNG_SIZE);
        break;
    case 146:
        cond_str = lang_str_get(STR_SUNNY);
        _set_weather_icon(condition_img, ICON_WEATHER_SUNNY_PNG_OFFSET, ICON_WEATHER_SUNNY_PNG_SIZE);
        break;
    default:
        cond_str = lang_str_get(STR_CLOUDY);
        _set_weather_icon(condition_img, ICON_WEATHER_CLOUDY_PNG_OFFSET, ICON_WEATHER_CLOUDY_PNG_SIZE);
        break;
    }
    if (condition_label)
        lv_label_set_text(condition_label, cond_str);
}

static void standby_weather_create(void)
{
    lv_common_text_create(standby_bg_get(), STANDBY_WEATHER_SEPARATOR_ID,
                          WEATHER_LINE_X, WEATHER_LINE_Y, WEATHER_LINE_W, WEATHER_LINE_H,
                          NULL, LV_OPA_COVER, 0XFFFFFF, LV_OPA_COVER, 0XFFFFFF,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0XFFFFFF, 0XFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_large);

    lv_common_text_create(standby_bg_get(), STANDBY_WEATHER_TEMP_CUR_ID,
                          WEATHER_TEMP_X, WEATHER_TEMP_Y, WEATHER_TEMP_W, WEATHER_TEMP_H,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_large_s);

    lv_common_text_create(standby_bg_get(), STANDBY_WEATHER_CONDITION_LABEL_ID,
                          WEATHER_COND_X, WEATHER_COND_Y, WEATHER_COND_W, WEATHER_COND_H,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_large);

    lv_common_img_btn_create(standby_bg_get(), STANDBY_WEATHER_CONDITION_IMG_ID,
                             WEATHER_IMG_X, WEATHER_IMG_Y, WEATHER_IMG_W, WEATHER_IMG_H,
                             NULL, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             NULL, LV_OPA_TRANSP, 0x00a8ff, LV_ALIGN_CENTER, NULL);

    lv_common_text_create(standby_bg_get(), STANDBY_WEATHER_TEMP_RANGE_ID,
                          WEATHER_RANGE_X, WEATHER_RANGE_Y, WEATHER_RANGE_W, WEATHER_RANGE_H,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "", 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_large);
}

/************************** AEC 模式切换测试按钮 **************************/

static void standby_aec_test_label_update(void)
{
    lv_obj_t *label = lv_obj_get_child_by_id(standby_bg_get(), STANDBY_AEC_TEST_BTN_ID);
    if (label == NULL)
        return;

    if (audio_input_aec_mode_get() == AEC_MODE_ANYCLOUD)
        lv_label_set_text(label, "AEC:ANYCLOUD");
    else
        lv_label_set_text(label, "AEC:SPEEX");
}

static void standby_aec_test_click(lv_event_t *ev)
{
    if (standby_click_wake_if_off())
        return;
    int cur = audio_input_aec_mode_get();
    int next = (cur == AEC_MODE_SPEEX) ? AEC_MODE_ANYCLOUD : AEC_MODE_SPEEX;
    audio_input_aec_mode_set(next);
    standby_aec_test_label_update();
}

static void standby_aec_test_create(void)
{
    lv_common_text_create(standby_bg_get(), STANDBY_AEC_TEST_BTN_ID,
                          AEC_TEST_X, AEC_TEST_Y, AEC_TEST_W, AEC_TEST_H,
                          standby_aec_test_click,
                          LV_OPA_COVER, 0x2A2A2A, LV_OPA_COVER, 0x444444,
                          4, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          4, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          NULL, 0xffffff, 0xffffff,
                          LV_TEXT_ALIGN_CENTER, lv_font_small);
    standby_aec_test_label_update();
}

/************************** 定时器回调 **************************/

static void _badge_label_update(int badge_id, unsigned long count)
{
    lv_obj_t *badge = lv_obj_get_child_by_id(standby_bg_get(), badge_id);
    if (!badge)
        return;

    char buf[8];
    if (count > 99)
        snprintf(buf, sizeof(buf), "%s", "99+");
    else
        snprintf(buf, sizeof(buf), "%lu", count);
    lv_label_set_text(badge, buf);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_HIDDEN);
}

static void _badge_query_and_update(const char *base_dir, unsigned int mask, int badge_id)
{
    Amy_media_context ctx = Amy_media_open(base_dir, mask | MEDIA_ATTR_NEW, 1);
    if (ctx)
    {
        unsigned long total = 0;
        Amy_media_first_page(ctx, NULL);
        Amy_media_new_total_get(ctx, &total);
        _badge_label_update(badge_id, total);
        Amy_media_close(ctx);
    }
}

static void standby_badge_counts_update(void)
{
    /* 待机背景容器可能已被清理（如页面退出后的延时刷新） */
    if (standby_bg_get() == NULL)
        return;

    /* 未插卡时无文件可查，角标固定显示 0 */
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
    {
        _badge_label_update(STANDBY_BADGE1_ID, 0);
        _badge_label_update(STANDBY_BADGE2_ID, 0);
        _badge_label_update(STANDBY_BADGE3_ID, 0);
        return;
    }
    char base_dir[64];
    snprintf(base_dir, sizeof(base_dir), "%s", TFCARD_MOUNT_PATH);

    /* 通话记录未读数 */
    _badge_query_and_update(base_dir,
                            MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO | MEDIA_MODE_MANUAL | MEDIA_MODE_AUTO | MEDIA_CHANNEL_ALL,
                            STANDBY_BADGE1_ID);

    /* 消息记录未读数 */
    _badge_query_and_update(base_dir,
                            MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO | MEDIA_MODE_MESSAGE | MEDIA_CHANNEL_ALL,
                            STANDBY_BADGE2_ID);

    /* 侦测记录未读数 */
    _badge_query_and_update(base_dir,
                            MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO | MEDIA_MODE_MOTION | MEDIA_CHANNEL_ALL,
                            STANDBY_BADGE3_ID);
}

static void standby_badge_refresh_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    standby_badge_counts_update();
}

/* 抓拍保存是异步的，稍后刷新未查看个数 */
static void standby_badge_refresh_delayed(void)
{
    lv_timer_create_recycle(standby_badge_refresh_timer_cb, 1000, NULL);
}

static void standby_idle_timer_cb(lv_timer_t *timer)
{
    standby_update_time_label();
    standby_update_date_label();
    standby_update_weather_display();
    standby_adjust_layout_by_tuya();
    page_common_status_bar_update();

    /* 睡眠模式不播放背景音乐：待在待机页时切到睡眠模式则立即停止 */
    if (standby_bg_music_is_running() &&
        user_data_get()->public_cfg.global_func.scene_mode == SCENE_MODE_SLEEP)
        standby_bg_music_stop();

    /* 所有待机分支统一：无移动侦测预览、无数码相框轮播时满 SCREEN_OFF_DELAY_S 秒后熄屏 */
    if (!s_screen_off && !motion_preview_active() && !standby_photo_frame_is_running() &&
        ++s_off_countdown >= SCREEN_OFF_DELAY_S)
        standby_screen_off();
}

/************************** 移动侦测上下文 **************************/

#define MOTION_HEADER_H 60
#define MOTION_TEXT_COLOR 0xdd3d3d

/* 静默模式与预览模式共用同一套侦测录制流程，仅通过 preview 区分是否显示画面 */
static struct
{
    bool active;              /* 侦测录制会话进行中 */
    bool preview;             /* true: 预览模式(显示画面); false: 静默模式(后台) */
    bool recording;           /* 录像进行中 */
    uint16_t record_duration; /* 单次触发时长(秒) */
    uint16_t record_time;     /* 已运行秒数 */
    lv_timer_t *timer;        /* 1s 倒计时定时器 */
    record_mode_type_def record_mode;
} s_motion_ctx;

static bool motion_preview_active(void)
{
    return s_motion_ctx.active && s_motion_ctx.preview;
}

static bool motion_guard_active(void)
{
    /* 用差值比较，规避 lv_tick 溢出 */
    return (int32_t)(lv_tick_get() - s_motion_guard_until) < 0;
}

/************************** 移动侦测 - 静默/预览 共用流程 **************************/

static void motion_stop(void);

static void motion_preview_rtsp_get_frame_cb(void *arg)
{
    video_mode_cb_attr *attr = (video_mode_cb_attr *)(arg);
    Amy_video_mode_register_device(attr->func, attr->user_data);
    lv_video_mode_enable(1);
}

static void motion_recording_label_update(void)
{
    lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), MOTION_HEADER_CONT_ID);
    if (top_cont == NULL)
        return;
    lv_obj_t *label = lv_obj_get_child_by_id(top_cont, MOTION_RECORDING_LABEL_ID);
    if (label == NULL)
        return;

    int remaining = s_motion_ctx.record_duration - s_motion_ctx.record_time;
    if (remaining < 0)
        remaining = 0;
    /* SD 卡未挂载时仅显示倒计时，清除录制模式文案（REC/SNAPSHOT） */
    if (Amy_tfcard_status() == SD_STATE_MOUNT)
    {
        const char *mode_text = (s_motion_ctx.record_mode == RECORD_MODE_PHOTO)
                                    ? lang_str_get(STR_SNAPSHOT)
                                    : lang_str_get(STR_REC);
        lv_label_set_text_fmt(label, "%s %02d", mode_text, remaining);
    }
    else
    {
        lv_label_set_text_fmt(label, "%02d", remaining);
    }
}

static void motion_sd_change_cb(void *arg)
{
    motion_recording_label_update();
    if (s_screen_off)
        return;
    /* 保留插拔提示弹窗，并同步刷新右上角录制文案 */
    page_common_tfcard_change_cb(arg);
}

static void motion_preview_channel_label_update(void)
{
    lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), MOTION_HEADER_CONT_ID);
    if (top_cont == NULL)
        return;
    lv_obj_t *label = lv_obj_get_child_by_id(top_cont, MOTION_CHANNEL_ID);
    if (label == NULL)
        return;

    int channel = monitor_channel_get();
    if (channel == MON_CHANNEL_DOOR2)
        lv_label_set_text(label, lang_str_get(STR_DOOR2));
    else
        lv_label_set_text(label, lang_str_get(STR_DOOR1));
}

static void motion_preview_rendering_setting(void)
{
    lv_area_t area[] = {
        {0, 0, 1023, 59},
        {929, 505, 929 + 67 - 1, 505 + 67 - 1},
        /* SD 插拔弹窗区域：视频模式下未注册的区域不会与视频混合，弹窗内容将不可见 */
        {329, 172, 329 + 366 - 1, 172 + 256 - 1},
    };
    for (int i = 0; i < sizeof(area) / sizeof(lv_area_t); i++)
        Amy_rendering_area(&area[i]);
}

static void motion_preview_header_create(void)
{
    lv_obj_t *cont = lv_common_img_btn_create(lv_scr_act_current(), MOTION_HEADER_CONT_ID,
                                              0, 0, 1024, MOTION_HEADER_H,
                                              NULL, false,
                                              LV_OPA_30, 0x000000,
                                              LV_OPA_30, 0x000000,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              NULL, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

    lv_common_text_create(cont, MOTION_CHANNEL_ID,
                          32, 0, 138, MOTION_HEADER_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_DOOR1),
                          MOTION_TEXT_COLOR, MOTION_TEXT_COLOR,
                          LV_TEXT_ALIGN_LEFT_CENTER, lv_font_large);
    motion_preview_channel_label_update();

    lv_common_text_create(cont, MOTION_TITLE_LABEL_ID,
                          250, 0, 1024 - 500, MOTION_HEADER_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_MOTION_DETECTION),
                          MOTION_TEXT_COLOR, MOTION_TEXT_COLOR,
                          LV_TEXT_ALIGN_CENTER_CENTER, lv_font_large);

    lv_common_text_create(cont, MOTION_RECORDING_LABEL_ID,
                          1024 - 250, 0, 230, MOTION_HEADER_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "30",
                          MOTION_TEXT_COLOR, MOTION_TEXT_COLOR,
                          LV_TEXT_ALIGN_RIGHT_CENTER, lv_font_large);
}

static void motion_preview_back_btn_click_cb(lv_event_t *e)
{
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

static void motion_preview_back_btn_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), MOTION_BACK_BTN_ID,
                                             929, 505, 67, 67,
                                             motion_preview_back_btn_click_cb, true,
                                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(btn, 21);
}

static void motion_timer_cb(lv_timer_t *timer)
{
    if (!s_motion_ctx.active)
        return;

    if (s_motion_ctx.record_time++ >= s_motion_ctx.record_duration)
    {
        motion_stop();
        lv_obj_add_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(lv_scr_act_current(), standby_screen_click_cb, LV_EVENT_CLICKED, NULL);
    }
    else if (s_motion_ctx.preview)
    {
        motion_recording_label_update();
    }
}

static void motion_start(record_mode_type_def md_mode, int duration_ms, bool preview)
{
    if (s_motion_ctx.active)
        return;
    if (preview)
    {
        standby_timer_close();
        /* 熄屏状态下触发侦测预览需点亮背光，否则画面不可见 */
        standby_screen_wake();
        /* 预览进行中重新计时，避免熄屏倒计时在预览期间触发 */
        s_off_countdown = 0;

        /* 屏幕底色改黑，并隐藏待机背景容器，预览前不露出待机UI或红色显示底色 */
        lv_obj_set_style_bg_color(lv_scr_act_current(), lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act_current(), LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_t *bg = standby_bg_get();
        if (bg)
            lv_obj_add_flag(bg, LV_OBJ_FLAG_HIDDEN);
        motion_preview_header_create();
        motion_preview_back_btn_create();

        Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, motion_preview_rtsp_get_frame_cb);
        lv_obj_remove_event_cb(lv_scr_act_current(), standby_screen_click_cb);
        lv_obj_clear_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
        motion_preview_rendering_setting();
    }
    /* 统一用 1s 倒计时定时器：record_duration=0 时首拍即停(约 1s)，与静默模式最短保持 1s 一致 */
    /* 不用 recycle 定时器，避免页面切换时 lv_timer_recycle 提前释放导致悬垂指针 */
    s_motion_ctx.timer = lv_timer_create(motion_timer_cb, 1000, NULL);

    monitor_video_open();

    s_motion_ctx.preview = preview;
    s_motion_ctx.record_duration = duration_ms / 1000;
    s_motion_ctx.record_time = 0;
    s_motion_ctx.record_mode = md_mode;

    /* 录制媒体类型参考设置 */
    s_motion_ctx.recording = false;

    bool snap_save = (md_mode == RECORD_MODE_PHOTO || md_mode == RECORD_MODE_PHOTO_VIDEO);
    monitor_video_snapshort(MEDIA_MODE_MOTION, snap_save);

    s_motion_ctx.active = true;
    if (md_mode == RECORD_MODE_VIDEO || md_mode == RECORD_MODE_PHOTO_VIDEO)
    {
        if (Amy_tfcard_status() == SD_STATE_MOUNT &&
            monitor_video_record_start(MEDIA_MODE_MOTION) == 0)
        {
            s_motion_ctx.recording = true;
        }
    }

    if (preview)
    {
        motion_recording_label_update();
        Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, motion_sd_change_cb);
    }
}

static void motion_stop(void)
{
    if (s_motion_ctx.active)
    {
        /* 录像停止是阻塞的，返回时侦测录像已写入媒体库 */
        if (s_motion_ctx.recording)
            monitor_video_record_stop();
        monitor_video_close();
        if (s_motion_ctx.preview)
            lv_video_mode_enable(0);
    }

    s_motion_ctx.active = false;
    s_motion_ctx.recording = false;
    s_motion_ctx.record_time = 0;

    if (s_motion_ctx.timer)
    {
        lv_timer_del(s_motion_ctx.timer);
        s_motion_ctx.timer = NULL;
    }

    if (s_motion_ctx.preview)
    {
        lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), MOTION_HEADER_CONT_ID);
        if (obj)
            lv_obj_del(obj);
        obj = lv_obj_get_child_by_id(lv_scr_act_current(), MOTION_BACK_BTN_ID);
        if (obj)
            lv_obj_del(obj);

        /* 恢复待机背景容器显示 */
        lv_obj_t *bg = standby_bg_get();
        if (bg)
            lv_obj_clear_flag(bg, LV_OBJ_FLAG_HIDDEN);
    }

    /* 侦测录制的视频/照片已写入媒体库，刷新未查看个数 */
    standby_badge_counts_update();
    /* 含拍照模式的抓拍是异步保存，稍后再刷新一次 */
    if (s_motion_ctx.record_mode == RECORD_MODE_PHOTO ||
        s_motion_ctx.record_mode == RECORD_MODE_PHOTO_VIDEO)
        standby_badge_refresh_delayed();

    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);
    /* 侦测结束后恢复待机页的 SD 插拔提示回调（熄屏期间保持禁用，避免不可见弹窗拦截唤醒） */
    if (!s_screen_off)
    {
        Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, standby_tfcard_change_cb);
    }

    /* 预览结束后重新计时熄屏 */
    s_off_countdown = 0;

    /* 本次侦测处理完毕，重新开始冷却，留出操作待机页的时间 */
    s_motion_guard_until = lv_tick_get() + MOTION_GUARD_MS;
}

/* =========================== 移动侦测事件回调 =========================== */
static void standby_human_detect_cb(void *arg)
{
    if (motion_guard_active())
        return;

    int device = *(int *)arg;
    int channel = device == DEVICE_TYPE_DOOR_ID1 ? MON_CHANNEL_DOOR1 : MON_CHANNEL_DOOR2;
    if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2)
        return;
    int door_idx = (channel == MON_CHANNEL_DOOR1) ? 0 : 1;
    device_single_cfg *dev = &user_data_get()->public_cfg.door_device[door_idx];
    if (!dev->device_valid)
        return;
    if (dev->door_cfg.human_detect.detect_level == DETECT_LEVEL_OFF)
        return;
    int duration_ms = (int)dev->door_cfg.human_detect.alarm_interval * 1000;
    record_mode_type_def md_mode = dev->door_cfg.human_detect.record_mode;
    bool preview = user_data_get()->private_cfg.detection_preview_enable;
    monitor_channel_set(channel);
    motion_start(md_mode, duration_ms, preview);
}

/************************** 页面生命周期 **************************/

/* 待机背景音乐播放条件：开关打开、无移动侦测、SD 已挂载、非直接熄屏、非睡眠模式 */
static bool standby_bg_music_should_play(void)
{
    bool motion_off = (user_data_get()->public_cfg.door_device[0].door_cfg.human_detect.detect_level == DETECT_LEVEL_OFF &&
                       user_data_get()->public_cfg.door_device[1].door_cfg.human_detect.detect_level == DETECT_LEVEL_OFF);
    return user_data_get()->private_cfg.scene.bg_music_sw && motion_off &&
           Amy_tfcard_status() == SD_STATE_MOUNT &&
           user_data_get()->private_cfg.screen_saver_en != SCREEN_SAVER_DISABLE &&
           user_data_get()->public_cfg.global_func.scene_mode != SCENE_MODE_SLEEP;
}

static void Amy_lv_page_enter(standby)
{
    standby_timer_close();

    /* 进入待机页复位熄屏状态，并保证背光点亮（熄屏后切走再回来/重启等场景） */
    s_screen_off = false;
    s_off_countdown = 0;
    backlight_enable(true);
    backlight_set(user_data_get()->private_cfg.display_brightness);

    user_scene_info *scene = &user_data_get()->private_cfg.scene;
    bool motion_off = (user_data_get()->public_cfg.door_device[0].door_cfg.human_detect.detect_level == DETECT_LEVEL_OFF &&
                       user_data_get()->public_cfg.door_device[1].door_cfg.human_detect.detect_level == DETECT_LEVEL_OFF);
    bool sd_ready = (Amy_tfcard_status() == SD_STATE_MOUNT);

    /* 数码相框模式 */
    if (scene->digital_photo_frame_sw && motion_off && sd_ready &&
        standby_photo_frame_start(scene->digital_photo_sw_time) == 0)
    {
        /* 任何待机分支下插拔 SD 卡都弹提示框 */
        Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, standby_tfcard_change_cb);
    }
    else if (user_data_get()->private_cfg.screen_saver_en == SCREEN_SAVER_DISABLE)
    {
        standby_screen_off();
    }
    else
    {
        /* 普通待机模式 */
        standby_bg_create();
        standby_panel_create();
        standby_notification_icons_create();
        page_common_status_bar_create(standby_bg_get());
        standby_clock_create();
        standby_date_create();
        standby_weather_create();
        standby_aec_test_create();
        standby_update_time_label();
        standby_update_date_label();
        standby_update_weather_display();
        standby_adjust_layout_by_tuya();
        standby_badge_counts_update();
        standby_idle_timer_cb(NULL);
        /* 任何待机分支下插拔 SD 卡都弹提示框 */
        Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, standby_tfcard_change_cb);
    }

    /* 所有待机分支统一启动熄屏倒计时：无预览、无相框轮播时满 SCREEN_OFF_DELAY_S 秒关闭背光 */
    lv_timer_create_recycle(standby_idle_timer_cb, 1000, NULL);

    /* 背景音乐（睡眠模式不播放） */
    if (standby_bg_music_should_play())
        standby_bg_music_start(scene->bg_music_vol);
    /* 注册移动侦测回调（仅在待机页面生效），进入后先冷却一段时间再允许触发 */
    s_motion_guard_until = lv_tick_get() + MOTION_GUARD_MS;
    Amy_event_cb_register(AMY_EVENT_ID_HUMAN_DETECT, standby_human_detect_cb);

    /* 屏幕点击退出到主界面 */
    lv_obj_add_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_scr_act_current(), standby_screen_click_cb, LV_EVENT_CLICKED, NULL);
}

static void Amy_lv_page_quit(standby)
{
    /* 熄屏状态下切走（来电/返回主界面等）时先点亮背光，保证目标页可见 */
    standby_screen_wake();
    motion_stop();
    standby_photo_frame_stop();
    standby_bg_music_stop();
    Amy_event_cb_register(AMY_EVENT_ID_HUMAN_DETECT, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);

    /* 注销屏幕点击回调 */
    lv_obj_remove_event_cb(lv_scr_act_current(), standby_screen_click_cb);
    lv_obj_clear_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
    standby_timer_restart(true, true);
}

Amy_lv_page_create(standby);
