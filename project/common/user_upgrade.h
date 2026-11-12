#ifndef _USER_UGPRADE_H_
#define _USER_UGPRADE_H_
#include <stdbool.h>

typedef enum
{
  DOOR_UPGRADE_OFFLINE,   /* 无任何门口机响应 */
  DOOR_UPGRADE_ONLINE,    /* 有门口机响应，开始升级 */
  DOOR_UPGRADE_PROGRESS,  /* 升级进度更新 */
  DOOR_UPGRADE_SUCCESS,   /* 全部升级成功 */
  DOOR_UPGRADE_FAILED,    /* 有门机传输失败 */
} user_upgrade_state;

bool outdoor_network_upgrade(void);

bool outdoor_network_upgrade_is_running(void);
#endif
