#include "queues.h"
#include "mempool.h"
#include "common/Amy_log.h"
#include <common/Amy_sdk.h>

typedef struct q_element_s
{
    void *data;
    struct q_element_s *next;
} q_element_t;

typedef struct queues_conetext
{
    q_element_t *frist_el;
    q_element_t *last_el;

    mp_pool *mp;

    unsigned int num_els;
    unsigned int max_els;
    unsigned char new_data;
    unsigned char sort;
    unsigned char asc_order;
    unsigned char block;

    int (*cmp_el_func)(void *, void *);

    Amy_mutex_t mutex;
    Amy_cond_t cond_get;
    Amy_cond_t cond_put;
} queues_conetext;

queues_conetext *Amy_queues_open(int max_elements, int sort, int asc_order, int (*cmp_func)(void *, void *), int block)
{
    queues_conetext *ctx = (queues_conetext *)Amy_malloc(sizeof(struct queues_conetext));
    if (!ctx)
    {
        AMY_LOG_ERROR("rt_malloc failed\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(queues_conetext));

    // 初始化内存池
    ctx->mp = Amy_mempool_init(sizeof(q_element_t) * max_elements, sizeof(q_element_t) * max_elements);
    if (!ctx->mp)
    {
        Amy_free(ctx);
        AMY_LOG_ERROR("rt_mp create failed\n");
        return NULL;
    }

    Amy_mutex_init(&ctx->mutex, NULL);
    Amy_cond_init(&ctx->cond_get, NULL);
    Amy_cond_init(&ctx->cond_put, NULL);

    ctx->asc_order = asc_order;
    ctx->block = block;
    ctx->cmp_el_func = cmp_func;
    ctx->sort = sort;
    ctx->new_data = 1;
    ctx->max_els = max_elements;
    ctx->num_els = 0;
    return ctx;
}

int Amy_queues_put(queues_conetext *d, void *data)
{
    // unsigned int put_events = 0;
    Amy_mutex_lock(&d->mutex);
    if (d->new_data == 0)
    {
        Amy_mutex_unlock(&d->mutex);
        // AMY_LOG_ERROR("no new data alllow\n");
        return -1;
    }

    if ((d->num_els == (AMY_UIN32_MAX - 1)) || ((d->max_els != 0) && (d->num_els == d->max_els)))
    {
        // 在非阻塞情况下，数据已满
        if (d->block == 0)
        {
            Amy_mutex_unlock(&d->mutex);
            AMY_LOG_ERROR("num elements failed,data over full:%d\n", d->num_els);
            return -1;
        }
        else
        {
            while ((d->new_data != 0) && ((d->num_els == (AMY_UIN32_MAX - 1)) || ((d->max_els != 0) && (d->num_els == d->max_els))))
            {
                // 在可以读取数据，并且数据满的情况下阻塞等待数据数据填充
                Amy_cond_wait(&d->cond_put, &d->mutex);
            }
            if (d->new_data == 0)
            {
                Amy_mutex_unlock(&d->mutex);
                AMY_LOG_ERROR("no new data alllow\n");
                return -1;
            }
        }
    }

    // 创建节点
    q_element_t *new_el = Amy_mempool_alloc(d->mp, sizeof(q_element_t));
    if (!new_el)
    {
        Amy_mutex_unlock(&d->mutex);
        AMY_LOG_ERROR("Amy_mempool_alloc failed\n");
        return -1;
    }

    new_el->data = data;
    new_el->next = NULL;

    // 没有排序处理
    if ((d->sort == 0) || (d->frist_el == NULL))
    {
        if (d->last_el == NULL)
        {
            d->frist_el = new_el;
        }
        else
        {
            d->last_el->next = new_el;
        }
        d->last_el = new_el;
    }
    else
    {
        // 满足升序要求
        int is_asc = (d->asc_order != 0) && (d->cmp_el_func(data, d->last_el->data) >= 0);
        // 满足降序要求
        int is_dees = (d->asc_order == 0) && (d->cmp_el_func(data, d->last_el->data) <= 0);
        // 都不满足
        if ((is_asc == 0) && (is_dees == 0))
        {
            q_element_t *s = d->frist_el;
            q_element_t *t = NULL;
            for (s = d->frist_el, t = s->next; ((s != NULL) && (t != NULL)); s = t, t = t->next)
            {
                // 升序
                if (d->asc_order != 0)
                {
                    if ((d->cmp_el_func(s->data, data) <= 0) && (d->cmp_el_func(data, t->data) <= 0))
                    {
                        break;
                    }
                }
                else
                { // 降序
                    if ((d->cmp_el_func(s->data, data) >= 0) && (d->cmp_el_func(data, t->data) >= 0))
                    {
                        break;
                    }
                }
            }
            s->next = new_el;
            new_el->next = t;
            if (t == NULL)
            {
                d->last_el = new_el;
            }
        }
        else
        { // 满足情况下，直接拉入到队尾
            d->last_el->next = new_el;
            d->last_el = new_el;
        }
    }
    d->num_els++;
    Amy_cond_signal(&d->cond_get);
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

int Amy_queues_get(queues_conetext *d, void **data, int (*find_func)(void *, void *), void *filter)
{
    // unsigned int get_events = 0;

    Amy_mutex_lock(&d->mutex);
    if (d->num_els == 0)
    {
        if (d->block == 0)
        {
            *data = NULL;
            Amy_mutex_unlock(&d->mutex);
            return -1;
        }
        else
        {
            while ((d->num_els == 0) && (d->new_data != 0))
            {
                // 在可以读取数据，并且数据满的情况下阻塞等待数据数据填充
                Amy_cond_wait(&d->cond_get, &d->mutex);
            }
            if ((d->num_els == 0) && (d->new_data == 0))
            {
                Amy_mutex_unlock(&d->mutex);
                return -1;
            }
        }
    }

    q_element_t *el_prev = NULL, *el = d->frist_el;
    while ((find_func != NULL) && (el != NULL) && (find_func(filter, el) != 0))
    {
        el_prev = el;
        el = el->next;
    }

    if ((el != NULL) && (el_prev == NULL))
    {
        d->frist_el = el->next;
        if (d->frist_el == NULL)
        {
            d->last_el = NULL;
        }
    }
    else if ((el != NULL) && (el_prev != NULL))
    {
        el_prev->next = el->next;
    }
    else
    {
        *data = NULL;
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }
    d->num_els--;
    *data = el->data;
    Amy_mempool_free(d->mp, el);
    Amy_cond_signal(&d->cond_put);
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

static int queues_flush(queues_conetext *d, void (*free_func)(void *, void *), void *user_data)
{
    q_element_t *s = d->frist_el;
    q_element_t *t = NULL;

    while (s != NULL)
    {
        t = s->next;
        if (free_func)
        {
            free_func(user_data, s->data);
        }
        else
        {
            Amy_free(s->data);
        }
        Amy_mempool_free(d->mp, s);
        s = t;
    }
    d->frist_el = d->last_el = NULL;
    d->num_els = 0;
    return 0;
}

int Amy_queues_flush(queues_conetext *d, void (*free_func)(void *, void *), void *user_data)
{
    Amy_mutex_lock(&d->mutex);
    queues_flush(d, free_func, user_data);
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

int Amy_queues_close(queues_conetext *d, void (*free_func)(void *, void *), void *user_data)
{
    Amy_mutex_lock(&d->mutex);
    queues_flush(d, free_func, user_data);
    Amy_cond_destroy(&d->cond_get);
    Amy_cond_destroy(&d->cond_put);
    Amy_mutex_unlock(&d->mutex);
    Amy_mempool_destory(d->mp);
    Amy_free(d);
    return 0;
}

int Amy_queues_enable(queues_conetext *d, unsigned char enable)
{
    Amy_mutex_lock(&d->mutex);
    d->new_data = enable;
    Amy_mutex_unlock(&d->mutex);
    if (d->new_data == 0)
    {
        Amy_cond_signal(&d->cond_get);
        Amy_cond_signal(&d->cond_put);
    }
    return 0;
}

unsigned char Amy_queues_enable_status(queues_conetext *d)
{
    return d->new_data;
}

unsigned int Amy_queues_new_els(queues_conetext *d)
{
    return d->num_els;
}

unsigned int Amy_queues_max_els(queues_conetext *d)
{
    return d->max_els;
}