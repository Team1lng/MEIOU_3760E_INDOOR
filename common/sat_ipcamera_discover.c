#include "sat_ipcamera_internal.h"
#include "onvif.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sat_user_common.h"
#include "Amy_lvgl.h"

/*==========================================================================
 * 全局变量定义
 *==========================================================================*/
struct ipcamera_info ipc_device[IPCAMERA_NUM_MAX];
int ipcamera_online_num = 0;
bool ipc_search_in_progress = false;
static bool ipc_device_busy[IPCAMERA_NUM_MAX];
static bool ipcamera_discovery_reserved = false;
static char ipc_search_device_type = 0;
static char ipc_search_ifname[16] = {0}; /* 搜索组播要绑定的网卡名（br0/eth2/wlan0） */
pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

#define NETWORK_SEND_DATA_MAX (100 * 1024)

/*==========================================================================
 * 内部工具函数
 *==========================================================================*/

void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
  if (dest == NULL || src == NULL || dest_size == 0)
    return;
  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

static void ipc_device_reset(struct ipcamera_info *device)
{
  if (device == NULL)
    return;
  memset(device->ipaddr, 0, sizeof(device->ipaddr));
  device->profile_token_num = 0;
  device->port = 80;
  device->auther_flag = AUTH_FLAG_NONE;

  for (int j = 0; j < IPCAMERA_PROFILE_MAX; j++)
  {
    memset(device->rtsp[j].profile_token, 0, sizeof(device->rtsp[j].profile_token));
    memset(device->rtsp[j].rtsp_url, 0, sizeof(device->rtsp[j].rtsp_url));
    device->rtsp[j].rtsp_url_type = 0;
  }

  memset(device->door_name, 0, sizeof(device->door_name));
  memset(device->sip_url, 0, sizeof(device->sip_url));
  memset(device->video_token, 0, sizeof(device->video_token));
}

static void ipc_device_param_init(void)
{
  ipcamera_online_num = 0;
  for (int i = 0; i < IPCAMERA_NUM_MAX; i++)
  {
    ipc_device_reset(&ipc_device[i]);
    ipc_device_busy[i] = false;
  }
}

/*==========================================================================
 * Per-index busy 标志辅助函数
 *==========================================================================*/
static bool ipc_any_busy(void)
{
  for (int i = 0; i < IPCAMERA_NUM_MAX; i++)
  {
    if (ipc_device_busy[i])
      return true;
  }
  return false;
}

/* 检查指定 index 是否可读（全局搜素未进行 && index 未 busy） */
static bool ipc_index_readable(int index)
{
  if (ipc_search_in_progress)
    return false;
  if (index < 0 || index >= IPCAMERA_NUM_MAX)
    return false;
  if (ipc_device_busy[index])
    return false;
  return true;
}

/* 列表展示用读取：ipaddr/door_name 由搜索写入，RTSP 线程 busy 期间只写 rtsp 字段，
   数据保持稳定，允许读取以便注册/失败的设备在发现列表里持续可见 */
static bool ipc_index_readable_stable(int index)
{
  if (ipc_search_in_progress)
    return false;
  if (index < 0 || index >= IPCAMERA_NUM_MAX)
    return false;
  return true;
}

/*==========================================================================
 * 内部异步发送框架（仅 discover.c 内部使用）
 *==========================================================================*/
typedef struct
{
  char type[64];
  char *data;
  char ip[32];
  int port;
  char user[64];
  char password[32];
  int timeout;
} ipcamera_network_param;

static void *ipc_camera_device_send_data_async_task(void *arg)
{
  pthread_detach(pthread_self());
  ipcamera_network_param *param = (ipcamera_network_param *)arg;
  ipc_camera_device_send_data(param->type, param->data, param->ip,
                              param->port, param->user, param->password,
                              param->timeout);
  if (param->data)
    free(param->data);
  free(param);
  return NULL;
}

static bool ipc_camera_device_send_data_async(char *data_type, char *data, int data_size,
                                              const char *ip, int port,
                                              const char *user, const char *password, int timeout)
{
  if (data_size > NETWORK_SEND_DATA_MAX)
  {
    printf("[%s:%d] data size exceeded\n", __func__, __LINE__);
    return false;
  }

  ipcamera_network_param *param = (ipcamera_network_param *)malloc(sizeof(ipcamera_network_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(ipcamera_network_param));

  strncpy(param->type, data_type, sizeof(param->type) - 1);

  param->data = (char *)malloc(NETWORK_SEND_DATA_MAX);
  if (param->data == NULL)
  {
    free(param);
    return false;
  }
  memset(param->data, 0, NETWORK_SEND_DATA_MAX);
  strncpy(param->data, data, NETWORK_SEND_DATA_MAX - 1);

  strncpy(param->ip, ip, sizeof(param->ip) - 1);
  param->port = port;
  strncpy(param->user, user, sizeof(param->user) - 1);
  strncpy(param->password, password, sizeof(param->password) - 1);
  param->timeout = timeout;

  pthread_t thread_id;
  int ret = pthread_create(&thread_id, sat_pthread_attr_get(),
                           ipc_camera_device_send_data_async_task, param);
  if (ret != 0)
  {
    free(param->data);
    free(param);
    return false;
  }
  return true;
}

/*==========================================================================
 * 线程参数结构体（discover 内部）
 *==========================================================================*/
typedef struct
{
  char name[64];
  int index;
  int timeout;
} name_set_param;

typedef struct
{
  char new_pwd[64];
  int index;
  int timeout;
} pwd_set_param;

typedef struct
{
  int index;
  int timeout;
} ipcamera_online_thread_param;

typedef struct
{
  int index;
  int timeout;
} door_version_thread_param;

/*==========================================================================
 * 设备搜索
 *==========================================================================*/
static void *ipc_online_search_task(void *arg)
{
  pthread_detach(pthread_self());
  int online_num = 0;
  char ipaddr[IPCAMERA_NUM_MAX][32];
  char name[IPCAMERA_NUM_MAX][32];
  memset(ipaddr, 0, sizeof(ipaddr));
  memset(name, 0, sizeof(name));

  ipc_camera_search(ipaddr, name, &online_num, ipc_search_device_type, ipc_search_ifname);

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress && online_num > 0)
  {
    ipc_device_param_init();

    for (int i = 0; i < online_num; i++)
    {
      char *port_sep = strchr(ipaddr[i], ':');
      if (port_sep != NULL)
      {
        *port_sep = '\0';
        sscanf(port_sep + 1, "%d", &ipc_device[i].port);
      }
      else
      {
        ipc_device[i].port = 80;
      }
      safe_strncpy(ipc_device[i].ipaddr, ipaddr[i], sizeof(ipc_device[i].ipaddr));
      safe_strncpy(ipc_device[i].door_name, name[i], sizeof(ipc_device[i].door_name));
      SAT_DEBUG("ipc_device[%d]: ip=%s, port=%d, name=%s",
                i, ipc_device[i].ipaddr, ipc_device[i].port, ipc_device[i].door_name);
    }

    ipcamera_online_num = online_num;

    ipcamera_cb_attr attr = {.type = EVENT_TYPE_SEARCH_DONE, .num = ipcamera_online_num};
    Amy_ipcamera_event_put(&attr);
  }
  ipc_search_in_progress = false;
  pthread_mutex_unlock(&ipc_mutex);
  return NULL;
}

/****************************************************************
**@功能: 启动IPC设备搜索
**@参数: type - 设备类型（0:IPC 1:DOOR CAMERA 2:IN DOOR）
*****************************************************************/
bool sat_ipcamera_device_discover_search(char type, const char *ifname)
{
  pthread_t tid;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_any_busy())
  {
    pthread_mutex_unlock(&ipc_mutex);
    SAT_DEBUG("ipc search is already in progress or device busy");
    return false;
  }
  ipc_search_in_progress = true;
  ipc_search_device_type = type;
  memset(ipc_search_ifname, 0, sizeof(ipc_search_ifname));
  if (ifname != NULL)
    strncpy(ipc_search_ifname, ifname, sizeof(ipc_search_ifname) - 1);
  pthread_mutex_unlock(&ipc_mutex);

  int ret = pthread_create(&tid, sat_pthread_attr_get(), ipc_online_search_task, NULL);
  if (ret != 0)
  {
    pthread_mutex_lock(&ipc_mutex);
    ipc_search_in_progress = false;
    pthread_mutex_unlock(&ipc_mutex);
    SAT_DEBUG("create search thread failed: %s", strerror(ret));
    return false;
  }
  return true;
}

/*==========================================================================
 * RTSP 获取
 *==========================================================================*/
static void ipc_get_rtsp_stream(struct ipcamera_info *device, int timeout)
{
  if (device == NULL)
    return;

  char profile[IPCAMERA_PROFILE_MAX][64];
  memset(profile, 0, sizeof(profile));
  int profile_num = 0;
  device->auther_flag = AUTH_FLAG_NONE;

  if (!ipc_profile_token_get(device->ipaddr, device->port, device->username,
                             device->password, profile, &profile_num, timeout))
  {
    SAT_DEBUG("profile token auth failed, try digest auth");
    if (ipc_profile_digest_get(device->ipaddr, device->port, device->username,
                               device->password, profile, &profile_num, timeout))
    {
      device->auther_flag = AUTH_FLAG_DIGEST;
    }
  }

  if (profile_num > 0)
  {
    device->profile_token_num = (profile_num > IPCAMERA_PROFILE_MAX)
                                    ? IPCAMERA_PROFILE_MAX
                                    : profile_num;
    for (int j = 0; j < device->profile_token_num; j++)
    {
      safe_strncpy(device->rtsp[j].profile_token, profile[j],
                   sizeof(device->rtsp[j].profile_token));

      if (device->auther_flag == AUTH_FLAG_NONE)
      {
        ipc_rtsp_token_get(device->ipaddr, device->port,
                           device->username, device->password,
                           device->rtsp[j].profile_token,
                           device->rtsp[j].rtsp_url,
                           device->sip_url,
                           sizeof(device->rtsp[j].rtsp_url),
                           TIMEOUT_SHORT);
      }
      else if (device->auther_flag == AUTH_FLAG_DIGEST)
      {
        ipc_rtsp_digest_get(device->ipaddr, device->port,
                            device->username, device->password,
                            device->rtsp[j].profile_token,
                            device->rtsp[j].rtsp_url,
                            device->sip_url,
                            sizeof(device->rtsp[j].rtsp_url),
                            TIMEOUT_SHORT);
      }
    }

    if (!ipc_camera_device_token_imaging_get_videosource(device->ipaddr, device->port,
                                                         device->username, device->password,
                                                         device->video_token, TIMEOUT_SHORT))
    {
      ipc_camera_device_digest_imaging_get_videosource(device->ipaddr, device->port,
                                                       device->username, device->password,
                                                       device->video_token, TIMEOUT_SHORT);
    }
  }
}

static void *ipc_rtsp_url_task(void *arg)
{
  pthread_detach(pthread_self());
  int target_index = *((int *)arg);
  free(arg);
  if (target_index < 0)
  {
    /* 全部设备：父函数已校验并置位 ipc_search_in_progress */
    for (int i = 0; i < ipcamera_online_num; i++)
      ipc_get_rtsp_stream(&ipc_device[i], TIMEOUT_SHORT);
    pthread_mutex_lock(&ipc_mutex);
    ipcamera_cb_attr attr = {.type = EVENT_TYPE_RTSP_DONE, .num = ipcamera_online_num};
    Amy_ipcamera_event_put(&attr);
    ipc_search_in_progress = false;
    pthread_mutex_unlock(&ipc_mutex);
    return NULL;
  }
  /* 单个设备：父函数已校验并置位 ipc_device_busy，且已清除旧 RTSP 数据 */
  ipc_get_rtsp_stream(&ipc_device[target_index], TIMEOUT_LONG);

  pthread_mutex_lock(&ipc_mutex);
  AMY_LOG_DEBUG("========================");
  ipc_device_busy[target_index] = false;
  ipcamera_cb_attr attr = {.type = EVENT_TYPE_RTSP_DONE, .num = ipcamera_online_num};
  Amy_ipcamera_event_put(&attr);
  pthread_mutex_unlock(&ipc_mutex);
  return NULL;
}

/****************************************************************
**@功能: 获取指定设备的RTSP流地址
**@参数: index - 设备索引（<0表示所有设备）
*****************************************************************/
bool sat_ipcamera_rtsp_url_get(int index)
{
  int *p_index = (int *)malloc(sizeof(int));
  if (p_index == NULL)
    return false;
  *p_index = index;
  if (index < 0)
  {
    /* 全部设备：检查全局搜索状态 */
    pthread_mutex_lock(&ipc_mutex);
    if (ipc_search_in_progress || ipc_any_busy() || ipcamera_online_num < 1)
    {
      pthread_mutex_unlock(&ipc_mutex);
      free(p_index);
      SAT_DEBUG("ipc search/rtsp get is in progress or device busy");
      return false;
    }
    ipc_search_in_progress = true;
    pthread_mutex_unlock(&ipc_mutex);
  }
  else
  {
    /* 单个设备：使用 per-index busy */
    pthread_mutex_lock(&ipc_mutex);
    if (index >= IPCAMERA_NUM_MAX || ipcamera_online_num < 1 ||
        ipc_device_busy[index])
    {
      pthread_mutex_unlock(&ipc_mutex);
      free(p_index);
      return false;
    }
    ipc_device_busy[index] = true;

    /* 清除旧 RTSP 数据 */
    for (int i = 0; i < IPCAMERA_PROFILE_MAX; i++)
    {
      memset(ipc_device[index].rtsp[i].profile_token, 0,
             sizeof(ipc_device[index].rtsp[i].profile_token));
      memset(ipc_device[index].rtsp[i].rtsp_url, 0,
             sizeof(ipc_device[index].rtsp[i].rtsp_url));
    }
    pthread_mutex_unlock(&ipc_mutex);
  }
  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), ipc_rtsp_url_task, p_index);
  if (ret != 0)
  {
    free(p_index);
    pthread_mutex_lock(&ipc_mutex);
    if (index < 0)
      ipc_search_in_progress = false;
    else
      ipc_device_busy[index] = false;
    pthread_mutex_unlock(&ipc_mutex);
    SAT_DEBUG("create rtsp thread failed: %s", strerror(ret));
    return false;
  }
  return true;
}

/*==========================================================================
 * 状态查询接口
 *==========================================================================*/
bool sat_ipcamera_device_status_get(void)
{
  bool status;
  pthread_mutex_lock(&ipc_mutex);
  status = ipc_search_in_progress || ipc_any_busy();
  pthread_mutex_unlock(&ipc_mutex);
  return status;
}

void sat_ipcamera_device_status_reset(void)
{
  pthread_mutex_lock(&ipc_mutex);
  ipc_search_in_progress = false;
  for (int i = 0; i < IPCAMERA_NUM_MAX; i++)
    ipc_device_busy[i] = false;
  pthread_mutex_unlock(&ipc_mutex);
}

int sat_ipcamera_online_num_get(void)
{
  int num = -1;
  pthread_mutex_lock(&ipc_mutex);
  if (!ipc_search_in_progress)
    num = ipcamera_online_num;
  pthread_mutex_unlock(&ipc_mutex);
  return num;
}

const char *sat_ipcamera_ipaddr_get(int index)
{
  const char *ip = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable_stable(index) && index < ipcamera_online_num)
    ip = ipc_device[index].ipaddr;
  pthread_mutex_unlock(&ipc_mutex);
  return ip;
}

int sat_ipcamera_port_get(int index)
{
  int port = -1;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index) && index < ipcamera_online_num)
    port = ipc_device[index].port;
  pthread_mutex_unlock(&ipc_mutex);
  return port;
}

const char *sat_ipcamera_username_get(int index)
{
  const char *user = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index))
    user = ipc_device[index].username;
  pthread_mutex_unlock(&ipc_mutex);
  return user;
}

const char *sat_ipcamera_password_get(int index)
{
  const char *pwd = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index))
    pwd = ipc_device[index].password;
  pthread_mutex_unlock(&ipc_mutex);
  return pwd;
}

const char *sat_ipcamera_door_name_get(int index)
{
  const char *name = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable_stable(index))
  {
    name = ipc_device[index].door_name;
    SAT_DEBUG("ipc_device[%d].door_name: %s", index, name);
  }
  pthread_mutex_unlock(&ipc_mutex);
  return name;
}

const char *sat_ipcamera_rtsp_addr_get(int index, int ch)
{
  const char *rtsp = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index) && index < ipcamera_online_num &&
      ch >= 0 && ch < ipc_device[index].profile_token_num)
    rtsp = ipc_device[index].rtsp[ch].rtsp_url;
  pthread_mutex_unlock(&ipc_mutex);
  return rtsp;
}

const char *sat_ipcamera_sip_addr_get(int index)
{
  const char *sip = NULL;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index) && index < ipcamera_online_num)
    sip = ipc_device[index].sip_url;
  pthread_mutex_unlock(&ipc_mutex);
  return sip;
}

int sat_ipcamera_profile_token_num_get(int index)
{
  int num = -1;
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_index_readable(index) && index < ipcamera_online_num)
    num = ipc_device[index].profile_token_num;
  pthread_mutex_unlock(&ipc_mutex);
  return num;
}

struct ipcamera_info *sat_ipcamera_node_data_get(int index)
{
  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_device_busy[index] ||
      index < 0 || index >= ipcamera_online_num)
  {
    printf("ipcamera_node_data_get: invalid state or index\n");
    pthread_mutex_unlock(&ipc_mutex);
    return NULL;
  }
  pthread_mutex_unlock(&ipc_mutex);
  return &ipc_device[index];
}

/*==========================================================================
 * 初始化与账号配置
 *==========================================================================*/
bool sat_ipcamera_discovery_reserve(void)
{
  /* 无条件置位：即使此刻有门机版本查询 busy，也在搜索结束前持续占用，
     避免搜索结束后到注册完成前的窗口被门口机轮询覆盖 */
  pthread_mutex_lock(&ipc_mutex);
  ipcamera_discovery_reserved = true;
  pthread_mutex_unlock(&ipc_mutex);
  return true;
}

void sat_ipcamera_discovery_release(void)
{
  pthread_mutex_lock(&ipc_mutex);
  ipcamera_discovery_reserved = false;
  pthread_mutex_unlock(&ipc_mutex);
}

bool sat_ipcamera_initialization_parameters(struct ipcamera_info *device, int total)
{
  if (device == NULL)
    return false;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_any_busy() || ipcamera_discovery_reserved || total > IPCAMERA_NUM_MAX)
  {
    pthread_mutex_unlock(&ipc_mutex);
    return false;
  }
  memcpy(ipc_device, device, sizeof(struct ipcamera_info) * total);
  ipcamera_online_num = total;
  pthread_mutex_unlock(&ipc_mutex);
  return true;
}

bool sat_ipcamera_user_password_set(int index, const char *username, const char *password)
{
  if (username == NULL || password == NULL)
    return false;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress)
  {
    pthread_mutex_unlock(&ipc_mutex);
    SAT_DEBUG("ipc_search_in_progress is true, cannot set user/password");
    return false;
  }

  if (index == INDEX_ALL_DEVICES)
  {
    if (ipc_any_busy())
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    for (int i = 0; i < IPCAMERA_NUM_MAX; i++)
    {
      safe_strncpy(ipc_device[i].username, username, sizeof(ipc_device[i].username));
      safe_strncpy(ipc_device[i].password, password, sizeof(ipc_device[i].password));
    }
  }
  else if (index >= 0 && index < IPCAMERA_NUM_MAX)
  {
    if (ipc_device_busy[index])
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    safe_strncpy(ipc_device[index].username, username, sizeof(ipc_device[index].username));
    safe_strncpy(ipc_device[index].password, password, sizeof(ipc_device[index].password));
  }
  else
  {
    pthread_mutex_unlock(&ipc_mutex);
    return false;
  }

  pthread_mutex_unlock(&ipc_mutex);
  return true;
}

/*==========================================================================
 * 设备操作（通过索引，搜索阶段使用）
 *==========================================================================*/
bool sat_ipcamera_device_delete(int index, int timeout, bool is_block)
{
  if (index < 0 || index >= IPCAMERA_NUM_MAX || timeout <= 0)
    return false;

  if (is_block)
  {
    pthread_mutex_lock(&ipc_mutex);
    if (ipc_search_in_progress || ipc_device_busy[index] || ipcamera_online_num < 1)
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    ipc_device_busy[index] = true;
    pthread_mutex_unlock(&ipc_mutex);

    char data[128] = {0};
    bool result = ipc_camera_device_send_data("Delete", data,
                                              ipc_device[index].ipaddr,
                                              ipc_device[index].port,
                                              ipc_device[index].username,
                                              ipc_device[index].password, timeout);
    pthread_mutex_lock(&ipc_mutex);
    ipc_device_busy[index] = false;
    pthread_mutex_unlock(&ipc_mutex);
    return result;
  }

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_device_busy[index] || ipcamera_online_num < 1)
  {
    pthread_mutex_unlock(&ipc_mutex);
    return false;
  }
  ipc_device_busy[index] = true;
  pthread_mutex_unlock(&ipc_mutex);

  char data[128] = {0};
  bool ret = ipc_camera_device_send_data_async("Delete", data, sizeof(data),
                                               ipc_device[index].ipaddr,
                                               ipc_device[index].port,
                                               ipc_device[index].username,
                                               ipc_device[index].password, timeout);
  pthread_mutex_lock(&ipc_mutex);
  ipc_device_busy[index] = false;
  pthread_mutex_unlock(&ipc_mutex);
  return ret;
}

// name_set 线程函数
static void *name_set_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  name_set_param *p = (name_set_param *)arg;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_device_busy[p->index] || ipcamera_online_num < 1)
  {
    pthread_mutex_unlock(&ipc_mutex);
    free(p);
    return NULL;
  }
  ipc_device_busy[p->index] = true;

  /* snapshot device connection info */
  char ip[32], user[64], pwd[32];
  int port, auth_flag;
  strncpy(ip, ipc_device[p->index].ipaddr, sizeof(ip) - 1);
  port = ipc_device[p->index].port;
  strncpy(user, ipc_device[p->index].username, sizeof(user) - 1);
  strncpy(pwd, ipc_device[p->index].password, sizeof(pwd) - 1);
  auth_flag = ipc_device[p->index].auther_flag;
  pthread_mutex_unlock(&ipc_mutex);

  bool ret = ipc_camera_device_name_set(p->name, ip, port, user, pwd, auth_flag, p->timeout);

  pthread_mutex_lock(&ipc_mutex);
  ipc_device_busy[p->index] = false;
  if (ret && strcmp(ipc_device[p->index].door_name, p->name) != 0)
    strncpy(ipc_device[p->index].door_name, p->name,
            sizeof(ipc_device[p->index].door_name) - 1);
  pthread_mutex_unlock(&ipc_mutex);
  free(p);
  return NULL;
}

bool sat_ipcamera_device_name_set(char *name, int index, int timeout, bool is_block)
{
  if (name == NULL || index < 0 || index >= IPCAMERA_NUM_MAX || timeout <= 0)
    return false;

  if (is_block)
  {
    pthread_mutex_lock(&ipc_mutex);
    if (ipc_search_in_progress || ipc_device_busy[index] || ipcamera_online_num < 1)
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    ipc_device_busy[index] = true;
    pthread_mutex_unlock(&ipc_mutex);

    bool result = ipc_camera_device_name_set(name,
                                             ipc_device[index].ipaddr,
                                             ipc_device[index].port,
                                             ipc_device[index].username,
                                             ipc_device[index].password,
                                             ipc_device[index].auther_flag, timeout);
    pthread_mutex_lock(&ipc_mutex);
    ipc_device_busy[index] = false;
    if (result && strcmp(ipc_device[index].door_name, name) != 0)
      strncpy(ipc_device[index].door_name, name,
              sizeof(ipc_device[index].door_name) - 1);
    pthread_mutex_unlock(&ipc_mutex);
    return result;
  }

  name_set_param *param = (name_set_param *)malloc(sizeof(name_set_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(name_set_param));
  strncpy(param->name, name, sizeof(param->name) - 1);
  param->index = index;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), name_set_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

// pwd_set 线程函数
static void *pwd_set_thread_func(void *arg)
{
  pthread_detach(pthread_self());
  pwd_set_param *p = (pwd_set_param *)arg;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || ipc_device_busy[p->index] || ipcamera_online_num < 1)
  {
    pthread_mutex_unlock(&ipc_mutex);
    free(p);
    return NULL;
  }
  ipc_device_busy[p->index] = true;

  /* snapshot device connection info */
  char ip[32], user[64], pwd[32];
  int port, auth_flag;
  strncpy(ip, ipc_device[p->index].ipaddr, sizeof(ip) - 1);
  port = ipc_device[p->index].port;
  strncpy(user, ipc_device[p->index].username, sizeof(user) - 1);
  strncpy(pwd, ipc_device[p->index].password, sizeof(pwd) - 1);
  auth_flag = ipc_device[p->index].auther_flag;
  pthread_mutex_unlock(&ipc_mutex);

  bool ret = ipc_camera_device_password_change(p->new_pwd, ip, port, user, pwd, auth_flag, p->timeout);

  pthread_mutex_lock(&ipc_mutex);
  ipc_device_busy[p->index] = false;
  if (ret && strcmp(ipc_device[p->index].password, p->new_pwd) != 0)
    strncpy(ipc_device[p->index].password, p->new_pwd,
            sizeof(ipc_device[p->index].password) - 1);
  pthread_mutex_unlock(&ipc_mutex);
  free(p);
  return NULL;
}

bool sat_ipcamera_device_password_set(char *new_pwd, int index, int timeout, bool is_block)
{
  if (new_pwd == NULL || index < 0 || index >= IPCAMERA_NUM_MAX || timeout <= 0)
    return false;

  if (is_block)
  {
    pthread_mutex_lock(&ipc_mutex);
    if (ipc_search_in_progress || ipc_device_busy[index] || ipcamera_online_num < 1)
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    ipc_device_busy[index] = true;
    pthread_mutex_unlock(&ipc_mutex);

    bool result = ipc_camera_device_password_change(new_pwd,
                                                    ipc_device[index].ipaddr,
                                                    ipc_device[index].port,
                                                    ipc_device[index].username,
                                                    ipc_device[index].password,
                                                    ipc_device[index].auther_flag, timeout);
    pthread_mutex_lock(&ipc_mutex);
    ipc_device_busy[index] = false;
    if (result && strcmp(ipc_device[index].password, new_pwd) != 0)
      strncpy(ipc_device[index].password, new_pwd,
              sizeof(ipc_device[index].password) - 1);
    pthread_mutex_unlock(&ipc_mutex);
    return result;
  }

  pwd_set_param *param = (pwd_set_param *)malloc(sizeof(pwd_set_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(pwd_set_param));
  strncpy(param->new_pwd, new_pwd, sizeof(param->new_pwd) - 1);
  param->index = index;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), pwd_set_thread_func, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}

static bool ipcamera_online_check_device_snapshot(int index, struct ipcamera_info *device)
{
  if (device == NULL)
    return false;

  pthread_mutex_lock(&ipc_mutex);
  if (ipc_search_in_progress || index < 0 || index >= ipcamera_online_num)
  {
    pthread_mutex_unlock(&ipc_mutex);
    return false;
  }

  memcpy(device, &ipc_device[index], sizeof(struct ipcamera_info));
  pthread_mutex_unlock(&ipc_mutex);
  return true;
}

static void ipcamera_online_check_result_update(int index, bool result, const char *name)
{
  pthread_mutex_lock(&ipc_mutex);
  ipcamera_online_check_attr attr = {.index = index, .is_online = result};
  Amy_ipcamera_online_event_put(&attr);
  if (result && name != NULL && index >= 0 && index < ipcamera_online_num &&
      strcmp(ipc_device[index].door_name, name) != 0)
  {
    strncpy(ipc_device[index].door_name, name,
            sizeof(ipc_device[index].door_name) - 1);
    ipc_device[index].door_name[sizeof(ipc_device[index].door_name) - 1] = '\0';
  }
  pthread_mutex_unlock(&ipc_mutex);
}

// online_check 线程函数
static void *ipcamera_online_check_task(void *arg)
{
  pthread_detach(pthread_self());
  ipcamera_online_thread_param *p = (ipcamera_online_thread_param *)arg;
  if (p == NULL)
    return NULL;

  struct ipcamera_info device;
  if (!ipcamera_online_check_device_snapshot(p->index, &device))
  {
    free(p);
    return NULL;
  }

  char name[64] = {0};
  bool result = ipc_camera_device_name_get(name,
                                           device.ipaddr,
                                           device.port,
                                           device.username,
                                           device.password,
                                           device.auther_flag,
                                           p->timeout);

  ipcamera_online_check_result_update(p->index, result, name);
  free(p);
  return NULL;
}

bool sat_ipcamera_online_checkt(int index, int timeout, bool is_block)
{
  if (index < 0 || index >= IPCAMERA_NUM_MAX || timeout <= 0)
    return false;

  if (is_block)
  {
    struct ipcamera_info device;
    if (!ipcamera_online_check_device_snapshot(index, &device))
      return false;

    char name[64] = {0};
    bool result = ipc_camera_device_name_get(name,
                                             device.ipaddr,
                                             device.port,
                                             device.username,
                                             device.password,
                                             device.auther_flag, timeout);

    ipcamera_online_check_result_update(index, result, name);
    return result;
  }

  /* async: 只读操作，不占 busy 标志 */
  ipcamera_online_thread_param *param =
      (ipcamera_online_thread_param *)malloc(sizeof(ipcamera_online_thread_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(ipcamera_online_thread_param));
  param->index = index;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), ipcamera_online_check_task, param);
  if (ret != 0)
  {
    free(param);
    pthread_mutex_lock(&ipc_mutex);
    ipc_device_busy[index] = false;
    pthread_mutex_unlock(&ipc_mutex);
    return false;
  }
  return true;
}

// version_get 线程函数（只读操作，不占 busy 标志）
static void *ipcamera_query_version_task(void *arg)
{
  pthread_detach(pthread_self());
  door_version_thread_param *p = (door_version_thread_param *)arg;
  if (p == NULL)
    return NULL;

  int index = p->index;

  /* snapshot device connection info under lock */
  pthread_mutex_lock(&ipc_mutex);
  if (ipcamera_online_num < 1 || index < 0 || index >= IPCAMERA_NUM_MAX)
  {
    free(p);
    pthread_mutex_unlock(&ipc_mutex);
    return NULL;
  }
  char ip[32], user[64], pass[32];
  int port;
  strncpy(ip, ipc_device[index].ipaddr, sizeof(ip) - 1);
  port = ipc_device[index].port;
  strncpy(user, ipc_device[index].username, sizeof(user) - 1);
  strncpy(pass, ipc_device[index].password, sizeof(pass) - 1);
  pthread_mutex_unlock(&ipc_mutex);

  /* network I/O without holding any lock or flag */
  char local_buf[32] = {0};
  bool result = ipc_camera_device_send_data("GetVersion", local_buf,
                                            ip, port, user, pass, p->timeout);

  pthread_mutex_lock(&ipc_mutex);
  door_version_cb_attr attr;
  attr.index = index;
  attr.result = result;
  strncpy(attr.version, local_buf, sizeof(attr.version) - 1);
  attr.version[sizeof(attr.version) - 1] = '\0';
  Amy_door_version_event_put(&attr);

  free(p);
  pthread_mutex_unlock(&ipc_mutex);
  return NULL;
}

bool sat_ipcamera_device_version_get(int index, int timeout, bool is_block, char *version_buf)
{
  if (index < 0 || index >= IPCAMERA_NUM_MAX || timeout <= 0)
    return false;
  if (is_block && version_buf == NULL)
    return false;

  if (is_block)
  {
    pthread_mutex_lock(&ipc_mutex);
    if (ipc_search_in_progress || ipc_device_busy[index] || ipcamera_online_num < 1)
    {
      pthread_mutex_unlock(&ipc_mutex);
      return false;
    }
    ipc_device_busy[index] = true;

    char ip[32], user[64], pass[32];
    int port;
    strncpy(ip, ipc_device[index].ipaddr, sizeof(ip) - 1);
    port = ipc_device[index].port;
    strncpy(user, ipc_device[index].username, sizeof(user) - 1);
    strncpy(pass, ipc_device[index].password, sizeof(pass) - 1);
    pthread_mutex_unlock(&ipc_mutex);

    char local_buf[128] = {0};
    bool result = ipc_camera_device_send_data("GetVersion", local_buf,
                                              ip, port, user, pass, timeout);
    pthread_mutex_lock(&ipc_mutex);
    ipc_device_busy[index] = false;
    pthread_mutex_unlock(&ipc_mutex);

    if (result)
    {
      strncpy(version_buf, local_buf, 127);
      version_buf[127] = '\0';
    }
    return result;
  }

  /* async: 只读操作，不占 busy 标志，直接 snapshot 后启动线程 */
  door_version_thread_param *param =
      (door_version_thread_param *)malloc(sizeof(door_version_thread_param));
  if (param == NULL)
    return false;
  memset(param, 0, sizeof(door_version_thread_param));
  param->index = index;
  param->timeout = timeout;

  pthread_t tid;
  int ret = pthread_create(&tid, sat_pthread_attr_get(), ipcamera_query_version_task, param);
  if (ret != 0)
  {
    free(param);
    return false;
  }
  return true;
}
