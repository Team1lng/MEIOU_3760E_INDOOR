#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lv_firmware.h"


int firmware_parse_label_config(lv_obj_t* screen, const char* line) {
    char* widget_name = firmware_parse_string(line, "name=");
    if (!widget_name) {
        return -1;
    }

    char* parent_name = firmware_parse_string(line, "parent=");
    if (!parent_name) {
        lv_mem_free(widget_name);
        return -1;
    }


    lv_obj_t* parent = lv_obj_get_by_id(screen, parent_name);
    if (!parent) {
        lv_mem_free(parent_name);
        lv_mem_free(widget_name);
        return -1;
    }
    lv_mem_free(parent_name);

    //以label作为btn控件比较合适，因为这里会渲染图片和文字，但是btn是没有label的。
    lv_label_t* label = (lv_label_t*)lv_label_create(parent);
    lv_obj_set_id((lv_obj_t*)label, widget_name);
    //  LV_LOG_ERROR("create button:%s\n",widget_name);
    lv_mem_free(widget_name);

    unsigned long x, y;
    if (firmware_parse_range(line, "pos=", &x, &y) == 0) {
        lv_obj_set_pos((lv_obj_t*)label, x, y);
    }

    unsigned long width, height;
    if (firmware_parse_range(line, "size=", &width, &height) == 0) {
        lv_obj_set_size((lv_obj_t*)label, width, height);
    }

    char* next,*extract;
    unsigned long extract_value;
    char* default_string = firmware_parse_string(line, "DEFALUT=");
    if (default_string) {
        //default text
        next = firmware_extract_between_to_string(default_string,NULL,":",&extract);
        lv_label_set_text((lv_obj_t*)label, extract);

        //default font color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_text_color((lv_obj_t*)label, lv_color_hex(extract_value), LV_PART_MAIN);
        //default font size
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);

        //default aligin
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_text_align((lv_obj_t*)label, qt_aligin_to_lvgl_text_aligin(extract_value), LV_PART_MAIN);

        //bg color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)label, lv_color_hex(extract_value), LV_PART_MAIN);
        lv_obj_set_style_bg_opa((lv_obj_t*)label, extract_value >> 24, LV_PART_MAIN);


        //bg image offset
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long offset = extract_value;
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long  size = extract_value;
        if ((offset >= 0) && (size > 0)) {
            firamware_resource_init(data, offset, size);
            lv_obj_set_style_bg_img_src((lv_obj_t*)label, &data, LV_PART_MAIN);
        }

        //bg image aligin
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_img_align((lv_obj_t*)label, qt_aligin_to_lvgl_aligin(extract_value), LV_PART_MAIN);

        // border
       next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_side((lv_obj_t*)label, extract_value, LV_PART_MAIN);

        //border color
         next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_color((lv_obj_t*)label, lv_color_hex(extract_value), LV_PART_MAIN);
        lv_obj_set_style_border_opa((lv_obj_t*)label, extract_value >> 24, LV_PART_MAIN);

        //border width
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_width((lv_obj_t*)label, extract_value, LV_PART_MAIN);

        //radius
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_radius((lv_obj_t*)label, extract_value, LV_PART_MAIN);

        lv_mem_free(default_string);
    }

    firmware_parse_event_process(screen, (lv_obj_t*)label, line);
    return 0;
}
