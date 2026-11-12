#ifndef __byte_array_buf_H__
#define __byte_array_buf_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pthread.h>
#include <stdint.h>

typedef struct {
    pthread_mutex_t       lock;
    pthread_mutexattr_t   attr;
    char                  *buf;
    uint8_t               need_lock;
    uint8_t               need_free;
    int                   max_size;
    int                   read_idx;
    int                   write_idx;
} byte_array_buf_t;

int byte_array_buf_clear(byte_array_buf_t* pnbuf);
int byte_array_buf_data_len(byte_array_buf_t* pnbuf);
int byte_array_buf_read(byte_array_buf_t* pnbuf, void* data, int len);
int byte_array_buf_write(byte_array_buf_t* pnbuf, void* data, int len);
int byte_array_buf_write_str(byte_array_buf_t* pnbuf, char* data);
int byte_array_buf_write_byte(byte_array_buf_t* pnbuf, uint8_t data);
int byte_array_buf_write_uint16_be(byte_array_buf_t* pnbuf, uint16_t data);
int byte_array_buf_write_uint32(byte_array_buf_t* pnbuf, uint32_t data);
int byte_array_buf_init(byte_array_buf_t* pnbuf, char* buf, int size, int need_lock);
int byte_array_buf_destroy(byte_array_buf_t* pnbuf);
int byte_array_buf_get(byte_array_buf_t* pnbuf, void* data, int len);
int byte_array_buf_write_ptr_lock(byte_array_buf_t* pnbuf, char** ptr);
int byte_array_buf_write_ptr_unlock(byte_array_buf_t* pnbuf, int advance_len);
int byte_array_buf_read_ptr_lock(byte_array_buf_t* pnbuf, char** ptr);
int byte_array_buf_read_ptr_unlock(byte_array_buf_t* pnbuf, int advance_len);
int byte_array_buf_trim(byte_array_buf_t* pnbuf);
int byte_array_buf_ridx_advance(byte_array_buf_t* pnbuf, int idx);
int byte_array_buf_widx_advance(byte_array_buf_t* pnbuf, int idx);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
