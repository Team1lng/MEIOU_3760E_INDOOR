#ifndef LV_FIRMWARE_RESOURCE_H_
#define LV_FIRMWARE_RESOURCE_H_
#include "lvgl.h"
#ifdef __cplusplus
extern "C"
{
#endif

    char* firmware_read_resouce_bin(firamware_resource* resource);

    void firmware_resource_destroy(void);

    void firamware_resource_set(const char*);

    void firmware_resource_release(void* data);

    int firmware_resource_cached_bytes(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // _PARSE_LVGL_H
