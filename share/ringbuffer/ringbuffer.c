#include "ringbuffer.h"
#include "common/Amy_log.h"
#include "common/Amy_sdk.h"

#define RINGBUFFER_TEST

typedef struct ringbuffer_context
{
    unsigned int head;
    unsigned int tail;
    char *data;
    unsigned int avild_length;
    unsigned int max_length;

    Amy_mutex_t mutex;
} ringbuffer_context;

ringbuffer_context *Amy_ringbuffer_open(unsigned int max_length)
{
    if (max_length < 2)
    {
        AMY_LOG_ERROR("max_legnth small\n");
        return NULL;
    }

    char *data = (char *)Amy_malloc(sizeof(struct ringbuffer_context) + max_length);
    if (!data)
    {
        AMY_LOG_ERROR("rt_malloc failed\n");
        return NULL;
    }

    ringbuffer_context *context = (ringbuffer_context *)data;
    context->head = 0;
    context->tail = 0;
    context->avild_length = 0;
    context->max_length = max_length;
    context->data = data + sizeof(struct ringbuffer_context);
    Amy_mutex_init(&context->mutex, NULL);
    return (void *)context;
}

int Amy_ringbuffer_read(ringbuffer_context *d, char *data, int size)
{
    Amy_mutex_lock(&d->mutex);
    if (size > d->avild_length)
    {
        Amy_mutex_unlock(&d->mutex);
        return 0;
    }

    /*                              read length
     *  |                |---------------|
     *  |---------|------|--------|
     *  |       tail    head     max
     */
    if (size > (d->max_length - d->head))
    {
        unsigned int read_length_a = d->max_length - d->head;
        unsigned int read_legnth_b = size - read_length_a;
        memcpy(data, d->data + d->head, read_length_a);
        memcpy(data + read_length_a, d->data, read_legnth_b);
        d->avild_length -= size;
        d->head = read_legnth_b;
    }
    else
    {
        memcpy(data, d->data + d->head, size);
        d->head += size;
        d->avild_length -= size;
        if (d->head == d->max_length)
        {
            d->head = 0;
        }
    }
    Amy_mutex_unlock(&d->mutex);
    return size;
}

int Amy_ringbuffer_write(ringbuffer_context *d, char *data, int size)
{
    Amy_mutex_lock(&d->mutex);
    if ((d->avild_length + size) > d->max_length)
    {
        AMY_LOG_ERROR("aviled length:%d,write length (%d) more than max length(%d)\n", d->avild_length, size, d->max_length);
        Amy_mutex_unlock(&d->mutex);
        return 0;
    }

    /*                               length
     *  |                |---------------|
     *  |---------|------|--------|
     *  |       head    tail     max
     */
    if ((d->max_length - d->tail) < size)
    {
        unsigned int write_length_a = d->max_length - d->tail;
        unsigned int write_length_b = size - write_length_a;
        if (data)
        {
            memcpy(d->data + d->tail, data, write_length_a);
            memcpy(d->data, data + write_length_a, write_length_b);
        }

        d->tail = write_length_b;
        d->avild_length += size;
    }
    else
    {
        if (data)
        {
            memcpy(d->data + d->tail, data, size);
        }

        d->tail += size;
        d->avild_length += size;
        // 判断tail 是否已经达到了数组尾部
        if (d->tail == d->max_length)
        {
            d->tail = 0;
        }
    }
    Amy_mutex_unlock(&d->mutex);
    return size;
}

int Amy_ringbuffer_close(ringbuffer_context *d)
{
    Amy_mutex_destroy(&d->mutex);
    Amy_free(d);
    return 0;
}

int Amy_ringbuffer_skip_byte(ringbuffer_context *d, unsigned int size)
{
    int ret = size;
    Amy_mutex_lock(&d->mutex);
    if (d->avild_length < size)
    {
        ret = 0;
        AMY_LOG_ERROR("delete length(%d) more than vailed length (%d)  \n", size, d->avild_length);
    }
    else
    {
        /*                           length
         *  |            |---------------|
         *  |------------|--------|
         *  |          head      max
         */
        if ((d->head + size) > d->max_length)
        {
            d->head = size - (d->max_length - d->head);
        }
        else
        { // 正常删除部分
            d->head += size;
        }
        d->avild_length -= size;
    }
    Amy_mutex_unlock(&d->mutex);
    return ret;
}

int Amy_ringbuffer_flush(ringbuffer_context *d)
{
    Amy_mutex_lock(&d->mutex);
    d->head = 0;
    d->tail = 0;
    d->avild_length = 0;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

unsigned int Amy_ringbuffer_used_byte(ringbuffer_context *d)
{
    return d->avild_length;
}

unsigned int Amy_ringbuffer_max_byte(ringbuffer_context *d)
{
    return d->max_length;
}

unsigned int Amy_ringbuffer_free_byte(ringbuffer_context *d)
{
    unsigned int free_length = 0;
    Amy_mutex_lock(&d->mutex);
    free_length = d->max_length - d->avild_length;
    Amy_mutex_unlock(&d->mutex);
    return free_length;
}