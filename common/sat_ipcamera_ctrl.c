#include "sat_ipcamera.h"
#include "onvif/onvif.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sat_user_common.h"
#include "Amy_lvgl.h"
#include "base64/include/libbase64.h"

/*==========================================================================
 * 通用异步发送框架
 *
 * 适用场景：底层统一调用 ipc_camera_device_send_data() 的所有接口
 * 调用方在创建线程前完成所有字段填充，线程函数只做发送和释放
 * 不适用：image / network / system_time（底层调专用 API，保留各自线程函数）
 *==========================================================================*/
typedef struct
{
  char cmd[32];      // 命令类型，如 "CallInvite" / "UnLock" 等
  void *data;        // 发送数据（堆内存，由调用方 malloc + 填充，线程函数负责 free）
  char ip[64];       // 目标设备 IP
  int port;          // 目标端口
  char user[64];     // 认证用户名
  char password[64]; // 认证密码
  int timeout;       // 超时时间（ms）
} async_send_param_t;

/************************************************************
** 通用异步发送线程函数
** 职责：执行发送 + 释放 param->data 和 param 本身
************************************************************/
static void *async_send_thread_func(void *arg)
{
  pthread_detach(pthread_self());

  async_send_param_t *param = (async_send_param_t *)arg;
  if (param == NULL)
    return NULL;

  bool ret = ipc_camera_device_send_data(param->cmd,
                                         (char *)param->data,
                                         param->ip, param->port,
                                         param->user, param->password,
                                         param->timeout);
  if (!ret)
  {
    printf("[%s:%d] async send failed: cmd=%s ip=%s\n",
           __func__, __LINE__, param->cmd, param->ip);
  }

  free(param->data);
  free(param);
  return NULL;
}

/************************************************************
** 内部辅助：创建并启动异步发送线程
** @param param  已填充完毕的参数结构体（堆内存，失败时由本函数释放）
** @return true  线程创建成功；false 线程创建失败
************************************************************/
static bool start_async_send(async_send_param_t *param)
{
  pthread_t thread_id;
  int ret = pthread_create(&thread_id, sat_pthread_attr_get(),
                           async_send_thread_func, param);
  if (ret != 0)
  {
    printf("[%s:%d] pthread_create failed, err=%d, cmd=%s\n",
           __func__, __LINE__, ret, param->cmd);
    free(param->data);
    free(param);
    return false;
  }
  return true;
}

/************************************************************
** 内部辅助：构造字符串类异步参数
** data 字段格式：调用方已完成格式化的字符串
************************************************************/
static async_send_param_t *build_string_param(const char *cmd, const char *value,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout)
{
  if (cmd == NULL || value == NULL || ip == NULL || user == NULL || password == NULL)
    return NULL;

  async_send_param_t *param = (async_send_param_t *)malloc(sizeof(async_send_param_t));
  if (param == NULL)
    return NULL;
  memset(param, 0, sizeof(async_send_param_t));
#define MAX_STRING_LEN 64
  char *data = (char *)malloc(MAX_STRING_LEN);
  if (data == NULL)
  {
    free(param);
    return NULL;
  }
  memcpy(data, value, MAX_STRING_LEN);

  strncpy(param->cmd, cmd, sizeof(param->cmd) - 1);
  param->data = data;
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->timeout = timeout;

  return param;
}

/************************************************************
** 内部辅助：文本协议统一发送（同步/异步同源）
************************************************************/
static bool send_text_protocol(const char *cmd, const char *value,
                               const char *ip, int port,
                               const char *user, const char *password,
                               int timeout, bool is_block)
{
  if (cmd == NULL || value == NULL || ip == NULL || user == NULL || password == NULL || timeout <= 0)
    return false;

  if (is_block)
  {
    return ipc_camera_device_send_data((char *)cmd, (char *)value, ip, port, user, password, timeout);
  }

  async_send_param_t *param = build_string_param(cmd, value, ip, port, user, password, timeout);
  if (param == NULL)
    return false;
  return start_async_send(param);
}

/************************************************************
** 内部辅助：构造卡片响应类异步参数（固定端口 80，固定账号）
** data 为原始内存块，memcpy 拷贝
************************************************************/
static async_send_param_t *build_card_param(const char *cmd,
                                            void *data, size_t size,
                                            const char *ip, int timeout)
{
  async_send_param_t *param = (async_send_param_t *)malloc(sizeof(async_send_param_t));
  if (param == NULL)
    return NULL;
  memset(param, 0, sizeof(async_send_param_t));

  void *data_copy = malloc(size);
  if (data_copy == NULL)
  {
    free(param);
    return NULL;
  }
  memcpy(data_copy, data, size);

  strncpy(param->cmd, cmd, sizeof(param->cmd) - 1);
  param->data = data_copy;
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = 80;
  strncpy(param->user, USER_NETWORK_AUTH_USER, sizeof(param->user) - 1);
  strncpy(param->password, USER_NETWORK_AUTH_PASSWORD, sizeof(param->password) - 1);
  param->timeout = timeout;

  return param;
}

/*==========================================================================
 * 特殊结构线程参数定义（不走 ipc_camera_device_send_data，保留各自线程）
 *==========================================================================*/
typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char cmd[1024]; // base64 编码前的原始命令
  int timeout;
} shell_cmd_param;

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  char video_token[128];
  int brightness[3];
  int saturation[3];
  int contrast[3];
  int timeout;
} image_get_param;

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  char video_token[128];
  int brightness;
  int saturation;
  int contrast;
  int timeout;
} image_setting_param;

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  struct ipcamera_network info;
  int timeout;
} network_param; // get / setting 共用，字段完全相同

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  char *status; // 外部缓冲区指针（异步写回）
  int timeout;
} mode_get_param;

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  int *channel; // 外部缓冲区指针（异步写回）
  int timeout;
} channel_get_param;

typedef struct
{
  char ip[32];
  int port;
  char user[64];
  char password[64];
  char auther_flag;
  int timeout;
} system_time_param;

typedef struct
{
  int device_type;
  int building;
  int room;
  char ip[64];
  int port;
  char user[64];
  char password[64];
  int timeout;
} door_register_param;

/*==========================================================================
 * Shell 命令（base64 编码，保留独立线程函数）
 *==========================================================================*/
static void *shellcmd_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  shell_cmd_param *p = (shell_cmd_param *)arg;

  size_t base64_size = strlen(p->cmd) * 2;
  char *base64_buf = (char *)malloc(base64_size);
  if (base64_buf == NULL)
  {
    printf("[%s:%d] malloc base64 buffer failed\n", __func__, __LINE__);
    free(p);
    return NULL;
  }
  memset(base64_buf, 0, base64_size);
  base64_encode(p->cmd, strlen(p->cmd), base64_buf, &base64_size, 0);
  ipc_camera_device_send_data("ShellCmd", base64_buf,
                              p->ip, p->port, p->user, p->password, p->timeout);
  free(base64_buf);
  free(p);
  return NULL;
}

bool sat_ipcamera_report_shellcmd(char *ip, int port, const char *user, const char *password,
                                  char *cmd, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || cmd == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
  {
    size_t base64_size = strlen(cmd) * 2;
    char *base64_buf = (char *)malloc(base64_size);
    if (base64_buf == NULL)
      return false;
    memset(base64_buf, 0, base64_size);
    base64_encode(cmd, strlen(cmd), base64_buf, &base64_size, 0);
    bool ret = ipc_camera_device_send_data("ShellCmd", base64_buf,
                                           ip, port, user, password, timeout);
    free(base64_buf);
    return ret;
  }

  shell_cmd_param *param = (shell_cmd_param *)malloc(sizeof(shell_cmd_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(shell_cmd_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  strncpy(param->cmd, cmd, sizeof(param->cmd) - 1);
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), shellcmd_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

/*==========================================================================
 * 图像参数（调专用 API，保留独立线程函数）
 *==========================================================================*/
static void *image_get_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  image_get_param *p = (image_get_param *)arg;
  ipc_camera_device_get_imaging(p->ip, p->port, p->user, p->password,
                                p->auther_flag, p->video_token,
                                p->brightness, p->saturation, p->contrast, p->timeout);
  free(p);
  return NULL;
}

bool sat_ipcamera_image_get(const char *ip, int port, const char *user, const char *password,
                            char auther_flag, const char *video_token, int brightness[3],
                            int saturation[3], int contrast[3], int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || video_token == NULL || brightness == NULL || saturation == NULL || contrast == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
    return ipc_camera_device_get_imaging(ip, port, user, password, auther_flag,
                                         video_token, brightness, saturation, contrast, timeout);

  image_get_param *param = (image_get_param *)malloc(sizeof(image_get_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(image_get_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  strncpy(param->video_token, video_token, sizeof(param->video_token) - 1);
  memcpy(param->brightness, brightness, sizeof(param->brightness));
  memcpy(param->saturation, saturation, sizeof(param->saturation));
  memcpy(param->contrast, contrast, sizeof(param->contrast));
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), image_get_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

static void *image_setting_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  image_setting_param *p = (image_setting_param *)arg;
  ipc_camera_device_setting_imaging(p->ip, p->port, p->user, p->password,
                                    p->auther_flag, p->video_token,
                                    p->brightness, p->saturation, p->contrast, p->timeout);
  free(p);
  return NULL;
}

bool sat_ipcamera_image_setting(const char *ip, int port, const char *user, const char *password,
                                char auther_flag, const char *video_token, int brightness,
                                int saturation, int contrast, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || video_token == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
    return ipc_camera_device_setting_imaging(ip, port, user, password, auther_flag,
                                             video_token, brightness, saturation, contrast, timeout);

  image_setting_param *param = (image_setting_param *)malloc(sizeof(image_setting_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(image_setting_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  strncpy(param->video_token, video_token, sizeof(param->video_token) - 1);
  param->brightness = brightness;
  param->saturation = saturation;
  param->contrast = contrast;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), image_setting_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

/*==========================================================================
 * 网络配置（调专用 API，保留独立线程函数）
 *==========================================================================*/
static void *network_get_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  network_param *p = (network_param *)arg;
  ipc_camera_device_get_network(p->ip, p->port, p->user, p->password,
                                p->auther_flag, &p->info, p->timeout);
  free(p);
  return NULL;
}

bool sat_ipcamera_network_get(const char *ip, int port, const char *user, const char *password,
                              char auther_flag, struct ipcamera_network *info, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || info == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
    return ipc_camera_device_get_network(ip, port, user, password, auther_flag, info, timeout);

  network_param *param = (network_param *)malloc(sizeof(network_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(network_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  memcpy(&param->info, info, sizeof(struct ipcamera_network));
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), network_get_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

static void *network_setting_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  network_param *p = (network_param *)arg;
  ipc_camera_device_setting_network(p->ip, p->port, p->user, p->password,
                                    p->auther_flag, &p->info, p->timeout);
  free(p);
  return NULL;
}

bool sat_ipcamera_network_setting(const char *ip, int port, const char *user, const char *password,
                                  char auther_flag, struct ipcamera_network *info, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || info == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
    return ipc_camera_device_setting_network(ip, port, user, password, auther_flag, info, timeout);

  network_param *param = (network_param *)malloc(sizeof(network_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(network_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  memcpy(&param->info, info, sizeof(struct ipcamera_network));
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), network_setting_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

/*==========================================================================
 * 工作模式 Get（需要回写 status 指针，保留独立线程函数）
 *==========================================================================*/
static void *mode_get_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  mode_get_param *p = (mode_get_param *)arg;
  char data[32] = {0};
  bool ret = ipc_camera_device_send_data("WorkModeGet", data,
                                         p->ip, p->port, p->user, p->password, p->timeout);
  if (ret && p->status != NULL)
    *p->status = (char)atoi(data);
  free(p);
  return NULL;
}

bool sat_ipcamera_device_mode_get(const char *ip, int port, const char *user, const char *password,
                                  char auther_flag, char *status, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || status == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
  {
    char data[32] = {0};
    if (!ipc_camera_device_send_data("WorkModeGet", data, ip, port, user, password, timeout))
      return false;
    *status = (char)atoi(data);
    return true;
  }

  mode_get_param *param = (mode_get_param *)malloc(sizeof(mode_get_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(mode_get_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  param->status = status;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), mode_get_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

bool sat_ipcamera_device_mode_setting(const char *ip, int port, const char *user, const char *password,
                                      char auther_flag, char status, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d", status);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("WorkModeSetting", data, ip, port, user, password, timeout, is_block);
}

/*==========================================================================
 * 视频通道 Get（需要回写 channel 指针，保留独立线程函数）
 *==========================================================================*/
static void *channel_get_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  channel_get_param *p = (channel_get_param *)arg;
  char data[32] = {0};
  bool ret = ipc_camera_device_send_data("VideoStreamChannelGet", data,
                                         p->ip, p->port, p->user, p->password, p->timeout);
  if (ret && p->channel != NULL)
    *p->channel = atoi(data);
  free(p);
  return NULL;
}

bool sat_ipcamera_device_channel_get(const char *ip, int port, const char *user, const char *password,
                                     char auther_flag, int *channel, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || channel == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  if (is_block)
  {
    char data[32] = {0};
    if (!ipc_camera_device_send_data("VideoStreamChannelGet", data,
                                     ip, port, user, password, timeout))
      return false;
    *channel = atoi(data);
    return true;
  }

  channel_get_param *param = (channel_get_param *)malloc(sizeof(channel_get_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(channel_get_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  param->channel = channel;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), channel_get_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

bool sat_ipcamera_device_channel_setting(const char *ip, int port, const char *user, const char *password,
                                         char auther_flag, int channel, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d", channel);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("VideoStreamChannelSetting", data, ip, port, user, password, timeout, is_block);
}

/*==========================================================================
 * MAC 更新（字符串 data，借用 build_card_param 做堆拷贝，修正账号）
 *==========================================================================*/
bool sat_ipcamera_device_mac_update(const char *ip, int port, const char *user, const char *password,
                                    char auther_flag, char *mac, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || mac == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol("UpdateMac", mac, ip, port, user, password, timeout, is_block);
}

/*==========================================================================
 * 系统时间（调专用 API，保留独立线程函数）
 *==========================================================================*/
static void *system_time_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  system_time_param *p = (system_time_param *)arg;

  struct tm *tm_data = (struct tm *)malloc(sizeof(struct tm));
  if (tm_data != NULL)
  {
    memset(tm_data, 0, sizeof(struct tm));
    ipc_camera_device_get_system_time(p->ip, p->port, p->user, p->password,
                                      p->auther_flag, tm_data, p->timeout);
    Amy_sync_time_event_put(tm_data);
    free(tm_data);
  }
  free(p);
  return NULL;
}

bool sat_ipcamera_system_time_get(const char *ip, int port, const char *user,
                                  const char *password, char auther_flag, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
    return false;

  if (is_block)
  {
    struct tm *tm_data = (struct tm *)malloc(sizeof(struct tm));
    if (tm_data == NULL)
      return false;
    memset(tm_data, 0, sizeof(struct tm));
    bool ret = ipc_camera_device_get_system_time(ip, port, user, password,
                                                 auther_flag, tm_data, timeout);
    Amy_sync_time_event_put(tm_data);
    free(tm_data);
    return ret;
  }

  system_time_param *param = (system_time_param *)malloc(sizeof(system_time_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(system_time_param));
  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->auther_flag = auther_flag;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), system_time_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

bool sat_ipcamera_device_talking_status_report(int status, const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
    return false;

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d", status);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("TalkingStatusReport", data, ip, port, user, password, timeout, is_block);
}

static bool call_send(const char *cmd, int device_type, int building, int room,
                      const char *ip, int port, const char *user, const char *password,
                      int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL)
    return false;
  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d-%d-%d", device_type, building, room);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol(cmd, data, ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 发送会话邀请
************************************************************/
bool sat_ipcamera_device_call_invite(int device_type, int building, int room, const char *ip,
                                     int port, const char *user, const char *password,
                                     int timeout, bool is_block)
{
  return call_send("CallInvite", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 通知会话发起方震铃
************************************************************/
bool sat_ipcamera_device_call_ringing(int device_type, int building, int room, const char *ip,
                                      int port, const char *user, const char *password,
                                      int timeout, bool is_block)
{
  return call_send("CallRing", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 会话繁忙
************************************************************/
bool sat_ipcamera_device_call_busy(int device_type, int building, int room, const char *ip,
                                   int port, const char *user, const char *password,
                                   int timeout, bool is_block)
{
  return call_send("CallBusy", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 会话错误
************************************************************/
bool sat_ipcamera_device_call_error(int device_type, int building, int room, const char *ip,
                                    int port, const char *user, const char *password,
                                    int timeout, bool is_block)
{
  return call_send("CallError", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 接受会话
************************************************************/
bool sat_ipcamera_device_call_accept(int device_type, int building, int room, const char *ip,
                                     int port, const char *user, const char *password,
                                     int timeout, bool is_block)
{
  return call_send("CallAccept", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 结束会话
************************************************************/
bool sat_ipcamera_device_call_end(int device_type, int building, int room, const char *ip,
                                  int port, const char *user, const char *password,
                                  int timeout, bool is_block)
{
  return call_send("CallEnd", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 心跳查询
************************************************************/
bool sat_ipcamera_device_call_heartbeat_request(int device_type, int building, int room, const char *ip,
                                                int port, const char *user, const char *password,
                                                int timeout, bool is_block)
{
  return call_send("CallHeartbeatRequest", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 心跳回复
************************************************************/
bool sat_ipcamera_device_call_heartbeat_response(int device_type, int building, int room, const char *ip,
                                                 int port, const char *user, const char *password,
                                                 int timeout, bool is_block)
{
  return call_send("CallHeartbeatResponse", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

static void *door_register_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  door_register_param *p = (door_register_param *)arg;
  if (p == NULL)
    return NULL;

  bool ret = call_send("Register", p->device_type, p->building, p->room,
                       p->ip, p->port, p->user, p->password, p->timeout, true);

  ipcamera_cb_attr attr = {
      .type = IPCAMERA_EVENT_TYPE_DOOR_REGISTER_DONE,
      .num = ret ? 1 : 0,
  };
  Amy_ipcamera_event_put(&attr);

  free(p);
  return NULL;
}

/************************************************************
** 函数说明: 注册门口机到指定房间，使其成为该房间专属设备
************************************************************/
bool sat_ipcamera_device_register(int device_type, int building, int room, const char *ip,
                                  int port, const char *user, const char *password,
                                  int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
    return false;

  if (!is_block)
  {
    door_register_param *param = (door_register_param *)malloc(sizeof(door_register_param));
    if (param == NULL)
      return false;
    memset(param, 0, sizeof(door_register_param));
    param->device_type = device_type;
    param->building = building;
    param->room = room;
    strncpy(param->ip, ip, sizeof(param->ip) - 1);
    param->port = port;
    strncpy(param->user, user, sizeof(param->user) - 1);
    strncpy(param->password, password, sizeof(param->password) - 1);
    param->timeout = timeout;

    pthread_t tid;
    int ret = pthread_create(&tid, sat_pthread_attr_get(), door_register_thread_func, param);
    if (ret != 0)
    {
      free(param);
      return false;
    }
    return true;
  }

  return call_send("Register", device_type, building, room,
                   ip, port, user, password, timeout, is_block);
}

/* =======================================================================
 * 内部：公共编码发送函数
 *
 * 对外接口的唯一实现，所有上层函数最终调用这里。
 * 不引入任何卡片业务头文件，对数据内容完全透明。
 *
 * @param xml_tag  XML 标签字符串（如 "AddCard"）
 * @param data     待编码数据指针（任意结构体，调用方负责构造）
 * @param data_len 数据字节大小
 * ===================================================================== */
static bool card_response_send(char *xml_tag,
                               const void *data, size_t data_len,
                               const char *ip, int port,
                               const char *user, const char *password,
                               int timeout, bool is_block)
{
  if (xml_tag == NULL || data == NULL || data_len == 0 ||
      ip == NULL || user == NULL || password == NULL)
  {
    SAT_ERROR("[SAT_IPCAMERA_CARD] %s: invalid param (NULL or zero len)", __func__);
    return false;
  }

  /* 1. 分配 base64 缓冲区（原始大小 × 2 足够） */
  size_t buf_size = data_len * 2;
  char *base64_buf = (char *)malloc(buf_size);
  if (base64_buf == NULL)
  {
    SAT_ERROR("[SAT_IPCAMERA_CARD] %s: malloc failed (size=%zu)", __func__, buf_size);
    return false;
  }
  memset(base64_buf, 0, buf_size);

  /* 2. base64 编码 */
  size_t base64_size = 0;
  base64_encode((const char *)data, data_len, base64_buf, &base64_size, 0);
  if (base64_size == 0)
  {
    SAT_ERROR("[SAT_IPCAMERA_CARD] %s: base64 encode failed", __func__);
    free(base64_buf);
    return false;
  }

  bool ret = false;

  if (is_block)
  {
    ret = ipc_camera_device_send_data(xml_tag, base64_buf,
                                      ip, port, user, password, timeout);
    free(base64_buf);
    return ret;
  }

  /* 3. 异步路径：build_card_param 内部拷贝 base64_buf，之后可安全释放 */
  async_send_param_t *param = build_card_param(xml_tag, base64_buf,
                                               base64_size + 1, ip, timeout);
  free(base64_buf);

  if (param == NULL)
    return false;

  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  return start_async_send(param);
}

/* =======================================================================
 * 对外接口
 *
 * 每个函数只传入固定的 XML 标签，其余全部透传给 card_response_send()。
 * ===================================================================== */
bool sat_ipcamera_device_card_manage_enter(const void *data, size_t data_len,
                                           const char *ip, int port,
                                           const char *user, const char *password,
                                           int timeout, bool is_block)
{
  return card_response_send("EnterCardManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_exit(const void *data, size_t data_len,
                                          const char *ip, int port,
                                          const char *user, const char *password,
                                          int timeout, bool is_block)
{
  return card_response_send("ExitCardManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_add_card(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block)
{
  return card_response_send("AddCard", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_del_card(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block)
{
  return card_response_send("DelCard", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_scan_card(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block)
{
  return card_response_send("ScanCard", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_query_card_whitelist(const void *data, size_t data_len,
                                                          const char *ip, int port,
                                                          const char *user, const char *password,
                                                          int timeout, bool is_block)
{
  return card_response_send("QueryCardWhiteList", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_del_card_whitelist(const void *data, size_t data_len,
                                                        const char *ip, int port,
                                                        const char *user, const char *password,
                                                        int timeout, bool is_block)
{
  return card_response_send("DelCardWhiteList", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_card_manage_modify_door_mask(const void *data, size_t data_len,
                                                      const char *ip, int port,
                                                      const char *user, const char *password,
                                                      int timeout, bool is_block)
{
  return card_response_send("ModifyCardDoorMask", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

/* =======================================================================
 * 密码管理接口
 * ===================================================================== */
bool sat_ipcamera_device_password_manage_enter(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block)
{
  return card_response_send("EnterPasswordManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_password_manage_exit(const void *data, size_t data_len,
                                              const char *ip, int port,
                                              const char *user, const char *password,
                                              int timeout, bool is_block)
{
  return card_response_send("ExitPasswordManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_password_manage_query(const void *data, size_t data_len,
                                               const char *ip, int port,
                                               const char *user, const char *password,
                                               int timeout, bool is_block)
{
  return card_response_send("QueryPassword", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_password_manage_add(const void *data, size_t data_len,
                                             const char *ip, int port,
                                             const char *user, const char *password,
                                             int timeout, bool is_block)
{
  return card_response_send("AddPassword", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_password_manage_del(const void *data, size_t data_len,
                                             const char *ip, int port,
                                             const char *user, const char *password,
                                             int timeout, bool is_block)
{
  return card_response_send("DelPassword", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_password_manage_set_perm(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block)
{
  return card_response_send("SetPasswordPerm", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

/* =======================================================================
 * 指纹管理接口
 * ===================================================================== */
bool sat_ipcamera_device_fingerprint_manage_enter(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block)
{
  return card_response_send("EnterFingerManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_exit(const void *data, size_t data_len,
                                                 const char *ip, int port,
                                                 const char *user, const char *password,
                                                 int timeout, bool is_block)
{
  return card_response_send("ExitFingerManage", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_query(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block)
{
  return card_response_send("QueryFinger", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_add(const void *data, size_t data_len,
                                                const char *ip, int port,
                                                const char *user, const char *password,
                                                int timeout, bool is_block)
{
  return card_response_send("AddFinger", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_del(const void *data, size_t data_len,
                                                const char *ip, int port,
                                                const char *user, const char *password,
                                                int timeout, bool is_block)
{
  return card_response_send("DelFinger", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_clear(const void *data, size_t data_len,
                                                  const char *ip, int port,
                                                  const char *user, const char *password,
                                                  int timeout, bool is_block)
{
  return card_response_send("ClearFinger", data, data_len,
                            ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_fingerprint_manage_set_perm(const void *data, size_t data_len,
                                                     const char *ip, int port,
                                                     const char *user, const char *password,
                                                     int timeout, bool is_block)
{
  return card_response_send("SetFingerPerm", data, data_len,
                            ip, port, user, password, timeout, is_block);
}
/*==========================================================================
 * 卡片响应类接口（固定端口 80 / admin / sat12345.，走 build_card_param）
 *==========================================================================*/
#define CARD_RESP_SEND(cmd_str, data, size, ip, timeout, is_block)                                     \
  do                                                                                                   \
  {                                                                                                    \
    if ((data) == NULL || (size) == 0 || (ip) == NULL || (timeout) <= 0)                               \
    {                                                                                                  \
      printf("[%s:%d] invalid params\n", __func__, __LINE__);                                          \
      return false;                                                                                    \
    }                                                                                                  \
    if (is_block)                                                                                      \
      return ipc_camera_device_send_data((cmd_str), (char *)(data),                                    \
                                         (ip), 80, USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, \
                                         (timeout));                                                   \
    async_send_param_t *_p = build_card_param((cmd_str), (data), (size),                               \
                                              (ip), (timeout));                                        \
    if (_p == NULL)                                                                                    \
      return false;                                                                                    \
    return start_async_send(_p);                                                                       \
  } while (0)

/************************************************************
** 函数说明: 卡片查询响应
************************************************************/
bool sat_ipcamera_card_call_query_response(void *data, size_t size, const char *ip,
                                           int timeout, bool is_block)
{
  CARD_RESP_SEND("CardQueryResponse", data, size, ip, timeout, is_block);
}

/************************************************************
** 函数说明: 卡片扫描响应
************************************************************/
bool sat_ipcamera_card_call_scan_response(void *data, size_t size, const char *ip,
                                          int timeout, bool is_block)
{
  CARD_RESP_SEND("CardScanResponse", data, size, ip, timeout, is_block);
}

/************************************************************
** 函数说明: 卡片添加响应
************************************************************/
bool sat_ipcamera_card_call_add_response(void *data, size_t size, const char *ip,
                                         int timeout, bool is_block)
{
  CARD_RESP_SEND("CardAddResponse", data, size, ip, timeout, is_block);
}

/************************************************************
** 函数说明: 卡片删除响应
************************************************************/
bool sat_ipcamera_card_call_del_response(void *data, size_t size, const char *ip,
                                         int timeout, bool is_block)
{
  CARD_RESP_SEND("CardDelResponse", data, size, ip, timeout, is_block);
}

/************************************************************
** 函数说明: 卡片清空响应
************************************************************/
bool sat_ipcamera_card_call_del_all_response(void *data, size_t size, const char *ip,
                                             int timeout, bool is_block)
{
  CARD_RESP_SEND("CardDelAllResponse", data, size, ip, timeout, is_block);
}

bool sat_ipcamera_device_unlock(const char *ip, int port, const char *user,
                                const char *password, int lock_time, int lock_type,
                                int language_id, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL)
    return false;

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d-%d-%d", lock_time, lock_type, language_id);
  if (ret < 0 || ret >= (int)sizeof(data))
  {
    printf("[%s:%d] format unlock param failed\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol("UnLock", data, ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_unlock_param(const char *ip, int port, const char *user,
                                      const char *password, int lock_time, int lock_type, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL)
    return false;

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d-%d", lock_time, lock_type);
  if (ret < 0 || ret >= (int)sizeof(data))
  {
    printf("[%s:%d] format unlock param failed\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol("UnLockTime", data, ip, port, user, password, timeout, is_block);
}

bool sat_ipcamera_device_talk_volume(int device_type, int building, int room,
                                     const char *ip, int port, const char *user,
                                     const char *password, int volume, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL)
    return false;

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d", volume);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("TalkVolume", data, ip, port, user, password, timeout, is_block);
}

#define IPCAMERA_CMD_NO_ONE_HOME_BROADCAST "NoOneAtHomeBroadcast"

/************************************************************
** 函数说明: 控制设备播报“无人在家”
************************************************************/
bool sat_ipcamera_device_no_one_home_broadcast(const char *ip, int port,
                                               const char *user, const char *password,
                                               int language_id, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  /* 首字段“1”触发一次播报, 第二字段为播报语言码 */
  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "1-%d", language_id);
  if (ret < 0 || ret >= (int)sizeof(data))
  {
    printf("[%s:%d] format broadcast param failed\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol(IPCAMERA_CMD_NO_ONE_HOME_BROADCAST, data,
                            ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 移动侦测开关控制
************************************************************/
bool sat_ipcamera_device_human_detect_ctrl(const char *ip, int port, const char *user,
                                           const char *password, int enable,
                                           int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  char data[32] = {0};
  int ret = snprintf(data, sizeof(data), "%d", enable);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("HumanDetectCtrl", data, ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 系统重启（门口机复位）
************************************************************/
bool sat_ipcamera_device_system_reboot(const char *ip, int port, const char *user,
                                       const char *password, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol("SystemReboot", "1", ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 系统复位（门口机恢复出厂配置）
************************************************************/
bool sat_ipcamera_device_factory_reset(const char *ip, int port, const char *user,
                                       const char *password, int timeout, bool is_block)
{
  if (ip == NULL || user == NULL || password == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  return send_text_protocol("FactoryReset", "1", ip, port, user, password, timeout, is_block);
}

/************************************************************
** 函数说明: 发送人形检测结果
************************************************************/
bool sat_ipcamera_human_detect_result(int state, const char *ip, int timeout, bool is_block)
{
  if (ip == NULL || timeout <= 0)
  {
    printf("[%s:%d] invalid params\n", __func__, __LINE__);
    return false;
  }

  char data[16] = {0};
  int ret = snprintf(data, sizeof(data), "%d", state);
  if (ret < 0 || ret >= (int)sizeof(data))
    return false;
  return send_text_protocol("HumanDetect", data, ip, 80,
                            USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD,
                            timeout, is_block);
}