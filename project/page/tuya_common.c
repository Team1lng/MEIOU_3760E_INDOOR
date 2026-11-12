
#include "tuya_common.h"
#include "stdio.h"
#include "page_tabel.h"
#include "page_common.h"
#include "Amy_tfcard.h"
#include "tuya/tuya_dp_callback.h"
#include "../monitor.h"
#include "../gpio.h"
#include "page_live.h"
#include "user_data.h"
#include "language.h"
#include "call_api.h"
#include "tuya/tuya_sdk.h"
static tuya_event_handler g_tuya_event_handler[TUYA_EVENT_CMD_TOTAL] = {NULL};

bool tuya_sdk_start_from_user_data(void)
{
  tuya_conf_info_t *tuya_info = &user_data_get()->private_cfg.tuya_info;

  if (tuya_info->uid[0] == '\0' || tuya_info->key[0] == '\0')
  {
    return false;
  }

  memset(tuya_info->pid, 0, sizeof(tuya_info->pid));
  strncpy(tuya_info->pid, "7nffaws9ukzebqh2", sizeof(tuya_info->pid) - 1);

  tuya_api_set_time_sync_enable(user_data_get()->public_cfg.auto_sync_network_time);
  tuya_sdk_init(tuya_info->pid, tuya_info->uid, tuya_info->key);
  printf("PID：%s\n", tuya_info->pid);
  printf("UID：%s\n", tuya_info->uid);
  printf("KEY：%s\n", tuya_info->key);
  tuya_event_module_init();
  return true;
}

// 注册涂鸦细分事件回调
void tuya_event_handler_register(int sub_event_id, tuya_event_handler handler)
{
  if (sub_event_id >= TUYA_EVENT_CMD_TOTAL || handler == NULL)
  {
    printf("tuya_event_handler_register: invalid param, sub_event_id=%d\n", sub_event_id);
    return;
  }
  g_tuya_event_handler[sub_event_id] = handler;
}

// 涂鸦细分处理器
static void tuya_event_dispatch(tuya_cb_attr *p_tuya_event)
{
  if (p_tuya_event == NULL)
  {
    printf("tuya_event_dispatch: invalid tuya event\n");
    return;
  }

  // 查找对应的处理器并调用
  tuya_event_handler handler = g_tuya_event_handler[p_tuya_event->event];
  if (handler != NULL)
  {
    handler(p_tuya_event->reserved);
  }
  else
  {
    printf("tuya_event_dispatch: no handler for event=%d\n", p_tuya_event->event);
  }
}

static void default_tuya_event_handle(void *arg)
{
  tuya_cb_attr *attr = (tuya_cb_attr *)arg;
  if (attr == NULL)
  {
    AMY_LOG_ERROR(": invalid param\n");
    return;
  }
  tuya_event_dispatch(attr);
}
#define TUYA_CHANNEL_MAX 4
static void tuya_video_start(int reserved)
{
  int channel = cctv_monitor_first_valid_channel_get();
  if (channel == -1)
  {
    channel = door_monitor_first_valid_channel_get();
  }
  if (channel == -1)
  {
    return;
  }
  if (is_channel_ipc_camera(channel) == 0)
  {
    device_info_attr attr;
    attr.building = user_data_get()->private_cfg.device.attr.building;
    attr.room = 0X00;
    attr.device_type = channel == MON_CHANNEL_DOOR1
                           ? DEVICE_TYPE_DOOR_ID1
                           : DEVICE_TYPE_DOOR_ID2;
    sat_call_dial(&attr);
  }
  monitor_channel_set(channel);

  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  monitor_ctx_set_tuya(true);
  Amy_lv_page_goto(lv_page(tuya), LV_SCR_LOAD_ANIM_NONE, );
}

static void tuya_video_stop(int reserved)
{
  monitor_ctx_set_tuya(false);
}

static void tuya_audio_start(int reserved)
{
}

static void tuya_audio_stop(int reserved)
{
}

static void tuya_online(int reserved)
{
  tuya_all_dp_response();
  report_valid_channel_to_tuya();
}

static void tuya_switch_channel(int reserved)
{
  int channel = reserved;
  AMY_LOG_INFO("channel is %d\r\n", channel);
}

// ===================== 开锁生命周期（DP上报定时器，全局生效） =====================

static void tuya_dp_lock1_close_cb(lv_timer_t *timer)
{
  tuya_dp_access_lock1_response(false);
  lv_timer_del(timer);
}

static void tuya_dp_lock2_close_cb(lv_timer_t *timer)
{
  tuya_dp_access_lock2_response(false);
  lv_timer_del(timer);
}

static void tuya_dp_gate2_close_cb(lv_timer_t *timer)
{
  lock_gpio_disable();
  tuya_dp_access_gate2_response(false);
  lv_timer_del(timer);
}

void tuya_ungate2_with_dp(void)
{
  lock_gpio_enable();
  tuya_dp_access_gate2_response(true);
  int lock_time = user_data_get()->private_cfg.unlock_delay;
  lv_timer_create(tuya_dp_gate2_close_cb, lock_time * 1000, NULL);
}

void tuya_unlock_lock1_with_dp(int channel)
{
  if (is_channel_ipc_camera(channel) != 0)
  {
    return;
  }
  struct ipcamera_info *dev = &user_data_get()->public_cfg.net_door_device[channel];
  int lock_time = user_data_get()->public_cfg.door_device[channel].door_cfg.gate_lock_close_time;
  if (dev->ipaddr[0] != '\0')
  {
    int unlock_lang = user_data_get()->private_cfg.unlock_voice_enable ? (int)language_id_get() : -1;
    sat_ipcamera_device_unlock(dev->ipaddr, dev->port,
                               dev->username, dev->password,
                               lock_time, 0, unlock_lang, lock_time * 1000, false);
  }
  tuya_dp_access_lock1_response(true);
  lv_timer_create(tuya_dp_lock1_close_cb, lock_time * 1000, NULL);
}

void tuya_unlock_lock2_with_dp(int channel)
{
  if (is_channel_ipc_camera(channel) != 0)
  {
    return;
  }
  struct ipcamera_info *dev = &user_data_get()->public_cfg.net_door_device[channel];
  int lock_time = user_data_get()->public_cfg.door_device[channel].door_cfg.gate_lock_close_time;
  if (dev->ipaddr[0] != '\0')
  {
    int unlock_lang = user_data_get()->private_cfg.unlock_voice_enable ? (int)language_id_get() : -1;
    sat_ipcamera_device_unlock(dev->ipaddr, dev->port,
                               dev->username, dev->password,
                               lock_time, 1, unlock_lang, lock_time * 1000, false);
  }
  tuya_dp_access_lock2_response(true);
  lv_timer_create(tuya_dp_lock2_close_cb, lock_time * 1000, NULL);
}

// ===================== Tuya事件处理器（复用上述生命周期函数） =====================

static void tuya_lock1_open(int reserved)
{
  int channel = monitor_channel_get();
  if (is_channel_ipc_camera(channel) != 0)
  {
    return;
  }
  tuya_unlock_lock1_with_dp(channel);
}

static void tuya_lock2_open(int reserved)
{
  int channel = monitor_channel_get();
  if (is_channel_ipc_camera(channel) != 0)
  {
    return;
  }
  tuya_unlock_lock2_with_dp(channel);
}

static void tuya_gate2_open(int reserved)
{
  tuya_ungate2_with_dp();
}

static void tuya_at_home_mode(int reserved)
{
  bool en = reserved;
  if (en)
  {
    scene_mode_switch_to(SCENE_MODE_HOME);
    tuya_dp_at_home_response(en);
    tuya_dp_away_mode_response(false);
    tuya_dp_sleep_mode_response(false);
  }
}

static void tuya_away_home_mode(int reserved)
{
  bool en = reserved;
  if (en)
  {
    scene_mode_switch_to(SCENE_MODE_AWAY);
    tuya_dp_at_home_response(false);
    tuya_dp_away_mode_response(en);
    tuya_dp_sleep_mode_response(false);
  }
}

static void tuya_sleep_mode(int reserved)
{
  bool en = reserved;
  if (en)
  {
    scene_mode_switch_to(SCENE_MODE_SLEEP);
    tuya_dp_at_home_response(false);
    tuya_dp_away_mode_response(false);
    tuya_dp_sleep_mode_response(en);
  }
}

static void tuya_sd_storage(int reserved)
{
  unsigned long total_kb, used_kb, free_kb;
  int ret = tfcard_capacity_get(&total_kb, &used_kb, &free_kb);
  if (ret == 0)
  {
    tuya_dp_sd_storage_response(total_kb, used_kb, free_kb);
  }
}

static void tuya_sd_status(int reserved)
{
  int status = Amy_tfcard_status();
  switch (status)
  {
  case SD_STATE_UNPLUG:
    tuya_dp_sd_status_response(5);
    break;
  case SD_STATE_ERROR:
    tuya_dp_sd_status_response(2);
    break;
  case SD_STATE_MOUNT:
    tuya_dp_sd_status_response(1);
    break;
  case SD_STATE_FULL:
    tuya_dp_sd_status_response(3);
    break;
  case SD_STATE_FORMAT:
    tuya_dp_sd_status_response(4);
    break;
  default:
    tuya_dp_sd_status_response(0);
    break;
  }
}

static void tuya_default_tfcard_format_finish_cb(void *arg)
{
  tuya_dp_sd_format_status_response(100);
  page_common_tfcard_format_finish_cb(arg);
  Amy_event_cb_register(AMY_EVENT_ID_SD_FORMAT_FINISH, page_common_tfcard_format_finish_cb);
}

static void tuya_sd_format(int reserved)
{
  int status = Amy_tfcard_status();
  switch (status)
  {
  case SD_STATE_UNPLUG:
    tuya_dp_sd_format_status_response(-2002);
    break;
  case SD_STATE_ERROR:
    tuya_dp_sd_format_status_response(-2003);
    break;
  case SD_STATE_FORMAT:
    tuya_dp_sd_format_status_response(-2000);
    break;
  case SD_STATE_MOUNT:
  case SD_STATE_FULL:
    Amy_event_cb_register(AMY_EVENT_ID_SD_FORMAT_FINISH, tuya_default_tfcard_format_finish_cb);
    Amy_tfcard_format();
    tuya_dp_sd_format_status_response(0);
    break;
  default:
    break;
  }
}

static void tuya_light_control(int open)
{
  // TODO: 发送 NET_COMMON_CMD_LIGHT (0x53) 到门口机控制灯光
  // TODO: 上报涂鸦灯光开关状态 tuya_dp_138_response_light_switch(open)
  AMY_LOG_INFO("light control: %s\r\n", open ? "on" : "off");
}

void atmosphere_ctrl(bool en)
{
  atmosphere_gpio_set(en);
  user_data_get()->public_cfg.atmosphere = en;
  user_data_save();
  tuya_dp_atmosphere_switch_response(en);
}

static void tuya_atmosphere_handler(int reserved)
{
  atmosphere_ctrl((bool)reserved);
}

static void default_tuya_event_handlers_init()
{
  tuya_event_handler_register(TUYA_EVENT_CMD_VIDEO_START, tuya_video_start);
  tuya_event_handler_register(TUYA_EVENT_CMD_VIDEO_STOP, tuya_video_stop);
  tuya_event_handler_register(TUYA_EVENT_CMD_AUDIO_START, tuya_audio_start);
  tuya_event_handler_register(TUYA_EVENT_CMD_AUDIO_STOP, tuya_audio_stop);
  tuya_event_handler_register(TUYA_EVENT_CMD_SWITCH_CH, tuya_switch_channel);

  tuya_event_handler_register(TUYA_EVENT_CMD_ONLINE, tuya_online);
  tuya_event_handler_register(TUYA_EVENT_CMD_LOCK1_OPEN, tuya_lock1_open);
  tuya_event_handler_register(TUYA_EVENT_CMD_LOCK2_OPEN, tuya_lock2_open);
  tuya_event_handler_register(TUYA_EVENT_CMD_GATE2_OPEN, tuya_gate2_open);
  tuya_event_handler_register(TUYA_EVENT_CMD_CH_AT_HOME, tuya_at_home_mode);
  tuya_event_handler_register(TUYA_EVENT_CMD_AWAY_HOME, tuya_away_home_mode);
  tuya_event_handler_register(TUYA_EVENT_CMD_CH_SLEEP, tuya_sleep_mode);
  tuya_event_handler_register(TUYA_EVENT_CMD_SD_STATUS, tuya_sd_status);
  tuya_event_handler_register(TUYA_EVENT_CMD_SD_STORAGE, tuya_sd_storage);
  tuya_event_handler_register(TUYA_EVENT_CMD_SD_FORMAT, tuya_sd_format);
  tuya_event_handler_register(TUYA_EVENT_CMD_LIGHT, tuya_light_control);
  tuya_event_handler_register(TUYA_EVENT_CMD_ATMOSPHERE, tuya_atmosphere_handler);
}

// 初始化涂鸦事件模块（注册总事件+细分处理器）
void tuya_event_module_init(void)
{
  default_tuya_event_handlers_init();

  Amy_event_cb_register(AMY_EVENT_ID_TUYA, default_tuya_event_handle);
}

// 复位涂鸦事件默认处理
void tuya_event_callback_default(void)
{
  default_tuya_event_handlers_init();
}

// 上传有效通道到云端
void report_valid_channel_to_tuya(void)
{
  tuya_ch_info_t info[TUYA_CHANNEL_MAX];
  memset(info, 0, sizeof(info));
  char *channel_str[TUYA_CHANNEL_MAX] = {"Door1", "Door2", "CCTV1", "CCTV2"};
  for (int i = 0; i < TUYA_CHANNEL_MAX; i++)
  {
    if (monitor_channel_is_valid(i) != 1)
    {
      continue;
    }
    info[i].ch = i;
    info[i].name = channel_str[i];
  }
  int channel = monitor_channel_get();
  tuya_channel_valid_report(channel, info, TUYA_CHANNEL_MAX);
}
