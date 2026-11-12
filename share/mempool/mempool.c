#include "mempool.h"
#include <stdint.h>
#include "common/Amy_log.h"
#include "common/Amy_sdk.h"

#define PAGE_SIZE (4096)
#define mem_size_t unsigned long long
#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

typedef struct _mp_chunk {
    mem_size_t alloc_mem;
    struct _mp_chunk* prev, * next;
    int is_free;
}mp_chunk;

typedef struct _mp_mempool_list {
    char* start;
    unsigned int id;
    mem_size_t mempool_size;
    mem_size_t alloc_mem;          // 统计值 当前池内已分配的内存总大小
    mem_size_t alloc_prog_mem;     // 统计值 当前池内实际分配给应用程序的内存总大小(减去内存管理元信息)
    mp_chunk* free_list, * alloc_list;
    struct _mp_mempool_list* next;
}mp_memory;

typedef struct mp_pool {
    unsigned int last_id;
    int auto_extend;
    mem_size_t mempool_size;       // 固定值 每个内存池最大内存
    mem_size_t max_mempool_size;   // 固定值 所有内存池加和总上限
    mem_size_t alloc_mempool_size; // 统计值 当前已分配的内存池总大小
    struct _mp_mempool_list* mlist;
    Amy_mutex_t lock;
} mp_pool;

#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
#define MP_CHUNKEND sizeof(struct _mp_chunk*)
#define MP_ALIGN_SIZE(_n) (((uintptr_t) (_n) + ((sizeof(long)) - 1)) & ~((uintptr_t) ((sizeof(long)) - 1)))

#define MP_INIT_MEMORY_STRUCT(mm, mempool_sz)   \
    do {                                        \
        mm->mempool_size = mempool_sz;          \
        mm->alloc_mem = 0;                      \
        mm->alloc_prog_mem = 0;                 \
        mm->free_list = (mp_chunk*) mm->start; \
        mm->free_list->is_free = 1;             \
        mm->free_list->alloc_mem = mempool_sz;  \
        mm->free_list->prev = NULL;             \
        mm->free_list->next = NULL;             \
        mm->alloc_list = NULL;                  \
    } while (0)

#define MP_DLINKLIST_INS_FRT(head, x) \
    do {                              \
        x->prev = NULL;               \
        x->next = head;               \
        if (head) head->prev = x;     \
        head = x;                     \
    } while (0)

#define MP_DLINKLIST_DEL(head, x)                 \
    do {                                          \
        if (!x->prev) {                           \
            head = x->next;                       \
            if (x->next) x->next->prev = NULL;    \
        } else {                                  \
            x->prev->next = x->next;              \
            if (x->next) x->next->prev = x->prev; \
        }                                         \
    } while (0)




void get_memory_list_count(mp_pool* mp, mem_size_t* mlist_len) {

    Amy_mutex_lock(&mp->lock);
    mem_size_t mlist_l = 0;
    mp_memory* mm = mp->mlist;
    while (mm) {
        mlist_l++;
        mm = mm->next;
    }
    *mlist_len = mlist_l;
    Amy_mutex_unlock(&mp->lock);
}

void get_memory_info(mp_pool* mp, mp_memory* mm, mem_size_t* free_list_len, mem_size_t* alloc_list_len) {
    Amy_mutex_lock(&mp->lock);
    mem_size_t free_l = 0, alloc_l = 0;
    mp_chunk* p = mm->free_list;
    while (p) {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p) {
        alloc_l++;
        p = p->next;
    }
    *free_list_len = free_l;
    *alloc_list_len = alloc_l;
    Amy_mutex_unlock(&mp->lock);
}

int get_memory_id(mp_memory* mm) {
    return mm->id;
}

static mp_memory* extend_memory_list(mp_pool* mp, mem_size_t new_mempool_sz) {
    char* s = (char*)Amy_malloc(sizeof(mp_memory) + new_mempool_sz * sizeof(char));
    if (!s) return NULL;

    mp_memory* mm = (mp_memory*)s;
    mm->start = s + sizeof(mp_memory);

    MP_INIT_MEMORY_STRUCT(mm, new_mempool_sz);
    mm->id = mp->last_id++;
    mm->next = mp->mlist;
    mp->mlist = mm;
    return mm;
}

static mp_memory* find_memory_list(mp_pool* mp, void* p) {
    mp_memory* tmp = mp->mlist;
    while (tmp) {
        if (tmp->start <= (char*)p &&
            tmp->start + mp->mempool_size > (char*)p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static int merge_free_chunk(mp_pool* mp, mp_memory* mm, mp_chunk* c) {
    mp_chunk* p0 = c, * p1 = c;
    while (p0->is_free) {
        p1 = p0;
        if ((char*)p0 - MP_CHUNKEND - MP_CHUNKHEADER <= mm->start) break;
        p0 = *(mp_chunk**)((char*)p0 - MP_CHUNKEND);
    }

    p0 = (mp_chunk*)((char*)p1 + p1->alloc_mem);
    while ((char*)p0 < mm->start + mp->mempool_size && p0->is_free) {
        MP_DLINKLIST_DEL(mm->free_list, p0);
        p1->alloc_mem += p0->alloc_mem;
        p0 = (mp_chunk*)((char*)p0 + p0->alloc_mem);
    }

    *(mp_chunk**)((char*)p1 + p1->alloc_mem - MP_CHUNKEND) = p1;
    return 0;
}

mp_pool* Amy_mempool_init(mem_size_t max_mempool_size, mem_size_t mempool_size) {
    if (mempool_size > max_mempool_size) {
        AMY_LOG_ERROR("MemPool Init ERROR! Mempoolsize is too big!\n");
        return NULL;
    }

    mp_pool* mp = (mp_pool*)Amy_malloc(sizeof(struct mp_pool));
    if (!mp) {
        return NULL;
    }

    mp->last_id = 0;
    max_mempool_size = (max_mempool_size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
    mempool_size = (mempool_size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
    if (mempool_size < max_mempool_size) mp->auto_extend = 1;
    mp->max_mempool_size = max_mempool_size;
    mp->alloc_mempool_size = mp->mempool_size = mempool_size; // 初始分配一个内存池

    Amy_mutex_init(&mp->lock, NULL);

    char* s = (char*)Amy_malloc(sizeof(mp_memory) + sizeof(char) * mp->mempool_size);
    if (!s) {
        Amy_free(mp);
        return NULL;
    }

    mp->mlist = (mp_memory*)s;
    mp->mlist->start = s + sizeof(mp_memory);
    MP_INIT_MEMORY_STRUCT(mp->mlist, mp->mempool_size);
    mp->mlist->next = NULL;
    mp->mlist->id = mp->last_id++;
    return mp;
}

void* Amy_mempool_alloc(mp_pool* mp, mem_size_t wantsize) {
    if (wantsize <= 0) {
        AMY_LOG_ERROR("wantsize is small low \n");
        return NULL;
    }

    mem_size_t total_needed_size = MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
    if (total_needed_size > mp->mempool_size) {
        AMY_LOG_ERROR("total_needed_size(%d) >  mp->mempool_size(%d)\n", total_needed_size, mp->mempool_size);
        return NULL;
    }

    mp_memory* mm = NULL;
    mp_chunk* _free = NULL, * _not_free = NULL;
    Amy_mutex_lock(&mp->lock);
FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm) {
        if (mp->mempool_size - mm->alloc_mem < total_needed_size) {
            mm = mm->next;
            continue;
        }

        _free = mm->free_list;
        _not_free = NULL;

        while (_free) {
            if (_free->alloc_mem >= total_needed_size) {
                // 如果free块分割后剩余内存足够大 则进行分割
                if (_free->alloc_mem - total_needed_size > MP_CHUNKHEADER + MP_CHUNKEND) {
                    // 从free块头开始分割出alloc块
                    _not_free = _free;

                    _free = (mp_chunk*)((char*)_not_free + total_needed_size);
                    *_free = *_not_free;
                    _free->alloc_mem -= total_needed_size;
                    *(mp_chunk**)((char*)_free + _free->alloc_mem - MP_CHUNKEND) = _free;

                    // update free_list
                    if (!_free->prev) {
                        mm->free_list = _free;
                    }
                    else {
                        _free->prev->next = _free;
                    }
                    if (_free->next) _free->next->prev = _free;

                    _not_free->is_free = 0;
                    _not_free->alloc_mem = total_needed_size;

                    *(mp_chunk**)((char*)_not_free + total_needed_size - MP_CHUNKEND) = _not_free;
                }
                // 不够 则整块分配为alloc
                else {
                    _not_free = _free;
                    MP_DLINKLIST_DEL(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }
                MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);

                mm->alloc_mem += _not_free->alloc_mem;
                mm->alloc_prog_mem += (_not_free->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);
                Amy_mutex_unlock(&mp->lock);
                return (void*)((char*)_not_free + MP_CHUNKHEADER);
            }
            _free = _free->next;
        }
        mm = mm->next;
    }

    if (mp->auto_extend) {
        // 超过总内存限制
        if (mp->alloc_mempool_size + total_needed_size > mp->max_mempool_size) {
            AMY_LOG_ERROR("(mp->alloc_mempool_size(%llu) + total_needed_size(%llu))>mp->max_mempool_size(%llu)\n", mp->alloc_mempool_size, total_needed_size, mp->max_mempool_size);
            goto err_out;
        }
        // 剩余可新增内存池大小
        mem_size_t add_mem_sz = mp->max_mempool_size - mp->mempool_size;
        // 如果空间足够则按 mempool_size 新增, 不足则分配剩下所有内存
        add_mem_sz = add_mem_sz >= mp->mempool_size ? mp->mempool_size : add_mem_sz;
        if (!extend_memory_list(mp, add_mem_sz)) {
            AMY_LOG_ERROR("extend memory liast failed");
            goto err_out;
        }
        // 更新实际分配内存
        mp->alloc_mempool_size += add_mem_sz;

        goto FIND_FREE_CHUNK;
    }

err_out:
    AMY_LOG_ERROR("No enough memory! \n");
    Amy_mutex_unlock(&mp->lock);
    return NULL;
}

int Amy_mempool_free(mp_pool* mp, void* p) {
    if (p == NULL || mp == NULL)
        return 1;

    Amy_mutex_lock(&mp->lock);
    mp_memory* mm = mp->mlist;
    if (mp->auto_extend)
        mm = find_memory_list(mp, p);

    mp_chunk* ck = (mp_chunk*)((char*)p - MP_CHUNKHEADER);

    MP_DLINKLIST_DEL(mm->alloc_list, ck);
    MP_DLINKLIST_INS_FRT(mm->free_list, ck);
    ck->is_free = 1;

    mm->alloc_mem -= ck->alloc_mem;
    mm->alloc_prog_mem -= (ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

    merge_free_chunk(mp, mm, ck);
    Amy_mutex_unlock(&mp->lock);
    return 0;
}

mp_pool* Amy_mempool_clear(mp_pool* mp) {
    if (!mp)
        return NULL;

    Amy_mutex_lock(&mp->lock);
    mp_memory* mm = mp->mlist;
    while (mm) {
        MP_INIT_MEMORY_STRUCT(mm, mm->mempool_size);
        mm = mm->next;
    }
    Amy_mutex_unlock(&mp->lock);
    return mp;
}

int Amy_mempool_destory(mp_pool* mp) {
    if (mp == NULL)
        return 1;
    Amy_mutex_lock(&mp->lock);
    mp_memory* mm = mp->mlist, * mm1 = NULL;
    while (mm) {
        mm1 = mm;
        mm = mm->next;
        Amy_free(mm1);
    }
    Amy_mutex_unlock(&mp->lock);
    Amy_mutex_destroy(&mp->lock);
    Amy_free(mp);
    return 0;
}

mem_size_t Amy_total_memory_total(mp_pool* mp) {
    return mp->alloc_mempool_size;
}

mem_size_t Amy_used_memory_get(mp_pool* mp) {
    Amy_mutex_lock(&mp->lock);
    mem_size_t total_alloc = 0;
    mp_memory* mm = mp->mlist;
    while (mm) {
        total_alloc += mm->alloc_mem;
        mm = mm->next;
    }
    Amy_mutex_unlock(&mp->lock);
    return total_alloc;
}

mem_size_t Amy_prog_memory_get(mp_pool* mp) {
    Amy_mutex_lock(&mp->lock);
    mem_size_t total_alloc_prog = 0;
    mp_memory* mm = mp->mlist;
    while (mm) {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
    Amy_mutex_unlock(&mp->lock);
    return total_alloc_prog;
}
