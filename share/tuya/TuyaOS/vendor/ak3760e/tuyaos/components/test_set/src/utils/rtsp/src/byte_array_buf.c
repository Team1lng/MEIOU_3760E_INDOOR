#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "byte_array_buf.h"

static inline void __advance_read_idx(byte_array_buf_t* pnbuf, int n)
{
    pnbuf->read_idx += n;
    if (pnbuf->read_idx == pnbuf->write_idx)
    {
        pnbuf->read_idx = 0;
        pnbuf->write_idx = 0;
    }
}

int byte_array_buf_init(byte_array_buf_t* pnbuf, char* buf, int size, int need_lock)
{
    memset(pnbuf, 0, sizeof(byte_array_buf_t));
    pnbuf->max_size = size;
    pnbuf->need_lock = need_lock;

    if (!buf){
        pnbuf->need_free = 1;
        pnbuf->buf = malloc(size);
        if (!pnbuf->buf){
            pnbuf->max_size = 0;
        }
    }
    else {
        pnbuf->buf = buf;
    }

    if (need_lock){
        pthread_mutexattr_init(&pnbuf->attr);
        pthread_mutexattr_settype(&pnbuf->attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&pnbuf->lock, &pnbuf->attr);
    }

    return 0;
}

int byte_array_buf_destroy(byte_array_buf_t* pnbuf)
{
    if (pnbuf->need_free && pnbuf->buf){
        free(pnbuf->buf);
        pnbuf->buf = NULL;
    }

    if (pnbuf->need_lock){
        pthread_mutexattr_destroy(&pnbuf->attr);
        pthread_mutex_destroy(&pnbuf->lock);
    }

    return 0;
}

/*
 * 将数据写入buf，将write_idx增加len个字节
 * data: 数据指针
 * len:  数据长度
 * 返回实际写入的数据长度
 */
int byte_array_buf_write(byte_array_buf_t* pnbuf, void* data, int len)
{
    if (pnbuf->need_lock){
        pthread_mutex_lock(&pnbuf->lock);
    }

    int bytes_avail = pnbuf->max_size - pnbuf->write_idx;
    if (len > bytes_avail) {
        len = bytes_avail;
    }

    if (len > 0)
    {
        memcpy(pnbuf->buf + pnbuf->write_idx, data, len);
        pnbuf->write_idx += len;
    }

    if (pnbuf->need_lock){
        pthread_mutex_unlock(&pnbuf->lock);
    }

    return len;
}

/*
 * 从buf中读取数据，将read_idx增加读取的字节数
 * data: 存放数据指针
 * len:  要读取的字节数
 * 返回实际读取的字节数
 */
int byte_array_buf_read(byte_array_buf_t* pnbuf, void* data, int len)
{
    int bytes_avail = byte_array_buf_data_len(pnbuf);
    if (len > bytes_avail){
        len = bytes_avail;
    }

    if (len > 0)
    {
        if (pnbuf->need_lock)
            pthread_mutex_lock(&pnbuf->lock);

        memcpy(data, pnbuf->buf + pnbuf->read_idx, len);

        __advance_read_idx(pnbuf, len);

        if (pnbuf->need_lock)
            pthread_mutex_unlock(&pnbuf->lock);
    }

    return len;
}

/*
 * buf中数据的字节数
 */
int byte_array_buf_data_len(byte_array_buf_t* pnbuf)
{
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);

    int len = pnbuf->write_idx - pnbuf->read_idx;

    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return len;
}

/*
 * 从buf中读取数据，但不更新read_idx
 * data: 存放数据的指针
 * len:  要读取的字节数
 * 返回实际读取的字节数
 */
int byte_array_buf_get(byte_array_buf_t* pnbuf, void* data, int len)
{
    int bytes_avail = byte_array_buf_data_len(pnbuf);
    if (len > bytes_avail){
        len = bytes_avail;
    }

    if (len > 0)
    {
        if (pnbuf->need_lock)
            pthread_mutex_lock(&pnbuf->lock);
        memcpy(data, pnbuf->buf + pnbuf->read_idx, len);
        if (pnbuf->need_lock)
            pthread_mutex_unlock(&pnbuf->lock);
    }

    return len;
}


/*
 * 获取buf中write_idx位置对应的地址，并对buf加锁
 * 返回可写入的字节数
 */
int byte_array_buf_write_ptr_lock(byte_array_buf_t* pnbuf, char** ptr){
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);

    int avail = pnbuf->max_size - pnbuf->write_idx;
    *ptr = pnbuf->buf + pnbuf->write_idx;

    return avail;
}

/*
 * 将write_idx增加advance_len个字节，并将buf解锁
 */
int byte_array_buf_write_ptr_unlock(byte_array_buf_t* pnbuf, int advance_len)
{
    int ret = 0;
    if (pnbuf->write_idx + advance_len > pnbuf->max_size){
        printf("advance error: write_idx(%d) len(%d) max_size(%d)\n", pnbuf->write_idx, advance_len, pnbuf->max_size);
        ret = -1;
    }else{
        pnbuf->write_idx += advance_len;
    }

    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return ret;
}

/*
 * 获取buf中read_idx对应位置的地址，并将buf加锁
 * 返回可读取的字节数
 */
int byte_array_buf_read_ptr_lock(byte_array_buf_t* pnbuf, char** ptr){
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);

    int avail = pnbuf->write_idx - pnbuf->read_idx;
    *ptr = pnbuf->buf + pnbuf->read_idx;

    return avail;
}

/*
 * 将read_idx增加advance_len个字节，并将buf解锁
 */
int byte_array_buf_read_ptr_unlock(byte_array_buf_t* pnbuf, int advance_len)
{
    int ret = 0;
    if (pnbuf->read_idx + advance_len > pnbuf->write_idx){
        printf("advance error: write_idx(%d) len(%d) read_idx(%d)\n", pnbuf->write_idx, advance_len, pnbuf->read_idx);
        ret = -1;
    }else{
        __advance_read_idx(pnbuf, advance_len);
    }

    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return ret;
}

/*
 * 将buf中的数据移动到头部
 */
int byte_array_buf_trim(byte_array_buf_t* pnbuf)
{
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);
    int len = byte_array_buf_data_len(pnbuf);
    if (pnbuf->read_idx > 0 && len > 0){
        memmove(pnbuf->buf, pnbuf->buf + pnbuf->read_idx, len);
        pnbuf->read_idx = 0;
        pnbuf->write_idx = len;
    }
    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return 0;
}

/*
 * 将read_idx增加idx个字节
 */
int byte_array_buf_ridx_advance(byte_array_buf_t* pnbuf, int idx)
{
    int ret = 0;
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);
    if (pnbuf->read_idx + idx > pnbuf->write_idx){
        printf("advance error: read_idx(%d) advance(%d) write_idx(%d)\n", pnbuf->read_idx, idx, pnbuf->write_idx);
        ret = -1;
    }else{
        __advance_read_idx(pnbuf, idx);
    }
    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return ret;
}

/*
 * 将write_idx增加idx个字节
 */
int byte_array_buf_widx_advance(byte_array_buf_t* pnbuf, int idx)
{
    int ret = 0;
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);
    if (pnbuf->write_idx + idx > pnbuf->max_size){
        printf("advance error: max_size(%d) advance(%d) write_idx(%d)\n", pnbuf->max_size, idx, pnbuf->write_idx);
        ret = -1;
    }else{
        pnbuf->write_idx += idx;
    }
    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return ret;
}

int byte_array_buf_clear(byte_array_buf_t* pnbuf)
{
    if (pnbuf->need_lock)
        pthread_mutex_lock(&pnbuf->lock);

    pnbuf->read_idx = 0;
    pnbuf->write_idx = 0;

    if (pnbuf->need_lock)
        pthread_mutex_unlock(&pnbuf->lock);

    return 0;
}

/*写入的数据不包括\0*/
int byte_array_buf_write_str(byte_array_buf_t* pnbuf, char* data)
{
    int len = strlen(data);
    return byte_array_buf_write(pnbuf, data, len);
}

int byte_array_buf_write_byte(byte_array_buf_t* pnbuf, uint8_t data)
{
    return byte_array_buf_write(pnbuf, &data, 1);
}
int byte_array_buf_write_uint16_be(byte_array_buf_t* pnbuf, uint16_t data)
{
    data = htons(data);
    return byte_array_buf_write(pnbuf, &data, 2);
}

int byte_array_buf_write_uint32(byte_array_buf_t* pnbuf, uint32_t data)
{
    return byte_array_buf_write(pnbuf, &data, 4);
}