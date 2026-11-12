#ifndef _AMY_QUEUES_H_
#define _AMY_QUEUES_H_
typedef struct queues_conetext queues_conetext;
// max_elements:队列最大个数
// sort:是否自动将队列排序
// asc_order:1为降序，0:升序
// cmp_func:比较规则
// user_data:释放函数需要的用户数据
// block:是否阻塞运行
queues_conetext* Amy_queues_open(int max_elements, int sort, int asc_order, int (*cmp_func)(void*, void*), int block);

int Amy_queues_put(queues_conetext* context, void* data);

int Amy_queues_get(queues_conetext* context, void** data, int (*find_func)(void*, void*), void* filter);

int Amy_queues_flush(queues_conetext* context, void (*free_func)(void*, void*), void* user_data);

int Amy_queues_close(queues_conetext* context, void (*free_func)(void*, void*), void* user_data);

int Amy_queues_enable(queues_conetext* context, unsigned char enable);

unsigned char Amy_queues_enable_status(queues_conetext* context);

unsigned int Amy_queues_new_els(queues_conetext* context);

unsigned int Amy_queues_max_els(queues_conetext* d);
#endif