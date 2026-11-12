/**
 * @file page_tuya.c
 * @brief 涂鸦云在线监控页面（精简版，无底部按钮）
 * @author XX
 * @date 2025
 */
#include "page_tabel.h"
#include "device/lv_disp_dev.h"
#include "ak_common_graphics.h"
#include "ak_common.h"
#include "page_common.h"
#include "lv_common.h"
#include "call_api.h"
#include "page_common_call.h"
#include "live/testRTSPClient.h"
#include "../monitor.h"
#include "page_live.h"
#include "tuya_common.h"
#include "tuya/tuya_dp_callback.h"
#include "user_data.h"
#include "user_standby.h"
#include <time.h>
#include <stdint.h>

/* =========================== 页面上下文 =========================== */
static struct
{
  lv_timer_t *stream_timeout_timer;
  int stream_timeout_channel;
} s_ctx;

/* =========================== 静态函数声明 =========================== */
static void tuya_page_channel_label_display(void);
static void tuya_page_date_label_display(void);
static void tuya_page_channel_label_create(lv_obj_t *parent);
static void tuya_page_date_label_create(lv_obj_t *parent);
static void tuya_page_center_label_create(void);
static void tuya_page_top_cont_create(void);
static void tuya_page_ui_create_all(void);
static void tuya_page_timer_handle(lv_timer_t *t);
static void tuya_page_rendering_setting(void);
static void tuya_page_video_stop(int reserved);
static void tuya_page_switch_channel(int reserved);
static void tuya_page_audio_start(int reserved);
static void tuya_page_audio_stop(int reserved);

/* 拉流超时相关 */
static void tuya_page_stream_timeout_cb(void *arg);
static void tuya_page_stream_timeout_timer_handle(lv_timer_t *t);
static void tuya_page_stream_timeout_timer_create(void);
static void tuya_page_stream_timeout_timer_delete(void);
static void tuya_page_rtsp_get_frame_cb(void *arg);

/* =========================== UI创建实现 =========================== */

static void tuya_page_channel_label_display(void)
{
  lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), tuya_top_cont_id);
  lv_obj_t *channel_label = lv_obj_get_child_by_id(top_cont, tuya_channel_id);
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1)
    lv_label_set_text(channel_label, lang_str_get(STR_DOOR1));
  else if (channel == MON_CHANNEL_DOOR2)
    lv_label_set_text(channel_label, lang_str_get(STR_DOOR2));
  else if (channel == MON_CHANNEL_CCTV1)
    lv_label_set_text(channel_label, lang_str_get(STR_CAMERA1));
  else if (channel == MON_CHANNEL_CCTV2)
    lv_label_set_text(channel_label, lang_str_get(STR_CAMERA2));
}

static void tuya_page_channel_label_create(lv_obj_t *parent)
{
  lv_common_text_create(parent, tuya_channel_id, 0, 0, 150, 70,
                        NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        lang_str_get(STR_DOOR1),
                        0XFFFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_large);
  tuya_page_channel_label_display();
}

static void tuya_page_date_label_display(void)
{
  lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), tuya_top_cont_id);
  if (top_cont == NULL)
    return;
  lv_obj_t *date_label = lv_obj_get_child_by_id(top_cont, tuya_date_id);
  if (date_label == NULL)
    return;

  time_t now = time(NULL);
  struct tm local_tm;
  Amy_time2localtime(now, local_tm);
  lv_label_set_text_fmt(date_label, "%04d-%02d-%02d %02d:%02d:%02d",
                        local_tm.tm_year, local_tm.tm_mon, local_tm.tm_mday,
                        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
}

static void tuya_page_date_label_create(lv_obj_t *parent)
{
  lv_common_text_create(parent, tuya_date_id, 160, 0, 300, 70,
                        NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        "2025/01/01 00:00:00", 0XFFFFFFFF, 0xFFFFFF,
                        LV_TEXT_ALIGN_CENTER_CENTER, lv_font_large);
  tuya_page_date_label_display();
}

static void tuya_page_center_label_create(void)
{
  lv_common_text_create(lv_scr_act_current(), tuya_center_label_id,
                        312, 250, 400, 70,
                        NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        lang_str_get(STR_APP_PREVIEW),
                        0XFFFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

static void tuya_page_top_cont_create(void)
{
  lv_obj_t *cont = lv_common_img_btn_create(lv_scr_act_current(), tuya_top_cont_id,
                                            0, 0, 1024, 70,
                                            NULL, false, LV_OPA_70, 0x000000, LV_OPA_70, 0x000000,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            NULL, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
  tuya_page_channel_label_create(cont);
  tuya_page_date_label_create(cont);
}

static void tuya_page_ui_create_all(void)
{
  tuya_page_top_cont_create();
  tuya_page_center_label_create();
}

/* =========================== 定时器处理 =========================== */

static void tuya_page_timer_handle(lv_timer_t *t)
{
  tuya_page_channel_label_display();
  tuya_page_date_label_display();
}

/* =========================== 拉流超时处理 =========================== */

static void tuya_page_stream_timeout_cb(void *arg)
{
  int timeout_channel = *(int *)arg;
  int current_channel = monitor_channel_get();

  if (timeout_channel != current_channel)
    return;

  int next_channel = monitor_next_valid_channel_get(current_channel);
  if (next_channel == MON_CHANNEL_UNKNOW)
    return;

  monitor_channel_set(next_channel);
  sat_call_hangup_current();
  if (is_channel_ipc_camera(next_channel) == 0)
  {
    device_info_attr dev_info = SAT_DEVICE_INFO_DEFAULT;
    dev_info.building = user_data_get()->private_cfg.device.attr.building;
    dev_info.room = 0X00;
    dev_info.device_type = (next_channel == MON_CHANNEL_DOOR1)
                               ? DEVICE_TYPE_DOOR_ID1
                               : DEVICE_TYPE_DOOR_ID2;
    sat_call_dial(&dev_info);
    sat_call_switch_call(&dev_info);
  }

  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  monitor_ctx_set_tuya(true);

  Amy_lv_page_goto(lv_page(tuya), LV_SCR_LOAD_ANIM_NONE, );
}

static void tuya_page_stream_timeout_timer_handle(lv_timer_t *t)
{
  tuya_page_stream_timeout_timer_delete();
  tuya_page_stream_timeout_cb(&s_ctx.stream_timeout_channel);
}

static void tuya_page_stream_timeout_timer_create(void)
{
  s_ctx.stream_timeout_channel = monitor_channel_get();
  s_ctx.stream_timeout_timer = lv_timer_create(
      tuya_page_stream_timeout_timer_handle, 8000, NULL);
}

static void tuya_page_stream_timeout_timer_delete(void)
{
  if (s_ctx.stream_timeout_timer != NULL)
  {
    lv_timer_del(s_ctx.stream_timeout_timer);
    s_ctx.stream_timeout_timer = NULL;
  }
}

static void tuya_page_rtsp_get_frame_cb(void *arg)
{
  tuya_page_stream_timeout_timer_delete();
  video_mode_cb_attr *attr = (video_mode_cb_attr *)(arg);
  Amy_video_mode_register_device(attr->func, attr->user_data);
  lv_video_mode_enable(1);
}

/* =========================== 渲染区域设置 =========================== */

static void tuya_page_rendering_setting(void)
{
  lv_area_t area[] = {
      {0, 0, 1023, 69},
      {0, 250, 1023, 319},
  };
  for (int i = 0; i < sizeof(area) / sizeof(lv_area_t); i++)
  {
    Amy_rendering_area(&area[i]);
  }
}

/* =========================== Tuya事件处理 =========================== */

static void tuya_page_video_stop(int reserved)
{
  tuya_audio_disable();
  monitor_ctx_set_tuya(false);
  sat_call_hangup_current();
  Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

static void tuya_page_switch_channel(int reserved)
{
  int channel = reserved;
  if (monitor_channel_is_valid(channel) != 1)
    return;
  tuya_page_stream_timeout_timer_delete();
  int current_channel = monitor_channel_get();
  if (is_channel_ipc_camera(current_channel) == 0)
    sat_call_hangup_current();
  monitor_channel_set(channel);
  if (is_channel_ipc_camera(channel) == 0)
  {
    device_info_attr dev_info = SAT_DEVICE_INFO_DEFAULT;
    dev_info.building = user_data_get()->private_cfg.device.attr.building;
    dev_info.room = 0X00;
    dev_info.device_type = (channel == MON_CHANNEL_DOOR1)
                               ? DEVICE_TYPE_DOOR_ID1
                               : DEVICE_TYPE_DOOR_ID2;
    sat_call_dial(&dev_info);
  }
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  monitor_ctx_set_tuya(true);
  Amy_lv_page_goto(lv_page(tuya), LV_SCR_LOAD_ANIM_NONE, );
}

static void tuya_page_audio_start(int reserved)
{
  open_server_speaker();
}

static void tuya_page_audio_stop(int reserved)
{
  close_server_speaker();
  // tuya_audio_disable();
}

/* =========================== 页面生命周期 =========================== */

static void Amy_lv_page_enter(tuya)
{
  tuya_audio_enable();
  standby_timer_close();
  monitor_video_open();
  monitor_ctx_set_tuya(true);
  report_valid_channel_to_tuya();
  // 切通道会重新进入本页面，需按当前通道重报支持锁列表
  tuya_dp_access_lock_support_response();

  tuya_page_stream_timeout_timer_create();

  tuya_page_rendering_setting();
  tuya_page_ui_create_all();
  lv_timer_create_recycle(tuya_page_timer_handle, 1000, NULL);

  tuya_event_handler_register(TUYA_EVENT_CMD_VIDEO_STOP, tuya_page_video_stop);
  tuya_event_handler_register(TUYA_EVENT_CMD_SWITCH_CH, tuya_page_switch_channel);
  tuya_event_handler_register(TUYA_EVENT_CMD_AUDIO_START, tuya_page_audio_start);
  tuya_event_handler_register(TUYA_EVENT_CMD_AUDIO_STOP, tuya_page_audio_stop);

  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, tuya_page_rtsp_get_frame_cb);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
  Amy_event_cb_register(AMY_EVENT_ID_CALL, common_call_extern_call_callback);
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, page_default_device_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_NETWORK_CONFLICT, page_default_network_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
}

static void Amy_lv_page_quit(tuya)
{
  standby_timer_restart(true, true);
  tuya_page_stream_timeout_timer_delete();
  tuya_audio_disable();
  lv_video_mode_enable(0);
  monitor_video_close();
  monitor_ctx_set_tuya(false);
  tuya_event_callback_default();
  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, page_default_device_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_NETWORK_CONFLICT, page_default_network_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_CALL, common_call_extern_call_callback);
}

Amy_lv_page_create(tuya);
