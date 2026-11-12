#ifndef SAT_IPCAMERA_INTERNAL_H
#define SAT_IPCAMERA_INTERNAL_H

#include "sat_ipcamera.h"
#include <pthread.h>
#include <stdbool.h>

/*==========================================================================
 * 宏定义
 *==========================================================================*/
#define AUTH_FLAG_NONE 0x00    // 无认证
#define AUTH_FLAG_DIGEST 0x01  // Digest认证
#define INDEX_ALL_DEVICES 0xFF // 所有设备索引
#define TIMEOUT_SHORT 1500     // 短超时(ms)
#define TIMEOUT_LONG 5000      // 长超时(ms)
#define EVENT_TYPE_SEARCH_DONE IPCAMERA_EVENT_TYPE_SEARCH_DONE
#define EVENT_TYPE_RTSP_DONE IPCAMERA_EVENT_TYPE_RTSP_DONE
#define EVENT_TYPE_DOOR_REGISTER_DONE IPCAMERA_EVENT_TYPE_DOOR_REGISTER_DONE

/*==========================================================================
 * 全局变量声明（定义在 sat_ipcamera_discover.c）
 * 仅供 discover.c 内部使用，ctrl.c 不得包含此头文件
 *==========================================================================*/
extern struct ipcamera_info ipc_device[IPCAMERA_NUM_MAX];
extern int ipcamera_online_num;
extern bool ipc_search_in_progress;
extern pthread_mutex_t ipc_mutex;

/*==========================================================================
 * 内部工具函数声明（定义在 sat_ipcamera_discover.c）
 *==========================================================================*/
void safe_strncpy(char *dest, const char *src, size_t dest_size);

#endif /* SAT_IPCAMERA_INTERNAL_H */