#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lv_firmware.h"


#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
struct system_time {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int milliseconds;
};
static void get_system_time(struct system_time *st) {
#ifdef _WIN32
    // Windows实现
    SYSTEMTIME winTime;
    GetLocalTime(&winTime);

    st->year = winTime.wYear;
    st->month = winTime.wMonth;
    st->day = winTime.wDay;
    st->hour = winTime.wHour;
    st->minute = winTime.wMinute;
    st->second = winTime.wSecond;
    st->milliseconds = winTime.wMilliseconds;
#else
    // Linux/Unix实现
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = localtime(&tv.tv_sec);

    st->year = tm->tm_year + 1900;
    st->month = tm->tm_mon + 1;
    st->day = tm->tm_mday;
    st->hour = tm->tm_hour;
    st->minute = tm->tm_min;
    st->second = tm->tm_sec;
    st->milliseconds = tv.tv_usec / 1000;
#endif
}

static void clock_refresh(lv_timer_t* t){
    lv_obj_t* obj = (lv_obj_t*)t->user_data;
    struct system_time time;
    get_system_time(&time);
    //时针
    lv_meter_indicator_t*  indic = lv_meter_needler_get_by_flag(obj,1);
    if(indic){
        int value =  ((time.hour%12)*60*60+ time.minute*60 + time.second);//lv_map((time.hour*60 + time.minute)%60,0,59,0,60);
        value = lv_map(value,0,43200,0,60);
        lv_meter_set_indicator_value(obj,indic,value);
    }

    //分针
    indic = lv_meter_needler_get_by_flag(obj,2);
    if(indic){
        int value =  (time.minute*60 + time.second);
         value = lv_map(value,0,3600,0,60);
        lv_meter_set_indicator_value(obj,indic,value);
    }

    //秒针
    indic = lv_meter_needler_get_by_flag(obj,3);
    if(indic){
        int value = time.second;
        lv_meter_set_indicator_value(obj,indic,value);
    }
}


int firmware_parse_clock_config(lv_obj_t* screen, const char* line) {

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

    lv_meter_scale_t* min_scale = lv_meter_add_scale((lv_obj_t*)meter);
    lv_meter_set_scale_ticks((lv_obj_t*)meter, min_scale, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range((lv_obj_t*)meter, min_scale, 0, 60, 360, 270);


    lv_meter_scale_t *hour_scale = lv_meter_add_scale((lv_obj_t*)meter);
    lv_meter_set_scale_ticks((lv_obj_t*)meter, hour_scale, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks((lv_obj_t*)meter, hour_scale, 1, 2, 20, lv_color_black(), 10);
    lv_meter_set_scale_range((lv_obj_t*)meter, hour_scale, 1, 12, 330, 300);

    //表盘
    char* next,*extract;
    unsigned long extract_value;
    char* scale_string = firmware_parse_string(line, "SCALE=");
    if (scale_string) {
        //周围填充宽度
        next = firmware_extract_between_to_value(scale_string,NULL,":",&extract_value,10);
        lv_obj_set_style_pad_all((lv_obj_t*)meter,extract_value,LV_PART_MAIN);
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
        //刻度宽度
        next = firmware_extract_between_to_value(titck_string,NULL,":",&extract_value,10);
        min_scale->tick_width = extract_value;
        // hour_scale->tick_width = extract_value;
        //刻度长度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        min_scale->tick_length = extract_value;
        //  hour_scale->tick_length = extract_value;
        //刻度颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        min_scale->tick_color = lv_color_hex(extract_value);
        hour_scale->tick_color = lv_color_hex(extract_value);
        lv_mem_free(titck_string);
    }

    //主刻度
    char* major_string = firmware_parse_string(line, "MAJOR=");
    if (major_string) {
        //宽度
        next = firmware_extract_between_to_value(major_string,NULL,":",&extract_value,10);
        // min_scale->tick_major_width = extract_value;
        hour_scale->tick_major_width = extract_value;
        //长度
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        //     min_scale->tick_major_length = extract_value;
        hour_scale->tick_major_length = extract_value;
        //颜色
        next = firmware_extract_between_to_value(next,NULL,":",&extract_value,10);
        min_scale->tick_major_color = lv_color_hex(extract_value);
        hour_scale->tick_major_color = lv_color_hex(extract_value);
        lv_mem_free(scale_string);
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
        hour_scale->label_gap = extract_value;
        lv_mem_free(text_string);
    }

    //时针
    char* needle_string = firmware_parse_string(line, "HOURNEEDLE=");
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
                lv_meter_indicator_t* indic  =lv_meter_add_needle_img((lv_obj_t*)meter, min_scale, &data, pivot_x, pivot_y);
                indic->user_flag = 1;
                // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);
            }
        }
        else{
            //宽度
            lv_meter_indicator_t* indic  = lv_meter_add_needle_line((lv_obj_t*)meter,min_scale,0,lv_color_hex(0),0);
            indic->user_flag = 1;

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
            // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);

        }
        lv_mem_free(needle_string);
    }



    //分针
    needle_string = firmware_parse_string(line, "MINNEEDLE=");
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
                lv_meter_indicator_t* indic  =lv_meter_add_needle_img((lv_obj_t*)meter, min_scale, &data, pivot_x, pivot_y);
                indic->user_flag = 2;
                // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);
            }
        }
        else{
            //宽度
            lv_meter_indicator_t* indic  = lv_meter_add_needle_line((lv_obj_t*)meter,min_scale,0,lv_color_hex(0),0);
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
            indic->user_flag = 2;
            // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);

        }
        lv_mem_free(needle_string);
    }

    //秒针
    needle_string = firmware_parse_string(line, "SECNEEDLE=");
    if (needle_string) {
        //图片
        next = firmware_extract_between_to_value(needle_string,NULL,":",&extract_value,10);
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
                lv_meter_indicator_t* indic  =lv_meter_add_needle_img((lv_obj_t*)meter, min_scale, &data, pivot_x, pivot_y);
                indic->user_flag = 3;
                // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);
            }
        }
        else{
            //宽度
            lv_meter_indicator_t* indic  = lv_meter_add_needle_line((lv_obj_t*)meter,min_scale,0,lv_color_hex(0),0);
            indic->user_flag = 3;

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

            // lv_meter_set_indicator_value((lv_obj_t*)meter,indic,cur_value);

        }
        lv_mem_free(needle_string);
    }


    //圆点
    char* dot_string = firmware_parse_string(line, "DOT=");
    if (dot_string) {
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

    lv_timer_create(clock_refresh,1000,meter);

    firmware_parse_event_process(screen, (lv_obj_t*)meter, line);
    return 0;
}
