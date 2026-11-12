/**
 * @file        sat_ipcamera_conflict.c
 * @brief       同栋+同房+同类型设备冲突探测（组播）
 * @details     基于UDP组播实现设备冲突检测，包含主动探测和常驻监听双机制
 *              主动探测：一次性冲突检查，支持同步/异步
 *              常驻监听：后台实时监听同组播域内的冲突设备上报
 *              优化：使用组播禁止本地回环实现，不再依赖MAC地址过滤
 */

#include "sat_ipcamera.h"
#include "sat_user_common.h"
#include "Amy_lvgl.h"
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "user_data.h"
#include "Amy_sdk.h"
/** 冲突包魔数字节0 */
#define DC_MAGIC0 'D'
/** 冲突包魔数字节1 */
#define DC_MAGIC1 'C'
/** 冲突包魔数字节2 */
#define DC_MAGIC2 'F'
/** 冲突包魔数字节3 */
#define DC_MAGIC3 '1'
/** 冲突包协议版本 */
#define DC_PKT_VER 1
/** 冲突检测UDP端口号 */
#define DC_UDP_PORT 37210
/** 冲突检测组播绑定网卡 */
#define DC_IFACE "br0"
/** 冲突检测主动探测间隔 */
#define DC_PROBE_INTERVAL_MS 10000
/** 冲突包固定长度 */
#define DC_PKT_SIZE sizeof(dc_conflict_pkt_t)

#pragma pack(push, 1)
/**
 * @brief 设备冲突检测组播数据包结构体
 * @note  1字节对齐，保证网络传输格式一致
 * @note  已移除MAC地址，通过禁止组播本地回环防止自收包
 */
typedef struct
{
  uint8_t magic[4];    /**< 魔数：固定为"DCF1" */
  uint8_t ver;         /**< 协议版本 */
  uint8_t building;    /**< 楼栋号 */
  uint16_t room_be;    /**< 房间号（大端序） */
  uint8_t device_type; /**< 设备类型 */
} dc_conflict_pkt_t;
#pragma pack(pop)

/**
 * @brief 异步检测工作线程参数结构体
 */
typedef struct
{
  int building;    /**< 检测楼栋号 */
  int room;        /**< 检测房间号 */
  int device_type; /**< 检测设备类型 */
  int timeout_ms;  /**< 检测超时时间（ms） */
} conflict_work_t;

/**
 * @brief LVGL主线程回调分发结构体
 */
typedef struct
{
  bool check_ok; /**< 检测是否执行成功 */
  bool conflict; /**< 是否检测到冲突 */
} conflict_lv_dispatch_t;

/**
 * @brief  根据楼栋和房间构建组播IP地址
 * @param[in]  building  楼栋号
 * @param[in]  room      房间号
 * @param[out] out       输出组播IP字符串缓冲区
 * @param[in]  out_len   缓冲区长度
 * @return  无
 * @note   组播地址规则：239.楼栋.房间高8位.房间低8位
 */
static void dc_build_mcast_ip(int building, int room, char *out, size_t out_len)
{
  if (!out || out_len < 16)
    return;

  int rh = (room >> 8) & 0xFF;
  int rl = room & 0xFF;
  snprintf(out, out_len, "239.%d.%d.%d", building & 0xFF, rh, rl);
}

/**
 * @brief  填充冲突检测数据包
 * @param[out] pkt          待填充的数据包
 * @param[in]  building     楼栋号
 * @param[in]  room         房间号
 * @param[in]  device_type  设备类型
 * @return  无
 */
static void dc_fill_pkt(dc_conflict_pkt_t *pkt, int building, int room, int device_type)
{
  pkt->magic[0] = DC_MAGIC0;
  pkt->magic[1] = DC_MAGIC1;
  pkt->magic[2] = DC_MAGIC2;
  pkt->magic[3] = DC_MAGIC3;
  pkt->ver = DC_PKT_VER;
  pkt->building = (uint8_t)(building & 0xFF);
  pkt->room_be = htons((uint16_t)(room & 0xFFFF));
  pkt->device_type = (uint8_t)(device_type & 0xFF);
}

/**
 * @brief  校验冲突数据包合法性
 * @param[in]  pkt  接收到的冲突包
 * @retval true  合法有效
 * @retval false 非法无效
 */
static bool dc_pkt_valid(const dc_conflict_pkt_t *pkt)
{
  if (pkt->magic[0] != DC_MAGIC0 || pkt->magic[1] != DC_MAGIC1 ||
      pkt->magic[2] != DC_MAGIC2 || pkt->magic[3] != DC_MAGIC3)
    return false;

  if (pkt->ver != DC_PKT_VER)
    return false;

  return true;
}

/**
 * @brief  设置组播禁止本地回环（核心：防止自己收到自己的包）
 * @param[in]  sock  套接字
 * @return  无
 */
static void dc_set_multicast_loop(int sock)
{
  unsigned char loop = 0; /* 0=禁止本地回环，1=允许 */
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
}

static void dc_send_probe(int sock, const char *mcast_ip, int building, int room, int device_type)
{
  dc_conflict_pkt_t tx;
  dc_fill_pkt(&tx, building, room, device_type);

  if (!sat_socket_udp_send(sock, (const char *)&tx, DC_PKT_SIZE, mcast_ip, DC_UDP_PORT, 500))
  {
    SAT_DEBUG("[DC] probe send fail");
  }
}

/**
 * @brief  允许接收同IP设备的组播包（解决IP重复导致收不到冲突探测包的问题）
 * @note   同一局域网内多台设备IP相同时，内核rp_filter会将源IP为本机地址的组播包丢弃。
 *         关闭br0的rp_filter并开启accept_local，确保同IP设备间组播互通。
 */
static int dc_allow_same_ip_multicast(void)
{
  const char *paths_rp[] = {
      "/proc/sys/net/ipv4/conf/br0/rp_filter",
      "/proc/sys/net/ipv4/conf/all/rp_filter",
      "/proc/sys/net/ipv4/conf/lo/rp_filter"};
  const char *path_accept = "/proc/sys/net/ipv4/conf/br0/accept_local";
  FILE *f;
  int ret = 0;

  // 关闭rp_filter
  for (size_t i = 0; i < sizeof(paths_rp) / sizeof(paths_rp[0]); i++)
  {
    f = fopen(paths_rp[i], "w");
    if (!f)
    {
      ret = -1;
      continue;
    }
    fputs("0\n", f);
    fclose(f);
  }

  // 开启接收本地源IP报文
  f = fopen(path_accept, "w");
  if (!f)
    return -1;
  fputs("1\n", f);
  fclose(f);

  return ret;
}

/*==========================================================================
 * 冲突事件统一投递（主动探测 / 被动监听共用，避免重复上报）
 *==========================================================================*/

static pthread_mutex_t s_dc_event_throttle_mx = PTHREAD_MUTEX_INITIALIZER;
static time_t s_dc_event_throttle_wall;

/**
 * @brief  冲突事件通知（防抖动，25秒内只上报一次）
 * @return  无
 */
static void dc_notify_conflict_event_to_ui(void)
{
  pthread_mutex_lock(&s_dc_event_throttle_mx);
  time_t now = time(NULL);
  if (s_dc_event_throttle_wall != 0 && (now - s_dc_event_throttle_wall) < 25)
  {
    pthread_mutex_unlock(&s_dc_event_throttle_mx);
    return;
  }
  s_dc_event_throttle_wall = now;
  pthread_mutex_unlock(&s_dc_event_throttle_mx);

  Amy_device_conflict_event_put();
}

/**
 * @brief  同步执行冲突探测（阻塞直至完成）
 * @param[in]  building      楼栋号
 * @param[in]  room          房间号
 * @param[in]  device_type   设备类型
 * @param[in]  timeout_ms    检测超时时间（ms）
 * @param[out] conflict_out  输出是否检测到冲突
 * @retval true  检测流程执行成功
 * @retval false 检测流程执行失败
 */
static bool conflict_check_sync(int building, int room, int device_type, int timeout_ms, bool *conflict_out)
{
  if (conflict_out == NULL)
    return false;
  *conflict_out = false;

  if (building < 1 || building > 255 || timeout_ms < 100)
    return false;

  char mcast_ip[20];
  dc_build_mcast_ip(building, room, mcast_ip, sizeof(mcast_ip));

  int sock = -1;
  if (!sat_socket_udp_open(&sock, DC_UDP_PORT, false) || sock < 0)
  {
    SAT_ERROR("[DC] udp open failed");
    return false;
  }

  /* 禁止组播本地回环，防止收到自己的包 */
  dc_set_multicast_loop(sock);

  if (!sat_socket_multicast_join_iface(sock, mcast_ip, DC_IFACE))
  {
    SAT_ERROR("[DC] multicast join %s failed", mcast_ip);
    sat_socket_close(sock);
    return false;
  }

  /* 连续发送3次探测包，提高可靠性 */
  for (int k = 0; k < 3; k++)
  {
    dc_send_probe(sock, mcast_ip, building, room, device_type);
    usleep(20 * 1000);
  }

  int remain = timeout_ms;
  while (remain > 0)
  {
    int slice = remain > 200 ? 200 : remain;
    char rbuf[128];
    struct sockaddr_in from;
    int n = sat_socket_udp_receive(sock, rbuf, sizeof(rbuf), &from, slice);
    remain -= slice;

    if (n < (int)DC_PKT_SIZE)
      continue;

    const dc_conflict_pkt_t *rx = (const dc_conflict_pkt_t *)rbuf;
    if (!dc_pkt_valid(rx))
      continue;

    int rb = rx->building;
    int rr = (int)ntohs(rx->room_be);
    int re = rx->device_type;

    /* 栋、房、设备类型完全一致 = 冲突 */
    if (rb != building || rr != room || re != device_type)
      continue;

    *conflict_out = true;
    break;
  }

  sat_socket_close(sock);
  if (*conflict_out)
    dc_notify_conflict_event_to_ui();

  return true;
}

/**
 * @brief  异步检测工作线程
 * @param[in]  arg  线程参数（conflict_work_t 结构体）
 * @return  NULL
 * @note   线程分离模式，自动释放资源
 */
static void *conflict_async_thread(void *arg)
{
  pthread_detach(pthread_self());
  conflict_work_t *w = (conflict_work_t *)arg;
  bool conflict = false;
  conflict_check_sync(w->building, w->room, w->device_type, w->timeout_ms, &conflict);
  free(w);
  return NULL;
}

/**
 * @brief  设备冲突检测对外接口（支持同步/异步）
 * @param[in]  building      楼栋号
 * @param[in]  room          房间号
 * @param[in]  device_type   设备类型
 * @param[in]  timeout_ms    超时时间（ms）
 * @param[in]  is_block      是否阻塞执行
 * @param[out] conflict_out  同步模式下输出冲突结果
 * @retval true  启动/执行成功
 * @retval false 启动/执行失败
 */
bool device_conflict_check(int building, int room, int device_type, int timeout_ms,
                           bool is_block, bool *conflict_out)
{
  if (building < 1 || building > 255 || timeout_ms < 100)
    return false;

  /* 同步阻塞模式 */
  if (is_block)
  {
    if (conflict_out == NULL)
      return false;
    return conflict_check_sync(building, room, device_type, timeout_ms, conflict_out);
  }

  conflict_work_t *w = (conflict_work_t *)malloc(sizeof(conflict_work_t));
  if (w == NULL)
    return false;

  w->building = building;
  w->room = room;
  w->device_type = device_type;
  w->timeout_ms = timeout_ms;

  pthread_t tid;
  if (pthread_create(&tid, sat_pthread_attr_get(), conflict_async_thread, w) != 0)
  {
    SAT_ERROR("[DC] pthread_create failed");
    free(w);
    return false;
  }
  return true;
}

/*==========================================================================
 * 常驻组播监听：空闲时也能收到他机发往「本机栋+房」组播组的探测包
 *==========================================================================*/

static volatile int s_listener_run;                               /**< 监听线程运行标志 */
static pthread_t s_listener_tid;                                  /**< 监听线程ID */
static pthread_mutex_t s_listener_mx = PTHREAD_MUTEX_INITIALIZER; /**< 线程安全互斥锁 */
static int s_listener_sock = -1;                                  /**< 监听socket */
static char s_listener_mcast[20];                                 /**< 当前监听的组播IP */
static char s_listener_if_ip[32];                                 /**< 加入组播时记录的 br0 IP */

/**
 * @brief  关闭监听socket（线程安全，需已加锁）
 * @return  无
 */
static void listener_close_sock_locked(void)
{
  if (s_listener_sock >= 0)
  {
    sat_socket_close(s_listener_sock);
    s_listener_sock = -1;
  }
  s_listener_mcast[0] = '\0';
  s_listener_if_ip[0] = '\0';
}

/**
 * @brief  确保加入正确的组播组（线程安全，需已加锁）
 * @param[in]  building  楼栋号
 * @param[in]  room      房间号
 * @retval true  加入成功
 * @retval false 加入失败
 */
static bool listener_ensure_mcast_join_locked(int building, int room)
{
  char mcast[20];
  char br0_ip[32] = {0};
  dc_build_mcast_ip(building, room, mcast, sizeof(mcast));

  /* 获取当前 br0 IP：socket 的 IP_MULTICAST_IF 绑定的是加入时的地址，
   * IP 变更后需重建 socket，否则 sendto 会因源地址失效而失败 */
  sat_ip_mac_addres_get(DC_IFACE, br0_ip, NULL, NULL);

  /* 组播组和 br0 IP 均未变化，直接返回 */
  if (s_listener_sock >= 0 && strcmp(mcast, s_listener_mcast) == 0 &&
      s_listener_if_ip[0] != '\0' && br0_ip[0] != '\0' &&
      strcmp(br0_ip, s_listener_if_ip) == 0)
    return true;

  /* 组播组或 br0 IP 变化，重建socket */
  listener_close_sock_locked();

  if (!sat_socket_udp_open(&s_listener_sock, DC_UDP_PORT, false) || s_listener_sock < 0)
  {
    SAT_ERROR("[DC] listener udp open failed");
    return false;
  }

  /* 禁止组播本地回环 */
  dc_set_multicast_loop(s_listener_sock);

  if (!sat_socket_multicast_join_iface(s_listener_sock, mcast, DC_IFACE))
  {
    SAT_ERROR("[DC] listener multicast join %s failed", mcast);
    sat_socket_close(s_listener_sock);
    s_listener_sock = -1;
    return false;
  }

  strncpy(s_listener_mcast, mcast, sizeof(s_listener_mcast) - 1);
  s_listener_mcast[sizeof(s_listener_mcast) - 1] = '\0';
  strncpy(s_listener_if_ip, br0_ip, sizeof(s_listener_if_ip) - 1);
  s_listener_if_ip[sizeof(s_listener_if_ip) - 1] = '\0';
  return true;
}

/**
 * @brief  冲突检测常驻监听线程
 * @param[in]  arg  线程参数
 * @return  NULL
 * @note   实时监听同栋同房设备的探测包，发现冲突立即上报
 */
static void *conflict_listener_thread(void *arg)
{
  (void)arg;
  pthread_detach(pthread_self());

  uint64_t last_probe_tick = 0;

  while (s_listener_run)
  {
    /* 每次循环刷新栋/房/设备号，设备号/房号变更（不重启）后探测使用新身份 */
    int b = user_data_get()->private_cfg.device.attr.building;
    int r = user_data_get()->private_cfg.device.attr.room;
    int e = user_data_get()->private_cfg.device.attr.device_type;

    /* 无效楼栋，关闭socket并等待 */
    if (b < 1 || b > 255)
    {
      pthread_mutex_lock(&s_listener_mx);
      listener_close_sock_locked();
      pthread_mutex_unlock(&s_listener_mx);
      usleep(500 * 1000);
      continue;
    }

    /* 确保加入正确组播 */
    pthread_mutex_lock(&s_listener_mx);
    if (!listener_ensure_mcast_join_locked(b, r))
    {
      pthread_mutex_unlock(&s_listener_mx);
      usleep(1000 * 1000);
      continue;
    }
    int sock = s_listener_sock;
    char mcast[sizeof(s_listener_mcast)];
    strncpy(mcast, s_listener_mcast, sizeof(mcast) - 1);
    mcast[sizeof(mcast) - 1] = '\0';
    pthread_mutex_unlock(&s_listener_mx);

    uint64_t now_tick;
    Amy_timestamp(now_tick);
    if (last_probe_tick == 0 || now_tick - last_probe_tick >= DC_PROBE_INTERVAL_MS)
    {
      dc_send_probe(sock, mcast, b, r, e);
      last_probe_tick = now_tick;
    }

    /* 阻塞接收数据（400ms超时） */
    char rbuf[128];
    struct sockaddr_in from;
    int n = sat_socket_udp_receive(sock, rbuf, sizeof(rbuf), &from, 400);
    if (n < (int)DC_PKT_SIZE)
      continue;

    const dc_conflict_pkt_t *rx = (const dc_conflict_pkt_t *)rbuf;
    if (!dc_pkt_valid(rx))
      continue;

    int rb = rx->building;
    int rr = (int)ntohs(rx->room_be);
    int re = rx->device_type;

    /* 只处理同栋、同房、同类型设备 */
    if (rb != b || rr != r || re != e)
      continue;

    dc_notify_conflict_event_to_ui();
  }

  /* 线程退出，清理资源 */
  pthread_mutex_lock(&s_listener_mx);
  listener_close_sock_locked();
  pthread_mutex_unlock(&s_listener_mx);
  return NULL;
}

/**
 * @brief  启动设备冲突常驻监听服务
 * @return  无
 * @note   全局只允许启动一次
 */
void device_conflict_listener_start(void)
{
  static bool s_once;
  if (s_once)
    return;
  s_once = true;

  s_listener_run = 1;

  /* 解决同IP设备间组播被内核rp_filter丢弃的问题 */
  dc_allow_same_ip_multicast();

  if (pthread_create(&s_listener_tid, sat_pthread_attr_get(), conflict_listener_thread, NULL) != 0)
  {
    SAT_ERROR("[DC] listener pthread_create failed");
    s_listener_run = 0;
    s_once = false;
    return;
  }
}