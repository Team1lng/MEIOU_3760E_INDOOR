#ifndef LV_FIRMWARE_BAR_H_
#define LV_FIRMWARE_BAR_H_
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

int firmware_parse_bar_config(lv_obj_t* screen, const char* line);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // _PARSE_LVGL_H
