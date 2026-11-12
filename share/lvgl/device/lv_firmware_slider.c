#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lv_firmware.h"
enum{
    VAL_SHOW_NONE = 0,
    VAL_SHOW_IN_VAL,
    VAL_SHOW_HEADER,
    VAL_SHOW_TAILL
};


static void bar_draw_value_cb(lv_event_t* e){
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part != LV_PART_INDICATOR) return;

    lv_obj_t * obj = lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%s", (int)lv_bar_get_value(obj),lv_bar_get_value_show_type(obj)?"%":"");

    lv_point_t txt_size;
    label_dsc.font = lv_obj_get_style_text_font(obj,LV_PART_INDICATOR);
    label_dsc.letter_space = lv_obj_get_style_text_letter_space(obj,LV_PART_INDICATOR);
    label_dsc.line_space = lv_obj_get_style_text_line_space(obj,LV_PART_INDICATOR);
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    uint32_t value_color = lv_bar_get_value_font_color(obj);
    label_dsc.color =  lv_color_hex(value_color);
    label_dsc.opa = value_color>>24;

    int border_width = 0;
    lv_area_t txt_area;
    int opa = lv_obj_get_style_opa(obj,LV_PART_KNOB);
    if(opa){
         border_width = lv_obj_get_style_border_width(obj,LV_PART_KNOB);
     }

    if(lv_bar_is_horizontal(obj)){
        int pad_width = lv_obj_get_style_pad_left(obj,LV_PART_KNOB);
        int r = opa?(lv_obj_get_height(obj)/2):0;
        txt_area.x2 = dsc->draw_area->x2 + r + border_width +pad_width + txt_size.x;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;

        txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
        txt_area.y2 = txt_area.y1 + txt_size.y - 1;
    }
    else{
        int pad_height = lv_obj_get_style_pad_top(obj,LV_PART_KNOB);
        int r = opa?(lv_obj_get_width(obj)/2):0;

        txt_area.y2 = dsc->draw_area->y1;
        txt_area.y1 = txt_area.y2 - (r + border_width +pad_height + txt_size.y);

        txt_area.x1 = dsc->draw_area->x1 + (lv_area_get_width(dsc->draw_area) - txt_size.x) / 2;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
    }

    /*If the indicator is long enough put the text inside on the right*/

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}


int firmware_parse_slider_config(lv_obj_t* screen, const char* line) {
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

    lv_slider_t* slider = (lv_slider_t*)lv_slider_create(parent);
    lv_obj_set_id((lv_obj_t*)slider, widget_name);
    lv_mem_free(widget_name);

    unsigned long x, y;
    if (firmware_parse_range(line, "pos=", &x, &y) == 0) {
        lv_obj_set_pos((lv_obj_t*)slider, x, y);
    }

    unsigned long width, height;
    if (firmware_parse_range(line, "size=", &width, &height) == 0) {
        lv_obj_set_size((lv_obj_t*)slider, width, height);
    }

    unsigned long min,max;
    if (firmware_parse_range(line, "valrange=", &min, &max) == 0) {
        lv_slider_set_range((lv_obj_t*)slider,min,max);
    }

    char* next,*extract;
    unsigned long extract_value;
    char* val_show_string = firmware_parse_string(line, "VALSHOW=");
    if (val_show_string) {
        //当前数值
        next = firmware_extract_between_to_value(val_show_string,NULL,":",&extract_value,10);
        lv_slider_set_value((lv_obj_t*)slider,extract_value,LV_ANIM_OFF);

        //显示位置
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_bar_set_value_show_pos((lv_obj_t*)slider,extract_value);

        //显示方式
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_bar_set_value_show_type((lv_obj_t*)slider,extract_value);

        //字体颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_bar_set_value_font_color((lv_obj_t*)slider,extract_value);

        //字体大小
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_bar_set_value_font_size((lv_obj_t*)slider,extract_value);

        lv_mem_free(val_show_string);
    }

    char* style_string = firmware_parse_string(line, "STYLE=");
    if (style_string) {
        //pad width
        next = firmware_extract_between_to_value(style_string,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_left((lv_obj_t*)slider,extract_value,LV_PART_MAIN);
        lv_obj_set_style_pad_right((lv_obj_t*)slider,extract_value,LV_PART_MAIN);

        //pad height
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_top((lv_obj_t*)slider,extract_value,LV_PART_MAIN);
        lv_obj_set_style_pad_bottom((lv_obj_t*)slider,extract_value,LV_PART_MAIN);

        //background color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)slider,lv_color_hex(extract_value),LV_PART_MAIN);
        lv_obj_set_style_bg_opa((lv_obj_t*)slider,extract_value>>24,LV_PART_MAIN);

        //force color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)slider,lv_color_hex(extract_value),LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa((lv_obj_t*)slider,extract_value>>24,LV_PART_INDICATOR);

        //bg image
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long offset = extract_value;
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long size = extract_value;
        //从flash读取显示 lv_obj_set_style_image_src
        if ((offset >= 0) && (size > 0)) {
            firamware_resource_init(data, offset, size);
            lv_obj_set_style_bg_img_src((lv_obj_t*)slider, &data, LV_PART_MAIN);
        }

        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_img_align((lv_obj_t*)slider, qt_aligin_to_lvgl_aligin(extract_value), LV_PART_MAIN);

        //border color
       // lv_obj_set_style_border_side((lv_obj_t*)slider,LV_BORDER_SIDE_FULL,LV_PART_MAIN);
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_outline_color((lv_obj_t*)slider, lv_color_hex(extract_value), LV_PART_MAIN);
        lv_obj_set_style_outline_opa((lv_obj_t*)slider, extract_value >> 24, LV_PART_MAIN);

        //border width
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_outline_width((lv_obj_t*)slider, extract_value, LV_PART_MAIN);

        //radius
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_radius((lv_obj_t*)slider, extract_value, LV_PART_MAIN);
        lv_obj_set_style_radius((lv_obj_t*)slider, extract_value, LV_PART_INDICATOR);

        lv_mem_free(style_string);
    }


    char* slider_string = firmware_parse_string(line, "SLIDER=");
    if (slider_string) {
        //滑块是否隐藏
        next = firmware_extract_between_to_value(slider_string,NULL,":",&extract_value,10);
        if(extract_value){
            lv_obj_set_style_opa((lv_obj_t*)slider,0,LV_PART_KNOB);
        }

        //补偿宽度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_left((lv_obj_t*)slider,extract_value,LV_PART_KNOB);
        lv_obj_set_style_pad_right((lv_obj_t*)slider,extract_value,LV_PART_KNOB);

        //pad height
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_top((lv_obj_t*)slider,extract_value,LV_PART_KNOB);
        lv_obj_set_style_pad_bottom((lv_obj_t*)slider,extract_value,LV_PART_KNOB);

        //bg color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)slider,lv_color_hex(extract_value),LV_PART_KNOB);
        lv_obj_set_style_bg_opa((lv_obj_t*)slider,extract_value>>24,LV_PART_KNOB);

        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long offset = extract_value;
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        unsigned long size = extract_value;
        //从flash读取显示 lv_obj_set_style_image_src
        if ((offset >= 0) && (size > 0)) {
            firamware_resource_init(data, offset, size);
            lv_obj_set_style_bg_img_src((lv_obj_t*)slider, &data, LV_PART_KNOB);
        }

        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_img_align((lv_obj_t*)slider, qt_aligin_to_lvgl_aligin(extract_value), LV_PART_KNOB);

        //border color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_side((lv_obj_t*)slider,LV_BORDER_SIDE_FULL,LV_PART_KNOB);
        lv_obj_set_style_border_color((lv_obj_t*)slider, lv_color_hex(extract_value), LV_PART_KNOB);
        lv_obj_set_style_border_opa((lv_obj_t*)slider, extract_value >> 24, LV_PART_KNOB);

        //border width
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_width((lv_obj_t*)slider, extract_value, LV_PART_KNOB);

        //radius
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_radius((lv_obj_t*)slider, extract_value, LV_PART_KNOB);
    }

    if(lv_bar_get_value_show_pos((lv_obj_t*)slider) != VAL_SHOW_NONE){
        lv_obj_add_event_cb((lv_obj_t*)slider,bar_draw_value_cb,LV_EVENT_DRAW_PART_END,NULL);
    }

    firmware_parse_event_process(screen, (lv_obj_t*)slider, line);
    return 0;
}
