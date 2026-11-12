#include "lv_firmware_resource.h"
#include "Amy_log.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct
{
    unsigned long offset;
    unsigned long size;
    void *data;
} firamware_data;

typedef struct _firamware_resource_map
{
    firamware_data d;
    char isfree;
    struct _firamware_resource_map *next;
} firamware_resource_map;

typedef struct
{
    char *ui_path;
    firamware_resource_map *head;
    int used_bytes;
    int used_cnt;
} firamware_resource_info_t;
static firamware_resource_info_t firamware_resource_context = {
    .ui_path = NULL,
    .head = NULL,
    .used_bytes = 0,
    .used_cnt = 0,
};

/* LVGL 堆仅 3MB，若缓存无限堆积会吃满堆且陷入"分配失败→回收不触发"的死锁，
 * 故设总量预算，超限后读新图前先逐出最老的已释放(isfree)节点腾地方。 */
#define FIRMWARE_RESOURCE_CACHE_LIMIT (512 * 1024)

void firamware_resource_set(const char *ui_path)
{
    if (firamware_resource_context.ui_path)
    {
        lv_mem_free(firamware_resource_context.ui_path);
    }
    firamware_resource_context.ui_path = lv_mem_alloc(strlen(ui_path) + 1);
    memset(firamware_resource_context.ui_path, 0, strlen(ui_path) + 1);
    strcpy(firamware_resource_context.ui_path, ui_path);
}

static char *find_firamware_resource(firamware_resource *resource)
{
    firamware_resource_map *node = firamware_resource_context.head;
    while (node)
    {
        if ((node->d.offset == resource->offset) && (node->d.size == resource->size))
        {
            node->isfree = 0; /* 命中即视为正被引用，防止被预算回收 */
            return node->d.data;
        }
        node = node->next;
    }
    return NULL;
}
static int firamware_resource_remove(firamware_resource_map *dst)
{
    for (firamware_resource_map *node = firamware_resource_context.head, *prev = NULL; node; prev = node, node = node->next)
    {
        if (node == dst)
        {
            // 第一个
            if (prev == NULL)
            {
                firamware_resource_context.head = node->next;
            }
            else
            {
                prev->next = node->next;
            }
            if (node->d.data)
            {
                lv_mem_free(node->d.data);
                node->d.data = NULL;
            }
            /* 在 lv_mem_free(node) 之前读 size/递减，否则是释放后访问(UAF) */
            firamware_resource_context.used_bytes -= (int)node->d.size;
            firamware_resource_context.used_cnt--;
            lv_mem_free(node);
            break;
        }
    }
    return 0;
}
static firamware_resource_map *firamware_resource_tail(void)
{
    firamware_resource_map *node = firamware_resource_context.head;
    if (node == NULL)
    {
        return NULL;
    }
    while (node->next)
    {
        node = node->next;
    }
    return node;
}

/* 缓存超预算时逐出最老的 isfree 节点，直到能容下 need 字节 */
static void firamware_resource_evict(unsigned long need)
{
    while ((long)firmware_resource_cached_bytes() + (long)need > (long)FIRMWARE_RESOURCE_CACHE_LIMIT)
    {
        firamware_resource_map *victim = NULL;
        for (firamware_resource_map *node = firamware_resource_context.head; node; node = node->next)
        {
            if (node->isfree)
            {
                victim = node;
                break;
            }
        }
        if (victim == NULL)
        {
            break; /* 全部正被引用，无可回收 */
        }
        firamware_resource_remove(victim);
    }
}

static bool firamware_resource_insert(void *data, unsigned long offset, unsigned long size)
{
    firamware_resource_map *node = lv_mem_alloc(sizeof(firamware_resource_map));
    node->next = NULL;
    node->d.data = data;
    node->d.offset = offset;
    node->d.size = size;
    node->isfree = 0;
    firamware_resource_map *tail = firamware_resource_tail();
    if (tail == NULL)
    {
        firamware_resource_context.head = node;
    }
    else
    {
        tail->next = node;
    }
    firamware_resource_context.used_bytes += size;
    firamware_resource_context.used_cnt++;
    // AMY_LOG_INFO("firmware insert resource :%dbytes cnt:%d\n", firamware_resource_context.used_bytes,firamware_resource_context.used_cnt);
    return true;
}

static char *firamware_resource_read(firamware_resource *resource)
{
    char *data = lv_mem_alloc(resource->size);
    if (!data)
    {
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        AMY_LOG_ERROR("resource alloc:%u failed, used:%u free:%u biggest_free:%u frag:%u%% free_cnt:%u\n",
                      (unsigned)resource->size,
                      (unsigned)(mon.total_size - mon.free_size),
                      (unsigned)mon.free_size,
                      (unsigned)mon.free_biggest_size,
                      (unsigned)mon.frag_pct,
                      (unsigned)mon.free_cnt);
        return NULL;
    }

    // AMY_LOG_INFO("read %s offset:%lu,size:%lu\n", firamware_resource_context.ui_path, resource->offset, resource->size);
    int fd = open(firamware_resource_context.ui_path, O_RDONLY);
    if (fd < 0)
    {
        AMY_LOG_ERROR("resource open:%s failed \n", firamware_resource_context.ui_path);
        lv_mem_free(data);
        return NULL;
    }
    lseek(fd, resource->offset, SEEK_SET);
    read(fd, data, resource->size);
    close(fd);
    return data;
}

char *firmware_read_resouce_bin(firamware_resource *resource)
{
    char *data = find_firamware_resource(resource);
    if (!data)
    {
        /* 必须先逐出旧缓存再分配新 buffer：若等 lv_mem_alloc 失败后才回收，
         * 分配早已失败，回收永远等不到触发(堆满死锁)。 */
        firamware_resource_evict(resource->size);
        data = firamware_resource_read(resource);
        if (data)
        {
            firamware_resource_insert(data, resource->offset, resource->size);
        }
    }
    // AMY_LOG_INFO("read offset:%lu,size:%lu data=%#x\n", resource->offset, resource->size, data);
    return data;
}

void firmware_resource_destroy(void)
{
    firamware_resource_map *node = firamware_resource_context.head;
    while (node)
    {
        node->d.offset = node->d.size = 0;
        if (node->d.data)
        {
            lv_mem_free(node->d.data);
            node->d.data = NULL;
        }
        firamware_resource_map *temp = node->next;
        node->next = NULL;
        firamware_resource_context.used_bytes -= node->d.size;
        AMY_LOG_INFO("firmware resource :%dbytes\n", firamware_resource_context.used_bytes);
        lv_mem_free(node);
        node = temp;
    }
    firamware_resource_context.head = NULL;
}

void firmware_resource_release(void *data)
{
    // firamware_resource_map* prev = NULL;
    for (firamware_resource_map *node = firamware_resource_context.head; node; node = node->next)
    {
        // AMY_LOG_ERROR("find data:%p node:%p\n", data, node->d.data);
        if (node->d.data == data)
        {
            node->isfree = 1;
            break;
        }
    }
}

int firmware_resource_cached_bytes(void)
{
    int total = 0;
    for (firamware_resource_map *node = firamware_resource_context.head; node; node = node->next)
    {
        total += (int)node->d.size;
    }
    return total;
}
