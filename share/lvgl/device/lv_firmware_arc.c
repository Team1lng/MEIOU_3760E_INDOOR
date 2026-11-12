#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lv_firmware.h"

int firmware_parse_meter_config(lv_obj_t* screen, const char* line) {

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

    lv_meter_t* meter = (lv_meter_t*)lv_meter_create(parent);
    lv_obj_set_id((lv_obj_t*)meter, widget_name);
    lv_mem_free(widget_name);

    unsigned long x, y;
    if (firmware_parse_range(line, "pos=", &x, &y) == 0) {
        lv_obj_set_pos((lv_obj_t*)meter, x, y);
    }

    unsigned long value;
    if (firmware_parse_value(line, "width=", &value,10) != 0) {
        lv_obj_del((lv_obj_t*)meter);
        return -1;
    }
    lv_obj_set_size((lv_obj_t*)meter,value,value);

    if(firmware_parse_value(line,"hide=",&value,10) == 0){
        if(value){
            lv_obj_add_flag((lv_obj_t*)meter,LV_OBJ_FLAG_HIDDEN);
        }
        else{
            lv_obj_clear_flag((lv_obj_t*)meter,LV_OBJ_FLAG_HIDDEN);
        }
    }

    if(firmware_parse_value(line,"disable=",&value,10) == 0){
        if(value){
            lv_obj_add_state((lv_obj_t*)meter,LV_STATE_DISABLED);
        }
        else{
            lv_obj_clear_state((lv_obj_t*)meter,LV_STATE_DISABLED);
        }
    }

    unsigned long cur_value = 0;
    lv_meter_scale_t* scale = lv_meter_add_scale((lv_obj_t*)meter);


    //表盘
    char* next,*extract;
    unsigned long extract_value;
    char* scale_string = firmware_parse_string(line, "SCALE=");
    if (scale_string) {
        //旋转方向
        next = firmware_extract_between_to_value(scale_string,NULL,":",&extract_value,10);
        scale->rotation_direction = extract_value;
        //周围填充宽度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_all((lv_obj_t*)meter,extract_value,LV_PART_MAIN);
        //当前值
        next = firmware_extract_between_to_value(next,NULL,":",&cur_value,10);
        //最小值
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->min = extract_value;
        //最大值
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->max = extract_value;

        //背景颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)meter,lv_color_hex(extract_value),LV_PART_MAIN);
        lv_obj_set_style_bg_opa((lv_obj_t*)meter,extract_value>>24,LV_PART_MAIN);

        unsigned long offset=0;
        next = firmware_extract_between_to_value(next,NULL,":",&offset,10);
        unsigned long size =0;
        next = firmware_extract_between_to_value(next,NULL,":",&size,10);
        //从flash读取显示 lv_obj_set_style_image_src
        if ((offset >= 0) && (size > 0)) {
            firamware_resource_init(data, offset, size);
            lv_obj_set_style_bg_img_src((lv_obj_t*)meter, &data, LV_PART_MAIN);
        }


        // border
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_side((lv_obj_t*)meter, extract_value, LV_PART_MAIN);
        //border color
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_color((lv_obj_t*)meter, lv_color_hex(extract_value), LV_PART_MAIN);
        lv_obj_set_style_border_opa((lv_obj_t*)meter, extract_value >> 24, LV_PART_MAIN);
        //border width
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_border_width((lv_obj_t*)meter, extract_value, LV_PART_MAIN);
        //radius
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_radius((lv_obj_t*)meter, extract_value, LV_PART_MAIN);

        lv_mem_free(scale_string);
    }


    char* titck_string = firmware_parse_string(line, "TITCK=");
    if (titck_string) {
        //刻度总个数
        next = firmware_extract_between_to_value(titck_string,NULL,":",&extract_value,10);
        scale->tick_cnt = extract_value;
        //刻度宽度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_width = extract_value;
        //刻度长度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_length = extract_value;
        //刻度颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_color = lv_color_hex(extract_value);
        //刻度角度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->rotation = extract_value;
        //刻度角度范围
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->angle_range = extract_value;

        lv_mem_free(titck_string);
    }

    //主刻度
    char* major_string = firmware_parse_string(line, "MAJOR=");
    if (major_string) {
        //间隔
        next = firmware_extract_between_to_value(major_string,NULL,":",&extract_value,10);
        scale->tick_major_nth = extract_value;
        //宽度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_major_width = extract_value;
        //长度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_major_length = extract_value;
        //颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->tick_major_color = lv_color_hex(extract_value);

        lv_mem_free(major_string);
    }


    //文本
    char* text_string = firmware_parse_string(line, "TEXT=");
    if (text_string) {
        //是否隐藏
        next = firmware_extract_between_to_value(text_string,NULL,":",&extract_value,10);
        lv_obj_set_style_text_opa((lv_obj_t*)meter,extract_value?LV_OPA_TRANSP:LV_OPA_COVER,LV_PART_TICKS);
        //字体大小
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        //字体颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_text_color((lv_obj_t*)meter,lv_color_hex(extract_value),LV_PART_TICKS);
        //字体与刻度的距离
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        scale->label_gap = extract_value;
        LV_LOG_ERROR("meter label gap:%d %lu\n",scale->label_gap,extract_value);

        lv_mem_free(text_string);
    }

    //指针
    char* needle_string = firmware_parse_string(line, "NEEDLE=");
    if (needle_string) {
        //类型
        next = firmware_extract_between_to_value(needle_string,NULL,":",&extract_value,10);
        //图片
        if(extract_value){
            unsigned long offset=0;
            next = firmware_extract_between_to_value(next,NULL,":",&offset,10);
            unsigned long size =0;
            next = firmware_extract_between_to_value(next,NULL,":",&size,10);
            if ((offset >= 0) && (size > 0)) {
                firamware_resource_init(data, offset, size);

                //中心X轴坐标
                unsigned long pivot_x = 0;
                next = firmware_extract_between_to_value(next,NULL,":",&pivot_x,10);
                //中心Y轴坐标
                unsigned long pivot_y = 0;
                next = firmware_extract_between_to_value(next,NULL,":",&pivot_y,10);
                lv_meter_indicator_t* indic  =lv_meter_add_needle_img((lv_obj_t*)meter, scale, &data, pivot_x, pivot_y);
                 lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);
            }
        }
        else{
            //宽度
            lv_meter_indicator_t* indic  = lv_meter_add_needle_line((lv_obj_t*)meter,scale,0,lv_color_hex(0),0);
            next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
            indic->type_data.needle_line.width = extract_value;
            //颜色
            next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
            indic->type_data.needle_line.color = lv_color_hex(extract_value);
            //针头
            next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
            indic->type_data.needle_line.r_mod = extract_value;
            //针尾
            next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
            indic->type_data.needle_line.tail_length = extract_value;

            lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);

        }
        lv_mem_free(needle_string);
    }
    //圆点
    char* dot_string = firmware_parse_string(line, "DOT=");
    if (dot_string) {
        lv_obj_set_style_pad_all((lv_obj_t*)meter,0,LV_PART_INDICATOR);
        //宽度
        next = firmware_extract_between_to_value(dot_string,NULL,":",&extract_value,10);
        lv_obj_set_style_width((lv_obj_t*)meter,extract_value,LV_PART_INDICATOR);
        //长度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_height((lv_obj_t*)meter,extract_value,LV_PART_INDICATOR);
        //颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_bg_color((lv_obj_t*)meter,lv_color_hex(extract_value),LV_PART_INDICATOR);
        //角度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        lv_obj_set_style_radius((lv_obj_t*)meter,extract_value,LV_PART_INDICATOR);

        lv_mem_free(dot_string);
    }

    //圆弧
    char* arc_string = NULL;
    char* scale_arc_start = strstr(line,"SCALEARC=");
    while((scale_arc_start != NULL)&&((arc_string = firmware_parse_string(scale_arc_start, "SCALEARC=")) != NULL)){
        lv_meter_indicator_t* indic  = lv_meter_add_arc((lv_obj_t*)meter,scale,0,lv_color_hex(0),0);

        //起始数据
        next = firmware_extract_between_to_value(arc_string,NULL,":",&extract_value,10);
        indic->start_value = extract_value;
        //结束数据
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->end_value = extract_value;
        //圆弧宽度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.arc.width = extract_value;
        //圆弧颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.arc.color = lv_color_hex(extract_value);
        //圆弧修正
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.arc.r_mod = extract_value;

        lv_mem_free(arc_string);
        arc_string = NULL;

        scale_arc_start = strstr(scale_arc_start + strlen("SCALEARC="),"SCALEARC=");
    }

    //刻度
    char* line_string = NULL;
    char* scale_line_start = strstr(line,"SCALELINE=");
    while(((scale_line_start != NULL)) && (line_string = firmware_parse_string(scale_line_start, "SCALELINE=")) != NULL){
        lv_meter_indicator_t* indic  = lv_meter_add_scale_lines((lv_obj_t*)meter,scale,lv_color_hex(0),lv_color_hex(0),false,0);

        //起始数据
        next = firmware_extract_between_to_value(line_string,NULL,":",&extract_value,10);
        indic->start_value = extract_value;
        //结束数据
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->end_value = extract_value;
        //起始颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.scale_lines.color_start = lv_color_hex(extract_value);
        //结束颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.scale_lines.color_end = lv_color_hex(extract_value);
        //是否渐变
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.scale_lines.local_grad = extract_value;
        //修正
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        indic->type_data.scale_lines.width_mod = extract_value;

        lv_mem_free(line_string);
        line_string = NULL;

        scale_line_start = strstr(scale_line_start + strlen("SCALELINE="),"SCALELINE=");
    }

    firmware_parse_event_process(screen, (lv_obj_t*)meter, line);
    return 0;
}
