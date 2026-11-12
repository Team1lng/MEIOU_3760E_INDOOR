#ifndef _AMY_MEDIA_FILE_H_
#define _AMY_MEDIA_FILE_H_
#include "Amy_sdk.h"
#include "queues/queues.h"
#include "mempool/mempool.h"

#define TFCARD_MOUNT_PATH "/mnt/tf"

typedef enum
{
    // 没有检测到存储设备
    SD_STATE_UNPLUG = 0,
    // 检测到对应的快设备，但是没有挂载
    SD_STATE_INSERT = (0x1 << 7),
    // 检测到存储设备,但是没有对应的块设备
    SD_STATE_ERROR = SD_STATE_INSERT | 0x01,
    // 设备已经正常挂载，存储空间在阈值内
    SD_STATE_MOUNT = SD_STATE_INSERT | 0x02,
    // 设备已经正常挂载，但是存储空间已经满了
    SD_STATE_FULL = SD_STATE_INSERT | 0x03,

    SD_STATE_FORMAT = SD_STATE_INSERT | 0x04,

    SD_STATE_TOTAL
} tf_card_status_e;

#define TFCARD_EVENT_FORMAT 0x01

int Amy_tfcard_init(void);
int Amy_tfcard_status(void);
int Amy_tfcard_inserted(void);
/* The UI acknowledges a state event only after its resource cleanup returns. */
bool Amy_tfcard_event_is_current(unsigned int sequence);
void Amy_tfcard_event_processed(unsigned int sequence);
/* Bind asynchronous persistence to one mounted card; begin/end must pair. */
unsigned int Amy_tfcard_generation(void);
bool Amy_tfcard_io_begin(unsigned int generation);
void Amy_tfcard_io_end(void);
bool Amy_tfcard_format_pending(void);
void Amy_tfcard_format_event_processed(void);
int Amy_tfcard_format(void);
int tfcard_capacity_get(unsigned long *total_kb, unsigned long *used_kb, unsigned long *free_kb); // hare set
/**
 * @brief 确保TF卡剩余容量高于总容量指定百分比
 * @param min_free_percent 最小剩余容量百分比（1~99）
 * @return 0=满足要求，-1=失败或无法满足
 */
int Amy_tfcard_ensure_free_percent(unsigned int min_free_percent);
#endif
