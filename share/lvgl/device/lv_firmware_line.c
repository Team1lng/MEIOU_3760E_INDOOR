#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lv_firmware.h"


int firmware_parse_line_config(lv_obj_t* screen, const char* line) {
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
    lv_line_t* line_obj = (lv_line_t*)lv_line_create(parent);
    lv_obj_set_id((lv_obj_t*)line_obj, widget_name);
    //  LV_LOG_ERROR("create button:%s\n",widget_name);
    lv_mem_free(widget_name);

    unsigned long x1, y1;
    if (firmware_parse_range(line, "pos1=", &x1, &y1)) {
        return -1;
    }

    unsigned long x2, y2;
    if (firmware_parse_range(line, "pos2=", &x2, &y2)) {
        return -1;
    }

    char* next,*extract;
    unsigned long extract_value;
    char* default_string = firmware_parse_string(line, "DEFALUT=");
    if (default_string) {
        next = firmware_extract_between_to_value(default_string,NULL,":",&extract_value,10);
        unsigned int line_width = extract_value;
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned int color = extract_value;
       // LV_LOG_ERROR("line width:%d,color=%d,%lu,%lu %lu,%lu\n",line_width,color,x1,y1,x2,y2);
        lv_point_t* point = lv_mem_alloc(sizeof(lv_point_t)*2);
        point[0].x = x1;
        point[0].y = y1;
        point[1].x = x2;
        point[1].y = y2;
        lv_line_set_points((lv_obj_t*)line_obj,point,2);
        lv_obj_set_style_line_color((lv_obj_t*)line_obj,lv_color_hex(color),LV_PART_MAIN);
        lv_obj_set_style_line_opa((lv_obj_t*)line_obj,color>>24,LV_PART_MAIN);
        lv_obj_set_style_line_width((lv_obj_t*)line_obj,line_width,LV_PART_MAIN);

        lv_mem_free(default_string);
    }
    firmware_parse_event_process(screen, (lv_obj_t*)line_obj, line);
    return 0;
}
