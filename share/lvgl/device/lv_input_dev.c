#include "lv_input_dev.h"
#include "Amy_log.h"
#include "src/hal/lv_hal_indev.h"

static void (*lv_input_device_handler)(lv_indev_drv_t* disp_drv, lv_indev_data_t* data) = NULL;

static void lv_indev_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (lv_input_device_handler != NULL) {
        lv_input_device_handler(drv, data);
    }
}





lv_indev_t* lv_input_device_init(void(*callback)(lv_indev_drv_t* disp_drv, lv_indev_data_t* data), void* user_data) {

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_indev_read_cb;
    indev_drv.user_data = user_data;

    lv_input_device_handler = callback;
    return lv_indev_drv_register(&indev_drv);
}
