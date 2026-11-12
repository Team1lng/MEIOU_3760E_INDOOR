
#include "utilities/uni_log.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_iot_config.h"
#include "ty_dp_define.h"
#include "ty_sdk_common.h"
#include "tuya_svc_upgrade.h"
#include "utilities/uni_log.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include "Amy_lvgl.h"
#include "Amy_sdk.h"
#include "sat_user_common.h"
#include "Amy_tfcard.h"
#include <strings.h>
#define OTA_TMP_FILE TFCARD_MOUNT_PATH "/ota_fw.bin"
#define DOOR_FW_IMG TFCARD_MOUNT_PATH "/SAT_ANYKA3918OS"
#define INDOOR_FW_IMG TFCARD_MOUNT_PATH "/SAT_ANYKA.IMG"
static FILE *g_ota_fp = NULL;
static unsigned int g_ota_total = 0;
static unsigned int g_ota_offset = 0;
static unsigned char g_ota_process = 0;
static int upgrade_data_cb(const FW_UG_S *fw, const unsigned int total, const unsigned int offset,
                           const unsigned char *data, const unsigned int len, unsigned int *remain_len, void *pri_data)
{
  int dev_type = fw->tp == DEV_ATTACH_MOD_1 ? OTA_DEV_TP_OUTDOOR : OTA_DEV_TP_INDOOR;
  if (offset == 0)
  {
    if (access(TFCARD_MOUNT_PATH, F_OK) != 0)
    {
      AMY_LOG_ERROR("ota: sd card not mounted\n");
      Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
      return -1;
    }
    g_ota_fp = fopen(OTA_TMP_FILE, "wb");
    if (!g_ota_fp)
    {
      AMY_LOG_ERROR("ota: create tmp file failed\n");
      Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
      return -1;
    }
    g_ota_total = total;
    g_ota_offset = 0;
    g_ota_process = 0;
  }

  if (!g_ota_fp)
    return -1;

  if (data && len > 0)
  {
    fwrite(data, 1, len, g_ota_fp);
  }
  if (g_ota_offset == 0)
    Amy_ota_event_put(OTA_STATE_DOWNLOAD_START, 0, dev_type);
  g_ota_offset += len;

  if (g_ota_total > 0)
  {
    int prog = (int)((unsigned long long)g_ota_offset * 100 / g_ota_total);
    /* 下载过快时按 1% 推进会瞬间灌满深度为 8 的事件队列，
       限制进度条至少前进 5% 才推送一次事件（100% 必须推送） */
    if (prog == 100 || (prog - g_ota_process) >= 5)
    {
      g_ota_process = prog;
      Amy_ota_event_put(OTA_STATE_DOWNLOADING, prog, dev_type);
    }

    tuya_ipc_upgrade_progress_report_by_type(prog, fw->tp);
  }

  return 0;
}

static int upgrade_notify_cb(const FW_UG_S *fw, const int download_reslut, void *priv_data)
{
  if (g_ota_fp)
  {
    fclose(g_ota_fp);
    g_ota_fp = NULL;
  }
  int dev_type = fw->tp == DEV_ATTACH_MOD_1 ? OTA_DEV_TP_OUTDOOR : OTA_DEV_TP_INDOOR;
  if (download_reslut != 0)
  {
    AMY_LOG_ERROR("ota: download failed, result=%d\n", download_reslut);
    Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
    unlink(OTA_TMP_FILE);
    return -1;
  }

  char file_md5[33] = {0};
  if (!file_md5_get(OTA_TMP_FILE, file_md5))
  {
    AMY_LOG_ERROR("ota: calc file md5 failed\n");
    Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
    tuya_ipc_upgrade_progress_status(NULL, fw->tp, TUS_UPGRD_EXEC);
    unlink(OTA_TMP_FILE);
    return -1;
  }

  if (strcasecmp(file_md5, fw->fw_md5) != 0)
  {
    AMY_LOG_ERROR("ota: md5 mismatch, local=%s, cloud=%s\n", file_md5, fw->fw_md5);
    Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
    tuya_ipc_upgrade_progress_status(NULL, fw->tp, TUS_DOWNLOAD_ERROR_HMAC);
    unlink(OTA_TMP_FILE);
    return -1;
  }

  AMY_LOG_INFO("ota: md5 verify pass, %s\n", file_md5);

  const char *target_img;
  if (fw->tp == DEV_ATTACH_MOD_1)
  {
    target_img = DOOR_FW_IMG;
    AMY_LOG_INFO("ota: door unit firmware\n");
  }
  else
  {
    target_img = INDOOR_FW_IMG;
    AMY_LOG_INFO("ota: indoor unit firmware\n");
  }

  if (rename(OTA_TMP_FILE, target_img) != 0)
  {
    AMY_LOG_ERROR("ota: rename to %s failed\n", target_img);
    Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
    tuya_ipc_upgrade_progress_status(NULL, fw->tp, TUS_UPGRD_EXEC);
    unlink(OTA_TMP_FILE);
    return -1;
  }

  Amy_ota_event_put(OTA_STATE_DOWNLOADED, 100, dev_type);
  tuya_ipc_upgrade_progress_status(NULL, fw->tp, TUS_UPGRD_FINI);
  AMY_LOG_INFO("ota: download success!!!\n");

  return 0;
}

int ipc_ugprade_cb(const FW_UG_S *fw)
{
  AMY_LOG_INFO("Rev upgrade info:\n uri=%s\nver:%s\nsize:%u\n", fw->fw_url, fw->sw_ver, fw->file_size);

  if (access(TFCARD_MOUNT_PATH, F_OK) != 0)
  {
    AMY_LOG_ERROR("ota: reject, sd card not mounted\n");
    int dev_type = fw->tp == DEV_ATTACH_MOD_1 ? OTA_DEV_TP_OUTDOOR : OTA_DEV_TP_INDOOR;
    Amy_ota_event_put(OTA_STATE_FAILED, 0, dev_type);
    tuya_ipc_upgrade_progress_status(NULL, fw->tp, TUS_UPGRD_EXEC);
    return -1;
  }

  tuya_ipc_upgrade_sdk(fw, upgrade_data_cb, upgrade_notify_cb, NULL);
  return 0;
}
