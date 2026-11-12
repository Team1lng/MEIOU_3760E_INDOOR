#include "user_upgrade.h"
#include "Amy_sdk.h"
#include "sat_user_common.h"
#include "Amy_lvgl.h"
#include "Amy_tfcard.h"
#include "user_data.h"
#define MULTICAST_IP "224.0.0.1"
#define MULTICAST_PORT 5000
#define UPGRADE_TCP_PORT 5001
#define NETWORK_PACKET_MAX (100 * 1024)

enum
{
  NETWORK_CMD_DEVICE_UPGRADE,
  NETWORK_CMD_TOTAL
};

/* =========================== 上下文结构体 =========================== */
typedef struct
{
  pthread_mutex_t mutex;
  bool running;
  int multicast_socket;
  int server_fd;
} outdoor_upgrade_ctx_t;

/* =========================== 唯一全局变量 =========================== */
static outdoor_upgrade_ctx_t g_upgrade_ctx = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .running = false,
    .multicast_socket = -1,
    .server_fd = -1,
};

/* =========================== 内部函数 =========================== */

/**
 * @brief 固件文件传输
 */
static bool upgrade_firmware_transfer(int socket, const char *upgrade_path)
{
  FILE *fp = fopen(upgrade_path, "rb");
  if (!fp)
  {
    SAT_ERROR(" Open firmware file failed: %s", upgrade_path);
    return false;
  }

  int firmware_size = sat_file_size_get(upgrade_path);
  if (firmware_size <= 0)
  {
    SAT_ERROR(" Invalid firmware size: %d", firmware_size);
    fclose(fp);
    return false;
  }

  unsigned char *packet = malloc(NETWORK_PACKET_MAX);
  if (!packet)
  {
    SAT_ERROR(" Malloc packet buffer failed");
    fclose(fp);
    return false;
  }

  int len = 0;
  int read_size = 0;
  bool success = true;

  while ((len = fread(packet, 1, NETWORK_PACKET_MAX, fp)) > 0)
  {
    if (!sat_socket_tcp_send(socket, packet, len, 1000))
    {
      SAT_ERROR(" TCP send failed at offset %d", read_size);
      success = false;
      break;
    }
    read_size += len;
    SAT_INFO(" Send progress: %.1f%%",
             read_size * 100.0f / firmware_size);
    Amy_update_door_event_put(DOOR_UPGRADE_PROGRESS,
                              (int)((long long)read_size * 100 / firmware_size));
  }

  free(packet);
  fclose(fp);

  if (success && read_size != firmware_size)
  {
    SAT_ERROR(" Send incomplete: %d / %d bytes",
              read_size, firmware_size);
    return false;
  }

  return success;
}

/**
 * @brief 等待门口机回连并逐一升级（升级所有响应门机）
 * @return -1-无门机响应；0-全部成功；>0-失败台数
 */
static int upgrade_outdoor_device(const char *upgrade_path)
{
  int failed_count = 0;
  int connected = 0;
  char file_md5[33] = {0};

  if (!file_md5_get(upgrade_path, file_md5))
  {
    SAT_ERROR("calc file md5 failed");
    return -1;
  }
  SAT_INFO("firmware md5: %s", file_md5);

  if (!sat_socket_tcp_open(&g_upgrade_ctx.server_fd, UPGRADE_TCP_PORT, 1))
  {
    SAT_ERROR(" TCP server open failed");
    return -1;
  }

  /* ===================== 组播指令增加 MD5 ===================== */
  char cmd[128] = {0};
  snprintf(cmd, sizeof(cmd), "%d %d %s",
           NETWORK_CMD_DEVICE_UPGRADE,
           UPGRADE_TCP_PORT,
           file_md5); // 把MD5发给设备

  SAT_INFO(" Sending multicast upgrade command...");

  if (!sat_socket_udp_send(g_upgrade_ctx.multicast_socket,
                           cmd, strlen(cmd),
                           MULTICAST_IP, MULTICAST_PORT, 500))
  {
    SAT_ERROR(" Multicast send failed");
    goto cleanup;
  }

  /* 轮询等待门口机回连，逐一升级 */
  for (int i = 0; i < DOOR_DEVICE_MAX; i++)
  {
    struct sockaddr_in client_addr = {0};
    char client_ip[32] = {0};

    int client_fd = sat_socket_tcp_accept(
        g_upgrade_ctx.server_fd, &client_addr, 3000);
    if (client_fd < 0)
    {
      SAT_INFO(" No more devices connected (i=%d)", i);
      break;
    }

    int recv_len = sat_socket_tcp_receive(
        client_fd,
        (unsigned char *)client_ip,
        sizeof(client_ip) - 1,
        500);
    if (recv_len <= 0)
    {
      SAT_WARN(" Receive device IP failed (i=%d)", i);
      sat_socket_close(client_fd);
      continue;
    }

    connected++;
    if (connected == 1)
    {
      SAT_INFO(" First door responded: %s", client_ip);
      Amy_update_door_event_put(DOOR_UPGRADE_ONLINE, 0);
    }

    SAT_INFO(" Start upgrading device: %s", client_ip);
    bool success = upgrade_firmware_transfer(client_fd, upgrade_path);

    if (success)
    {
      /* 等待设备端处理完毕（对端关闭连接） */
      char ack[32] = {0};
      while (sat_socket_tcp_receive(client_fd,
                                    (unsigned char *)ack,
                                    sizeof(ack), 500) > 0)
      {
        usleep(1000);
      }
      SAT_INFO(" Upgrade device %s success", client_ip);
    }
    else
    {
      failed_count++;
      SAT_ERROR(" Upgrade device %s failed", client_ip);
    }

    sat_socket_close(client_fd);
  }

cleanup:
  sat_socket_close(g_upgrade_ctx.server_fd);
  g_upgrade_ctx.server_fd = -1;

  if (connected == 0)
    return -1;
  return failed_count;
}

/**
 * @brief 升级任务线程入口（单次尝试，重试由 UI 层编排）
 */
static void *outdoor_upgrade_task(void *arg)
{
  const char *upgrade_path = TFCARD_MOUNT_PATH "/SAT_ANYKA3918OS";
  int result = DOOR_UPGRADE_FAILED;

  /* 打开组播 socket */
  if (!sat_socket_udp_open(&g_upgrade_ctx.multicast_socket,
                           MULTICAST_PORT, false))
  {
    SAT_ERROR(" UDP open failed");
    goto task_exit;
  }

  if (!sat_socket_multicast_join(g_upgrade_ctx.multicast_socket,
                                 MULTICAST_IP))
  {
    SAT_ERROR(" Multicast join failed");
    goto task_exit;
  }

  SAT_INFO(" Multicast joined %s:%d", MULTICAST_IP, MULTICAST_PORT);

  int status = upgrade_outdoor_device(upgrade_path);
  if (status < 0)
    result = DOOR_UPGRADE_OFFLINE;
  else if (status == 0)
    result = DOOR_UPGRADE_SUCCESS;
  else
    result = DOOR_UPGRADE_FAILED;

task_exit:
  if (g_upgrade_ctx.multicast_socket >= 0)
  {
    sat_socket_close(g_upgrade_ctx.multicast_socket);
    g_upgrade_ctx.multicast_socket = -1;
  }
  if (g_upgrade_ctx.server_fd >= 0)
  {
    sat_socket_close(g_upgrade_ctx.server_fd);
    g_upgrade_ctx.server_fd = -1;
  }

  /* 先置 running=false 再投递最终事件，保证 UI 收到 FAILED 后
     能立即重调 outdoor_network_upgrade() 而不被 is_running 拦截 */
  pthread_mutex_lock(&g_upgrade_ctx.mutex);
  g_upgrade_ctx.running = false;
  pthread_mutex_unlock(&g_upgrade_ctx.mutex);

  Amy_update_door_event_put(result, 0);

  SAT_INFO(" Upgrade task exit");
  return NULL;
}

/* =========================== 对外接口 =========================== */

/**
 * @brief 启动门口机网络升级（升级所有响应的门口机）
 * @return true-任务启动成功，false-已有任务在运行
 */
bool outdoor_network_upgrade(void)
{
  pthread_mutex_lock(&g_upgrade_ctx.mutex);

  if (g_upgrade_ctx.running)
  {
    SAT_WARN(" Upgrade task already running");
    pthread_mutex_unlock(&g_upgrade_ctx.mutex);
    return false;
  }

  g_upgrade_ctx.running = true;
  g_upgrade_ctx.multicast_socket = -1;
  g_upgrade_ctx.server_fd = -1;

  pthread_t tid;
  if (pthread_create(&tid, sat_pthread_attr_get(),
                     outdoor_upgrade_task, NULL) != 0)
  {
    SAT_ERROR(" Create upgrade thread failed");
    g_upgrade_ctx.running = false;
    pthread_mutex_unlock(&g_upgrade_ctx.mutex);
    return false;
  }
  pthread_detach(tid);

  pthread_mutex_unlock(&g_upgrade_ctx.mutex);
  SAT_INFO(" Upgrade task started");
  return true;
}

/**
 * @brief 查询升级任务是否正在运行
 */
bool outdoor_network_upgrade_is_running(void)
{
  pthread_mutex_lock(&g_upgrade_ctx.mutex);
  bool status = g_upgrade_ctx.running;
  pthread_mutex_unlock(&g_upgrade_ctx.mutex);
  return status;
}