#include "tuya_audio_recv.h"
// 音频接收上下文（内部使用，对外不暴露具体结构）
typedef struct
{
    mp_pool *mp;
    ringbuffer_context *aq;
    Amy_mutex_t mutex;
    int is_initialized;
} tuya_recv_context;

// 静态全局变量，内部管理
static tuya_recv_context tuya_audio_dev = {
    .mp = NULL,
    .aq = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .is_initialized = false,
};

// 初始化函数，返回结果
int tuya_recv_audio_ringbuffer_init(void)
{
    tuya_recv_context *d = &tuya_audio_dev;
    // 确保只初始化一次
    Amy_mutex_lock(&d->mutex);
    if (d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        return 0; // 已经初始化，直接返回成功
    }
    // 初始化互斥锁
    if (Amy_mutex_init(&d->mutex, NULL) != 0)
    {
        AMY_LOG_ERROR("mutex init failed\n");
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }

    // 初始化内存池
    d->mp = Amy_mempool_init(10 * (AUDIO_FRAME_SIZE + sizeof(stream_data)),
                             10 * (AUDIO_FRAME_SIZE + sizeof(stream_data)));
    if (d->mp == NULL)
    {
        AMY_LOG_ERROR("mempool init failed\n");
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }

    // 初始化环形缓冲区（0.5秒：16kHz×16bit 单声道 = 32000B/s，50×320=16000B）
    d->aq = Amy_ringbuffer_open(AUDIO_FRAME_SIZE * 50);
    if (!d->aq)
    {
        AMY_LOG_ERROR("ringbuffer open failed\n");
        Amy_mempool_destory(d->mp);
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }

    d->is_initialized = 1;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

// 销毁函数
int tuya_recv_audio_ringbuffer_destroy(void)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        return 0; // 未初始化，直接返回成功
    }

    // 关闭环形缓冲区
    if (d->aq != NULL)
    {
        Amy_ringbuffer_close(d->aq);
        d->aq = NULL;
    }

    // 销毁内存池
    if (d->mp != NULL)
    {
        Amy_mempool_destory(d->mp);
        d->mp = NULL;
    }

    d->is_initialized = 0;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

// 获取缓冲区，不再需要句柄参数
stream_data *tuya_recv_audio_buffer_get(int size)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    // 检查是否已初始化
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("not initialized\n");
        return NULL;
    }
    stream_data *ast = NULL;
    ast = Amy_mempool_alloc(d->mp, sizeof(stream_data) + size);
    if (!ast)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("mempool failed\n");
        return NULL;
    }
    ast->data = (unsigned char *)ast + sizeof(stream_data);
    ast->size = size;
    Amy_mutex_unlock(&d->mutex);
    return ast;
}

// 释放缓冲区，不再需要句柄参数
void tuya_recv_audio_buffer_release(stream_data *ast)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    // 检查是否已初始化
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("not initialized\n");
        return;
    }
    Amy_mempool_free((mp_pool *)d->mp, (void *)ast);
    Amy_mutex_unlock(&d->mutex);
}

// 写入音频帧，不再需要句柄参数
void tuya_recv_audio_frame_write(unsigned char *data, int size)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    // 检查是否已初始化
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("not initialized\n");
        return;
    }

    unsigned int new_els = 0;
    new_els = Amy_ringbuffer_free_byte(d->aq);
    if (new_els < size)
    {
        Amy_ringbuffer_flush(d->aq);
    }
    Amy_ringbuffer_write(d->aq, (char *)data, size);
    Amy_mutex_unlock(&d->mutex);
}

// 获取音频帧，不再需要句柄参数
int tuya_recv_audio_frame_get(stream_data *ast)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    // 检查是否已初始化
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("not initialized\n");
        return -1;
    }

    int ret = 0;
    ret = Amy_ringbuffer_read(d->aq, (char *)ast->data, ast->size);
    Amy_timestamp(ast->timestamp);
    ast->keyframe = 0;
    Amy_mutex_unlock(&d->mutex);
    return ret;
}

// 清空缓冲队列的函数
void tuya_recv_audio_ringbuffer_clear(void)
{
    tuya_recv_context *d = &tuya_audio_dev;
    Amy_mutex_lock(&d->mutex);
    if (!d->is_initialized)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("not initialized\n");
        return;
    }
    Amy_ringbuffer_flush(d->aq); // 清空环形缓冲区
    Amy_mutex_unlock(&d->mutex);
}