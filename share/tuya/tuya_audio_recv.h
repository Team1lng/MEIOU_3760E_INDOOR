#ifndef TUYA_AUDIO_RECV_H
#define TUYA_AUDIO_RECV_H
//注意：考虑到是单一音频队列，句柄由内部自动管理
#include <stdint.h>
#include "mempool/mempool.h"
#include "ringbuffer/ringbuffer.h"
#include "Amy_sdk.h"
// 音频帧大小定义（根据实际情况调整）
#define AUDIO_FRAME_SIZE 320

// 初始化音频接收环形缓冲区
// 返回值：0 - 成功，-1 - 失败
int tuya_recv_audio_ringbuffer_init(void);

// 销毁音频接收环形缓冲区
// 返回值：0 - 成功，-1 - 失败
int tuya_recv_audio_ringbuffer_destroy(void);

// 获取音频缓冲区
// 参数：size - 缓冲区大小
// 返回值：成功返回缓冲区指针，失败返回NULL
stream_data* tuya_recv_audio_buffer_get(int size);

// 释放音频缓冲区
// 参数：ast - 要释放的缓冲区指针
void tuya_recv_audio_buffer_release(stream_data* ast);

// 写入音频帧数据
// 参数：data - 音频数据指针，size - 数据大小
void tuya_recv_audio_frame_write(unsigned char* data, int size);

// 读取音频帧数据
// 参数：ast - 用于存储读取数据的缓冲区
// 返回值：成功返回读取的字节数，失败返回-1
int tuya_recv_audio_frame_get(stream_data* ast);

// 清空缓冲队列的函数
void tuya_recv_audio_ringbuffer_clear(void);
#endif // TUYA_AUDIO_RECV_H