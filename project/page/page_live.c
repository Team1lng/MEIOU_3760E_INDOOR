/**
 * @file monitor.c
 * @brief 监控页面功能实现
 * @author XX
 * @date 2025
 */
#include "page_tabel.h"
#include "device/lv_disp_dev.h"
#include "ak_common_graphics.h"
#include "ak_common.h"
#include "Amy_media_mgr.h"
#include "page_common.h"
#include "lv_common.h"
#include "Amy_tfcard.h"
#include "Amy_file_manager.h"
#include "Amy_music_player.h"
#include <time.h>
#include <stdint.h>
#include "call_api.h"
#include "sat_user_common.h"
#include "sat_ipcamera.h"
#include "tuya_common.h"
#include "live/testRTSPClient.h"
#include "../monitor.h"
#include "../gpio.h"
#include "../pwm.h"
#include "../intercom.h"
#include "page_common_call.h"
#include "page_live.h"
#include "user_data.h"
#include "user_standby.h"

/* =========================== 工具函数 =========================== */
static int ring_duration_to_sec(ring_duration_type_def d)
{
  switch (d)
  {
  case RING_DURATION_20S:
    return 20;
  case RING_DURATION_30S:
    return 30;
  case RING_DURATION_60S:
    return 60;
  default:
    return 30;
  }
}

/* =========================== 类型定义 =========================== */
typedef struct
{
  uint16_t timeout;
  bool door_open_status;
  bool gate_open_status;
  bool door2_open_status;
  monitor_state monitor_status;
  bool tuya;
  device_info_attr device_info;
} page_monitor_ctx_t;

/* =========================== 全局变量 =========================== */
static page_monitor_ctx_t g_page_monitor_ctx =
    {

};

/* =========================== 外部函数声明 =========================== */
void monitor_ctx_set_tuya(bool tuya);
void monitor_status_set(monitor_state state);

/* =========================== 静态函数声明 =========================== */
// 按钮事件处理
static void channel_switch_btn_click(lv_event_t *e);
static void snapshot_btn_click(lv_event_t *e);
static void video_record_btn_click(lv_event_t *e);
static void gate_open_btn_click(lv_event_t *e);
static void door_open_btn_click(lv_event_t *e);
static void door2_open_btn_click(lv_event_t *e);
static void adjust_btn_click(lv_event_t *e);
static void talk_btn_click(lv_event_t *e);
static void hangup_btn_click(lv_event_t *e);

// 事件回调函数
static void monitor_snapshort_event_cb(void *arg);
static void monitor_record_event_cb(void *arg);
static void monitor_tfcard_change_cb(void *arg);
static void monitor_inside_call_callback(void *arg);

// 通过网络命令设置门口机通话音量（TalkVolume 协议）
static void monitor_door_talk_volume_set(device_info_attr door, int volume);

// UI创建函数
static void monitor_channel_switch_btn_create(void);
static void monitor_snapshot_btn_create(void);
static void monitor_record_btn_create(void);
static void monitor_gate_open_btn_create(void);
static void monitor_door_open_btn_create(void);
static void monitor_door2_open_btn_create(void);
static void monitor_adjust_btn_create(void);
static void monitor_adjust_btn_display(void);
static void monitor_adjust_setting_cont_create(void);
static void monitor_adjust_setting_cont_display(void);
static void monitor_talk_btn_create(void);
static void monitor_hangup_btn_create(void);
static void monitor_action_btn_layout_update(void);

static void monitor_channel_label_create(lv_obj_t *parent);
static void monitor_date_label_create(lv_obj_t *parent);
static void monitor_timeout_label_create(lv_obj_t *parent);
static void monitor_top_cont_create(void);
static void monitor_ui_create_all(void);

// 显示更新函数
static void monitor_channel_switch_btn_display(void);
static void monitor_snapshot_btn_display(bool start);
static void monitor_record_btn_display(bool start);
static void monitor_gate_open_btn_display(void);
static void monitor_door_open_btn_display(void);
static void monitor_door2_open_btn_display(void);
static void monitor_talk_btn_display(void);
static void monitor_hangup_btn_display(void);
static void monitor_channel_label_display(void);
static void monitor_date_label_display(void);
static void monitor_timeout_label_display(void);
// 定时器处理函数
static void monitor_timer_handle(lv_timer_t *t);
// UI渲染区域设置函数
static void monitor_rendering_setting(void);
// 资源初始函数
static void monitor_resource_init(void);
static void monitor_media_init(void);
// 铃声播放
static void monitor_incoming_ring_play(void);
static void monitor_ring_stop(void);
// 留言模式
static void record_message_action(void);
static void record_message_task_cb(lv_timer_t *t);

// 会话处理函数
static void monitor_inside_incoming_handle(device_info_attr device_info);
static void monitor_inside_outgoing_ringing_handle(void);
static void monitor_inside_outgoing_process_handle(void);
static void monitor_inside_connected_handle(device_info_attr device_info);
static void monitor_end_handle(call_cb_attr *attr);

/* =========================== 内部工具函数实现 =========================== */
// 判断当前是否通话中（来去电通用）
static inline bool monitor_is_talking(void)
{
  monitor_state s = g_page_monitor_ctx.monitor_status;
  return s == MONITOR_INCOMING_TALKING || s == MONITOR_OUTGOING_TALKING;
}

// 判断当前是否来电方向
static inline bool monitor_is_incoming(void)
{
  monitor_state s = g_page_monitor_ctx.monitor_status;
  return s == MONITOR_INCOMING_CONNECTING || s == MONITOR_INCOMING_TALKING;
}

/* =========================== 页面上下文 =========================== */
static struct
{
  bool record_message_ing;
  lv_timer_t *record_message_task;
  lv_timer_t *chime_timer;
  lv_timer_t *ring_stop_task; /* 内置/自定义铃声播放指定时长后停止 */
  int custom_ring_select;     /* 本次自定义铃声的 select(1 基准)，单曲播完未到时重复播放用 */
  int auto_rec_timer;
  lv_timer_t *auto_record_close_task;
  bool snapshort_ing; /* 抓拍进行中 */
} s_ctx;

/* =========================== Chime 门钟控制 =========================== */

static void chime_timer_cb(lv_timer_t *t)
{
  chime_gpio_disable();
  s_ctx.chime_timer = NULL;
  lv_timer_del(t);
}

/* =========================== 自动录像定时关闭 =========================== */

static void auto_record_close_task_cb(lv_timer_t *t)
{
  s_ctx.auto_rec_timer++;
  int channel = monitor_channel_get();
  if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2)
  {
    lv_timer_del(s_ctx.auto_record_close_task);
    s_ctx.auto_record_close_task = NULL;
    return;
  }
  int record_time = user_data_get()->public_cfg.door_device[channel].door_cfg.record_time;
  if (s_ctx.auto_rec_timer > record_time)
  {
    monitor_video_record_stop();
    lv_timer_del(s_ctx.auto_record_close_task);
    s_ctx.auto_record_close_task = NULL;
  }
}

/* =========================== 铃声播放 =========================== */

/* 内置铃声单次播放结束但未到指定时长时返回非 0，让播放线程重复播放，直到被停止 */
static int monitor_ring_loop_cb(int index)
{
  return 1;
}

/* 内置/自定义铃声播放指定时长后自动停止（内部再走 monitor_ring_stop 统一停止） */
static void monitor_ring_stop_cb(lv_timer_t *t)
{
  monitor_ring_stop();
}

/* 统一停止当前呼叫铃声：内置铃声 + 自定义铃声通道一起停 */
static void monitor_ring_stop(void)
{
  Amy_audio_play_stop();
  if (s_ctx.ring_stop_task != NULL)
  {
    lv_timer_del(s_ctx.ring_stop_task);
    s_ctx.ring_stop_task = NULL;
  }
  Amy_music_player_set_finish_cb(NULL);
  Amy_music_player_stop(); /* 内部会清掉 finish_cb，避免停止后再触发重复播放 */
  s_ctx.custom_ring_select = 0;
}

/* 播放线程调用：自定义铃声单曲自然播完但未到指定时长时，入队事件交由 LVGL 线程重复播放，
   避免与停止定时器在播放线程/UI 线程间并发产生“停止后又重播”的竞态 */
static void monitor_custom_ring_finish_cb(void)
{
  Amy_music_finish_event_put();
}

/* LVGL 线程事件回调：重复播放同一首自定义铃声，直到停止定时器触发或呼叫结束 */
static void monitor_custom_ring_finish_event_cb(void *arg)
{
  (void)arg;
  /* 停止定时器仍挂起 = 本次响铃还在指定时长内；超时/挂断/留言切换后定时器已被删除，不再重复 */
  if (s_ctx.ring_stop_task == NULL || s_ctx.custom_ring_select <= 0)
    return;
  if (s_ctx.custom_ring_select > Amy_fm_get_count(FM_DIR_MUSIC))
    return;
  Amy_music_player_play(s_ctx.custom_ring_select - 1);
}

/* 启动/重置铃声停止定时器：ring_time 秒后统一停止当前铃声（内置与自定义共用） */
static void monitor_ring_stop_timer_start(int ring_time)
{
  if (s_ctx.ring_stop_task != NULL)
  {
    lv_timer_del(s_ctx.ring_stop_task);
    s_ctx.ring_stop_task = NULL;
  }
  s_ctx.ring_stop_task = lv_timer_create(monitor_ring_stop_cb, (uint32_t)ring_time * 1000, NULL);
}

static void monitor_incoming_ring_play(void)
{
  int channel = monitor_channel_get();
  if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2)
    return;

  door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[channel].door_cfg;

  /* 门呼叫响铃应用三个时段：命中时段取对应 Ring，未命中不响铃 */
  time_t now = time(NULL);
  struct tm local_tm;
  Amy_time2localtime(now, local_tm);
  int active = ringtone_schedule_active_index(cfg, local_tm.tm_hour, local_tm.tm_min);
  if (active < 0)
    return;

  ringtone_config_type_def *rt = &cfg->ringtone[active];
  int ring_time = ring_duration_to_sec(rt->duration);

  /* 播放音量取该 Ring 的 volume(0-6)，不再用全局 doorbell_ring_volume */
  int ring_volume = (rt->volume * 100) / 6;
  if (ring_volume < 1)
    ring_volume = 1;
  if (ring_volume > 100)
    ring_volume = 100;

  if (Amy_tfcard_status() == SD_STATE_MOUNT && cfg->message_sw)
  {
    g_page_monitor_ctx.timeout = (uint16_t)(ring_time + cfg->message_time);
  }
  else
  {
    g_page_monitor_ctx.timeout = (uint16_t)ring_time;
  }

  /* 音量 0：静音，不播报铃声（监控画面与超时照常） */
  if (rt->volume <= 0)
    return;

  /* Customized 且 SD 有可用音乐：播放对应自定义铃声，ring_time 后停止 */
  if (rt->mode == 1)
  {
    Amy_fm_refresh(FM_DIR_MUSIC);
    int music_count = Amy_fm_get_count(FM_DIR_MUSIC);
    if (music_count > 0 && rt->select >= 1 && rt->select <= music_count)
    {
      Amy_music_player_set_finish_cb(NULL);
      Amy_music_player_stop();
      Amy_music_player_set_volume(ring_volume);
      s_ctx.custom_ring_select = rt->select;
      Amy_music_player_set_finish_cb(monitor_custom_ring_finish_cb); /* 单曲播完未到时重复播放 */
      Amy_music_player_play(rt->select - 1);
      monitor_ring_stop_timer_start(ring_time);
      return;
    }
  }

  /* Standard 或自定义不可用：播内置铃声（rt->type ∈ 1..6 即资源索引，type=1 → RING_7 默认音色）。
     与自定义铃声一致播放 ring_time 秒；单次播完未到时长则重复播放，到时停止 */
  Amy_audio_play(rt->type, ring_volume, true, NULL, monitor_ring_loop_cb);
  monitor_ring_stop_timer_start(ring_time);
}

/* =========================== 留言录制 =========================== */

static void record_message_task_cb(lv_timer_t *t)
{
  s_ctx.record_message_ing = true;

  /* 留言录制前先结束上一段自动录像，让自动录像与留言各成一段；
     自动录像的定时关闭计时随之作废，避免按 record_time 提前截断留言录像 */
  monitor_video_record_stop();
  if (s_ctx.auto_record_close_task != NULL)
  {
    lv_timer_del(s_ctx.auto_record_close_task);
    s_ctx.auto_record_close_task = NULL;
  }

  monitor_video_record_start(MEDIA_MODE_MESSAGE);

  s_ctx.record_message_task = NULL;
  lv_timer_del(t);
}

static void record_message_action(void)
{
  s_ctx.record_message_ing = true;

  if (s_ctx.record_message_task == NULL)
  {
    s_ctx.record_message_task = lv_timer_create(record_message_task_cb, 2000, NULL);
  }
}

/* =========================== 外部函数实现 =========================== */

void monitor_status_set(monitor_state state)
{
  g_page_monitor_ctx.monitor_status = state;
}

void monitor_ctx_set_tuya(bool tuya)
{
  g_page_monitor_ctx.tuya = tuya;
}

void monitor_device_info_set(const device_info_attr *info)
{
  if (info == NULL)
    return;
  memcpy(&g_page_monitor_ctx.device_info, info, sizeof(device_info_attr));
}

// 获取云平台监控状态
bool monitor_ctx_get_tuya(void)
{
  bool ret = false;

  ret = g_page_monitor_ctx.tuya;

  return ret;
}

/* =========================== 会话处理实现 =========================== */

/**
 * @brief 通过网络通信更改门口机音频输出大小（TalkVolume 协议）
 * @param door 目标门口机设备信息
 * @param volume 音量 1~10
 */
static void monitor_door_talk_volume_set(device_info_attr door, int volume)
{
  if (door.device_type != DEVICE_TYPE_DOOR_ID1 && door.device_type != DEVICE_TYPE_DOOR_ID2)
    return;

  char ip[32] = {0};
  if (generate_ip(door.building, door.room, door.device_type, ip) != 0)
    return;

  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_talk_volume(info->device_type, info->building, info->room,
                                  ip, 80, "admin", "123456789", volume, 1000, false);
}

static void monitor_inside_incoming_handle(device_info_attr device_info)
{
  if (g_page_monitor_ctx.monitor_status == MONITOR_SESSION_CLOSED)
  {
    common_call_incoming_handle(device_info);
  }
}

static void monitor_inside_outgoing_ringing_handle(void)
{
  monitor_video_open();
}

static void monitor_inside_outgoing_process_handle(void)
{
}

static void monitor_inside_connected_handle(device_info_attr device_info)
{
}

static void monitor_end_notify_timer_cb(lv_timer_t *t)
{
  lv_common_screen_click_unlock();
  lv_timer_del(t);
  common_call_session_complete_handle();
}

static void monitor_end_handle(call_cb_attr *attr)
{
  if (memcmp(&attr->device_info, &g_page_monitor_ctx.device_info, sizeof(device_info_attr)) == 0)
  {
    g_page_monitor_ctx.monitor_status = MONITOR_SESSION_CLOSED;
    switch (attr->reason)
    {
    case REASON_BUSY:
      lv_common_screen_click_lock();
      lv_common_msgbox_notify_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_DEVICE_BUSY));
      lv_timer_create_recycle(monitor_end_notify_timer_cb, 3000, NULL);
      break;
    case REASON_NO_RESPONSE:
#if 0
      lv_common_screen_click_lock();
      lv_common_msgbox_notify_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_CALL_TIMEOUT));
      lv_timer_create_recycle(monitor_end_notify_timer_cb, 3000, NULL);
#endif
      break;
    case REASON_NO_INTERACT_TIMEOUT:
      lv_common_screen_click_lock();
      lv_common_msgbox_notify_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_WAIT_TIMEOUT));
      lv_timer_create_recycle(monitor_end_notify_timer_cb, 3000, NULL);
      break;
    case REASON_MAX_DURATION_TIMEOUT:
      lv_common_screen_click_lock();
      lv_common_msgbox_notify_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_CALL_TIMED_OUT));
      lv_timer_create_recycle(monitor_end_notify_timer_cb, 3000, NULL);
      break;
    case REASON_HANGUP:
      common_call_session_complete_handle();
      break;
    default:
      break;
    }
  }
}

/* =========================== 按钮事件实现 =========================== */
/**
 * @brief 通道切换按钮点击回调
 */
static void channel_switch_btn_click(lv_event_t *e)
{
  device_info_attr pending_device_info[10];
  int pending_count = 0;
  sat_get_all_pending_calls(pending_device_info, 10, &pending_count);
  if (pending_count > 0)
  {
    return;
  }

  int current_channel = monitor_channel_get();
  int next_channel = current_channel;

  if (current_channel == MON_CHANNEL_DOOR1)
  {
    if (monitor_channel_is_valid(MON_CHANNEL_CCTV1) == 1)
      next_channel = MON_CHANNEL_CCTV1;
    else if (monitor_channel_is_valid(MON_CHANNEL_DOOR2) == 1)
      next_channel = MON_CHANNEL_DOOR2;
    else if (monitor_channel_is_valid(MON_CHANNEL_CCTV2) == 1)
      next_channel = MON_CHANNEL_CCTV2;
    else
      next_channel = MON_CHANNEL_DOOR1; /* 原地保持不变 */
  }
  else if (current_channel == MON_CHANNEL_CCTV1)
  {
    if (monitor_channel_is_valid(MON_CHANNEL_DOOR2) == 1)
      next_channel = MON_CHANNEL_DOOR2;
    else if (monitor_channel_is_valid(MON_CHANNEL_CCTV2) == 1)
      next_channel = MON_CHANNEL_CCTV2;
    else
      next_channel = MON_CHANNEL_DOOR1;
  }
  else if (current_channel == MON_CHANNEL_DOOR2)
  {
    if (monitor_channel_is_valid(MON_CHANNEL_CCTV2) == 1)
      next_channel = MON_CHANNEL_CCTV2;
    else
      next_channel = MON_CHANNEL_DOOR1;
  }
  else if (current_channel == MON_CHANNEL_CCTV2)
  {
    next_channel = MON_CHANNEL_DOOR1;
  }
  else
  {
    return;
  }

  if (next_channel == current_channel)
    return;

  if (is_channel_ipc_camera(current_channel) == 0)
  {
    sat_call_hangup_current();
  }

  monitor_channel_set(next_channel);
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);

  if (is_channel_ipc_camera(next_channel) == 0)
  {
    device_info_attr dev_info = SAT_DEVICE_INFO_DEFAULT;
    dev_info.building = user_data_get()->private_cfg.device.attr.building;
    dev_info.room = 0X00;
    dev_info.device_type = (next_channel == MON_CHANNEL_DOOR1)
                               ? DEVICE_TYPE_DOOR_ID1
                               : DEVICE_TYPE_DOOR_ID2;
    sat_call_dial(&dev_info);
    monitor_device_info_set(&dev_info);
  }

  monitor_ctx_set_tuya(false);

  Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}
static void page_live_msgbox_confirm_cb(lv_event_t *ev)
{
  lv_common_screen_click_unlock();
  lv_common_msgbox_del();
}
/**
 * @brief 抓拍按钮点击回调
 */
static void snapshot_btn_click(lv_event_t *e)
{
  int status = Amy_tfcard_status();
  if (!(status & SD_STATE_INSERT))
  {
    lv_common_screen_click_lock();
    lv_common_msgbox_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_PLEASE_INSTER_SD), page_live_msgbox_confirm_cb, NULL);
    return;
  }
  if (Amy_tfcard_ensure_free_percent(10) != 0)
  {
    lv_common_screen_click_lock();
    lv_common_msgbox_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_SD_NO_MEMORY), page_live_msgbox_confirm_cb, NULL);
    return;
  }
  if (monitor_video_snapshort(MEDIA_MODE_MANUAL, true) == 0)
  {
    monitor_snapshot_btn_display(true);
  }
}

/**
 * @brief 录像按钮点击回调
 */
static void video_record_btn_click(lv_event_t *e)
{
  int status = Amy_tfcard_status();
  if (!(status & SD_STATE_INSERT))
  {
    lv_common_screen_click_lock();
    lv_common_msgbox_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_PLEASE_INSTER_SD), page_live_msgbox_confirm_cb, NULL);
    return;
  }
  if (Amy_tfcard_ensure_free_percent(10) != 0)
  {
    lv_common_screen_click_lock();
    lv_common_msgbox_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_SD_NO_MEMORY), page_live_msgbox_confirm_cb, NULL);
    return;
  }
  if (monitor_video_record_start(MEDIA_MODE_MANUAL))
  {
    monitor_video_record_stop();
  }
  else
  {
    monitor_record_btn_display(true);
  }
}

static void monitor_ota_event_cb(void *arg)
{
  ota_event_t *ev = (ota_event_t *)arg;
  if (ev->state != OTA_STATE_DOWNLOAD_START)
    return;

  sat_call_hangup_all();
  monitor_video_record_stop();
  monitor_video_close();
  monitor_ring_stop();
  Amy_lv_page_goto(lv_page(ota), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_gate_open_release(lv_timer_t *t)
{
  g_page_monitor_ctx.gate_open_status = false;
  monitor_gate_open_btn_display();
  lv_timer_del(t);
}

static void gate_open_btn_click(lv_event_t *e)
{
  if (g_page_monitor_ctx.gate_open_status == true)
    return;

  int current_channel = monitor_channel_get();
  if (current_channel != MON_CHANNEL_DOOR1 && current_channel != MON_CHANNEL_DOOR2)
    return;
  door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[current_channel].door_cfg;
  int lock_time = cfg->gate_lock_close_time;
  tuya_ungate2_with_dp();
  g_page_monitor_ctx.gate_open_status = true;
  monitor_gate_open_btn_display();
  lv_timer_create_recycle(monitor_gate_open_release, lock_time * 1000, NULL);
}

static void monitor_door_open_release(lv_timer_t *t)
{
  g_page_monitor_ctx.door_open_status = false;
  monitor_door_open_btn_display();
  lv_timer_del(t);
}

static void monitor_door2_open_release(lv_timer_t *t)
{
  g_page_monitor_ctx.door2_open_status = false;
  monitor_door2_open_btn_display();
  lv_timer_del(t);
}

/**
 * @brief 开门按钮点击回调
 */
static void door_open_btn_click(lv_event_t *e)
{
  if (g_page_monitor_ctx.door_open_status == true)
    return;

  int current_channel = monitor_channel_get();
  if (current_channel != MON_CHANNEL_DOOR1 && current_channel != MON_CHANNEL_DOOR2)
    return;
  door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[current_channel].door_cfg;
  int lock_time = cfg->gate_lock_close_time;
  tuya_unlock_lock1_with_dp(current_channel);
  g_page_monitor_ctx.door_open_status = true;
  monitor_door_open_btn_display();
  lv_timer_create_recycle(monitor_door_open_release, lock_time * 1000, NULL);
}

/**
 * @brief 开锁2按钮点击回调
 */
static void door2_open_btn_click(lv_event_t *e)
{
  if (g_page_monitor_ctx.door2_open_status == true)
    return;

  int current_channel = monitor_channel_get();
  if (current_channel != MON_CHANNEL_DOOR1 && current_channel != MON_CHANNEL_DOOR2)
    return;
  door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[current_channel].door_cfg;
  int lock_time = cfg->gate_lock_close_time;
  tuya_unlock_lock2_with_dp(current_channel);
  g_page_monitor_ctx.door2_open_status = true;
  monitor_door2_open_btn_display();
  lv_timer_create_recycle(monitor_door2_open_release, lock_time * 1000, NULL);
}

/**
 * @brief 对讲按钮点击回调
 */
static void talk_btn_click(lv_event_t *e)
{
  if (monitor_is_talking() ||
      g_page_monitor_ctx.monitor_status == MONITOR_SESSION_CLOSED)
  {
    return;
  }
  if (g_page_monitor_ctx.tuya == true)
  {
    return;
  }
  g_page_monitor_ctx.monitor_status = monitor_is_incoming()
                                          ? MONITOR_INCOMING_TALKING
                                          : MONITOR_OUTGOING_TALKING;
  g_page_monitor_ctx.timeout = 120;
  monitor_ring_stop();
  open_client_speaker();
  open_server_speaker();
  sat_call_answer_current();
  monitor_talk_btn_display();
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[channel].door_cfg;
    monitor_talk_volume_set(cfg->indoor_talk_volume);
    monitor_door_talk_volume_set(g_page_monitor_ctx.device_info, cfg->outdoor_talk_volume);
  }
}

/**
 * @brief 挂断按钮点击回调
 */
static void hangup_btn_click(lv_event_t *e)
{
  sat_call_hangup_current();
  common_call_session_complete_handle();
}

/**
 * @brief 调节按钮点击回调 —— 切换设置面板显示/隐藏
 */
static void adjust_btn_click(lv_event_t *e)
{
  monitor_adjust_setting_cont_display();
}

/* =========================== 事件回调实现 =========================== */

/**
 * @brief 视频模式回调处理
 */
static void monitor_rtsp_get_frame_cb(void *arg)
{
  video_mode_cb_attr *attr = (video_mode_cb_attr *)(arg);
  Amy_video_mode_register_device(attr->func, attr->user_data);
  lv_video_mode_enable(1);
}

/**
 * @brief 抓拍状态事件回调
 */
static void monitor_snapshort_event_cb(void *arg)
{
  int start = *(int *)arg;
  s_ctx.snapshort_ing = (start != 0);
  monitor_snapshot_btn_display(start);
  /* 抓拍开始/结束时立即刷新右上角状态，不等下一秒定时器 */
  monitor_timeout_label_display();
}

/**
 * @brief 录像状态事件回调
 */
static void monitor_record_event_cb(void *arg)
{
  int start = *(int *)arg;
  monitor_record_btn_display(start != 0);
  /* 录像开始/结束时立即刷新右上角 REC 状态，不等下一秒定时器 */
  monitor_timeout_label_display();
}

/**
 * @brief 监控页拔卡处理：自定义铃声停止播放，停一切录像/抓拍，右上角回落纯倒计时
 */
static void monitor_sd_unplug_handle(void)
{
  /* 自定义铃声音源在 SD：先停播放；monitor_ring_stop 会清 custom_ring_select，
     使 monitor_custom_ring_finish_event_cb 的重复播放守卫失效，不会继续重播 */
  if (s_ctx.custom_ring_select > 0)
    monitor_ring_stop();

  /* 停录像（自动/手动/留言），并作废自动录像的定时关闭 */
  if (monitor_video_record_mode_get() != -1)
    monitor_video_record_stop();
  if (s_ctx.auto_record_close_task != NULL)
  {
    lv_timer_del(s_ctx.auto_record_close_task);
    s_ctx.auto_record_close_task = NULL;
  }

  /* 取消尚未开始的留言录制，清进行中的留言状态 */
  if (s_ctx.record_message_task != NULL)
  {
    lv_timer_del(s_ctx.record_message_task);
    s_ctx.record_message_task = NULL;
  }
  s_ctx.record_message_ing = false;

  /* 抓拍无停止接口，仅复位状态与图标；在途抓拍会因 SD 不在而保存失败并自行回调复位 */
  s_ctx.snapshort_ing = false;
  monitor_snapshot_btn_display(false);
  monitor_record_btn_display(false);

  monitor_timeout_label_display();
}

/**
 * @brief TF 卡状态变化
 */
static void monitor_tfcard_change_cb(void *arg)
{
  page_common_tfcard_change_cb(arg);
  if (Amy_tfcard_status() & SD_STATE_INSERT)
    return;
  monitor_sd_unplug_handle();
}

/**
 *@brief
 *
 *@param arg
 */
static void monitor_inside_call_callback(void *arg)
{
  call_cb_attr *attr = (call_cb_attr *)arg;
  int call_event = attr->call_event;
  // int device_type = attr->device_info.device_type;
  switch (call_event)
  {
  case AMY_CALL_EVENT_ID_CALL_INCOMING:
    monitor_inside_incoming_handle(attr->device_info);
    break;
  case AMY_CALL_EVENT_ID_CALL_OUTGOING_RINGING:
    monitor_inside_outgoing_ringing_handle();
    break;
  case AMY_CALL_EVENT_ID_CALL_OUTGOING_PROCESS:
    monitor_inside_outgoing_process_handle();
    break;
  case AMY_CALL_EVENT_ID_CALL_CONNECTED:
    monitor_inside_connected_handle(attr->device_info);
    break;
  case AMY_CALL_EVENT_ID_CALL_END:
    monitor_end_handle(attr);
    break;
  default:
    break;
  }
}
static void monitor_tuya_video_start(int reserved)
{
  if (monitor_is_talking() || g_page_monitor_ctx.monitor_status == MONITOR_SESSION_CLOSED)
  {
    return;
  }

  Amy_lv_page_goto(lv_page(tuya), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_tuya_light_control(int open)
{
  int channel = monitor_channel_get();
  if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2)
    return;

  struct ipcamera_info *door_device = &user_data_get()->public_cfg.net_door_device[channel];
  // TODO: 发送 NET_COMMON_CMD_LIGHT (0x53) 到门口机控制灯光
  // network_send_cmd_data(device, NET_COMMON_CMD_LIGHT, open ? 1 : 0);
  // TODO: 上报涂鸦灯光开关状态
  // tuya_dp_138_response_light_switch(open);
  AMY_LOG_INFO("monitor light control: %s, ip=%s\r\n", open ? "on" : "off", door_device->ipaddr);
}

static void monitor_tuya_atmosphere_handler(int reserved)
{
  atmosphere_ctrl((bool)reserved);
}

/* =========================== UI创建实现 =========================== */
/**
 * @brief 通道切换按钮显示控制
 */
static void monitor_channel_switch_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_channel_switch_id);
  if (obj == NULL)
    return;

  if (g_page_monitor_ctx.tuya)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_img_opa(obj, LV_OPA_20, LV_PART_MAIN);
  }
  else
  {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_img_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  }
  monitor_action_btn_layout_update();
}
/**
 * @brief 创建通道切换按钮
 */
static void monitor_channel_switch_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_MONITOR_PNG_OFFSET, ICON_LIVE_MONITOR_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_MONITOR_FOCUS_PNG_OFFSET, ICON_LIVE_MONITOR_FOCUS_PNG_SIZE);
  lv_obj_t *c = lv_common_img_btn_create(lv_scr_act_current(), monitor_channel_switch_id, 218, 493, 87, 87,
                                         channel_switch_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(c, 21);
  monitor_channel_switch_btn_display();
}

/**
 * @brief 抓拍按钮显示控制
 */
static void monitor_snapshot_btn_display(bool start)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_snapshot_id);
  if (obj == NULL)
    return;
  if (start)
  {
    lv_resource_init(icon, ICON_LIVE_SNAPING_PNG_OFFSET, ICON_LIVE_SNAPING_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_LIVE_SNAPING_FOCUS_PNG_OFFSET, ICON_LIVE_SNAPING_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  else
  {
    lv_resource_init(icon, ICON_LIVE_SNAP_PNG_OFFSET, ICON_LIVE_SNAP_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_LIVE_SNAP_FOCUS_PNG_OFFSET, ICON_LIVE_SNAP_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  monitor_action_btn_layout_update();
}
/**
 * @brief 创建抓拍按钮
 */
static void monitor_snapshot_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_SNAP_PNG_OFFSET, ICON_LIVE_SNAP_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_SNAP_FOCUS_PNG_OFFSET, ICON_LIVE_SNAP_FOCUS_PNG_SIZE);
  lv_obj_t *s = lv_common_img_btn_create(lv_scr_act_current(), monitor_snapshot_id, 308, 493, 87, 87,
                                         snapshot_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(s, 21);
  monitor_snapshot_btn_display(false);
}

/**
 * @brief 录像按钮显示控制
 */
static void monitor_record_btn_display(bool start)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_video_record_id);
  if (obj == NULL)
    return;
  if (start)
  {
    lv_resource_init(icon, ICON_LIVE_RECORDING_UNFOCUS_PNG_OFFSET, ICON_LIVE_RECORDING_UNFOCUS_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_LIVE_RECORDING_FOCUS_PNG_OFFSET, ICON_LIVE_RECORDING_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  else
  {
    lv_resource_init(icon, ICON_LIVE_RECORD_PNG_OFFSET, ICON_LIVE_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_LIVE_RECORD_FOCUS_PNG_OFFSET, ICON_LIVE_RECORD_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  monitor_action_btn_layout_update();
}

/**
 * @brief 创建录像按钮
 */
static void monitor_record_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_RECORD_PNG_OFFSET, ICON_LIVE_RECORD_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_RECORD_FOCUS_PNG_OFFSET, ICON_LIVE_RECORD_FOCUS_PNG_SIZE);
  lv_obj_t *r = lv_common_img_btn_create(lv_scr_act_current(), monitor_video_record_id, 386, 493, 87, 87,
                                         video_record_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(r, 21);
  monitor_record_btn_display(false);
}

static void monitor_gate_open_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_gate_open_id);
  if (obj == NULL)
    return;
  int channel = monitor_channel_get();

  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    if (g_page_monitor_ctx.gate_open_status != true)
    {
      lv_resource_init(icon, ICON_LIVE_GATE_LOCK_PNG_OFFSET, ICON_LIVE_GATE_LOCK_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_GATE_LOCK_FOCUS_PNG_OFFSET, ICON_LIVE_GATE_LOCK_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
    else
    {
      lv_resource_init(icon, ICON_LIVE_GATE_UNLOCK_PNG_OFFSET, ICON_LIVE_GATE_UNLOCK_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_GATE_UNLOCK_FOCUS_PNG_OFFSET, ICON_LIVE_GATE_UNLOCK_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
  }
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
  {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
  monitor_action_btn_layout_update();
}

static void monitor_gate_open_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_GATE_LOCK_PNG_OFFSET, ICON_LIVE_GATE_LOCK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_GATE_LOCK_FOCUS_PNG_OFFSET, ICON_LIVE_GATE_LOCK_FOCUS_PNG_SIZE);
  lv_obj_t *g = lv_common_img_btn_create(lv_scr_act_current(), monitor_gate_open_id, 470, 493, 87, 87,
                                         gate_open_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(g, 21);
  monitor_gate_open_btn_display();
}

/**
 * @brief 开门按钮显示控制
 */
static void monitor_door_open_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_door_open_id);
  if (obj == NULL)
    return;
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    if (g_page_monitor_ctx.door_open_status != true)
    {
      lv_resource_init(icon, ICON_LIVE_LOCK1_PNG_OFFSET, ICON_LIVE_LOCK1_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_LOCK1_FOCUS_PNG_OFFSET, ICON_LIVE_LOCK1_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
    else
    {
      lv_resource_init(icon, ICON_LIVE_UNLOCK1_PNG_OFFSET, ICON_LIVE_UNLOCK1_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_UNLOCK1_FOUCS_PNG_OFFSET, ICON_LIVE_UNLOCK1_FOUCS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
  }
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
  {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
  monitor_action_btn_layout_update();
}

/**
 * @brief 创建开门按钮
 */
static void monitor_door_open_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_LOCK1_PNG_OFFSET, ICON_LIVE_LOCK1_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_LOCK1_FOCUS_PNG_OFFSET, ICON_LIVE_LOCK1_FOCUS_PNG_SIZE);
  lv_obj_t *d = lv_common_img_btn_create(lv_scr_act_current(), monitor_door_open_id, 557, 493, 87, 87,
                                         door_open_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(d, 21);
  monitor_door_open_btn_display();
}

/**
 * @brief 开锁2按钮显示控制
 */
static void monitor_door2_open_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_door2_open_id);
  if (obj == NULL)
    return;
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    if (g_page_monitor_ctx.door2_open_status != true)
    {
      lv_resource_init(icon, ICON_LIVE_LOCK2_PNG_OFFSET, ICON_LIVE_LOCK2_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_LOCK2_FOCUS_PNG_OFFSET, ICON_LIVE_LOCK2_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
    else
    {
      lv_resource_init(icon, ICON_LIVE_UNLOCK2_PNG_OFFSET, ICON_LIVE_UNLOCK2_PNG_SIZE);
      lv_resource_init(icon_focus, ICON_LIVE_UNLOCK2_FOCUS_PNG_OFFSET, ICON_LIVE_UNLOCK2_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
    }
  }
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
  {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
  monitor_action_btn_layout_update();
}

/**
 * @brief 创建开锁2按钮
 */
static void monitor_door2_open_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_LOCK2_PNG_OFFSET, ICON_LIVE_LOCK2_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_LOCK2_FOCUS_PNG_OFFSET, ICON_LIVE_LOCK2_FOCUS_PNG_SIZE);
  lv_obj_t *d2 = lv_common_img_btn_create(lv_scr_act_current(), monitor_door2_open_id, 644, 493, 87, 87,
                                          door2_open_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(d2, 21);
  monitor_door2_open_btn_display();
}

/**
 * @brief 调节按钮显示控制
 */
static void monitor_adjust_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_adjust_id);
  if (obj == NULL)
    return;
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  monitor_action_btn_layout_update();
}

/**
 * @brief 创建调节按钮
 */
static void monitor_adjust_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_ADJUST_PNG_OFFSET, ICON_LIVE_ADJUST_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_ADJUST_FOCUS_PNG_OFFSET, ICON_LIVE_ADJUST_FOCUS_PNG_SIZE);
  lv_obj_t *a = lv_common_img_btn_create(lv_scr_act_current(), monitor_adjust_id, -6, 493, 87, 87,
                                         adjust_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(a, 21);
  monitor_adjust_btn_display();
}

/* ========== 调节设置面板 ========== */

static void monitor_adjust_slider_sub_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_user_data(e);
  int value = lv_slider_get_value(slider);
  int min = lv_slider_get_min_value(slider);
  if (value > min)
  {
    lv_slider_set_value(slider, value - 1, LV_ANIM_ON);
    lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
  }
}

static void monitor_adjust_slider_add_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_user_data(e);
  int value = lv_slider_get_value(slider);
  int max = lv_slider_get_max_value(slider);
  if (value < max)
  {
    lv_slider_set_value(slider, value + 1, LV_ANIM_ON);
    lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
  }
}

void monitor_adjust_door_volume_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_current_target(e);
  int value = lv_slider_get_value(slider);
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    user_data_get()->public_cfg.door_device[channel].door_cfg.outdoor_talk_volume = value;
    monitor_door_talk_volume_set(g_page_monitor_ctx.device_info, value);
    user_data_save();
  }
}

static void monitor_adjust_indoor_volume_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_current_target(e);
  int value = lv_slider_get_value(slider);
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    user_data_get()->public_cfg.door_device[channel].door_cfg.indoor_talk_volume = value;
    monitor_talk_volume_set(value);
    user_data_save();
  }
}

static void monitor_adjust_brightness_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_current_target(e);
  int value = lv_slider_get_value(slider) * 10;
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
    user_data_get()->public_cfg.door_device[channel].door_cfg.display_brightness = value;
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
    user_data_get()->public_cfg.cctv_display_brightness[channel - MON_CHANNEL_CCTV1] = value;
  backlight_set(value);
  user_data_save();
}

static void monitor_adjust_setting_cont_display(void)
{
  lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_adjust_setting_cont_id);
  if (cont == NULL)
  {
    monitor_adjust_setting_cont_create();
    return;
  }
  if (lv_obj_has_flag(cont, LV_OBJ_FLAG_HIDDEN))
  {
    lv_obj_del(cont);
    monitor_adjust_setting_cont_create();
  }
  else
  {
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
  }
}

static void monitor_adjust_setting_cont_create(void)
{
  int channel = monitor_channel_get();
  bool is_cctv = (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2);

  lv_obj_t *cont = lv_common_setting_cont_create(lv_scr_act_current(), monitor_adjust_setting_cont_id,
                                                 0, 0, 324, 0,
                                                 NULL, LV_OPA_80, 0x202020, LV_OPA_80, 0x202020,
                                                 10, 1, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0x404040,
                                                 10, 1, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0x404040);
  lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);

  if (is_cctv)
  {
    lv_obj_set_pos(cont, 450, 280);
    lv_obj_set_size(cont, 324, 70);
  }
  else
  {
    lv_obj_set_pos(cont, 450, 207);
    lv_obj_set_size(cont, 324, 226);
  }

  int row_y[3] = {15, 80, 145};

  if (!is_cctv)
  {
    /* ---- 门口机声音大小 ---- */
    lv_resource_init(icon_door_sub, ICON_LIVE_DOOR_VOLUME_SUB_PNG_OFFSET, ICON_LIVE_DOOR_VOLUME_SUB_PNG_SIZE);
    lv_resource_init(icon_door_sub_focus, ICON_LIVE_DOOR_VOLUME_SUB_FOCUS_PNG_OFFSET, ICON_LIVE_DOOR_VOLUME_SUB_FOCUS_PNG_SIZE);
    lv_resource_init(icon_door_add, ICON_LIVE_DOOR_VOLUME_ADD_PNG_OFFSET, ICON_LIVE_DOOR_VOLUME_ADD_PNG_SIZE);
    lv_resource_init(icon_door_add_focus, ICON_LIVE_DOOR_VOLUME_ADD_FOCUS_PNG_OFFSET, ICON_LIVE_DOOR_VOLUME_ADD_FOCUS_PNG_SIZE);

    lv_obj_t *slider1 = lv_slider_create(cont);
    lv_obj_set_pos(slider1, 60, row_y[0] + 17);
    lv_obj_set_size(slider1, 196, 10);
    lv_slider_set_range(slider1, 1, 10);
    lv_slider_set_value(slider1, user_data_get()->public_cfg.door_device[channel].door_cfg.outdoor_talk_volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(slider1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider1, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider1, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider1, lv_color_hex(0x02A9FF), LV_PART_INDICATOR);
    lv_obj_add_event_cb(slider1, monitor_adjust_door_volume_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_clear_flag(slider1, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *btn1_sub = lv_common_img_btn_create(cont, monitor_adjust_door_sub_id, 17, row_y[0] + 8, 31, 25,
                                                  NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_door_sub, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_door_sub_focus);
    lv_obj_add_event_cb(btn1_sub, monitor_adjust_slider_sub_cb, LV_EVENT_CLICKED, slider1);

    lv_obj_t *btn1_add = lv_common_img_btn_create(cont, monitor_adjust_door_add_id, 267, row_y[0] + 8, 31, 25,
                                                  NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_door_add, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_door_add_focus);
    lv_obj_add_event_cb(btn1_add, monitor_adjust_slider_add_cb, LV_EVENT_CLICKED, slider1);

    /* ---- 室内机声音大小 ---- */
    lv_resource_init(icon_indoor_sub, ICON_LIVE_MONITOR_VOLUME_SUB_PNG_OFFSET, ICON_LIVE_MONITOR_VOLUME_SUB_PNG_SIZE);
    lv_resource_init(icon_indoor_sub_focus, ICON_LIVE_MONITOR_VOLUME_SUB_FOCUS_PNG_OFFSET, ICON_LIVE_MONITOR_VOLUME_SUB_FOCUS_PNG_SIZE);
    lv_resource_init(icon_indoor_add, ICON_LIVE_MONITOR_VOLUME_ADD_PNG_OFFSET, ICON_LIVE_MONITOR_VOLUME_ADD_PNG_SIZE);
    lv_resource_init(icon_indoor_add_focus, ICON_LIVE_MONITOR_VOLUME_ADD_FOCUS_PNG_OFFSET, ICON_LIVE_MONITOR_VOLUME_ADD_FOCUS_PNG_SIZE);

    lv_obj_t *slider2 = lv_slider_create(cont);
    lv_obj_set_pos(slider2, 60, row_y[1] + 17);
    lv_obj_set_size(slider2, 196, 10);
    lv_slider_set_range(slider2, 1, 10);
    lv_slider_set_value(slider2, user_data_get()->public_cfg.door_device[channel].door_cfg.indoor_talk_volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(slider2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider2, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider2, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider2, lv_color_hex(0x02A9FF), LV_PART_INDICATOR);
    lv_obj_add_event_cb(slider2, monitor_adjust_indoor_volume_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_clear_flag(slider2, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *btn2_sub = lv_common_img_btn_create(cont, monitor_adjust_indoor_sub_id, 18, row_y[1] + 8, 31, 25,
                                                  NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_indoor_sub, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_indoor_sub_focus);
    lv_obj_add_event_cb(btn2_sub, monitor_adjust_slider_sub_cb, LV_EVENT_CLICKED, slider2);

    lv_obj_t *btn2_add = lv_common_img_btn_create(cont, monitor_adjust_indoor_add_id, 268, row_y[1] + 8, 31, 25,
                                                  NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_indoor_add, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_indoor_add_focus);
    lv_obj_add_event_cb(btn2_add, monitor_adjust_slider_add_cb, LV_EVENT_CLICKED, slider2);
  }

  /* ---- 图像亮度 ---- */
  int brightness_y = is_cctv ? 15 : row_y[2];

  lv_resource_init(icon_brightness_sub, ICON_LIVE_BRIGHTNESS_SUB_PNG_OFFSET, ICON_LIVE_BRIGHTNESS_SUB_PNG_SIZE);
  lv_resource_init(icon_brightness_sub_focus, ICON_LIVE_BRIGHTNESS_SUB_FOCUS_PNG_OFFSET, ICON_LIVE_BRIGHTNESS_SUB_FOCUS_PNG_SIZE);
  lv_resource_init(icon_brightness_add, ICON_LIVE_BRIGHTNESS_ADD_PNG_OFFSET, ICON_LIVE_BRIGHTNESS_ADD_PNG_SIZE);
  lv_resource_init(icon_brightness_add_focus, ICON_LIVE_BRIGHTNESS_ADD_FOCUS_PNG_OFFSET, ICON_LIVE_BRIGHTNESS_ADD_FOCUS_PNG_SIZE);

  int brightness = is_cctv ? user_data_get()->public_cfg.cctv_display_brightness[channel - MON_CHANNEL_CCTV1]
                           : user_data_get()->public_cfg.door_device[channel].door_cfg.display_brightness;

  lv_obj_t *slider3 = lv_slider_create(cont);
  lv_obj_set_pos(slider3, 60, brightness_y + 17);
  lv_obj_set_size(slider3, 196, 10);
  lv_slider_set_range(slider3, 0, 10);
  lv_slider_set_value(slider3, (brightness + 5) / 10, LV_ANIM_OFF);
  lv_obj_set_style_bg_opa(slider3, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider3, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider3, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider3, lv_color_hex(0x02A9FF), LV_PART_INDICATOR);
  lv_obj_add_event_cb(slider3, monitor_adjust_brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_clear_flag(slider3, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *btn3_sub = lv_common_img_btn_create(cont, monitor_adjust_brightness_sub_id, 14, brightness_y + 8, 34, 30,
                                                NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                &icon_brightness_sub, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_brightness_sub_focus);
  lv_obj_add_event_cb(btn3_sub, monitor_adjust_slider_sub_cb, LV_EVENT_CLICKED, slider3);

  lv_obj_t *btn3_add = lv_common_img_btn_create(cont, monitor_adjust_brightness_add_id, 268, brightness_y + 8, 30, 30,
                                                NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                &icon_brightness_add, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_brightness_add_focus);
  lv_obj_add_event_cb(btn3_add, monitor_adjust_slider_add_cb, LV_EVENT_CLICKED, slider3);
}

/**
 * @brief 对讲按钮显示控制
 */
static void monitor_talk_btn_display(void)
{
  lv_obj_t *obj = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_talk_id);
  if (obj == NULL)
    return;
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    if (monitor_is_talking())
    {
      lv_resource_init(icon, ICON_LIVE_TALK_FOCUS_PNG_OFFSET, ICON_LIVE_TALK_FOCUS_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_bg_img_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    }
    else // INCOMING_CONNECTING / OUTGOING_CONNECTING / MONITOR_SESSION_CLOSED
    {
      lv_resource_init(icon, ICON_LIVE_TALK_PNG_OFFSET, ICON_LIVE_TALK_PNG_SIZE);
      lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
      lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_bg_img_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    }
  }
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
  {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
  if (g_page_monitor_ctx.tuya > 0)
  {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_img_opa(obj, LV_OPA_20, LV_PART_MAIN);
  }
  monitor_action_btn_layout_update();
}

/**
 * @brief 创建对讲按钮
 */
static void monitor_talk_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_TALK_PNG_OFFSET, ICON_LIVE_TALK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_TALK_FOCUS_PNG_OFFSET, ICON_LIVE_TALK_FOCUS_PNG_SIZE);
  lv_obj_t *t = lv_common_img_btn_create(lv_scr_act_current(), monitor_talk_id, 634, 493, 87, 87,
                                         talk_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(t, 21);
  monitor_talk_btn_display();
}

/**
 * @brief 挂断按钮显示控制
 */
static void monitor_hangup_btn_display(void)
{
  monitor_action_btn_layout_update();
}

/**
 * @brief 底部图标自动布局
 * @note  通道为门口机时，talk/hangup 会根据音量按钮是否创建自动左移/右移
 */
static void monitor_action_btn_layout_update(void)
{
  enum
  {
    MONITOR_BOTTOM_BTN_W = 87,
    MONITOR_BOTTOM_BTN_GAP = 10,
    MONITOR_SCREEN_W = 1024
  };

  lv_obj_t *cands[] = {
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_channel_switch_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_snapshot_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_video_record_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_gate_open_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_door_open_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_door2_open_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_adjust_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_talk_id),
      lv_obj_get_child_by_id(lv_scr_act_current(), monitor_hangup_id),
  };

  lv_obj_t *visible_btns[sizeof(cands) / sizeof(cands[0])];
  int visible_cnt = 0;
  for (int i = 0; i < (int)(sizeof(cands) / sizeof(cands[0])); i++)
  {
    if (cands[i] != NULL && !lv_obj_has_flag(cands[i], LV_OBJ_FLAG_HIDDEN))
    {
      visible_btns[visible_cnt++] = cands[i];
    }
  }

  if (visible_cnt <= 0)
  {
    return;
  }

  int total_w = visible_cnt * MONITOR_BOTTOM_BTN_W + (visible_cnt - 1) * MONITOR_BOTTOM_BTN_GAP;
  int start_x = (MONITOR_SCREEN_W - total_w) / 2;

  for (int i = 0; i < visible_cnt; i++)
  {
    lv_obj_set_style_x(visible_btns[i], start_x + i * (MONITOR_BOTTOM_BTN_W + MONITOR_BOTTOM_BTN_GAP), LV_PART_MAIN);
  }
}
/**
 * @brief 创建挂断按钮
 */
static void monitor_hangup_btn_create(void)
{
  lv_resource_init(icon, ICON_LIVE_HANGUP_PNG_OFFSET, ICON_LIVE_HANGUP_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_LIVE_HANGUP_FOCUS_PNG_OFFSET, ICON_LIVE_HANGUP_FOCUS_PNG_SIZE);
  lv_obj_t *h = lv_common_img_btn_create(lv_scr_act_current(), monitor_hangup_id, 718, 493, 87, 87,
                                         hangup_btn_click, true, LV_OPA_TRANSP, 0X101010, LV_OPA_TRANSP, 0x808080,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(h, 21);
  monitor_hangup_btn_display();
}

/**
 * @brief 更新通道标签显示
 */
static void monitor_channel_label_display(void)
{
  lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_top_cont_id);
  lv_obj_t *channel_label = lv_obj_get_child_by_id(top_cont, monitor_channel_id);
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1)
  {
    lv_label_set_text(channel_label, lang_str_get(STR_DOOR1));
  }
  else if (channel == MON_CHANNEL_DOOR2)
  {
    lv_label_set_text(channel_label, lang_str_get(STR_DOOR2));
  }
  else if (channel == MON_CHANNEL_CCTV1)
  {
    lv_label_set_text(channel_label, lang_str_get(STR_CAMERA1));
  }
  else if (channel == MON_CHANNEL_CCTV2)
  {
    lv_label_set_text(channel_label, lang_str_get(STR_CAMERA2));
  }
}

/**
 * @brief 创建通道标签
 */
static void monitor_channel_label_create(lv_obj_t *parent)
{
  lv_common_text_create(parent, monitor_channel_id, 0, 0, 200, 70,
                        NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        lang_str_get(STR_DOOR1), 0XFFFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_large);
  monitor_channel_label_display();
}

/**
 * @brief 更新日期标签显示
 */
static void monitor_date_label_display(void)
{
  lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_top_cont_id);
  if (top_cont == NULL)
    return;
  lv_obj_t *date_label = lv_obj_get_child_by_id(top_cont, monitor_date_id);
  if (date_label == NULL)
    return;

  static bool colon = false;
  time_t now = time(NULL);
  struct tm local_tm;
  Amy_time2localtime(now, local_tm);
  if (colon)
  {
    colon = false;
    lv_label_set_text_fmt(date_label, "%02d/%02d  %02d:%02d",
                          local_tm.tm_mon, local_tm.tm_mday,
                          local_tm.tm_hour, local_tm.tm_min);
  }
  else
  {
    colon = true;
    lv_label_set_text_fmt(date_label, "%02d/%02d  %02d %02d",
                          local_tm.tm_mon, local_tm.tm_mday,
                          local_tm.tm_hour, local_tm.tm_min);
  }
}

/**
 * @brief 创建日期标签
 */
static void monitor_date_label_create(lv_obj_t *parent)
{
  lv_obj_t *label = lv_common_text_create(parent, monitor_date_id, 350, 0, 324, 70,
                                          NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          "01/01  00:00", 0XFFFFFFFF, 0xFFFFFF,
                                          LV_TEXT_ALIGN_CENTER_CENTER, lv_font_large);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  monitor_date_label_display();
}

/**
 * @brief 更新超时标签显示
 */
static void monitor_timeout_label_display(void)
{
  lv_obj_t *top_cont = lv_obj_get_child_by_id(lv_scr_act_current(), monitor_top_cont_id);
  if (top_cont == NULL)
    return;
  lv_obj_t *timeout_label = lv_obj_get_child_by_id(top_cont, monitor_timeout_id);
  if (timeout_label == NULL)
    return;

  /* 手动录像或呼叫自动录像进行中：右上角加 REC 前缀；留言(MESSAGE)录像不显示 */
  int record_mode = monitor_video_record_mode_get();
  bool recording_ind = (record_mode != -1) &&
                       (record_mode & (MEDIA_MODE_MANUAL | MEDIA_MODE_AUTO));

  if (s_ctx.record_message_ing)
  {
    lv_label_set_text_fmt(timeout_label, "%s %02d",
                          lang_str_get(STR_MESSAGE),
                          g_page_monitor_ctx.timeout);
  }
  else if (recording_ind)
  {
    lv_label_set_text_fmt(timeout_label, "REC %02d", g_page_monitor_ctx.timeout);
  }
  else if (s_ctx.snapshort_ing)
  {
    /* 抓拍进行中显示"拍摄中"，完成后回到纯倒计时 */
    lv_label_set_text_fmt(timeout_label, "%s %02d",
                          lang_str_get(STR_SNAPSHOT),
                          g_page_monitor_ctx.timeout);
  }
  else
  {
    lv_label_set_text_fmt(timeout_label, "%02d", g_page_monitor_ctx.timeout);
  }
}

/**
 * @brief 创建超时标签
 */
static void monitor_timeout_label_create(lv_obj_t *parent)
{
  lv_common_text_create(parent, monitor_timeout_id, 674, 0, 324, 70,
                        NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        "30", 0XFF0000, 0XFF0000, LV_TEXT_ALIGN_RIGHT_CENTER, lv_font_large);
  monitor_timeout_label_display();
}

/**
 * @brief 创建顶部容器
 */
static void monitor_top_cont_create(void)
{
  lv_obj_t *cont = lv_common_img_btn_create(lv_scr_act_current(), monitor_top_cont_id, 0, 0, 1024, 70,
                                            NULL, false, LV_OPA_70, 0x000000, LV_OPA_70, 0x000000,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            NULL, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
  monitor_channel_label_create(cont);
  monitor_date_label_create(cont);
  monitor_timeout_label_create(cont);
}

/**
 * @brief 创建所有UI控件（统一入口）
 */
static void monitor_ui_create_all(void)
{
  monitor_channel_switch_btn_create(); // 通道切换按键创建
  monitor_snapshot_btn_create();       // 抓拍按键创建
  monitor_record_btn_create();         // 视频录制按键创建
  monitor_gate_open_btn_create();      // 开gate锁按键创建
  monitor_door_open_btn_create();      // 开door锁按键创建
  monitor_door2_open_btn_create();     // 开door2锁按键创建
  monitor_adjust_btn_create();         // 调节按键创建
  monitor_talk_btn_create();           // 通话按键创建
  monitor_hangup_btn_create();         // 挂断按键创建
  monitor_action_btn_layout_update();  // 底部图标自动布局
  monitor_top_cont_create();           // 顶部容器创建
}

/* =========================== 定时器处理实现 =========================== */
/**
 * @brief 监控页面定时器回调
 */
static void monitor_timer_handle(lv_timer_t *t)
{
  g_page_monitor_ctx.timeout--;

  if (g_page_monitor_ctx.timeout == 0)
  {
    sat_call_hangup_current();
    common_call_session_complete_handle();
  }
  else
  {
    // 响铃→留言 阶段切换: 倒计时到达 message_time 时触发留言录制
    if (!s_ctx.record_message_ing &&
        g_page_monitor_ctx.monitor_status == MONITOR_INCOMING_CONNECTING)
    {
      int channel = monitor_channel_get();
      if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
      {
        door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[channel].door_cfg;
        int message_time = cfg->message_time;
        if (message_time > 0 &&
            Amy_tfcard_status() == SD_STATE_MOUNT && cfg->message_sw &&
            g_page_monitor_ctx.timeout == (uint16_t)message_time)
        {
          monitor_ring_stop();
          common_call_door_no_one_home_broadcast(channel);
          record_message_action();
        }
      }
    }

    monitor_channel_label_display();
    monitor_date_label_display();
    monitor_timeout_label_display();
  }
}

/* =========================== UI渲染区域设置 =========================== */
static void monitor_rendering_setting(void)
{
  lv_area_t area[] = {
      {0, 0, 1023, 69},
      {0, 493, 1023, 493 + 87},
      {450, 207, 450 + 323, 207 + 225},
      {329, 172, 329 + 365, 172 + 255},
  };
  for (int i = 0; i < sizeof(area) / sizeof(lv_area_t); i++)
  {
    Amy_rendering_area(&area[i]);
  }
}

/* =========================== 媒体资源初始化 =========================== */
/**
 * @brief 监控页面音视频初始化（封装原视频/音频启动逻辑）
 */
static void monitor_media_init(void)
{
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2 ||
      monitor_is_incoming())
  {
    monitor_video_open();
    if (monitor_is_incoming())
    {
      scene_mode_type_def scene = user_data_get()->public_cfg.global_func.scene_mode;
      bool mute = user_data_get()->private_cfg.mute_enable;

      int status = Amy_tfcard_status();

      // 离家模式: 跳过自动录像，直接进入留言录制
      if (scene == SCENE_MODE_AWAY && !s_ctx.record_message_ing)
      {
        door_config_type_def *cfg = &user_data_get()->public_cfg.door_device[channel].door_cfg;
        if (status == SD_STATE_MOUNT && cfg->message_sw)
        {
          g_page_monitor_ctx.timeout = (uint16_t)cfg->message_time;
          record_message_action();
          return;
        }
      }

      if (status == SD_STATE_MOUNT)
      {
        if (Amy_tfcard_ensure_free_percent(10) != 0)
        {
          lv_common_screen_click_lock();
          lv_common_msgbox_create_nobg(329, 172, 366, 256, 42, lang_str_get(STR_SD_NO_MEMORY), page_live_msgbox_confirm_cb, NULL);
        }
        else
        {
          record_mode_type_def record_mode = user_data_get()->public_cfg.door_device[channel].door_cfg.global_record_mode;
          bool snap_save = (record_mode == RECORD_MODE_PHOTO || record_mode == RECORD_MODE_PHOTO_VIDEO);
          if (record_mode != RECORD_MODE_OFF)
          {
            monitor_video_snapshort(MEDIA_MODE_AUTO, snap_save);
          }
          if (record_mode == RECORD_MODE_VIDEO || record_mode == RECORD_MODE_PHOTO_VIDEO)
          {
            if (monitor_video_record_start(MEDIA_MODE_AUTO) == 0)
            {
              s_ctx.auto_rec_timer = 0;
              if (s_ctx.auto_record_close_task == NULL)
              {
                s_ctx.auto_record_close_task = lv_timer_create(auto_record_close_task_cb, 1000, NULL);
              }
            }
          }
        }
      }

      if (scene == SCENE_MODE_HOME && !mute)
      {
        monitor_incoming_ring_play();
      }
      else
      {
        monitor_ring_stop();
      }

      if (!mute)
      {
        chime_gpio_enable();
        if (s_ctx.chime_timer == NULL)
        {
          s_ctx.chime_timer = lv_timer_create(chime_timer_cb, 5000, NULL);
        }
      }
    }
  }
}

/* =========================== 页面资源初始化 =========================== */
static void monitor_resource_init(void)
{
  g_page_monitor_ctx.timeout = 30;
  g_page_monitor_ctx.gate_open_status = false;
  g_page_monitor_ctx.door_open_status = false;
  g_page_monitor_ctx.door2_open_status = false;
  s_ctx.record_message_ing = false;
  s_ctx.snapshort_ing = false;
}

/* =========================== 页面生命周期实现 =========================== */
/**
 * @brief 进入监控页面
 */
static void Amy_lv_page_enter(live)
{
  standby_timer_close();
  monitor_resource_init();
  int channel = monitor_channel_get();
  if (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2)
    backlight_set(user_data_get()->public_cfg.door_device[channel].door_cfg.display_brightness);
  else if (channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2)
    backlight_set(user_data_get()->public_cfg.cctv_display_brightness[channel - MON_CHANNEL_CCTV1]);
  monitor_media_init();
  monitor_rendering_setting();
  monitor_ui_create_all();
  lv_timer_create_recycle(monitor_timer_handle, 1000, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_SNAPSHORT, monitor_snapshort_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_RECORD, monitor_record_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, monitor_rtsp_get_frame_cb);
  Amy_event_cb_register(AMY_EVENT_ID_CALL, monitor_inside_call_callback);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, monitor_tfcard_change_cb);
  tuya_event_handler_register(TUYA_EVENT_CMD_VIDEO_START, monitor_tuya_video_start);
  tuya_event_handler_register(TUYA_EVENT_CMD_LIGHT, monitor_tuya_light_control);
  tuya_event_handler_register(TUYA_EVENT_CMD_ATMOSPHERE, monitor_tuya_atmosphere_handler);
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_NETWORK_CONFLICT, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_OTA, monitor_ota_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_MUSIC_FINISH, monitor_custom_ring_finish_event_cb);
  lv_obj_event_pressed_callback_register(NULL);
}

/**
 * @brief 退出监控页面
 */
static void Amy_lv_page_quit(live)
{
  standby_timer_restart(true, true);
  monitor_video_record_stop();
  monitor_ring_stop();
  if (s_ctx.record_message_task != NULL)
  {
    lv_timer_del(s_ctx.record_message_task);
    s_ctx.record_message_task = NULL;
  }
  s_ctx.record_message_ing = false;
  if (s_ctx.chime_timer != NULL)
  {
    lv_timer_del(s_ctx.chime_timer);
    s_ctx.chime_timer = NULL;
  }
  chime_gpio_disable();
  if (s_ctx.auto_record_close_task != NULL)
  {
    lv_timer_del(s_ctx.auto_record_close_task);
    s_ctx.auto_record_close_task = NULL;
  }
  lv_video_mode_enable(0);
  monitor_video_close();
  user_data_save();
  backlight_set(user_data_get()->private_cfg.display_brightness);
  tuya_event_callback_default();
  Amy_event_cb_register(AMY_EVENT_ID_SNAPSHORT, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_RECORD, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, page_default_device_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_NETWORK_CONFLICT, page_default_network_conflict_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_CALL, common_call_extern_call_callback);
  Amy_event_cb_register(AMY_EVENT_ID_MUSIC_FINISH, NULL);
  lv_obj_event_pressed_callback_register(page_default_obj_press_event_cb);
}

/* =========================== 页面注册 =========================== */
Amy_lv_page_create(live);