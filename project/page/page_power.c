#include "page_tabel.h"
#include "video_decode.h"
#include "lv_common.h"
#include "Amy_lvgl.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "fcntl.h"
#include "ak_common_graphics.h"
#include "ak_common.h"
#include "ak_mem.h"
#include "device/lv_disp_dev.h"
#include "../gpio.h"
#include "../pwm.h"
#include "avilib/video_play.h"
#include "Amy_tfcard.h"
#include "live/testRTSPClient.h"
#include "tuya/tuya_sdk.h"
#include "sat_ipcamera.h"
#include "user_network.h"
#include "network_conflict_check.h"
#include "call_api.h"
#include "page_common.h"
#include "user_data.h"
#include "language_xls.h"
#include "card_common_net_proto.h"
#include "sha1/sha1.h"
#include "tuya_common.h"
#include "tuya/tuya_uuid_and_key.h"
#include "page_live.h"
#include "sat_user_common.h"
#include "page_common_call.h"
#include "sat_wifi.h"
#include "call_event_hook.h"
#include "audio_output.h"
#include "../business/call/call_event_indicate.h"
#include "../business/call/call_comm_port.h"
#include "../business/device/device_business.h"
#include "../monitor.h"
#include "../key.h"
#include "boot_time.h"
/************************** 页面上下文 **************************/
static struct
{
  video_decode_context *mjpeg_decodec_ctx;
  bool goto_tuya_uuid_register;
} s_ctx;

// SD卡挂载由独立线程异步完成，开机时 power 页的涂鸦凭证判定可能早于挂载，
// 导致卡上的 XLS 被误判为不存在。此处等到挂载线程给出结论再判定；
// 未插卡时不必等待，避免无卡机器每次开机白等一个超时。
static void tuya_wait_tfcard_ready(void)
{
  const unsigned int timeout_ms = 1000;
  const unsigned int step_ms = 50;

  for (unsigned int waited = 0; waited < timeout_ms; waited += step_ms)
  {
    if (!Amy_tfcard_inserted())
    {
      return;
    }
    if (Amy_tfcard_status() & SD_STATE_INSERT)
    {
      return;
    }
    Amy_msleep(step_ms);
  }
}

static void lv_delay_goto_page_cb(lv_timer_t *t)
{
  boot_time_tick("home_page_goto");

  // 判定须放在跳转前最后一刻：SD卡挂载完成前 /mnt/tf 并不存在，
  // 此时扫描会把"未挂载"当成"卡上无 XLS"
  tuya_wait_tfcard_ready();

  // 涂鸦凭证已存在则正常进 standby；仅主机(ID1)在只有SD卡XLS而未注册时先进入注册页
  s_ctx.goto_tuya_uuid_register = false;
  if (tuya_key_and_uuid_init())
  {
    unsigned char uuid[64] = {0};
    unsigned char key[64] = {0};
    if (!tuya_uuid_and_key_read(uuid, key))
    {
      s_ctx.goto_tuya_uuid_register = true;
    }
  }

  if (s_ctx.goto_tuya_uuid_register)
  {
    Amy_lv_page_goto(lv_page(tuya_uuid_register), LV_SCR_LOAD_ANIM_NONE, );
  }
  else
  {
    Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
  }
}

static int logo_frame_get_cb(video_mode_attr *attr, void *user_data)
{
  struct ak_vdec_frame frame = {0};
  if (video_decodec_get_frame(user_data, &frame, NULL) == 0)
  {
    attr->sw = frame.frame_obj.data.pitch_width;
    attr->sh = frame.frame_obj.data.pitch_height;
    attr->spx = 0;
    attr->spy = 0;
    attr->spw = frame.width;
    attr->sph = frame.height;
    attr->sfmt = GP_FORMAT_YUV420SP;
    ak_mem_dma_vaddr2paddr((void *)frame.frame_obj.data.data, (unsigned long *)&attr->sphy);

    attr->dpx = 0;
    attr->dpy = 0;
    attr->dpw = 1024;
    attr->dph = 600;
  }
  else
  {
    memset(attr, 0, sizeof(video_mode_attr));
    attr->dpx = 0;
    attr->dpy = 0;
    attr->dpw = 1024;
    attr->dph = 600;
  }
  return 0;
}

static void *media_server_task(void *arg)
{
  while (1)
  {
    // 每次重建前重新读取 br0 IP 并设置绑定地址，确保 IP 变更后（手动调用
    // rtsp_server_request_restart）重启服务器时绑定到新 IP
    char local_ip[32] = {0};
    sat_ip_mac_addres_get("br0", local_ip, NULL, NULL);
    setMulticastInterface(local_ip);

    // 阻塞运行 RTSP 服务器；收到重启请求时返回并进入下一次循环
    openRTSPServer();
    usleep(1000 * 1000);
  }
  return NULL;
}

static void *wifi_init_task(void *arg)
{
  (void)arg;
  wifi_context *wifi_ctx = wifi_context_get();
  user_data_info *info = user_data_get();
  if (info->public_cfg.pairing_mode == PAIRING_MODE_WLAN)
  {
    wifi_device_connecting(wifi_ctx);
  }
#if NET_SUPPORT_CABLE
  else
  {
    wifi_device_close(wifi_ctx);
    wifi_power_enable(false);
    user_data_save();
  }
#endif
  return NULL;
}

static int logo_decodec_display(const char *parttion_name)
{
  unsigned long parttion_size = 0;
  char mtd_name[32] = {0};
  if (Amy_mtd_parttion_get("LOGO", mtd_name, sizeof(mtd_name), &parttion_size))
  {
    AMY_LOG_ERROR("mtd parttion get failed\n");
    return -1;
  }

  char mtd_path[64] = {0};
  sprintf(mtd_path, "/dev/%s", mtd_name);
  s_ctx.mjpeg_decodec_ctx = video_decodec_open(1024, 600, MJPEG_ENC_TYPE);
  video_mode_cb_attr attr;
  attr.func = logo_frame_get_cb;
  attr.user_data = s_ctx.mjpeg_decodec_ctx;
  Amy_video_mode_event_put(&attr);

  stream_data *stream = video_decodec_buf_get(s_ctx.mjpeg_decodec_ctx, parttion_size);
  if (!stream)
  {
    video_decodec_close(s_ctx.mjpeg_decodec_ctx);
    AMY_LOG_ERROR("mjpeg decode get buffer failed\n");
    return -1;
  }
  int fd = open(mtd_path, O_RDONLY);
  if (fd < 0)
  {
    video_decodec_close(s_ctx.mjpeg_decodec_ctx);
    AMY_LOG_ERROR("open %s failed\n", mtd_path);
    return -1;
  }
  int ret = read(fd, stream->data, stream->size);
  close(fd);
  if (ret != stream->size)
  {
    video_decodec_buf_put(s_ctx.mjpeg_decodec_ctx, stream);
    AMY_LOG_ERROR("read data failed\n");
    return -1;
  }
  if (video_decodec_send_stream(s_ctx.mjpeg_decodec_ctx, stream))
  {
    video_decodec_buf_put(s_ctx.mjpeg_decodec_ctx, stream);
    AMY_LOG_ERROR("mjpeg decode send stream failed\n");
    return -1;
  }

  return 0;
}

static void monitor_rtsp_get_frame_cb(void *arg)
{
  video_mode_cb_attr *attr = (video_mode_cb_attr *)(arg);
  Amy_video_mode_register_device(attr->func, attr->user_data);
  lv_video_mode_enable(1);
}

static void Amy_lv_page_enter(power)
{
  boot_time_tick("power page enter");
  logo_decodec_display("LOGO");
  // logo 帧回调依赖此事件，须在解码完成前注册
  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, monitor_rtsp_get_frame_cb);
  boot_time_tick("logo decode");

  user_data_init();
  boot_time_tick("user_data_init");

  user_data_info *info = user_data_get();

  backlight_enable(true);
  backlight_set(info->private_cfg.display_brightness);

  lv_timer_create_recycle(lv_delay_goto_page_cb, 3000, NULL);
  boot_time_tick("power enter done");
}

static void Amy_lv_page_quit(power)
{

  // 重初始化放到退出页时执行，期间 logo 仍停留在屏幕（framebuffer 保留最后一帧）
  boot_time_tick("power quit init");
  user_data_info *info = user_data_get();
  language_id_set(info->private_cfg.lang);
  init_language_xls_info();
  boot_time_tick("language init");

  speak_enable(true);
  audio_output_keep_open(1, 8000);
  boot_time_tick("audio keep open");

  user_network_init();
  user_network_pairing_init();
  call_comm_init();

  call_config_t call_config = {
      .local_info = {
          .building = info->private_cfg.device.attr.building,
          .room = info->private_cfg.device.attr.room,
          .device_type = info->private_cfg.device.attr.device_type,
      },
      .auto_answer = false,
      .max_sessions = 1,
      .max_call_duration_ms = CALL_DEFAULT_MAX_DURATION_MS,
      .no_interact_timeout_ms = 0, // 0=关闭框架检测，内线超时改由 page_call 页面层计时
      .record_enable = true,
      .record_path = "/home/data/",
      .on_state_changed = sat_state_changed,
      .comm_interface = g_call_comm_interface,
  };
  sat_call_init(&call_config);
  boot_time_tick("sat_call_init");

  pthread_t thread_id;
  Amy_pthread_create(&thread_id, media_server_task, NULL, 32 * 1024, 12);

  tuya_conf_info_t *tuya_info = &info->private_cfg.tuya_info;
  memset(tuya_info->uid, 0, sizeof(tuya_info->uid));
  memset(tuya_info->key, 0, sizeof(tuya_info->key));

  if (tuya_key_and_uuid_init())
  {
    unsigned char uuid[64] = {0};
    unsigned char key[64] = {0};
    if (tuya_uuid_and_key_read(uuid, key))
    {
      strncpy(tuya_info->uid, (const char *)uuid, sizeof(tuya_info->uid) - 1);
      strncpy(tuya_info->key, (const char *)key, sizeof(tuya_info->key) - 1);
      tuya_sdk_start_from_user_data();
    }
  }

  device_conflict_listener_start();
  page_common_door_status_init();
  page_common_standby_check_init();

  pthread_t wifi_tid;
  Amy_pthread_create(&wifi_tid, wifi_init_task, NULL, 32 * 1024, 12);

  boot_time_tick("power quit done");
  lv_video_mode_enable(0);
  Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);
  if (s_ctx.mjpeg_decodec_ctx)
  {
    video_decodec_close(s_ctx.mjpeg_decodec_ctx);
    s_ctx.mjpeg_decodec_ctx = NULL;
  }
  Amy_event_cb_register(AMY_EVENT_ID_CALL, common_call_extern_call_callback);
  Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
  Amy_event_cb_register(AMY_EVENT_ID_SD_FORMAT_FINISH, page_common_tfcard_format_finish_cb);
  lv_obj_event_pressed_callback_register(page_default_obj_press_event_cb);
}

Amy_lv_page_create(power);