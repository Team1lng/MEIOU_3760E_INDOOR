#ifndef _PARSE_LVGL_H
#define _PARSE_LVGL_H

#include "lvgl.h"

//#include "fal_def.h"
//#include "fal.h"
#ifdef __cplusplus
extern "C" {
#endif

#define firamware_resource_init(img,offset,size) \
firamware_resource img = { {'S','@'},  offset,  size};

int firmware_parse_event_process(lv_obj_t* screen, lv_obj_t* obj, const char* line);


firamware_resource* firamware_resource_alloc(unsigned long offset, unsigned long size);

int firmware_parse_range(const char* src_str, const char* prefix, unsigned long* out_min, unsigned long* out_max);

char* firmware_parse_string(const char* src_str, const char* prefix);

int firmware_parse_value(const char* src_str, const char* prefix, unsigned long* out_value,int base);

char* firmware_extract_between_to_value(char* str,const char* start,const char* end,unsigned long* extract,int base);

char* firmware_extract_between_to_string(char* str,const char* start,const char* end,char** extract);

char* firmware_alloc_extract_between(const char* str, const char* start, const char* end);

int parse_lvgl_config(const struct fal_partition *part_dev);

lv_text_align_t qt_aligin_to_lvgl_text_aligin(unsigned int qt_aligin);

lv_align_t qt_aligin_to_lvgl_aligin(unsigned int qt_aligin);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // _PARSE_LVGL_H
