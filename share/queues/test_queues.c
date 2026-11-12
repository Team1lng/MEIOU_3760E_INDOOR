#if 1 // defined(TEST_MODEL) && defined(TEST_QUEUES_MODEL)
#include "queues.h"
#include "common/Amy_log.h"
#include "common/Amy_sdk.h"
#include "mempool.h"
typedef struct
{
    int index;
    unsigned long receive_byte;
    char name[64];
    int run;
    void* queue;
    Amy_sem_t sem;
    Amy_thread_t tid;
    mp_pool* mp;
} thread_info;

static int addres_compare_callback(void* new, void* current)
{
    //  AMY_LOG_ERROR("---------------------------%p:%p---------------------\n", new, current);
    return new - current;
}
static void queue_mp_free(void* user_data, void* data)
{
    Amy_mempool_free(user_data, data);
}

static void* queue_send_thrad(void* arg)
{
    thread_info* ctx = (thread_info*)arg;
    while (ctx->run) {
        char* data = Amy_mempool_alloc(ctx->mp, 64);
        if (Amy_queues_put(ctx->queue, data) != 0)
        {
            Amy_mempool_free(ctx->mp, data);
        }
        Amy_msleep(500);
    }
   // AMY_LOG_ERROR("thread %s finish\n", ctx->name);
    Amy_sem_post(&ctx->sem);
    return NULL;
}

static void* queue_thread(void* arg)
{
    thread_info send_ctx[2];
    thread_info* ctx = (thread_info*)arg;

    memset(send_ctx[0].name, 0, sizeof(send_ctx[0].name));
    strcpy(send_ctx[0].name, "send0 thread");
    send_ctx[0].queue = ctx->queue;
    Amy_sem_init(&send_ctx[0].sem, 0, 0);
    send_ctx[0].mp = ctx->mp;
    send_ctx[0].run = 1;
    Amy_pthread_create(&send_ctx[0].tid, queue_send_thrad, &send_ctx[0], 100 * 1024, 26);

    memset(send_ctx[1].name, 0, sizeof(send_ctx[1].name));
    strcpy(send_ctx[1].name, "send1 thread");
    send_ctx[1].queue = ctx->queue;
    Amy_sem_init(&send_ctx[1].sem, 0, 0);
    send_ctx[1].mp = ctx->mp;
    send_ctx[1].run = 1;
    Amy_pthread_create(&send_ctx[1].tid, queue_send_thrad, &send_ctx[1], 100 * 1024, 26);

    while (ctx->run)
    {
        void* data = NULL;

        if (Amy_queues_get(ctx->queue, &data, NULL, NULL) == 0)
        {
            ctx->receive_byte++;
            AMY_LOG_DEBUG("queue %d receive block:%lu total:%llu,used:%llu,drop:%llu\n",
                ctx->index,
                ctx->receive_byte,
                Amy_total_memory_total(ctx->mp),
                Amy_used_memory_get(ctx->mp),
                Amy_prog_memory_get(ctx->mp));

            Amy_mempool_free(ctx->mp, data);
        }
        Amy_msleep(ctx->index*100);
    }

    send_ctx[0].run = 0;
    Amy_sem_wait(&send_ctx[0].sem);
    Amy_pthread_join(send_ctx[0].tid, NULL);
    Amy_sem_destroy(&send_ctx[0].sem);

    send_ctx[1].run = 0;
    Amy_sem_wait(&send_ctx[1].sem);
    Amy_pthread_join(send_ctx[1].tid, NULL);
    Amy_sem_destroy(&send_ctx[1].sem);

  //  AMY_LOG_ERROR("thread %s finish\n", ctx->name);
    Amy_sem_post(&ctx->sem);
    return NULL;
}

int test_main(void)
{
    thread_info ctx[4] ;//= { 0 };
    //unsigned int loop = 0;

    while (1)
    {
#if 1
        memset(ctx[0].name, 0, sizeof(ctx[0].name));
        strcpy(ctx[0].name, "thread main 1");
        ctx[0].queue = Amy_queues_open(1024, 0, 0, NULL, 0);
        Amy_sem_init(&ctx[0].sem, 0, 0);
        ctx[0].mp = Amy_mempool_init(10240, 1024);
        ctx[0].index = 1;
        ctx[0].receive_byte = 0;
        ctx[0].run = 1;
        Amy_pthread_create(&ctx[0].tid, queue_thread, &ctx[0], 100 * 1024, 26);
#endif
#if 1
        memset(ctx[1].name, 0, sizeof(ctx[1].name));
        strcpy(ctx[1].name, "thread main 2");
        ctx[1].queue = Amy_queues_open(1024, 0, 0, NULL, 1);
        Amy_sem_init(&ctx[1].sem, 0, 0);
        ctx[1].mp = Amy_mempool_init(10240, 1024);
        ctx[1].index = 2;
        ctx[1].receive_byte = 0;
        ctx[1].run = 1;
        Amy_pthread_create(&ctx[1].tid, queue_thread, &ctx[1], 100 * 1024, 26);
#endif
#if 1
        memset(ctx[2].name, 0, sizeof(ctx[2].name));
        strcpy(ctx[2].name, "thread main 3");
        ctx[2].queue = Amy_queues_open(1024, 1, 0, addres_compare_callback, 0);
        Amy_sem_init(&ctx[2].sem, 0, 0);
        ctx[2].mp = Amy_mempool_init(10240, 1024);
        ctx[2].index = 3;
        ctx[2].receive_byte = 0;
        ctx[2].run = 1;
        Amy_pthread_create(&ctx[2].tid, queue_thread, &ctx[2], 100 * 1024, 26);
#endif
#if 1
        memset(ctx[3].name, 0, sizeof(ctx[3].name));
        strcpy(ctx[3].name, "thread main 4");
        ctx[3].queue = Amy_queues_open(1024, 1, 1, addres_compare_callback, 1);
        Amy_sem_init(&ctx[3].sem, 0, 0);;
        ctx[3].mp = Amy_mempool_init(10240, 1024);
        ctx[3].index = 4;
        ctx[3].receive_byte = 0;
        ctx[3].run = 1;
        Amy_pthread_create(&ctx[3].tid, queue_thread, &ctx[3], 100 * 1024, 26);
#endif
        for (int i = 0; i < 4; i++)
        {
            Amy_msleep(1000);
            AMY_LOG_DEBUG("delay:%ds\n", i + 1);
        }

        for (int i = 0; i < 4; i++)
        {
            ctx[i].run = 0;
            Amy_queues_enable(ctx[i].queue, 0);
            Amy_sem_post(&ctx[i].sem);
            Amy_pthread_join(ctx[i].tid, NULL);
            Amy_sem_destroy(&ctx[i].sem);
            Amy_queues_close(ctx[i].queue, queue_mp_free, ctx[i].mp);
            Amy_mempool_destory(ctx[i].mp);
        }
    }
    return 0;
}

#endif