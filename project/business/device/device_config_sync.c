/**
 * @file        user_network.c
 * @brief       设备网络配置同步模块（组播同步公共配置）
 * @details
 *              实现主机与分机之间的配置自动同步：
 *              - 主机配置变更后组播推送
 *              - 分机定时拉取配置
 *              - 同一栋楼/房间内设备自动组网同步
 */

#include "sat_user_common.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "user_data.h"
#include "user_time.h"

/** 配置同步定时拉取间隔（秒） */
#define USER_NETWORK_SYNC_PULL_INTERVAL_SEC 60

/** 启动后延迟拉取时间（秒） */
#define USER_NETWORK_SYNC_STARTUP_DELAY_SEC 5

/** 配置同步魔法字 */
#define USER_NETWORK_SYNC_MAGIC 0x53464743u

/** 配置同步数据版本 */
#define USER_NETWORK_SYNC_VERSION 1u

/** 公共配置组播同步端口（与 sat_ipcamera_conflict 的 37210 区分） */
#define CS_UDP_PORT 37211

/** 组播包头部魔法字符 */
#define CS_MAGIC0 'C'
#define CS_MAGIC1 'S'
#define CS_MAGIC2 'Y'
#define CS_MAGIC3 '1'

/** 组播包协议版本 */
#define CS_PKT_VER 1

/** 分机→组播：请求主机下发配置（主机响应；分机收到同类型包应忽略） */
#define CS_MSG_PULL_REQ 1

/** 主机→组播：推送完整 public 配置（仅分机应用） */
#define CS_MSG_PUSH 2

/**
 * @brief 配置同步有效数据结构体
 * @note  用于主机推送完整公共配置
 */
typedef struct
{
  uint32_t magic;              /**< 魔法字 */
  uint32_t version;            /**< 数据版本 */
  sys_public_cfg_t public_cfg; /**< 公共配置 */
} user_network_sync_payload_t;

#pragma pack(push, 1)
/**
 * @brief 配置同步组播包头部（1字节对齐）
 */
typedef struct
{
  uint8_t magic[4];        /**< 固定魔法字 'CSY1' */
  uint8_t ver;             /**< 协议版本 */
  uint8_t msg_type;        /**< 消息类型：1=拉取请求 2=推送 */
  uint8_t building;        /**< 楼号 */
  uint16_t room_be;        /**< 房间号（大端） */
  uint8_t sender_ext;      /**< 发送者扩展类型 */
  uint16_t payload_len_be; /**< 数据体长度（大端），PULL=0 */
} cs_pkt_hdr_t;
#pragma pack(pop)

static volatile int s_cfg_sync_listener_run;                               /**< 监听线程运行标志 */
static pthread_t s_cfg_sync_listener_tid;                                  /**< 监听线程ID */
static pthread_mutex_t s_cfg_sync_listener_mx = PTHREAD_MUTEX_INITIALIZER; /**< 线程锁 */
static int s_cfg_sync_listener_sock = -1;                                  /**< UDP 套接字 */
static char s_cfg_sync_listener_mcast[20];                                 /**< 当前加入的组播IP */

/** 接收缓冲区最大长度 */
#define CFG_SYNC_RECV_MAX (sizeof(cs_pkt_hdr_t) + sizeof(user_network_sync_payload_t) + 256)
static char s_cfg_sync_rx_buf[CFG_SYNC_RECV_MAX]; /**< 接收缓冲区 */

/**
 * @brief  判断当前设备是否为主机
 * @retval true  - 主机
 * @retval false - 分机
 */
static bool user_network_is_master(void)
{
  return (user_data_get()->private_cfg.device.attr.device_type == 1);
}

/**
 * @brief  构造本组房间的组播IP
 * @param  building  楼号
 * @param  room      房间号
 * @param  out       输出IP字符串
 * @param  out_len   输出缓冲区长度
 * @return 无
 * @note   格式：239.栋.房高8位.房低8位
 */
static void cfg_sync_build_mcast_ip(int building, int room, char *out, size_t out_len)
{
  if (!out || out_len < 16)
    return;

  int rh = (room >> 8) & 0xFF;
  int rl = room & 0xFF;
  snprintf(out, out_len, "239.%d.%d.%d", building & 0xFF, rh, rl);
}

/**
 * @brief  关闭组播回环（不接收自己发的包）
 * @param  sock  UDP套接字
 * @return 无
 */
static void cfg_sync_set_multicast_loop(int sock)
{
  unsigned char loop = 0;
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
}

/**
 * @brief  填充组播包头部
 * @param  h            包头部指针
 * @param  msg_type     消息类型
 * @param  building     楼号
 * @param  room         房间号
 * @param  sender_ext   发送者扩展信息
 * @param  body_len     数据体长度
 * @return 无
 */
static void cfg_sync_fill_hdr(cs_pkt_hdr_t *h, uint8_t msg_type, int building, int room,
                              int sender_ext, uint16_t body_len)
{
  h->magic[0] = CS_MAGIC0;
  h->magic[1] = CS_MAGIC1;
  h->magic[2] = CS_MAGIC2;
  h->magic[3] = CS_MAGIC3;
  h->ver = CS_PKT_VER;
  h->msg_type = msg_type;
  h->building = (uint8_t)(building & 0xFF);
  h->room_be = htons((uint16_t)(room & 0xFFFF));
  h->sender_ext = (uint8_t)(sender_ext & 0xFF);
  h->payload_len_be = htons(body_len);
}

/**
 * @brief  校验组播包头部合法性
 * @param  h  包头部指针
 * @retval true  合法
 * @retval false 非法
 */
static bool cfg_sync_hdr_valid(const cs_pkt_hdr_t *h)
{
  if (h->magic[0] != CS_MAGIC0 || h->magic[1] != CS_MAGIC1 ||
      h->magic[2] != CS_MAGIC2 || h->magic[3] != CS_MAGIC3)
    return false;

  if (h->ver != CS_PKT_VER)
    return false;

  if (h->msg_type != CS_MSG_PULL_REQ && h->msg_type != CS_MSG_PUSH)
    return false;

  return true;
}

/**
 * @brief  构建配置同步数据体
 * @param  payload  输出数据体指针
 * @return 无
 */
static void user_network_sync_payload_build(user_network_sync_payload_t *payload)
{
  user_data_info *cfg = user_data_get();
  memset(payload, 0, sizeof(*payload));

  payload->magic = USER_NETWORK_SYNC_MAGIC;
  payload->version = USER_NETWORK_SYNC_VERSION;
  memcpy(&payload->public_cfg, &cfg->public_cfg, sizeof(sys_public_cfg_t));

  struct tm tm;
  user_time_read(&tm);
  payload->public_cfg.time = tm;
}

/**
 * @brief  校验配置同步数据体合法性
 * @param  payload  数据体指针
 * @retval true  合法
 * @retval false 非法
 */
static bool user_network_sync_payload_validate_bin(const user_network_sync_payload_t *payload)
{
  if (payload->magic != USER_NETWORK_SYNC_MAGIC || payload->version != USER_NETWORK_SYNC_VERSION)
  {
    SAT_ERROR("[NET] config sync payload invalid (magic=0x%x version=%u)",
              payload->magic, payload->version);
    return false;
  }
  return true;
}

/**
 * @brief  应用收到的配置同步数据
 * @param  payload  配置数据体
 * @retval true  应用成功
 * @retval false 应用失败
 */
static bool user_network_sync_payload_apply(const user_network_sync_payload_t *payload)
{
  if (payload == NULL)
    return false;

  user_data_info *cfg = user_data_get();
  memcpy(&cfg->public_cfg, &payload->public_cfg, sizeof(sys_public_cfg_t));
  user_time_set(&cfg->public_cfg.time);

  if (!user_data_save())
  {
    SAT_ERROR("[NET] config sync apply save failed");
    return false;
  }
  return true;
}

/**
 * @brief  向组播地址发送原始UDP包
 * @param  mcast_ip  组播IP
 * @param  pkt       数据包指针
 * @param  pkt_len   数据包长度
 * @retval true  发送成功
 * @retval false 发送失败
 */
static bool cfg_sync_mcast_send_raw(const char *mcast_ip, const void *pkt, size_t pkt_len)
{
  int sock = -1;
  if (!sat_socket_udp_open(&sock, CS_UDP_PORT, false) || sock < 0)
  {
    SAT_ERROR("[NET][CS] udp open failed");
    return false;
  }

  cfg_sync_set_multicast_loop(sock);

  if (!sat_socket_multicast_join(sock, mcast_ip))
  {
    SAT_ERROR("[NET][CS] multicast join %s failed", mcast_ip);
    sat_socket_close(sock);
    return false;
  }

  sat_socket_udp_send(sock, (const char *)pkt, pkt_len, mcast_ip, CS_UDP_PORT, 500);

  sat_socket_close(sock);
  return true;
}

/**
 * @brief  主机：组播广播推送完整公共配置
 * @retval true  推送成功
 * @retval false 推送失败
 */
static bool cfg_sync_mcast_broadcast_push(void)
{
  device_attr_info *attr = &user_data_get()->private_cfg.device.attr;
  if (attr->building < 1 || attr->building > 255)
    return false;

  char mcast_ip[20];
  cfg_sync_build_mcast_ip(attr->building, attr->room, mcast_ip, sizeof(mcast_ip));

  user_network_sync_payload_t body;
  user_network_sync_payload_build(&body);

  size_t pkt_len = sizeof(cs_pkt_hdr_t) + sizeof(body);
  char *pkt = (char *)malloc(pkt_len);
  if (!pkt)
    return false;

  cs_pkt_hdr_t *h = (cs_pkt_hdr_t *)pkt;
  cfg_sync_fill_hdr(h, CS_MSG_PUSH, attr->building, attr->room, attr->device_type, sizeof(body));
  memcpy(pkt + sizeof(cs_pkt_hdr_t), &body, sizeof(body));

  bool ok = cfg_sync_mcast_send_raw(mcast_ip, pkt, pkt_len);
  free(pkt);
  return ok;
}

/**
 * @brief  分机：发送配置拉取请求
 * @retval true  发送成功
 * @retval false 发送失败
 */
static bool cfg_sync_mcast_pull_request_send(void)
{
  device_attr_info *attr = &user_data_get()->private_cfg.device.attr;
  if (attr->building < 1 || attr->building > 255)
    return false;

  char mcast_ip[20];
  cfg_sync_build_mcast_ip(attr->building, attr->room, mcast_ip, sizeof(mcast_ip));

  cs_pkt_hdr_t h;
  cfg_sync_fill_hdr(&h, CS_MSG_PULL_REQ, attr->building, attr->room, attr->device_type, 0);
  return cfg_sync_mcast_send_raw(mcast_ip, &h, sizeof(h));
}

/**
 * @brief  主机配置推送任务（单独线程）
 * @param  arg  线程参数
 * @return NULL
 */
static void *user_network_master_push_task(void *arg)
{
  (void)arg;
  cfg_sync_mcast_broadcast_push();
  return NULL;
}

/**
 * @brief  分机定时拉取任务
 * @param  arg  线程参数
 * @return NULL
 */
static void *user_network_slave_pull_task(void *arg)
{
  (void)arg;
  sleep(USER_NETWORK_SYNC_STARTUP_DELAY_SEC);

  while (1)
  {
    if (!user_network_is_master())
      cfg_sync_mcast_pull_request_send();

    sleep(USER_NETWORK_SYNC_PULL_INTERVAL_SEC);
  }
  return NULL;
}

/**
 * @brief  加锁关闭监听套接字
 * @return 无
 */
static void cfg_sync_listener_close_sock_locked(void)
{
  if (s_cfg_sync_listener_sock >= 0)
  {
    sat_socket_close(s_cfg_sync_listener_sock);
    s_cfg_sync_listener_sock = -1;
  }
  s_cfg_sync_listener_mcast[0] = '\0';
}

/**
 * @brief  确保加入正确的组播组
 * @param  building  楼号
 * @param  room      房间号
 * @retval true  加入成功
 * @retval false 加入失败
 */
static bool cfg_sync_listener_ensure_mcast_join_locked(int building, int room)
{
  char mcast[20];
  cfg_sync_build_mcast_ip(building, room, mcast, sizeof(mcast));

  // 组播未变化则直接返回
  if (s_cfg_sync_listener_sock >= 0 && !strcmp(mcast, s_cfg_sync_listener_mcast))
    return true;

  cfg_sync_listener_close_sock_locked();

  if (!sat_socket_udp_open(&s_cfg_sync_listener_sock, CS_UDP_PORT, false) || s_cfg_sync_listener_sock < 0)
  {
    SAT_ERROR("[NET][CS] listener udp open failed");
    return false;
  }

  cfg_sync_set_multicast_loop(s_cfg_sync_listener_sock);

  if (!sat_socket_multicast_join(s_cfg_sync_listener_sock, mcast))
  {
    SAT_ERROR("[NET][CS] listener multicast join %s failed", mcast);
    sat_socket_close(s_cfg_sync_listener_sock);
    s_cfg_sync_listener_sock = -1;
    return false;
  }

  strncpy(s_cfg_sync_listener_mcast, mcast, sizeof(s_cfg_sync_listener_mcast) - 1);
  s_cfg_sync_listener_mcast[sizeof(s_cfg_sync_listener_mcast) - 1] = 0;
  return true;
}

/**
 * @brief  处理收到的组播包
 * @param  rbuf  接收缓冲区
 * @param  n     数据长度
 * @return 无
 */
static void cfg_sync_handle_received(const char *rbuf, int n)
{
  if (n < (int)sizeof(cs_pkt_hdr_t))
    return;

  const cs_pkt_hdr_t *h = (const cs_pkt_hdr_t *)rbuf;
  if (!cfg_sync_hdr_valid(h))
    return;

  // 只处理同栋、同房间的包
  int rb = h->building;
  int rr = (int)ntohs(h->room_be);
  device_attr_info *me = &user_data_get()->private_cfg.device.attr;
  if (rb != me->building || rr != me->room)
    return;

  // 处理拉取请求（仅主机处理）
  if (h->msg_type == CS_MSG_PULL_REQ)
  {
    if (!user_network_is_master())
      return;

    uint16_t plen = ntohs(h->payload_len_be);
    if (plen != 0 || n != (int)sizeof(cs_pkt_hdr_t))
      return;

    cfg_sync_mcast_broadcast_push();
    return;
  }

  // 处理配置推送（仅分机处理）
  if (h->msg_type == CS_MSG_PUSH)
  {
    if (user_network_is_master())
      return;

    uint16_t plen = ntohs(h->payload_len_be);
    if (plen != sizeof(user_network_sync_payload_t))
      return;

    if (n < (int)(sizeof(cs_pkt_hdr_t) + sizeof(user_network_sync_payload_t)))
      return;

    user_network_sync_payload_t payload;
    memcpy(&payload, rbuf + sizeof(cs_pkt_hdr_t), sizeof(payload));

    if (!user_network_sync_payload_validate_bin(&payload))
      return;

    user_network_sync_payload_apply(&payload);
  }
}

/**
 * @brief  配置同步监听线程
 * @param  arg  线程参数
 * @return NULL
 */
static void *cfg_sync_listener_thread(void *arg)
{
  pthread_detach(pthread_self());

  while (s_cfg_sync_listener_run)
  {
    device_attr_info *attr = &user_data_get()->private_cfg.device.attr;
    if (attr->building < 1 || attr->building > 255)
    {
      pthread_mutex_lock(&s_cfg_sync_listener_mx);
      cfg_sync_listener_close_sock_locked();
      pthread_mutex_unlock(&s_cfg_sync_listener_mx);
      usleep(500 * 1000);
      continue;
    }

    pthread_mutex_lock(&s_cfg_sync_listener_mx);
    if (!cfg_sync_listener_ensure_mcast_join_locked(attr->building, attr->room))
    {
      pthread_mutex_unlock(&s_cfg_sync_listener_mx);
      usleep(1000 * 1000);
      continue;
    }
    int sock = s_cfg_sync_listener_sock;
    pthread_mutex_unlock(&s_cfg_sync_listener_mx);

    struct sockaddr_in from;
    int recvn = sat_socket_udp_receive(sock, s_cfg_sync_rx_buf, sizeof(s_cfg_sync_rx_buf), &from, 400);
    if (recvn < (int)sizeof(cs_pkt_hdr_t))
      continue;

    cfg_sync_handle_received(s_cfg_sync_rx_buf, recvn);
  }

  pthread_mutex_lock(&s_cfg_sync_listener_mx);
  cfg_sync_listener_close_sock_locked();
  pthread_mutex_unlock(&s_cfg_sync_listener_mx);
  return NULL;
}

/**
 * @brief  启动配置同步监听线程（只启动一次）
 * @return 无
 */
static void cfg_sync_listener_start(void)
{
  static bool s_once;
  if (s_once)
    return;
  s_once = true;

  s_cfg_sync_listener_run = 1;
  if (pthread_create(&s_cfg_sync_listener_tid, sat_pthread_attr_get(),
                     cfg_sync_listener_thread, NULL) != 0)
  {
    SAT_ERROR("[NET][CS] listener pthread_create failed");
    s_cfg_sync_listener_run = 0;
    s_once = false;
  }
}

/**
 * @brief  设备配置同步启动（外部调用）
 * @return 无
 */
void device_config_sync_start(void)
{
  cfg_sync_listener_start();
  if (!user_network_is_master())
    return;
  pthread_t sync_tid;
  pthread_create(&sync_tid, sat_pthread_attr_get(), user_network_slave_pull_task, NULL);
  pthread_detach(sync_tid);
}

/**
 * @brief  配置变更通知（主机主动推送）
 * @return 无
 */
void device_notify_config_changed(void)
{
  pthread_t tid;
  if (!user_network_is_master())
    return;

  if (pthread_create(&tid, sat_pthread_attr_get(), user_network_master_push_task, NULL) == 0)
    pthread_detach(tid);
}