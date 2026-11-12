#ifndef _AMY_MEMPOOL_H_
#define _AMY_MEMPOOL_H_
typedef  unsigned long long mem_size_t;

typedef struct mp_pool mp_pool;
mp_pool* Amy_mempool_init(mem_size_t max_mempool_size, mem_size_t mempool_size);
void* Amy_mempool_alloc(mp_pool* mp, mem_size_t wantsize);
int Amy_mempool_free(mp_pool* mp, void* p);
int Amy_mempool_destory(mp_pool* mp);
mem_size_t Amy_total_memory_total(mp_pool* mp);
mem_size_t Amy_used_memory_get(mp_pool* mp);
mem_size_t Amy_prog_memory_get(mp_pool* mp);
#endif
