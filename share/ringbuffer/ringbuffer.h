#ifndef _AMY_RINGBUFFER_H_
#define _AMY_RINGBUFFER_H_
typedef struct ringbuffer_context ringbuffer_context;

ringbuffer_context* Amy_ringbuffer_open(unsigned int max_length);

int Amy_ringbuffer_read(ringbuffer_context* context, char* data, int size);

int Amy_ringbuffer_write(ringbuffer_context* context, char* data, int size);

int Amy_ringbuffer_skip_byte(ringbuffer_context* context, unsigned int size);

int Amy_ringbuffer_flush(ringbuffer_context* context);

unsigned int Amy_ringbuffer_used_byte(ringbuffer_context* context);

unsigned int Amy_ringbuffer_max_byte(ringbuffer_context* d);

unsigned int Amy_ringbuffer_free_byte(ringbuffer_context* context);

int Amy_ringbuffer_close(ringbuffer_context* context);
#endif
