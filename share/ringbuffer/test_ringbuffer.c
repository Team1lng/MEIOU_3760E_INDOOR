
#if defined(TEST_MODEL) && defined(TEST_RINGBUFFER_MODEL)

#include "ringbuffer.h"
#include "common/Amy_log.h"
#include <rtthread.h>

static int is_thread_run = 0;
static rt_sem_t exit_thread_sem;
static void thread_run(void *arg)
{
    AMY_LOG_DEBUG("ringbuffer read thread runing\n");
    const int data_size = 400;
    char *data = (char *)rt_malloc(data_size);
    while (is_thread_run)
    {
        for (int size = data_size; size > 0; size--)
        {
            if (!is_thread_run)
            {
                break;
            }
            if (Amy_ringbuffer_read(arg, data, size) != size)
            {
                rt_thread_mdelay(50);
                continue;
            }
            rt_thread_mdelay(3);
        }
    }
    rt_free(data);
    AMY_LOG_DEBUG("ringbuffer read thread stop\n");
    rt_sem_release(exit_thread_sem); // 发送结束信号
}

int test_main(void)
{
    unsigned long extren_loop = 0;
    while (1)
    {

        void *ringbuffer = Amy_ringbuffer_open(1024 * 1024);

        exit_thread_sem = rt_sem_create("exit_sem", 0, RT_IPC_FLAG_FIFO);

        is_thread_run = 1;
        rt_thread_t tid = rt_thread_create("ringbuffer_test", thread_run, ringbuffer, 1280000, 25, 5);
        rt_thread_startup(tid);

        const int data_size = 400;
        char *data = (char *)rt_malloc(data_size);
        for (unsigned int i = 0; i < 3; i++)
        {
            for (int size = 1; size < data_size; size += 100)
            {
                if (Amy_ringbuffer_write(ringbuffer, data, size) != size)
                {
                    AMY_LOG_ERROR("write %d failed\n", size);
                }
                rt_thread_mdelay(5);
            }
            AMY_LOG_DEBUG("inside_loop:%d\n", i);
        }
        rt_free(data);
        is_thread_run = 0;
        rt_sem_take(exit_thread_sem, RT_WAITING_FOREVER); // 阻塞等待
        rt_thread_delete(tid);
        rt_sem_delete(exit_thread_sem);
        Amy_ringbuffer_close(ringbuffer);
        AMY_LOG_DEBUG("extren_loop:%lu\n", extren_loop++);
    }
}

#endif