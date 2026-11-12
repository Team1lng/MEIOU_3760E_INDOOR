#ifndef _LV_COMMON_H_
#define _LV_COMMON_H_
#include "page_tabel.h"

/**
 * @description: 设置控件基本属性
 **/
#define lv_common_style_set_common(obj, id, x, y, w, h, align, selector) \
    {                                                                    \
        if (id >= 0)                                                     \
            lv_obj_set_child_id(obj, id);                                \
        lv_obj_set_style_x(obj, x, selector);                            \
        lv_obj_set_style_y(obj, y, selector);                            \
        lv_obj_set_style_width(obj, w, selector);                        \
        lv_obj_set_style_height(obj, h, selector);                       \
        lv_obj_set_style_align(obj, align, selector);                    \
    }

/**
 * @description: 设置按下基本属性
 **/
#define lv_common_style_set_event(obj, cb, click, default_bg_opa, default_bg_color, default_selector, press_bg_opa, press_bg_color, press_selector) \
    {                                                                                                                                               \
        lv_obj_set_style_bg_opa(obj, default_bg_opa, default_selector);                                                                             \
        if (default_bg_opa > LV_OPA_MIN)                                                                                                            \
        {                                                                                                                                           \
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_bg_color), default_selector);                                                       \
        }                                                                                                                                           \
        lv_obj_set_style_bg_opa(obj, press_bg_opa, press_selector);                                                                                 \
        if (press_bg_opa > LV_OPA_MIN)                                                                                                              \
        {                                                                                                                                           \
            lv_obj_set_style_bg_color(obj, lv_color_hex(press_bg_color), press_selector);                                                           \
        }                                                                                                                                           \
        if (click)                                                                                                                                  \
        {                                                                                                                                           \
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);                                                                                            \
        }                                                                                                                                           \
        else                                                                                                                                        \
        {                                                                                                                                           \
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);                                                                                          \
        }                                                                                                                                           \
        if (cb != NULL)                                                                                                                             \
        {                                                                                                                                           \
            lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, NULL);                                                                                   \
        }                                                                                                                                           \
    }

/**
 * @description: 设置文本基本属性
 **/
#define lv_common_style_set_text(obj, string, font_default_color, font_press_color, align, font, press_part, defalut_part) \
    {                                                                                                                      \
        if (font != NULL)                                                                                                  \
        {                                                                                                                  \
            lv_obj_set_style_text_font(obj, font, defalut_part);                                                           \
        }                                                                                                                  \
        lv_obj_set_style_text_align(obj, align, defalut_part);                                                             \
        lv_obj_set_style_text_color(obj, lv_color_hex(font_default_color), defalut_part);                                  \
        lv_obj_set_style_text_color(obj, lv_color_hex(font_press_color), press_part);                                      \
        if (string != NULL)                                                                                                \
        {                                                                                                                  \
            lv_label_set_text(obj, string);                                                                                \
        }                                                                                                                  \
    }

/**
 * @description: 设置文本框基本属性
 **/
#define lv_common_style_set_textarea(obj, string, font_default_color, font_focused_color, align, font, press_part, defalut_part, max_char) \
    {                                                                                                                                      \
        lv_common_style_set_text(obj, NULL, font_default_color, font_focused_color, align, font, press_part, defalut_part);                \
        lv_textarea_set_one_line(obj, false);                                                                                              \
        lv_textarea_set_max_length(obj, max_char);                                                                                         \
        if (string != NULL)                                                                                                                \
        {                                                                                                                                  \
            lv_textarea_set_text(obj, string);                                                                                             \
        }                                                                                                                                  \
    }

/**
 * @description: 设置图片基本属性
 **/
#define lv_common_style_set_img(obj, img_src, press_img_recolor_opa, press_img_recolor, align, default_part, press_part, press_img_src) \
    {                                                                                                                                   \
        if (img_src != NULL)                                                                                                            \
        {                                                                                                                               \
            lv_obj_set_style_bg_img_src(obj, img_src, default_part);                                                                    \
            lv_obj_set_style_bg_img_align(obj, align, default_part);                                                                    \
            if (press_img_src != NULL)                                                                                                  \
            {                                                                                                                           \
                lv_obj_set_style_bg_img_src(obj, press_img_src, press_part);                                                            \
                lv_obj_set_style_bg_img_align(obj, align, press_part);                                                                  \
            }                                                                                                                           \
            else                                                                                                                        \
            {                                                                                                                           \
                lv_obj_set_style_bg_img_recolor_opa(obj, press_img_recolor_opa, press_part);                                            \
                lv_obj_set_style_bg_img_recolor(obj, lv_color_hex(press_img_recolor), press_part);                                      \
            }                                                                                                                           \
        }                                                                                                                               \
    }

/**
 * @description: 设置填充属性
 **/
#define lv_common_style_set_pad(obj, top, left, buttom, right, column, row, part) \
    {                                                                             \
        lv_obj_set_style_pad_top(obj, top, part);                                 \
        lv_obj_set_style_pad_left(obj, left, part);                               \
        lv_obj_set_style_pad_bottom(obj, buttom, part);                           \
        lv_obj_set_style_pad_right(obj, right, part);                             \
        lv_obj_set_style_pad_column(obj, column, part);                           \
        lv_obj_set_style_pad_row(obj, row, part);                                 \
    }

/**
 * @description: 设置边框属性
 **/
#define lv_common_style_set_boader(obj, radius, boarder_opa, boarder_width, boarder_side, boarder_color, part) \
    {                                                                                                          \
        if (radius >= 0)                                                                                       \
        {                                                                                                      \
            lv_obj_set_style_radius(obj, radius, part);                                                        \
        }                                                                                                      \
        lv_obj_set_style_border_opa(obj, boarder_opa, part);                                                   \
        if (boarder_opa > LV_OPA_MIN)                                                                          \
        {                                                                                                      \
            lv_obj_set_style_border_width(obj, boarder_width, part);                                           \
            lv_obj_set_style_border_side(obj, boarder_side, part);                                             \
            lv_obj_set_style_border_color(obj, lv_color_hex(boarder_color), part);                             \
        }                                                                                                      \
    }

/**
 * @description: 通用按钮创建
 **/
lv_obj_t *lv_common_img_btn_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                   lv_event_cb_t cb, bool click, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                   int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                   int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                   void *img_src, lv_opa_t press_img_recolor_opa, uint32_t press_img_recolor, lv_align_t align,
                                   void *press_img_src);

/**
 * @description: 通用标题文本显示
 **/
lv_obj_t *lv_common_text_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                const char *string, uint32_t font_default_color, uint32_t font_press_color, lv_text_align_t align, const lv_font_t *font);

/**
 * @description: 图片按钮创建
 **/
lv_obj_t *lv_common_img_text_btn_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                        lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                        int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                        int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                        int title_x, int title_y, int title_w, int title_h, int title_id,
                                        const char *title_string, uint32_t title_default_color, uint32_t title_press_color, lv_text_align_t title_align, lv_font_t *title_font,
                                        int img_x, int img_y, int img_w, int img_h, int img_id,
                                        void *img_src, lv_opa_t press_img_recolor_opa, uint32_t press_img_recolor, lv_align_t img_align,
                                        void *press_img_src);

lv_obj_t *lv_common_number_input_keyboard_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                                 lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                                 int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                                 int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                                 uint32_t font_default_color, uint32_t font_press_color, lv_text_align_t align, const lv_font_t *font,
                                                 int row, int column);
;

/**
 * @description: 键盘创建
 **/
lv_obj_t *lv_common_keyboard_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                    lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                    int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                    int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                    int defalut_item_radius, int defalut_item_boarder_width, lv_border_side_t default_item_boarder_side, int boarder_item_default_opa, uint32_t boarde_item_default_color,
                                    int press_item_radius, int press_item_boarder_width, lv_border_side_t press_item_boarder_side, int boarder_item_press_opa, uint32_t boarder_item_press_color,
                                    uint32_t font_default_color, uint32_t font_press_color, lv_text_align_t align, const lv_font_t *font,
                                    int pad_top, int pad_left, int pad_right, int pad_bottom, int pad_column, int pad_row,
                                    lv_opa_t default_parent_bg_opa, uint32_t default_parent_bg_color, lv_opa_t press_parent_bg_opa, uint32_t press_parent_bg_color);

/**
 * @description: 文本输入框
 **/
lv_obj_t *lv_common_textarea_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                    lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                    lv_opa_t default_cursor_opa, uint32_t default_cursor_color, lv_opa_t press_cursor_opa, uint32_t press_cursor_color,
                                    int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                    int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                    const char *string, uint32_t font_default_color, uint32_t font_focused_color, lv_text_align_t align, const lv_font_t *font, int max_char,
                                    int text_offset_y, int anim_time);

/**
 * @description: 滚筒
 **/
lv_obj_t *lv_common_roller_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                  lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                  int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                  int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                  int row_cont, int line_space, int min, int max, uint32_t default_text_color, uint32_t select_text_color, lv_text_align_t text_align, lv_font_t *text_font,
                                  lv_roller_mode_t mode, const void *img_src);

/**
 * @description: 滑块显示
 **/
lv_obj_t *lv_common_slider_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                  lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color,
                                  int bg_radius, int bg_boarder_width, lv_border_side_t bg_boarder_side, int bg_boarder_opa, uint32_t bg_boarder_color,
                                  int slider_radius, int slider_boarder_width, lv_border_side_t slider_boarder_side, int slider_boarder_opa, uint32_t slider_boarder_color,

                                  int text_x, int text_y, int text_w, int text_h, int text_id,
                                  const char *text_string, uint32_t text_font_color, lv_text_align_t text_align, lv_font_t *text_font,

                                  int slider_x, int slider_y, int slider_w, int slider_h, int slider_id,
                                  lv_opa_t slider_bg_opa, uint32_t slider_bg_color, lv_opa_t slider_foure_opa, uint32_t slider_foure_color, lv_event_cb_t cb_slider,

                                  int left_btn_x, int left_btn_y, int left_btn_w, int left_btn_h, int left_btn_id,
                                  const void *left_btn_img_src, lv_opa_t left_btn_press_img_recolor_opa, uint32_t left_btn_press_img_recolor, lv_align_t left_btn_img_align, lv_event_cb_t cb_sub,

                                  int right_btn_x, int right_btn_y, int right_btn_w, int right_btn_h, int right_btn_id,
                                  const void *right_btn_img_src, lv_opa_t right_btn_press_img_recolor_opa, uint32_t right_btn_press_img_recolor, lv_align_t right_btn_img_align, lv_event_cb_t cb_add,

                                  int knob_radius, int knob_size, uint32_t knob_bg_color, lv_opa_t knob_opa, const void *knob_img_src,
                                  int min, int max, int cur_value);

/**
 * @description: 通用消息框创建
 **/
lv_obj_t *lv_common_msgbox_create(int x, int y, int w, int h, int pad,
                                  const char *msg,
                                  lv_event_cb_t con_cb, lv_event_cb_t can_cb);

lv_obj_t *lv_common_msgbox_create_nobg(int x, int y, int w, int h, int pad,
                                       const char *msg,
                                       lv_event_cb_t con_cb, lv_event_cb_t can_cb);

void lv_common_msgbox_del(void);

/**
 * @description: 通知型消息框创建 (无底部按钮, 由调用方定时关闭)
 **/
lv_obj_t *lv_common_msgbox_notify_create(int x, int y, int w, int h, int pad,
                                         const char *msg);

lv_obj_t *lv_common_msgbox_notify_create_nobg(int x, int y, int w, int h, int pad,
                                              const char *msg);

/**
 * @description: 升级进度消息框创建 (无按钮, 内嵌进度条 + 百分比)
 **/
lv_obj_t *lv_common_msgbox_progress_create(int x, int y, int w, int h, int pad,
                                           const char *msg);

/**
 * @description: 通用选择框创建
 **/
lv_obj_t *lv_common_optionbox_create(int x, int y, int w, int h,
                                     const char *title,
                                     lv_event_cb_t con_cb, lv_event_cb_t can_cb);

void lv_common_optionbox_del(void);

/**
 * @description: 设置页面公共按键容器 - 双标签双图片版
 **/
lv_obj_t *lv_common_setting_btn_double_label_double_img_create(
    lv_obj_t *parent,           // 父对象
    int id,                     // 容器ID
    int x, int y, int w, int h, // 容器位置和尺寸
    // 容器样式配置
    lv_event_cb_t container_cb, // 容器点击回调（NULL表示无点击功能）
    lv_opa_t default_bg_opa, uint32_t default_bg_color,
    lv_opa_t press_bg_opa, uint32_t press_bg_color,
    int default_radius, int default_border_width, lv_border_side_t default_border_side,
    int border_default_opa, uint32_t border_default_color,
    int press_radius, int press_border_width, lv_border_side_t press_border_side,
    int border_press_opa, uint32_t border_press_color,
    // 第一个标签配置（主标签，与容器绑定）
    int label1_id, int label1_x, int label1_y, int label1_w, int label1_h,
    const char *label1_text, uint32_t label1_default_color, uint32_t label1_press_color,
    lv_text_align_t label1_align, lv_font_t *label1_font,
    // 第二个标签配置（副标签，仅显示，无点击）
    int label2_id, int label2_x, int label2_y, int label2_w, int label2_h,
    const char *label2_text, uint32_t label2_default_color, uint32_t label2_press_color,
    lv_text_align_t label2_align, lv_font_t *label2_font,
    // 第一个图片配置（img_btn，可独立点击）
    int img1_id, int img1_x, int img1_y, int img1_w, int img1_h,
    const void *img1_src, lv_opa_t img1_press_recolor_opa, uint32_t img1_press_recolor,
    lv_align_t img1_align, lv_event_cb_t img1_cb, // 图片1点击回调
    const void *img1_focus_src,                   // 图片1按压态资源（可为NULL）
    // 第二个图片配置（img_btn，可独立点击）
    int img2_id, int img2_x, int img2_y, int img2_w, int img2_h,
    const void *img2_src, lv_opa_t img2_press_recolor_opa, uint32_t img2_press_recolor,
    lv_align_t img2_align, lv_event_cb_t img2_cb, // 图片2点击回调
    const void *img2_focus_src                    // 图片2按压态资源（可为NULL）
);

/***********************************************
** 作者: hare
** 日期: 2025-12-25
** 说明: 右侧容器中创建文本显示控件 5个text
***********************************************/
lv_obj_t *lv_common_setting_cont_btn_four_label_create(
    lv_obj_t *parent,           // 父对象
    int id,                     // 容器ID
    int x, int y, int w, int h, // 容器位置和尺寸
    // 容器样式配置
    lv_event_cb_t container_cb, // 容器点击回调（NULL表示无点击功能）
    lv_opa_t default_bg_opa, uint32_t default_bg_color,
    lv_opa_t press_bg_opa, uint32_t press_bg_color,
    int default_radius, int default_border_width, lv_border_side_t default_border_side,
    int border_default_opa, uint32_t border_default_color,
    int press_radius, int press_border_width, lv_border_side_t press_border_side,
    int border_press_opa, uint32_t border_press_color,
    // 第一个标签配置（主标签，与容器绑定）
    int label1_id, int label1_x, int label1_y, int label1_w, int label1_h,
    const char *label1_text, uint32_t label1_default_color, uint32_t label1_press_color,
    lv_text_align_t label1_align, lv_font_t *label1_font,
    // 第二个标签配置（副标签，仅显示，无点击）
    int label2_id, int label2_x, int label2_y, int label2_w, int label2_h,
    const char *label2_text, uint32_t label2_default_color, uint32_t label2_press_color,
    lv_text_align_t label2_align, lv_font_t *label2_font,
    // 第三个标签配置（副标签，仅显示，无点击）
    int label3_id, int label3_x, int label3_y, int label3_w, int label3_h,
    const char *label3_text, uint32_t label3_default_color, uint32_t label3_press_color,
    lv_text_align_t label3_align, lv_font_t *label3_font,
    // 第三个标签配置（副标签，仅显示，无点击）
    int label4_id, int label4_x, int label4_y, int label4_w, int label4_h,
    const char *label4_text, uint32_t label4_default_color, uint32_t label4_press_color,
    lv_text_align_t label4_align, lv_font_t *label4_font,
    // 第五个标签配置（副标签，仅显示，无点击）
    int label5_id, int label5_x, int label5_y, int label5_w, int label5_h,
    const char *label5_text, uint32_t label5_default_color, uint32_t label5_press_color,
    lv_text_align_t label5_align, lv_font_t *label5_font);

lv_obj_t *lv_common_spinner_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                   uint32_t color_bg, uint32_t color_indic,
                                   uint16_t speed, uint16_t arc_length, uint16_t arc_width);

lv_obj_t *lv_common_spinner_anim_create(int x, int y, int w, int h,
                                        uint32_t color_bg, uint32_t color_indic,
                                        uint16_t speed, uint16_t arc_length, uint16_t arc_width,
                                        const char *title);

void lv_common_spinner_del(void);
/***********************************************
** 作者: hare
** 日期: 2026.1.6
** 说明: 设置一个单容器
***********************************************/
lv_obj_t *lv_common_setting_cont_create(
    lv_obj_t *parent,           // 父对象
    int id,                     // 容器ID
    int x, int y, int w, int h, // 容器位置和尺寸
    // 容器样式配置
    lv_event_cb_t container_cb, // 容器点击回调（NULL表示无点击功能）
    lv_opa_t default_bg_opa, uint32_t default_bg_color,
    lv_opa_t press_bg_opa, uint32_t press_bg_color,
    int default_radius, int default_border_width, lv_border_side_t default_border_side,
    int border_default_opa, uint32_t border_default_color,
    int press_radius, int press_border_width, lv_border_side_t press_border_side,
    int border_press_opa, uint32_t border_press_color);
/***********************************************
** 作者: hare
** 日期: 2026.1.8
** 说明: 设置一个单容器 双文本
***********************************************/
lv_obj_t *lv_common_setting_btn_double_label_create(
    lv_obj_t *parent,
    int id,
    int x, int y, int w, int h,
    lv_event_cb_t container_cb,
    lv_opa_t default_bg_opa, uint32_t default_bg_color,
    lv_opa_t press_bg_opa, uint32_t press_bg_color,
    int default_radius, int default_border_width, lv_border_side_t default_border_side,
    int border_default_opa, uint32_t border_default_color,
    int press_radius, int press_border_width, lv_border_side_t press_border_side,
    int border_press_opa, uint32_t border_press_color,
    int label1_id, int label1_x, int label1_y, int label1_w, int label1_h,
    const char *label1_text, uint32_t label1_default_color, uint32_t label1_press_color,
    lv_text_align_t label1_align, lv_font_t *label1_font,
    int label2_id, int label2_x, int label2_y, int label2_w, int label2_h,
    const char *label2_text, uint32_t label2_default_color, uint32_t label2_press_color,
    lv_text_align_t label2_align, lv_font_t *label2_font);

/**
 * @description: 箭头按键按压联动：按压/释放时同步改变 title 和 label 文本颜色
 **/
void lv_common_arrow_link_press(lv_obj_t *arrow, lv_obj_t *title, lv_obj_t *label);

/************************** 新版 UI 设计通用尺寸 **************************/
#define SCREEN_W 1024
#define SCREEN_H 600
#define SIDEBAR_W 190

/************************** 新版 UI 设计通用颜色 **************************/
#define LV_COMMON_BG_COLOR 0x2b2f44    /* 页面背景 */
#define LV_COMMON_PANEL_COLOR 0x212223 /* 侧栏/内容面板 */
#define LV_COMMON_SEP_COLOR 0x02a9ff   /* 分隔线中心高亮色 */

/**
 * @description: 纯色矩形（背景/面板等装饰）
 **/
lv_obj_t *lv_common_rect_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color);

/**
 * @description: 标准页面背景布局（全屏底色 + 侧栏 + 内容面板）
 **/
void lv_common_bg_create(lv_obj_t *parent);

/**
 * @description: 横向渐变分隔线（中间亮、两端隐入面板底色）
 **/
void lv_common_grad_sep_create(lv_obj_t *parent, int x, int y, int w);

/**
 * @description: 锁定/恢复整屏对象的点击与滚动 (替代满屏透明对象做模态拦截)
 * @note: lock 与 unlock 必须成对、同一屏幕内调用; 不支持嵌套;
 *        内部按对象 id 保存, 锁定期间对象被删除/重建不影响恢复。
 * @return: 成功返回锁定的对象数; 已处于锁定态返回 -1; 对象过多超出容量返回 -2
 **/
int lv_common_screen_click_lock(void);
void lv_common_screen_click_unlock(void);

#endif