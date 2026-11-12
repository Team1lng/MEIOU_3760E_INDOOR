#include "lv_common.h"
#include "page_common.h"
/**
 * @description: 通用按钮创建
 **/
lv_obj_t *lv_common_img_btn_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                   lv_event_cb_t cb, bool click, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                   int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                   int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                   void *img_src, lv_opa_t press_img_recolor_opa, uint32_t press_img_recolor, lv_align_t align,
                                   void *press_img_src)
{

    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj)
    {
        return obj;
    }

    obj = lv_obj_create(parent);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

    lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_common_style_set_boader(obj, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN);
    lv_common_style_set_boader(obj, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_press_color, LV_STATE_PRESSED);
    lv_common_style_set_img(obj, img_src, press_img_recolor_opa, press_img_recolor, align, LV_PART_MAIN, LV_STATE_PRESSED, press_img_src);
    lv_common_style_set_event(obj, cb, click, default_bg_opa, default_bg_color, LV_PART_MAIN, press_bg_opa, press_bg_color, LV_STATE_PRESSED);
    return obj;
}

/**
 * @description: 通用标题文本显示
 **/
lv_obj_t *lv_common_text_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                const char *string, uint32_t font_default_color, uint32_t font_press_color, lv_text_align_t align, const lv_font_t *font)
{

    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);

    if (obj != NULL)
    {
        return obj;
    }

    obj = lv_label_create(parent);
    lv_common_style_set_common(obj, id, x, y,
                               w > 0 ? w : LV_SIZE_CONTENT,
                               h > 0 ? h : LV_SIZE_CONTENT,
                               LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_common_style_set_text(obj, string, font_default_color, font_press_color, align, font, LV_STATE_PRESSED, LV_PART_MAIN);
    lv_common_style_set_boader(obj, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN);
    lv_common_style_set_boader(obj, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_default_color, LV_STATE_PRESSED);
    lv_common_style_set_event(obj, cb, cb, default_bg_opa, default_bg_color, LV_PART_MAIN, press_bg_opa, press_bg_color, LV_STATE_PRESSED);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING);
    return obj;
}

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
                                        void *press_img_src)
{
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);

    if (obj != NULL)
    {
        return obj;
    }

    lv_obj_t *title_obj = NULL, *img_obj = NULL;

    {
        obj = lv_obj_create(parent);

        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_common_style_set_boader(obj, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN);
        lv_common_style_set_boader(obj, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_default_color, LV_STATE_PRESSED);
        lv_common_style_set_event(obj, cb, true, default_bg_opa, default_bg_color, LV_PART_MAIN, press_bg_opa, press_bg_color, LV_STATE_PRESSED);
    }

    if (img_src)
    {
        img_obj = lv_common_img_btn_create(obj, img_id, img_x, img_y, img_w, img_h,
                                           NULL, false, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           img_src, press_img_recolor_opa, press_img_recolor, img_align,
                                           press_img_src);
    }
    if (title_string)
    {
        title_obj = lv_common_text_create(obj, title_id, title_x, title_y, title_w, title_h,
                                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                          title_string, title_default_color, title_press_color, title_align, title_font);
    }
    if (title_obj)
        lv_obj_set_state_bind(obj, title_obj);
    if (title_obj && img_obj)
        lv_obj_set_state_bind(title_obj, img_obj);
    return obj;
}

lv_obj_t *lv_common_number_input_keyboard_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                                 lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                                 int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                                 int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                                 uint32_t font_default_color, uint32_t font_press_color, lv_text_align_t align, const lv_font_t *font,
                                                 int row, int column)
{
    lv_obj_t *btnmatrix = lv_obj_get_child_by_id(parent, id);

    if (btnmatrix != NULL)
    {

        return btnmatrix;
    }

    btnmatrix = lv_btnmatrix_create(parent);
    lv_common_style_set_common(btnmatrix, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2023-2-1 8:32:8
    ** 说明: 设置按钮属性
    ***********************************************/
    {
        static const char *btnm_map[] = {
            "1", "2", "3", "\n",
            "4", "5", "6", "\n",
            "7", "8", "9", "\n",
            " ", "0", " ", ""};

        static const char *btnm_img_map[] = {
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL};
        if (btnm_img_map[11] == NULL)
        {
            // lv_resource_alloc(delete_icon, ICON_KEYBOARD_DELETE_NUM_PNG_OFFSET, ICON_KEYBOARD_DELETE_NUM_PNG_SIZE);
            // btnm_img_map[11] = (void *)delete_icon;
            lv_resource_alloc(confirm_icon, ICON_KEYBOARD_CONFIRM_NUM_PNG_OFFSET, ICON_KEYBOARD_CONFIRM_NUM_PNG_SIZE);
            btnm_img_map[9] = (void *)confirm_icon;
        }
        lv_btnmatrix_set_map(btnmatrix, btnm_map);

        lv_obj_set_style_pad_row(btnmatrix, row, LV_PART_MAIN);
        lv_obj_set_style_pad_column(btnmatrix, column, LV_PART_MAIN);

        lv_common_style_set_text(btnmatrix, NULL, font_default_color, font_press_color, align, font, LV_STATE_PRESSED, LV_PART_MAIN);
        lv_common_style_set_event(btnmatrix, cb, true, default_bg_opa, default_bg_color, LV_PART_ITEMS, press_bg_opa, press_bg_color, LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_add_event_cb(btnmatrix, cb, LV_EVENT_VALUE_CHANGED, NULL);

        lv_common_style_set_boader(btnmatrix, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_ITEMS);
        lv_common_style_set_boader(btnmatrix, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_default_color, LV_PART_ITEMS | LV_STATE_PRESSED);

        lv_obj_clear_flag(btnmatrix, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_btnmatrix_set_btn_bg_map(btnmatrix, (const void **)btnm_img_map);
    }

    return btnmatrix;
}

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
                                    lv_opa_t default_parent_bg_opa, uint32_t default_parent_bg_color, lv_opa_t press_parent_bg_opa, uint32_t press_parent_bg_color)
{
    lv_obj_t *keyboard = lv_obj_get_child_by_id(parent, id);

    if (keyboard != NULL)
    {
        return keyboard;
    }

    lv_common_grad_sep_create(parent, x, y - 4, SCREEN_W);

    keyboard = lv_keyboard_create(parent);

    lv_common_style_set_common(keyboard, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_common_style_set_event(keyboard, cb, cb, default_bg_opa, default_bg_color, LV_PART_ITEMS | LV_STATE_DEFAULT, press_bg_opa, press_bg_color, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_common_style_set_boader(keyboard, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_common_style_set_boader(keyboard, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_default_color, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_common_style_set_boader(keyboard, defalut_item_radius, boarder_item_default_opa, defalut_item_boarder_width, default_item_boarder_side, boarde_item_default_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_common_style_set_boader(keyboard, press_item_radius, boarder_item_press_opa, press_item_boarder_width, press_item_boarder_side, boarder_item_press_color, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_common_style_set_text(keyboard, NULL, font_default_color, font_press_color, align, font, LV_PART_ITEMS | LV_STATE_PRESSED, LV_PART_ITEMS);
    lv_common_style_set_pad(keyboard, pad_top, pad_left, pad_bottom, pad_right, pad_column, pad_row, LV_PART_MAIN);

    lv_common_style_set_event(keyboard, NULL, false, default_parent_bg_opa, default_parent_bg_color, LV_PART_MAIN | LV_STATE_DEFAULT, press_parent_bg_opa, press_parent_bg_color, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    static const void *btnm_img_map[3][44];

    /* 删除键: lc/uc index 11, spec index 10 */
    if (btnm_img_map[0][12] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_DELETE_PNG_OFFSET, ICON_KEYBOARD_DELETE_PNG_SIZE);
        btnm_img_map[0][12] = icon;
        btnm_img_map[1][12] = icon;
        btnm_img_map[2][10] = icon;
    }
    /* 换行键: lc/uc index 23 */
    if (btnm_img_map[0][23] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_ENTER_PNG_OFFSET, ICON_KEYBOARD_ENTER_PNG_SIZE);
        btnm_img_map[0][23] = icon;
        btnm_img_map[1][23] = icon;
    }
    /* 取消键: 所有模式 index 35 */
    if (btnm_img_map[0][35] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_CANCEL_PNG_OFFSET, ICON_KEYBOARD_CANCEL_PNG_SIZE);
        btnm_img_map[0][35] = icon;
        btnm_img_map[1][35] = icon;
        btnm_img_map[2][35] = icon;
    }
    /* 左箭头: 所有模式 index 36 */
    if (btnm_img_map[0][36] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_LEFT_SHIT_PNG_OFFSET, ICON_KEYBOARD_LEFT_SHIT_PNG_SIZE);
        btnm_img_map[0][36] = icon;
        btnm_img_map[1][36] = icon;
        btnm_img_map[2][36] = icon;
    }
    /* 空格键: 所有模式 index 37 */
    if (btnm_img_map[0][37] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_SPACE_PNG_OFFSET, ICON_KEYBOARD_SPACE_PNG_SIZE);
        btnm_img_map[0][37] = icon;
        btnm_img_map[1][37] = icon;
        btnm_img_map[2][37] = icon;
    }
    /* 右箭头: 所有模式 index 38 */
    if (btnm_img_map[0][38] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_RIGHT_SHIT_PNG_OFFSET, ICON_KEYBOARD_RIGHT_SHIT_PNG_SIZE);
        btnm_img_map[0][38] = icon;
        btnm_img_map[1][38] = icon;
        btnm_img_map[2][38] = icon;
    }
    /* 确认键: 所有模式 index 39 */
    if (btnm_img_map[0][39] == NULL)
    {
        lv_resource_alloc(icon, ICON_KEYBOARD_CONFIRM_PNG_OFFSET, ICON_KEYBOARD_CONFIRM_PNG_SIZE);
        btnm_img_map[0][39] = icon;
        btnm_img_map[1][39] = icon;
        btnm_img_map[2][39] = icon;
    }

    lv_keyboard_update_img_map(keyboard, btnm_img_map);

    return keyboard;
}

/**
 * @description: 文本输入框
 **/
lv_obj_t *lv_common_textarea_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                    lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                    lv_opa_t default_cursor_opa, uint32_t default_cursor_color, lv_opa_t press_cursor_opa, uint32_t press_cursor_color,
                                    int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                    int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                    const char *string, uint32_t font_default_color, uint32_t font_focused_color, lv_text_align_t align, const lv_font_t *font, int max_char,
                                    int text_offset_y, int anim_time)
{
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);

    if (obj != NULL)
    {
        return obj;
    }

    obj = lv_textarea_create(parent);
    lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

    lv_common_style_set_textarea(obj, string, font_default_color, font_focused_color, align, font, LV_STATE_FOCUSED, LV_PART_MAIN, max_char);
    lv_common_style_set_event(obj, cb, true, default_bg_opa, default_bg_color, LV_PART_MAIN, press_bg_opa, press_bg_color, LV_STATE_FOCUSED);
    lv_common_style_set_boader(obj, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN);
    lv_common_style_set_boader(obj, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_press_color, LV_STATE_FOCUSED);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_anim_time(obj, anim_time, LV_STATE_FOCUSED | LV_PART_CURSOR);
    lv_obj_set_style_pad_top(obj, text_offset_y, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(obj, text_offset_y, LV_PART_MAIN);
    lv_obj_set_style_pad_left(obj, 13, LV_PART_MAIN); // 左侧间距10px
    return obj;
}

/**
 * @description: 滚筒
 **/
lv_obj_t *lv_common_roller_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                  lv_event_cb_t cb, lv_opa_t default_bg_opa, uint32_t default_bg_color, lv_opa_t press_bg_opa, uint32_t press_bg_color,
                                  int defalut_radius, int defalut_boarder_width, lv_border_side_t default_boarder_side, int boarder_default_opa, uint32_t boarder_default_color,
                                  int press_radius, int press_boarder_width, lv_border_side_t press_boarder_side, int boarder_press_opa, uint32_t boarder_press_color,
                                  int row_cont, int line_space, int min, int max, uint32_t default_text_color, uint32_t select_text_color, lv_text_align_t text_align, lv_font_t *text_font,
                                  lv_roller_mode_t mode, const void *img_src)
{
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }

    obj = lv_roller_create(parent);

    lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

    lv_common_style_set_event(obj, cb, cb, default_bg_opa, default_bg_color, LV_PART_MAIN, press_bg_opa, press_bg_color, LV_STATE_PRESSED);
    lv_common_style_set_boader(obj, defalut_radius, boarder_default_opa, defalut_boarder_width, default_boarder_side, boarder_default_color, LV_PART_MAIN);
    lv_common_style_set_boader(obj, press_radius, boarder_press_opa, press_boarder_width, press_boarder_side, boarder_press_color, LV_STATE_PRESSED);
    lv_common_style_set_text(obj, NULL, default_text_color, select_text_color, text_align, text_font, LV_PART_SELECTED, LV_PART_MAIN);

    if (img_src)
        lv_obj_set_style_bg_img_src(obj, (void *)img_src, LV_PART_MAIN);
    lv_obj_set_style_anim_time(obj, 500, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(obj, line_space, LV_PART_MAIN);
    lv_roller_set_visible_row_count(obj, row_cont);
    // lv_obj_set_ext_click_area(obj, 34);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_SELECTED);
    char opt[512] = {0};
    for (int i = min; i <= max; i++)
    {
        char buf[8] = {0};
        snprintf(buf, sizeof(buf), "%02d%s", i, i == max ? "" : "\n");
        strcat(opt, buf);
    }
    lv_roller_set_options(obj, opt, mode);

    return obj;
}

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
                                  int min, int max, int cur_value)
{

    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }

    {
        obj = lv_obj_create(parent);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_common_style_set_event(obj, NULL, false, default_bg_opa, default_bg_color, LV_PART_MAIN, default_bg_opa, default_bg_color, LV_STATE_PRESSED);
        lv_common_style_set_boader(obj, bg_radius, bg_boarder_opa, bg_boarder_width, bg_boarder_side, bg_boarder_color, LV_PART_MAIN);
    }
    lv_obj_t *text_obj = NULL, *slider_obj = NULL, *left_btn_obj = NULL, *right_btn_obj = NULL;

    if ((text_id >= 0))
    {
        text_obj = lv_common_text_create(obj, text_id, text_x, text_y, text_w, text_h,
                                         NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                         text_string, text_font_color, text_font_color, text_align, text_font);
    }

    {
        slider_obj = lv_obj_get_child_by_id(obj, slider_id);
        if (slider_obj == NULL)
        {
            slider_obj = lv_slider_create(obj);
            lv_common_style_set_common(slider_obj, slider_id, slider_x, slider_y, slider_w, slider_h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
            lv_common_style_set_boader(slider_obj, slider_radius, slider_boarder_opa, slider_boarder_width, slider_boarder_side, slider_boarder_color, LV_PART_MAIN);
            lv_common_style_set_event(slider_obj, NULL, true, slider_bg_opa, slider_bg_color, LV_PART_MAIN, slider_foure_opa, slider_foure_color, LV_PART_INDICATOR);
            lv_obj_set_ext_click_area(slider_obj, 30);
            if (text_obj != NULL)
            {
                lv_obj_add_event_cb(slider_obj, cb_slider, LV_EVENT_VALUE_CHANGED, text_obj);
            }
            if (cb != NULL)
            {
                lv_obj_add_event_cb(slider_obj, cb, LV_EVENT_VALUE_CHANGED, NULL);
            }

            lv_bar_set_range(slider_obj, min, max);
            lv_slider_set_mode(slider_obj, LV_SLIDER_MODE_NORMAL);
            lv_bar_set_value(slider_obj, cur_value, LV_ANIM_OFF);

            lv_obj_set_style_radius(slider_obj, knob_radius, LV_PART_KNOB);
            lv_obj_set_style_pad_all(slider_obj, knob_size, LV_PART_KNOB);
            lv_obj_set_style_bg_opa(slider_obj, knob_opa, LV_PART_KNOB);
            lv_obj_set_style_bg_color(slider_obj, lv_color_hex(knob_bg_color), LV_PART_KNOB);
            if (knob_img_src != NULL)
            {
                lv_obj_set_style_bg_img_src(slider_obj, (void *)knob_img_src, LV_PART_KNOB);
            }
        }
    }

    if (left_btn_id >= 0)
    {
        left_btn_obj = lv_common_img_btn_create(obj, left_btn_id, left_btn_x, left_btn_y, left_btn_w, left_btn_h,
                                                NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                (void *)left_btn_img_src, left_btn_press_img_recolor_opa, left_btn_press_img_recolor, left_btn_img_align, NULL);

        lv_obj_add_event_cb(left_btn_obj, cb_sub, LV_EVENT_CLICKED, slider_obj);
        lv_obj_set_ext_click_area(left_btn_obj, 10);
    }

    if (right_btn_id >= 0)
    {
        right_btn_obj = lv_common_img_btn_create(obj, right_btn_id, right_btn_x, right_btn_y, right_btn_w, right_btn_h,
                                                 NULL, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 (void *)right_btn_img_src, right_btn_press_img_recolor_opa, right_btn_press_img_recolor, right_btn_img_align, NULL);

        lv_obj_add_event_cb(right_btn_obj, cb_add, LV_EVENT_CLICKED, slider_obj);
        lv_obj_set_ext_click_area(right_btn_obj, 10);
    }
    return obj;
}

static lv_event_cb_t s_msgbox_con_cb = NULL;
static lv_event_cb_t s_msgbox_can_cb = NULL;

static void msgbox_btn_confirm_click(lv_event_t *ev)
{
    lv_common_screen_click_unlock();
    if (s_msgbox_con_cb)
    {
        s_msgbox_con_cb(ev);
    }
    else
    {
        lv_common_msgbox_del();
    }
}

static void msgbox_btn_cancel_click(lv_event_t *ev)
{
    lv_common_screen_click_unlock();
    if (s_msgbox_can_cb)
    {
        s_msgbox_can_cb(ev);
    }
    else
    {
        lv_common_msgbox_del();
    }
}

void lv_common_msgbox_del(void)
{
    /* 带背景模式遮罩为屏幕直接子节点, 无背景模式容器直接挂屏幕 */
    lv_obj_t *msgbox = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_MSGBOX_BG);
    if (msgbox == NULL)
        msgbox = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_MSGBOX_CONT);
    if (msgbox != NULL)
    {
        lv_obj_del(msgbox);
    }
    s_msgbox_con_cb = NULL;
    s_msgbox_can_cb = NULL;
}

/**
 * @description: 通用消息框主体创建 (背景遮罩 + 容器 + 渐变 + 文本)
 * @param title_off_y 文本垂直偏移 (有按钮时上移让位, 无按钮时居中)
 **/
static lv_obj_t *lv_common_msgbox_body_create(int x, int y, int w, int h, int pad,
                                              const char *msg, int title_off_y, bool need_bg)
{
    /* x/y 传 -1 表示按屏幕居中 */
    if (x == -1)
        x = (lv_obj_get_width(lv_scr_act_current()) - w) / 2;
    if (y == -1)
        y = (lv_obj_get_height(lv_scr_act_current()) - h) / 2;

    /* 清理旧弹窗: 带背景时遮罩为屏幕直接子节点, 无背景时容器直接挂屏幕 */
    lv_obj_t *old = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_MSGBOX_BG);
    if (old == NULL)
        old = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_MSGBOX_CONT);
    if (old != NULL)
    {
        lv_obj_del(old);
    }

    lv_obj_t *parent = lv_scr_act_current();
    if (need_bg)
    {
        /* 全屏透明遮罩 (拦截背景点击, 不关闭弹窗) */
        lv_obj_t *msg_bg = lv_obj_create(lv_scr_act_current());
        lv_obj_clear_flag(msg_bg, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
        lv_common_style_set_common(msg_bg, LV_COMMON_MSGBOX_BG, 0, 0, 1024, 600, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(msg_bg, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_add_flag(msg_bg, LV_OBJ_FLAG_CLICKABLE);
        parent = msg_bg;
    }

    /* 消息框容器: #161E26背景 圆角18 */
    lv_obj_t *msgbox = lv_obj_create(parent);
    lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
    lv_common_style_set_common(msgbox, LV_COMMON_MSGBOX_CONT, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_common_style_set_event(msgbox, NULL, false,
                              LV_OPA_COVER, 0x161E26, LV_PART_MAIN,
                              LV_OPA_COVER, 0x161E26, LV_STATE_PRESSED);
    lv_obj_set_style_radius(msgbox, 18, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(msgbox, true, LV_PART_MAIN);

    /* 顶部渐变色块 (left: bg→sep, right: sep→bg) */
    lv_obj_t *grad_left = lv_obj_create(msgbox);
    lv_obj_clear_flag(grad_left, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
    lv_common_style_set_common(grad_left, -1, 0, pad, w / 2, 2, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(grad_left, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(grad_left, lv_color_hex(0x161E26), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(grad_left, lv_color_hex(LV_COMMON_SEP_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(grad_left, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(grad_left, 0, LV_PART_MAIN);

    lv_obj_t *grad_right = lv_obj_create(msgbox);
    lv_obj_clear_flag(grad_right, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
    lv_common_style_set_common(grad_right, -1, w / 2, pad, w / 2, 2, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(grad_right, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(grad_right, lv_color_hex(LV_COMMON_SEP_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(grad_right, lv_color_hex(0x161E26), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(grad_right, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(grad_right, 0, LV_PART_MAIN);

    /* 消息文本 (居中, 左右留边距20) */
    lv_obj_t *title = lv_label_create(msgbox);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, msg);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, lv_font_normal, LV_PART_MAIN);
    lv_obj_set_width(title, w - 40);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, title_off_y);
    lv_obj_set_child_id(title, LV_COMMON_MSGBOX_TITLE);

    return msgbox;
}

static lv_obj_t *lv_common_msgbox_create_impl(int x, int y, int w, int h, int pad,
                                              const char *msg,
                                              lv_event_cb_t con_cb, lv_event_cb_t can_cb,
                                              bool need_bg)
{
    s_msgbox_con_cb = con_cb;
    s_msgbox_can_cb = can_cb;

    lv_obj_t *msgbox = lv_common_msgbox_body_create(x, y, w, h, pad, msg, -30, need_bg);

    /* 底部按钮 */
    lv_resource_init(icon_cancel, ICON_COMMON_MSGBOX_CANCEL_PNG_OFFSET, ICON_COMMON_MSGBOX_CANCEL_PNG_SIZE);
    lv_resource_init(icon_confirm, ICON_COMMON_MSGBOX_CONFIRM_PNG_OFFSET, ICON_COMMON_MSGBOX_CONFIRM_PNG_SIZE);
    lv_resource_init(icon_ok, ICON_COMMON_MSGBOX2_CONFIRM_PNG_OFFSET, ICON_COMMON_MSGBOX2_CONFIRM_PNG_SIZE);

    if (!can_cb)
    {
        /* 单按键 OK */
        lv_event_cb_t con_cb_fn = msgbox_btn_confirm_click;
        lv_obj_t *btn = lv_common_img_text_btn_create(msgbox,
                                                      LV_COMMON_MSGBOX_CONFIRM,
                                                      (w - 160) / 2, h - 58, 160, 48,
                                                      con_cb_fn,
                                                      LV_OPA_TRANSP, 0x00,
                                                      LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, 160, 48, 0,
                                                      lang_str_get(STR_CONFIRM), 0xffffff, 0xff0000,
                                                      LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
                                                      0, 0, 160, 48, 1,
                                                      &icon_ok, LV_OPA_TRANSP, 0xffffff, LV_ALIGN_CENTER, NULL);
        lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    }
    else
    {
        /* 双按键 No (左) + Yes (右) */
        lv_event_cb_t can_cb_fn = msgbox_btn_cancel_click;
        lv_event_cb_t con_cb_fn = msgbox_btn_confirm_click;
        lv_obj_t *btn_no = lv_common_img_text_btn_create(msgbox,
                                                         LV_COMMON_MSGBOX_CANCEL,
                                                         w / 2, h - 69, w / 2, 69,
                                                         can_cb_fn,
                                                         LV_OPA_TRANSP, 0x00,
                                                         LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, w / 2, 69, 0,
                                                         lang_str_get(STR_NO), 0xffffff, 0xff0000,
                                                         LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
                                                         0, 0, w / 2, 69, 1,
                                                         &icon_cancel, LV_OPA_TRANSP, 0xffffff, LV_ALIGN_CENTER, NULL);
        lv_obj_set_style_radius(btn_no, 0, LV_PART_MAIN);

        lv_obj_t *btn_yes = lv_common_img_text_btn_create(msgbox,
                                                          LV_COMMON_MSGBOX_CONFIRM,
                                                          0, h - 69, w / 2, 69,
                                                          con_cb_fn,
                                                          LV_OPA_TRANSP, 0x00,
                                                          LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, w / 2, 69, 0,
                                                          lang_str_get(STR_YES), 0xffffff, 0xff0000,
                                                          LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
                                                          0, 0, w / 2, 69, 1,
                                                          &icon_confirm, LV_OPA_TRANSP, 0xffffff, LV_ALIGN_CENTER, NULL);
        lv_obj_set_style_radius(btn_yes, 0, LV_PART_MAIN);
    }

    return msgbox;
}

/**
 * @description: 通用消息框创建 (EMOS-Indoor风格)
 **/
lv_obj_t *lv_common_msgbox_create(int x, int y, int w, int h, int pad,
                                  const char *msg,
                                  lv_event_cb_t con_cb, lv_event_cb_t can_cb)
{
    return lv_common_msgbox_create_impl(x, y, w, h, pad, msg, con_cb, can_cb, true);
}

/**
 * @description: 通用消息框创建 (无背景遮罩, 视频模式下避免整屏重绘)
 **/
lv_obj_t *lv_common_msgbox_create_nobg(int x, int y, int w, int h, int pad,
                                       const char *msg,
                                       lv_event_cb_t con_cb, lv_event_cb_t can_cb)
{
    lv_common_screen_click_lock();
    return lv_common_msgbox_create_impl(x, y, w, h, pad, msg, con_cb, can_cb, false);
}

/**
 * @description: 通知型消息框创建 (无底部按钮, 由调用方定时关闭)
 **/
lv_obj_t *lv_common_msgbox_notify_create(int x, int y, int w, int h, int pad,
                                         const char *msg)
{
    s_msgbox_con_cb = NULL;
    s_msgbox_can_cb = NULL;
    return lv_common_msgbox_body_create(x, y, w, h, pad, msg, 0, true);
}

/**
 * @description: 通知型消息框创建 (无背景遮罩, 视频模式下避免整屏重绘, 由调用方定时关闭)
 **/
lv_obj_t *lv_common_msgbox_notify_create_nobg(int x, int y, int w, int h, int pad,
                                              const char *msg)
{
    s_msgbox_con_cb = NULL;
    s_msgbox_can_cb = NULL;
    return lv_common_msgbox_body_create(x, y, w, h, pad, msg, 0, false);
}

/**
 * @description: 升级进度消息框创建 (无按钮, 内嵌进度条 + 百分比 label)
 * @return 消息框容器指针
 **/
lv_obj_t *lv_common_msgbox_progress_create(int x, int y, int w, int h, int pad,
                                           const char *msg)
{
    s_msgbox_con_cb = NULL;
    s_msgbox_can_cb = NULL;
    lv_obj_t *msgbox = lv_common_msgbox_body_create(x, y, w, h, pad, msg, -30, true);

    /* 提示语放到提示框顶部（分隔线下方） */
    lv_obj_t *title = lv_obj_get_child_by_id(msgbox, LV_COMMON_MSGBOX_TITLE);
    if (title != NULL)
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, pad + 20);

    /* 进度条放中间 */
    lv_obj_t *bar = lv_bar_create(msgbox);
    lv_obj_set_size(bar, w - 160, 14);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x393939), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xEFCC8C), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_child_id(bar, LV_COMMON_MSGBOX_PROGRESS_BAR);

    /* 百分比 label 位于进度条下方 */
    lv_obj_t *pct = lv_label_create(msgbox);
    lv_label_set_text(pct, "0%");
    lv_obj_set_style_text_color(pct, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(pct, lv_font_normal, LV_PART_MAIN);
    lv_obj_align(pct, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_child_id(pct, LV_COMMON_MSGBOX_PROGRESS_LABEL);

    return msgbox;
}

void lv_common_optionbox_del(void)
{
    lv_obj_t *optionbox = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_OPTIONBOX_CONT);
    if (optionbox != NULL)
    {
        lv_obj_del(optionbox);
    }
}

/**
 * @description: 通用选择框创建
 **/
lv_obj_t *lv_common_optionbox_create(int x, int y, int w, int h,
                                     const char *title,
                                     lv_event_cb_t con_cb, lv_event_cb_t can_cb)
{
    x = 213;
    y = 102;
    w = 598;
    h = 396;

    lv_obj_t *opt_bg = lv_common_img_btn_create(lv_scr_act_current(), LV_COMMON_OPTIONBOX_CONT, 0, 0, 1024, 600,
                                                NULL, true, LV_OPA_80, 0x000000, LV_OPA_80, 0x000000,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_add_flag(opt_bg, LV_OBJ_FLAG_ADV_HITTEST);

    lv_obj_t *optionbox = lv_common_img_btn_create(opt_bg, LV_COMMON_OPTIONBOX_CONT, x, y, w, h,
                                                   NULL, false, LV_OPA_COVER, 0XFFFFFF, LV_OPA_COVER, 0XFFFFFF,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);

    lv_common_text_create(optionbox, LV_COMMON_OPTIONBOX_TITLE, 0, 0, w, 48,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x323237,
                          0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x323237,
                          title, 0, 0, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    int btn_start_y = h - 72;

    lv_resource_init(icon1, ICON_COMMON_MSGBOX_CANCEL_PNG_OFFSET, ICON_COMMON_MSGBOX_CANCEL_PNG_SIZE);
    lv_resource_init(icon2, ICON_COMMON_MSGBOX_CONFIRM_PNG_OFFSET, ICON_COMMON_MSGBOX_CONFIRM_PNG_SIZE);
    if (!con_cb)
    {
        lv_common_img_btn_create(optionbox, LV_COMMON_OPTIONBOX_CONFIRM, -2, btn_start_y, w + 2, 72,
                                 con_cb, true, LV_OPA_COVER, 0X0566E7, LV_OPA_COVER, 0X0566E7,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 &icon2, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    }
    else
    {
        lv_common_img_btn_create(optionbox, LV_COMMON_OPTIONBOX_CONFIRM, -2, btn_start_y, w / 2 + 3, 72,
                                 can_cb, true, LV_OPA_COVER, 0XEBEBEB, LV_OPA_COVER, 0XEBEBEB,
                                 0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0,
                                 0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0,
                                 &icon1, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);

        lv_common_img_btn_create(optionbox, LV_COMMON_OPTIONBOX_CANCEL, w / 2 - 1, btn_start_y, w / 2 + 3, 72,
                                 con_cb, true, LV_OPA_COVER, 0X0566E7, LV_OPA_COVER, 0X0566E7,
                                 0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0,
                                 0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0,
                                 &icon2, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    }

    return optionbox;
}

/**
 * @description: 设置页面公共按键容器 - 双标签双图片版
 **/
lv_obj_t *lv_common_setting_btn_double_label_double_img_create(
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
    lv_text_align_t label2_align, lv_font_t *label2_font,
    int img1_id, int img1_x, int img1_y, int img1_w, int img1_h,
    const void *img1_src, lv_opa_t img1_press_recolor_opa, uint32_t img1_press_recolor,
    lv_align_t img1_align, lv_event_cb_t img1_cb, const void *img1_focus_src,
    int img2_id, int img2_x, int img2_y, int img2_w, int img2_h,
    const void *img2_src, lv_opa_t img2_press_recolor_opa, uint32_t img2_press_recolor,
    lv_align_t img2_align, lv_event_cb_t img2_cb, const void *img2_focus_src)
{
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }

    {
        obj = lv_obj_create(parent);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

        lv_common_style_set_event(obj, container_cb, container_cb,
                                  default_bg_opa, default_bg_color, LV_PART_MAIN,
                                  press_bg_opa, press_bg_color, LV_STATE_PRESSED);
        lv_common_style_set_boader(obj, default_radius, border_default_opa,
                                   default_border_width, default_border_side,
                                   border_default_color, LV_PART_MAIN);
        lv_common_style_set_boader(obj, press_radius, border_press_opa,
                                   press_border_width, press_border_side,
                                   border_press_color, LV_STATE_PRESSED);
    }

    lv_obj_t *label1_obj = NULL, *label2_obj = NULL;
    lv_obj_t *img1_obj = NULL, *img2_obj = NULL;

    if ((label1_text != NULL) && (label1_id >= 0))
    {
        label1_obj = lv_common_text_create(obj, label1_id,
                                           label1_x, label1_y, label1_w, label1_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label1_text, label1_default_color,
                                           label1_press_color, label1_align, label1_font);
    }

    if (label2_id >= 0 && label2_text != NULL)
    {
        label2_obj = lv_common_text_create(obj, label2_id,
                                           label2_x, label2_y, label2_w, label2_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label2_text, label2_default_color,
                                           label2_press_color, label2_align, label2_font);
    }

    if (img1_id >= 0)
    {
        img1_obj = lv_common_img_btn_create(obj, img1_id,
                                            img1_x, img1_y, img1_w, img1_h,
                                            img1_cb, (img1_cb != NULL),
                                            LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            (void *)img1_src, img1_press_recolor_opa,
                                            img1_press_recolor, img1_align,
                                            (void *)img1_focus_src);
    }

    if (img2_id >= 0)
    {
        img2_obj = lv_common_img_btn_create(obj, img2_id,
                                            img2_x, img2_y, img2_w, img2_h,
                                            img2_cb, (img2_cb != NULL),
                                            LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            (void *)img2_src, img2_press_recolor_opa,
                                            img2_press_recolor, img2_align,
                                            (void *)img2_focus_src);
    }

    if (img1_obj && img1_cb)
        lv_common_arrow_link_press(img1_obj, label1_obj, label2_obj);
    if (img2_obj && img2_cb)
        lv_common_arrow_link_press(img2_obj, label1_obj, label2_obj);

    lv_obj_t *bind_obj = obj;

    if (label1_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label1_obj);
        bind_obj = label1_obj;
    }

    if (label2_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label2_obj);
        bind_obj = label2_obj;
    }

    if (img1_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, img1_obj);
    }

    if (img2_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, img2_obj);
    }

    return obj;
}

/***********************************************
** 作者: hare
** 日期: 2025-12-25
** 说明: 右侧容器中创建文本显示控件 4个text
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
    // 第四个标签配置（副标签，仅显示，无点击）
    int label4_id, int label4_x, int label4_y, int label4_w, int label4_h,
    const char *label4_text, uint32_t label4_default_color, uint32_t label4_press_color,
    lv_text_align_t label4_align, lv_font_t *label4_font,
    // 第五个标签配置（副标签，仅显示，无点击）
    int label5_id, int label5_x, int label5_y, int label5_w, int label5_h,
    const char *label5_text, uint32_t label5_default_color, uint32_t label5_press_color,
    lv_text_align_t label5_align, lv_font_t *label5_font)
{
    // 检查是否已存在该ID的对象
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置容器属性
    ***********************************************/
    {
        obj = lv_obj_create(parent);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

        lv_common_style_set_event(obj, container_cb, container_cb,
                                  default_bg_opa, default_bg_color, LV_PART_MAIN,
                                  press_bg_opa, press_bg_color, LV_STATE_PRESSED);
        lv_common_style_set_boader(obj, default_radius, border_default_opa,
                                   default_border_width, default_border_side,
                                   border_default_color, LV_PART_MAIN);
        lv_common_style_set_boader(obj, press_radius, border_press_opa,
                                   press_border_width, press_border_side,
                                   border_press_color, LV_STATE_PRESSED);
    }

    lv_obj_t *label1_obj = NULL, *label2_obj = NULL, *label3_obj = NULL, *label4_obj = NULL, *label5_obj = NULL;

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置第一个标签（主标签）
    ***********************************************/
    if ((label1_text != NULL) && (label1_id >= 0))
    {
        label1_obj = lv_common_text_create(obj, label1_id,
                                           label1_x, label1_y, label1_w, label1_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label1_text, label1_default_color,
                                           label1_press_color, label1_align, label1_font);
    }

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置第二个标签（副标签）
    ***********************************************/
    if (label2_id >= 0 && label2_text != NULL)
    {
        label2_obj = lv_common_text_create(obj, label2_id,
                                           label2_x, label2_y, label2_w, label2_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label2_text, label2_default_color,
                                           label2_press_color, label2_align, label2_font);
    }

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置第三个标签（副标签）
    ***********************************************/
    if (label3_id >= 0 && label3_text != NULL)
    {
        label3_obj = lv_common_text_create(obj, label3_id,
                                           label3_x, label3_y, label3_w, label3_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label3_text, label3_default_color,
                                           label3_press_color, label3_align, label3_font);
    }
    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置第四个标签（副标签）
    ***********************************************/
    if (label4_id >= 0 && label4_text != NULL)
    {
        label4_obj = lv_common_text_create(obj, label4_id,
                                           label4_x, label4_y, label4_w, label4_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label4_text, label4_default_color,
                                           label4_press_color, label4_align, label4_font);
    }
    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置第五个标签（副标签）
    ***********************************************/
    if (label5_id >= 0 && label5_text != NULL)
    {
        label5_obj = lv_common_text_create(obj, label5_id,
                                           label5_x, label5_y, label5_w, label5_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label5_text, label5_default_color,
                                           label5_press_color, label5_align, label5_font);
    }

    /***********************************************
     ** 作者: leo.liu
     ** 日期: 2025-12-09 17:30:00
     ** 说明: 绑定状态
     ***********************************************/
    lv_obj_t *bind_obj = obj;

    if (label1_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label1_obj);
        bind_obj = label1_obj;
    }

    if (label2_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label2_obj);
        bind_obj = label2_obj;
    }

    if (label3_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label3_obj);
        bind_obj = label3_obj;
    }
    if (label4_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label4_obj);
        bind_obj = label4_obj;
    }
    if (label5_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label5_obj);
        bind_obj = label5_obj;
    }
    return obj;
}

lv_obj_t *lv_common_spinner_create(lv_obj_t *parent, int id, int x, int y, int w, int h,
                                   uint32_t color_bg, uint32_t color_indic,
                                   uint16_t speed, uint16_t arc_length, uint16_t arc_width)
{
    lv_obj_t *preload = lv_spinner_create(parent, speed, arc_length);
    lv_common_style_set_common(preload, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    // 旋转器创建
    static lv_style_t style; // 创建样式
    static lv_style_t bc_style;

    lv_style_init(&style);                                  // 初始化样式
    lv_style_set_arc_color(&style, lv_color_hex(color_bg)); // 设置圆弧颜色
    lv_style_set_arc_width(&style, arc_width);              // 设置圆弧宽度；

    lv_style_init(&bc_style);                                     // 初始化样式
    lv_style_set_arc_color(&bc_style, lv_color_hex(color_indic)); // 设置背景圆环颜色
    lv_style_set_arc_width(&bc_style, arc_width);                 // 设置背景圆环宽度

    lv_obj_add_style(preload, &style, LV_PART_INDICATOR); // 应用到圆弧部分；
    lv_obj_add_style(preload, &bc_style, LV_PART_MAIN);   // 应用到背景圆环部分；

    return preload;
}

lv_obj_t *lv_common_spinner_anim_create(int x, int y, int w, int h,
                                        uint32_t color_bg, uint32_t color_indic,
                                        uint16_t speed, uint16_t arc_length, uint16_t arc_width,
                                        const char *title)
{
    lv_obj_t *parent = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_SPINNER_BG);
    if (parent != NULL)
    {
        lv_obj_del(parent);
    }
    // 创建半透明背景
    parent = lv_common_img_btn_create(lv_scr_act_current(), LV_COMMON_SPINNER_BG,
                                      0, 0, 1024, 600,
                                      NULL, true,
                                      LV_OPA_80, 0x000000,
                                      LV_OPA_80, 0x000000,
                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                      NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_ADV_HITTEST);
    // 创建加载动画
    lv_obj_t *spinner = lv_common_spinner_create(parent,
                                                 LV_COMMON_SPINNER_SPINNER,
                                                 (1024 - 102) / 2, (600 - 102) / 2 - 50, 102, 102,
                                                 0x0096FF, 0XFFFFFF,
                                                 1000, 45, 12);

    // 创建提示文本
    lv_common_text_create(parent,
                          LV_COMMON_SPINNER_TEXT,
                          0, 350, 1024, 50,
                          NULL, LV_OPA_TRANSP, 0X303030, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          title, 0XFFFFFFFF, 0xFFFFFF,
                          LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
    return spinner;
}

void lv_common_spinner_del(void)
{
    lv_obj_t *optionbox = lv_obj_get_child_by_id(lv_scr_act_current(), LV_COMMON_SPINNER_BG);
    if (optionbox != NULL)
    {
        lv_obj_del(optionbox);
    }
}
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
    int border_press_opa, uint32_t border_press_color)
{
    // 检查是否已存在该ID的对象
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }

    /***********************************************
    ** 作者: leo.liu
    ** 日期: 2025-12-09 17:30:00
    ** 说明: 设置容器属性
    ***********************************************/
    {
        obj = lv_obj_create(parent);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

        lv_common_style_set_event(obj, container_cb, container_cb,
                                  default_bg_opa, default_bg_color, LV_PART_MAIN,
                                  press_bg_opa, press_bg_color, LV_STATE_PRESSED);
        lv_common_style_set_boader(obj, default_radius, border_default_opa,
                                   default_border_width, default_border_side,
                                   border_default_color, LV_PART_MAIN);
        lv_common_style_set_boader(obj, press_radius, border_press_opa,
                                   press_border_width, press_border_side,
                                   border_press_color, LV_STATE_PRESSED);
    }
    return obj;
}

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
    lv_text_align_t label2_align, lv_font_t *label2_font)
{
    lv_obj_t *obj = lv_obj_get_child_by_id(parent, id);
    if (obj != NULL)
    {
        return obj;
    }
    {
        obj = lv_obj_create(parent);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);

        lv_common_style_set_common(obj, id, x, y, w, h, LV_ALIGN_DEFAULT, LV_PART_MAIN);

        lv_common_style_set_event(obj, container_cb, container_cb,
                                  default_bg_opa, default_bg_color, LV_PART_MAIN,
                                  press_bg_opa, press_bg_color, LV_STATE_PRESSED);
        lv_common_style_set_boader(obj, default_radius, border_default_opa,
                                   default_border_width, default_border_side,
                                   border_default_color, LV_PART_MAIN);
        lv_common_style_set_boader(obj, press_radius, border_press_opa,
                                   press_border_width, press_border_side,
                                   border_press_color, LV_STATE_PRESSED);
    }

    lv_obj_t *label1_obj = NULL, *label2_obj = NULL;

    if ((label1_text != NULL) && (label1_id >= 0))
    {
        label1_obj = lv_common_text_create(obj, label1_id,
                                           label1_x, label1_y, label1_w, label1_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label1_text, label1_default_color,
                                           label1_press_color, label1_align, label1_font);
    }

    if (label2_id >= 0 && label2_text != NULL)
    {
        label2_obj = lv_common_text_create(obj, label2_id,
                                           label2_x, label2_y, label2_w, label2_h,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           label2_text, label2_default_color,
                                           label2_press_color, label2_align, label2_font);
    }
    lv_obj_t *bind_obj = obj;

    if (label1_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label1_obj);
        bind_obj = label1_obj;
    }

    if (label2_obj != NULL)
    {
        lv_obj_set_state_bind(bind_obj, label2_obj);
        bind_obj = label2_obj;
    }
    return obj;
}

lv_obj_t *lv_common_rect_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *rect = lv_obj_create(parent);
    lv_obj_set_pos(rect, x, y);
    lv_obj_set_size(rect, w, h);
    lv_obj_set_style_bg_color(rect, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rect, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(rect, 0, LV_PART_MAIN);
    lv_obj_clear_flag(rect, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return rect;
}

void lv_common_bg_create(lv_obj_t *parent)
{
    lv_common_rect_create(parent, 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    lv_common_rect_create(parent, 0, 0, 190, SCREEN_H, LV_COMMON_PANEL_COLOR);
    lv_common_rect_create(parent, 468, 0, 556, SCREEN_H, LV_COMMON_PANEL_COLOR);
}

static void arrow_link_pressed_cb(lv_event_t *ev)
{
    lv_obj_t *obj = lv_event_get_user_data(ev);
    if (obj)
        lv_obj_set_style_text_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN);
}

static void arrow_link_released_cb(lv_event_t *ev)
{
    lv_obj_t *obj = lv_event_get_user_data(ev);
    if (obj)
        lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
}

void lv_common_arrow_link_press(lv_obj_t *arrow, lv_obj_t *title, lv_obj_t *label)
{
    lv_obj_add_event_cb(arrow, arrow_link_pressed_cb, LV_EVENT_PRESSED, title);
    lv_obj_add_event_cb(arrow, arrow_link_released_cb, LV_EVENT_RELEASED, title);
    lv_obj_add_event_cb(arrow, arrow_link_pressed_cb, LV_EVENT_PRESSED, label);
    lv_obj_add_event_cb(arrow, arrow_link_released_cb, LV_EVENT_RELEASED, label);
}

void lv_common_grad_sep_create(lv_obj_t *parent, int x, int y, int w)
{
    lv_obj_t *left = lv_common_rect_create(parent, x, y, w / 2, 2, LV_COMMON_PANEL_COLOR);
    lv_obj_set_style_bg_grad_color(left, lv_color_hex(LV_COMMON_SEP_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(left, LV_GRAD_DIR_HOR, LV_PART_MAIN);

    lv_obj_t *right = lv_common_rect_create(parent, x + w / 2, y, w / 2, 2, LV_COMMON_SEP_COLOR);
    lv_obj_set_style_bg_grad_color(right, lv_color_hex(LV_COMMON_PANEL_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(right, LV_GRAD_DIR_HOR, LV_PART_MAIN);
}

/************************** 屏幕点击/滚动锁定 (模态弹窗拦截) **************************/
/* 目的: 为弹窗模态提供不创建满屏对象、不触发整屏重绘的拦截方式。
 * 用法: 先调用 lock 清掉屏幕现有对象的点击/滚动, 再创建弹窗内容(新对象不受影响保持可点),
 *       关闭弹窗删除内容后调用 unlock 恢复背景。
 * 说明: 保存的是对象 id 而非对象指针: 锁定期间对象被删除/重建, unlock 仍能按 id 定位恢复,
 *       对象确实不存在时则安全跳过。 */
#define LV_COMMON_CLICK_LOCK_MAX 512
#define LV_COMMON_CLICK_LOCK_INVALID_ID 0xFFFFFFFFUL
#define LV_COMMON_CLICK_LOCK_TEMP_BASE 0xFFFFFF00UL
#define LV_COMMON_CLICK_FLAGS \
    (LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE)

typedef struct
{
    unsigned long id;
    lv_obj_flag_t flags; /* 记录时处于置位的点击/滚动 flag */
    bool temp_id;        /* true 表示 id 为本次锁定临时分配, 恢复后需清回无效值 */
} lv_common_click_lock_item_t;

static lv_common_click_lock_item_t s_click_lock_items[LV_COMMON_CLICK_LOCK_MAX];
static uint32_t s_click_lock_cnt = 0;
static uint32_t s_click_lock_temp_seq = 0;
static bool s_click_lock_active = false;

/* 取对象当前置位的点击/滚动 flag */
static lv_obj_flag_t click_lock_obj_flags(lv_obj_t *obj)
{
    lv_obj_flag_t flags = 0;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
        flags |= LV_OBJ_FLAG_CLICKABLE;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE))
        flags |= LV_OBJ_FLAG_CLICK_FOCUSABLE;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_SCROLLABLE))
        flags |= LV_OBJ_FLAG_SCROLLABLE;
    return flags;
}

/* 统计 obj 子树中需要被锁定的对象数 */
static uint32_t click_lock_count(lv_obj_t *obj)
{
    uint32_t total = (click_lock_obj_flags(obj) != 0) ? 1 : 0;
    uint32_t cnt = lv_obj_get_child_cnt(obj);
    for (uint32_t i = 0; i < cnt; i++)
    {
        total += click_lock_count(lv_obj_get_child(obj, i));
    }
    return total;
}

/* 递归记录并清掉 obj 子树内各对象的点击/滚动 flag */
static void click_lock_apply(lv_obj_t *obj)
{
    lv_obj_flag_t flags = click_lock_obj_flags(obj);
    if (flags != 0)
    {
        unsigned long id = lv_obj_get_id(obj);
        bool temp = false;
        if (id == LV_COMMON_CLICK_LOCK_INVALID_ID)
        {
            id = LV_COMMON_CLICK_LOCK_TEMP_BASE - s_click_lock_temp_seq;
            s_click_lock_temp_seq++;
            lv_obj_set_id(obj, id);
            temp = true;
        }
        s_click_lock_items[s_click_lock_cnt].id = id;
        s_click_lock_items[s_click_lock_cnt].flags = flags;
        s_click_lock_items[s_click_lock_cnt].temp_id = temp;
        s_click_lock_cnt++;
        lv_obj_clear_flag(obj, flags);
    }

    uint32_t cnt = lv_obj_get_child_cnt(obj);
    for (uint32_t i = 0; i < cnt; i++)
    {
        click_lock_apply(lv_obj_get_child(obj, i));
    }
}

int lv_common_screen_click_lock(void)
{
    if (s_click_lock_active)
        return -1;

    lv_obj_t *scr = lv_scr_act();
    if (scr == NULL)
        return -1;

    uint32_t need = click_lock_count(scr);
    if (need > LV_COMMON_CLICK_LOCK_MAX)
        return -2;

    s_click_lock_cnt = 0;
    s_click_lock_temp_seq = 0;
    s_click_lock_active = true;
    click_lock_apply(scr);
    return (int)s_click_lock_cnt;
}

void lv_common_screen_click_unlock(void)
{
    if (!s_click_lock_active)
        return;

    for (uint32_t i = 0; i < s_click_lock_cnt; i++)
    {
        lv_obj_t *obj = lv_obj_get_by_id(lv_scr_act(), s_click_lock_items[i].id);
        if (obj == NULL)
            continue;
        lv_obj_add_flag(obj, s_click_lock_items[i].flags);
        if (s_click_lock_items[i].temp_id)
        {
            lv_obj_set_id(obj, LV_COMMON_CLICK_LOCK_INVALID_ID);
        }
    }

    s_click_lock_cnt = 0;
    s_click_lock_temp_seq = 0;
    s_click_lock_active = false;
}
