#ifndef LV_FIRMWARE_BUTTON_H_
#define LV_FIRMWARE_BUTTON_H_
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

int firmware_parse_btn_config(lv_obj_t* screen, const char* line);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // _PARSE_LVGL_H
