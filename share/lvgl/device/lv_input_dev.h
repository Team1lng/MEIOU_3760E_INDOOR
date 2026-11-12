#ifndef LV_INPUT_DEV_H
#define LV_INPUT_DEV_H
#include "src/hal/lv_hal_indev.h"
#ifdef __cplusplus
extern "C" {
#endif


lv_indev_t * lv_input_device_init(void(*callback)(lv_indev_drv_t* disp_drv,lv_indev_data_t* data),void* user_data);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // LV_DISP_DEV_H
