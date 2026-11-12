#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

#include "wheel_timer.h"

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define TICK_DURATION_MS    1                 //一次Tick的间隔
#define WHEEL_LENGTH_FINE   128               //细粒度时间轮长度
#define WHEEL_LENGTH_COARSE 128               //粗粒度时间轮长度
#define HASH_TABLE_LEN      128               //存放所有定时器的哈希表长度

#define TYDEBUG printf

typedef struct list_entry_t {
    struct list_entry_t* prev;
    struct list_entry_t* next;
} list_entry_t;

typedef struct wheel_timer_t{
    list_entry_t            entry;
    uint64_t                deadline;         //到期时间（毫秒）
    uint32_t                remain_rounds;    //粗粒度时间轮的圈数
    void*                   private_data;     //回调参数
    wheel_timer_expire_cb   expire_cb;        //回调
} wheel_timer_t;

typedef struct wheel_timer_box_t {
    list_entry_t            entry;
    wheel_timer_t*          timer;
} wheel_timer_box_t;

typedef struct {
    uint64_t                start_time;       //初始化的起始时间
    list_entry_t            coarse_wheel[WHEEL_LENGTH_COARSE];  //粗粒度时间轮
    list_entry_t            fine_wheel[WHEEL_LENGTH_FINE];  //细粒度时间轮
    list_entry_t            pending_head; //新建定时器时先加入此队列
    list_entry_t            cancelled_head; //取消定时器时先加入此队列
    list_entry_t            expired_head; //定时器超时执行完成后加入此队列
    list_entry_t            timer_hash_table[HASH_TABLE_LEN]; //存储所有定时器的哈希表
    pthread_cond_t          cond;
    pthread_mutex_t         mutex;
    pthread_t               tid;
    int                     inited;
    int                     thread_exit;
    uint64_t                tick;
    uint64_t                next_deadline;
} wheel_timer_ctx_t;

static wheel_timer_ctx_t g_ctx;

static uint64_t __get_monotonic_time_ms()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static inline WHEEL_TIMER_HANDLE __wheel_timer_to_handle(wheel_timer_t* ptimer)
{
    if (NULL == ptimer){
        return WHEEL_TIMER_INVALID_HANDLE;
    }
    return (WHEEL_TIMER_HANDLE)ptimer;
}

static inline wheel_timer_t* __wheel_timer_from_handle(WHEEL_TIMER_HANDLE handle)
{
    if (WHEEL_TIMER_INVALID_HANDLE == handle){
        return NULL;
    }

    return (wheel_timer_t*)handle;
}

#ifdef WHEEL_TIMER_DEBUG
static void __wheel_timer_debug()
{
    TYDEBUG("-----------------start------------------\n");
    printf("pending: ");
    list_entry_t* cur = g_ctx.pending_head.next;
    while(cur){
        wheel_timer_box_t *box = container_of(cur, wheel_timer_box_t, entry);
        printf("%p->", box->timer);
        cur = cur->next;
    }
    printf("\ncancel: ");
    cur = g_ctx.cancelled_head.next;
    while(cur){
        wheel_timer_box_t *box = container_of(cur, wheel_timer_box_t, entry);
        printf("%p->", box->timer);
        cur = cur->next;
    }
    printf("\nfine wheel:");
    for(int i = 0; i < WHEEL_LENGTH_FINE; i++){
        cur = g_ctx.fine_wheel[i].next;
        int first = 1;
        while(cur){
            if (first){
                first = 0;
                printf("\n[%d]->", i);
            }
            printf("%p->", cur);
            cur = cur->next;
        }
    }
    printf("\ncoarse wheel:");
    for(int i = 0; i < WHEEL_LENGTH_COARSE; i++){
        cur = g_ctx.coarse_wheel[i].next;
        int first = 1;
        while(cur){
            if (first){
                first = 0;
                printf("\n[%d]->", i);
            }
            printf("%p->", cur);
            cur = cur->next;
        }
    }
    printf("\n");
    TYDEBUG("--------------end------------------\n");
    
}
#endif

static void __add_entry_at_head(list_entry_t* phead, list_entry_t* pentry)
{
    list_entry_t* pnext = phead->next;
    if (pnext){
        pnext->prev = pentry;
    }
    pentry->next = pnext;
    phead->next = pentry;
    pentry->prev = phead;

#ifdef WHEEL_TIMER_DEBUG
    __wheel_timer_debug();
#endif
}

static void __remove_entry(list_entry_t* pentry)
{
    list_entry_t* prev = pentry->prev;
    list_entry_t* next = pentry->next;
    if (prev){
        prev->next = next;
    }
    if (next){
        next->prev = prev;
    }

#ifdef WHEEL_TIMER_DEBUG
    __wheel_timer_debug();
#endif
}

static inline int __hash(void* ptimer)
{
    return (((long)ptimer / sizeof(long)) % HASH_TABLE_LEN);
}

/**
 * 将cancel队列中的定时器移除
 */
static int __process_cancelled_timeouts()
{
    pthread_mutex_lock(&g_ctx.mutex);
    list_entry_t* cancel = g_ctx.cancelled_head.next;
    while(cancel){
        int found = 0;
        list_entry_t* cancel_next = cancel->next;
        wheel_timer_box_t *cancel_box = container_of(cancel, wheel_timer_box_t, entry);

        //遍历pending_timeouts列表
        list_entry_t* pending = g_ctx.pending_head.next;
        while(pending){
            list_entry_t* pending_next = pending->next;

            wheel_timer_box_t* pending_box = container_of(pending, wheel_timer_box_t, entry);
            if (pending_box->timer == cancel_box->timer){
                __remove_entry(pending);
                free(pending_box);
                found = 1;
                break;
            }

            pending = pending_next;
        }
        //若没有找到则肯定在时间轮上或expired列表上，直接从双向链表中移除
        if (!found){
            //TYDEBUG("unlink timer: %p\n", cancel_box->timer);
            __remove_entry(&cancel_box->timer->entry);
        }

        __remove_entry(cancel);
        free(cancel_box->timer);
        free(cancel_box);

        cancel = cancel_next;
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    return 0;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

/**
 * 将pending队列中的定时器加入时间轮
 */
static int __process_pending_timeouts(uint64_t now_tick)
{
    pthread_mutex_lock(&g_ctx.mutex);
    list_entry_t* pending = g_ctx.pending_head.next;
    int count = 0;
    while(pending && ++count < 10000){
        list_entry_t* pending_next = pending->next;

        wheel_timer_box_t* pending_box = container_of(pending, wheel_timer_box_t, entry);
        wheel_timer_t* ptimer = pending_box->timer;

        uint64_t calc_tick = ptimer->deadline / TICK_DURATION_MS;

        calc_tick = MAX(calc_tick, now_tick);

        uint32_t calc_round = calc_tick / (WHEEL_LENGTH_COARSE * WHEEL_LENGTH_FINE);
        uint32_t now_round = now_tick / (WHEEL_LENGTH_COARSE * WHEEL_LENGTH_FINE);
        ptimer->remain_rounds = calc_round - now_round;
        if ((calc_tick & (~(WHEEL_LENGTH_FINE - 1))) > (now_tick & (~(WHEEL_LENGTH_FINE - 1)))){
            //放入粗粒度时间轮
            int idx = (calc_tick / WHEEL_LENGTH_FINE) & (WHEEL_LENGTH_COARSE - 1);
            __add_entry_at_head(&g_ctx.coarse_wheel[idx], &ptimer->entry);
        }
        else{
            //放入细粒度时间轮
            int idx = calc_tick & (WHEEL_LENGTH_FINE - 1);
            __add_entry_at_head(&g_ctx.fine_wheel[idx], &ptimer->entry);
        }

        __remove_entry(pending);
        free(pending_box);

        pending = pending_next;
    }
    if (count >= 10000){
        TYDEBUG("pending list exceed limit\n");
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    return 0;
}

static int __execute_expired_timeouts(uint64_t now_tick)
{
    //TYDEBUG("\n");
    int fine_idx = now_tick & (WHEEL_LENGTH_FINE - 1);
    int coarse_idx = (now_tick / WHEEL_LENGTH_FINE) & (WHEEL_LENGTH_COARSE - 1);
    //先将粗粒度时间轮上的定时器放入细粒度时间轮
    list_entry_t* cur = g_ctx.coarse_wheel[coarse_idx].next;
    while(cur){
        list_entry_t* next = cur->next;

        wheel_timer_t* ptimer = container_of(cur, wheel_timer_t, entry);
        uint64_t calc_tick = ptimer->deadline / TICK_DURATION_MS;
        calc_tick = MAX(calc_tick, now_tick);

        uint32_t calc_round = calc_tick / (WHEEL_LENGTH_COARSE * WHEEL_LENGTH_FINE);
        uint32_t now_round = now_tick / (WHEEL_LENGTH_COARSE * WHEEL_LENGTH_FINE);
        ptimer->remain_rounds = calc_round - now_round;
        //TYDEBUG("execute: ptimer(%p) remain_rounds(%u) now_tick(%llu) calc_tick(%llu)\n", ptimer, ptimer->remain_rounds, now_tick, calc_tick);

        if (0 == ptimer->remain_rounds){

            int idx = calc_tick & (WHEEL_LENGTH_FINE - 1);
            //TYDEBUG("unlink timer from coarse wheel: %p\n", ptimer);
            __remove_entry(cur);
            //TYDEBUG("link timer to fine wheel: %p\n", ptimer);
            __add_entry_at_head(&g_ctx.fine_wheel[idx], cur);
        }

        cur = next;
    }

    //处理细粒度时间轮上到期的定时器
    cur = g_ctx.fine_wheel[fine_idx].next;
    while(cur){
        list_entry_t* next = cur->next;

        wheel_timer_t* ptimer = container_of(cur, wheel_timer_t, entry);
        WHEEL_TIMER_HANDLE handle = __wheel_timer_to_handle(ptimer);
        ptimer->expire_cb(handle, ptimer->private_data);
        wheel_timer_remove(handle);
        //执行完后放入expire队列等待删除
        //TYDEBUG("unlink from wheel after expire: %p\n", ptimer);
        __remove_entry(cur);
        //TYDEBUG("add to expired: %p\n", ptimer);
        __add_entry_at_head(&g_ctx.expired_head, cur);

        cur = next;
    }
    return 0;
}

static uint64_t __wait_for_next_tick()
{
    //遍历时间轮寻找下一个定时器对应的tick
    uint64_t now_tick = g_ctx.tick;
    uint64_t next_tick = now_tick + 1;
    uint64_t next_coarse_tick = (now_tick + WHEEL_LENGTH_FINE) & (~(WHEEL_LENGTH_FINE - 1));
    for(; next_tick < next_coarse_tick; next_tick++){
        int idx = next_tick & (WHEEL_LENGTH_FINE - 1);
        list_entry_t* phead = &g_ctx.fine_wheel[idx];
        //如果next_tick对应的链表中有定时器，则将其设为下一次睡眠的时间
        if (phead->next){
            break;
        }
    }

    if (next_tick == next_coarse_tick){
        //如果遍历完细粒度时间轮还没找到定时器，则继续遍历粗粒度时间轮
        uint64_t end_tick = next_tick + WHEEL_LENGTH_FINE * WHEEL_LENGTH_COARSE;
        for(; next_tick < end_tick; next_tick += WHEEL_LENGTH_FINE){
            int idx = (next_tick / WHEEL_LENGTH_FINE) & (WHEEL_LENGTH_COARSE - 1);
            list_entry_t* phead = &g_ctx.coarse_wheel[idx];
            if (phead->next){
                break;
            }
        }
    }

    uint64_t next_deadline = next_tick * TICK_DURATION_MS;
    uint64_t now = __get_monotonic_time_ms() - g_ctx.start_time;
    if (next_deadline <= now){
        return next_deadline;
    }
    else{
        g_ctx.next_deadline = next_deadline;

        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);

        uint64_t msec = next_deadline - now + ts.tv_nsec / 1000000;
        ts.tv_sec += msec / 1000;
        ts.tv_nsec = (msec % 1000) * 1000000;

        pthread_mutex_lock(&g_ctx.mutex);
        //当pending列表中没有定时器才进入睡眠
        if (!g_ctx.pending_head.next){
            //TYDEBUG("sleep %llu\n", next_deadline - now);
            pthread_cond_timedwait(&g_ctx.cond, &g_ctx.mutex, &ts);
        }
        pthread_mutex_unlock(&g_ctx.mutex);

    }

    now = __get_monotonic_time_ms() - g_ctx.start_time;
    //TYDEBUG("now: %llu\n", now);
    next_deadline = MIN(next_deadline, now);

    return next_deadline;
}

static void* __wheel_timer_proc(void* arg)
{
    while(!g_ctx.thread_exit){
        uint64_t deadline = __wait_for_next_tick();
        g_ctx.tick = deadline / TICK_DURATION_MS;
        __process_cancelled_timeouts();
        __process_pending_timeouts(g_ctx.tick);

        __execute_expired_timeouts(g_ctx.tick);
    }
    return NULL;
}

int wheel_timer_init()
{
    if (g_ctx.inited){
        TYDEBUG("inited already\n");
        return -1;
    }

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.start_time = __get_monotonic_time_ms();
    g_ctx.tick = 0;
    g_ctx.next_deadline = g_ctx.start_time + 100000;
    pthread_cond_init(&g_ctx.cond, NULL);
    pthread_mutex_init(&g_ctx.mutex, NULL);

    pthread_create(&g_ctx.tid, NULL, __wheel_timer_proc, NULL);
    g_ctx.inited = 1;

    return 0;
}

int wheel_timer_deinit()
{
    if (!g_ctx.inited){
        TYDEBUG("not inited\n");
        return -1;
    }

    g_ctx.thread_exit = 1;
    pthread_cond_signal(&g_ctx.cond);
    pthread_join(g_ctx.tid, NULL);

    pthread_cond_destroy(&g_ctx.cond);
    pthread_mutex_destroy(&g_ctx.mutex);

    g_ctx.inited = 0;

    return 0;
}

WHEEL_TIMER_HANDLE wheel_timer_add(uint64_t delay_ms, wheel_timer_expire_cb cb, void* pri_data)
{
    if (NULL == cb){
        return WHEEL_TIMER_INVALID_HANDLE;
    }


    wheel_timer_t* ptimer = (wheel_timer_t*)malloc(sizeof(wheel_timer_t));
    if (ptimer == NULL){
        return WHEEL_TIMER_INVALID_HANDLE;
    }
    memset(ptimer, 0, sizeof(*ptimer));

    //初始化定时器
    uint64_t now = __get_monotonic_time_ms();
    uint64_t deadline = now - g_ctx.start_time + delay_ms;
    ptimer->deadline = deadline;
    ptimer->expire_cb = cb;
    ptimer->private_data = pri_data;

    //加入pending_timeouts和timer_hash_table列表中
    wheel_timer_box_t* pbox = (wheel_timer_box_t*)malloc(sizeof(*pbox));
    if (NULL == pbox){
        free(ptimer);
        return WHEEL_TIMER_INVALID_HANDLE;
    }
    memset(pbox, 0, sizeof(*pbox));
    pbox->timer = ptimer;

    wheel_timer_box_t* pbox_ht = (wheel_timer_box_t*)malloc(sizeof(*pbox_ht));
    if (NULL == pbox_ht){
        free(ptimer);
        free(pbox);
        return WHEEL_TIMER_INVALID_HANDLE;
    }
    memset(pbox_ht, 0, sizeof(*pbox_ht));
    pbox_ht->timer = ptimer;

    pthread_mutex_lock(&g_ctx.mutex);
    __add_entry_at_head(&g_ctx.pending_head, &pbox->entry);
    if (deadline < g_ctx.next_deadline){
        //TYDEBUG("signal\n");
        pthread_cond_signal(&g_ctx.cond);
    }

    int ht_bucket = __hash(ptimer);
    __add_entry_at_head(&g_ctx.timer_hash_table[ht_bucket], &pbox_ht->entry);

    pthread_mutex_unlock(&g_ctx.mutex);

    //TYDEBUG("add timer: %p\n", ptimer);

    return __wheel_timer_to_handle(ptimer);
}

int wheel_timer_remove(WHEEL_TIMER_HANDLE handle)
{
    //TYDEBUG("remove timer: %p\n", handle);
    wheel_timer_t* ptimer = __wheel_timer_from_handle(handle);
    if (NULL == ptimer){
        return -1;
    }

    //加入cancelled_timeouts列表中
    wheel_timer_box_t* pbox= (wheel_timer_box_t*)malloc(sizeof(*pbox));
    if (NULL == pbox){
        return -1;
    }
    memset(pbox, 0, sizeof(*pbox));
    pbox->timer = ptimer;

    pthread_mutex_lock(&g_ctx.mutex);
    //从哈希表中查找定时器，防止重复删除
    int ht_bucket = __hash(ptimer);
    list_entry_t* cur = g_ctx.timer_hash_table[ht_bucket].next;
    int found = 0;
    while(cur){
        list_entry_t* next = cur->next;
        wheel_timer_box_t* box = container_of(cur, wheel_timer_box_t, entry);
        if (box->timer == ptimer){
            found = 1;

            __remove_entry(cur);
            free(box);
            break;
        }
        cur = next;
    }

    if (found){
        __add_entry_at_head(&g_ctx.cancelled_head, &pbox->entry);
    }
    else{
        free(pbox);
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    return 0;
}