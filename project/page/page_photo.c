#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_tfcard.h"
#include "Amy_file_manager.h"
#include <string.h>
#include <stdbool.h>

/************************** 布局常量 **************************/
/*
 * 布局参考 page_event_record.c：
 *   左侧导航栏：图标垂直居中 + 底部文字标签
 *   右侧面板：标题栏 + 分隔线 + 滚动列表
 *   列表条目：52px 行高、半透明底、底部 2px teal 分割线
 */

/* 左侧导航栏 */
#define SIDEBAR_H 600

/* 图标垂直居中: (600 - 102) / 2 = 249, 略向上偏移以容纳文字 */
#define SIDEBAR_ICON_X 30
#define SIDEBAR_ICON_Y 190
#define SIDEBAR_ICON_W 130
#define SIDEBAR_ICON_H 130

/* 图标下方 15px 偏移 */
#define SIDEBAR_TEXT_X 0
#define SIDEBAR_TEXT_Y (SIDEBAR_ICON_Y + SIDEBAR_ICON_H + 15)
#define SIDEBAR_TEXT_W SIDEBAR_W
#define SIDEBAR_TEXT_H 28

/* 右侧列表面板 */
#define PANEL_X 190
#define PANEL_Y 35
#define PANEL_W 690
#define PANEL_H 565

/* 标题栏（面板内） */
#define TITLE_TEXT_X 10
#define TITLE_TEXT_Y 0
#define TITLE_TEXT_W 200
#define TITLE_TEXT_H 32

/* 页面指示器：右对齐 */
#define PAGE_TEXT_X (PANEL_W - 120)
#define PAGE_TEXT_Y 0
#define PAGE_TEXT_W 110
#define PAGE_TEXT_H 32

/* 分隔线（面板内） */
#define SEP_LINE_Y 40
#define SEP_LINE_X1 12
#define SEP_LINE_X2 (PANEL_W - 2)

/* 滚动列表区域 */
#define LIST_X 9
#define LIST_Y 44
#define LIST_W (PANEL_W - 18)
#define LIST_ITEM_H 52
#define LIST_VISIBLE_MAX 10
#define LIST_VISIBLE_H (LIST_ITEM_H * LIST_VISIBLE_MAX)

/* 分页：每页最多 10 条 */
#define LIST_PAGE_SIZE 10

/* 列表条目布局 */
#define ITEM_ICON_X 16
#define ITEM_ICON_Y 11
#define ITEM_ICON_W 38
#define ITEM_ICON_H 29

#define ITEM_NAME_X 66
#define ITEM_NAME_W 500

/* 条目样式：半透明底 + teal 底线 + 按下高亮 */
#define ITEM_BG_OPA LV_OPA_30
#define ITEM_BG_COLOR 0x202020
#define ITEM_BORDER_COLOR 0x00959E
#define ITEM_BORDER_WIDTH 2
#define ITEM_PRESS_BG_COLOR 0x800010

/* 操作弹窗（参考 page_event_record.c） */
#define POPUP_CONT_W 304
#define POPUP_CONT_H 313
#define POPUP_CONT_X ((1024 - POPUP_CONT_W) / 2)
#define POPUP_CONT_Y ((600 - POPUP_CONT_H) / 2)
#define POPUP_BTN_W 217
#define POPUP_BTN_H 48
#define POPUP_BTN_X 43
#define POPUP_BTN_Y_START 25
#define POPUP_BTN_GAP (POPUP_BTN_H + 25)

/************************** 页面上下文 **************************/
static struct
{
    int view_start_idx;
    int cur_page_items;
    int total_files;
    bool at_top_on_scroll_begin;
    bool at_bottom_on_scroll_begin;
    bool loading_page;
    int selected_idx;
} s_ctx = {.selected_idx = -1};

int g_fm_photo_index = 0;

/************************** 函数声明 **************************/
static void photo_back_obj_click(lv_event_t *ev);
static void photo_item_row_click(lv_event_t *ev);
static void photo_scroll_event_cb(lv_event_t *ev);

static void photo_sidebar_create(void);
static void photo_list_panel_create(void);
static void photo_title_bar_create(void);
static void photo_page_info_update(void);
static void photo_back_create(void);
static void photo_list_cont_create(void);
static void photo_page_fill(void);
static void photo_popup_del(void);
static void photo_popup_create(void);
static void photo_list_reload(void);

/************************** 回调实现 **************************/

static void photo_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(media), LV_SCR_LOAD_ANIM_NONE, );
}

static void photo_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
    page_common_tfcard_change_cb(arg);
}

static void photo_item_row_click(lv_event_t *ev)
{
    if (s_ctx.loading_page)
        return;

    lv_obj_t *target = lv_event_get_current_target(ev);
    int idx = (int)(uintptr_t)lv_obj_get_user_data(target);

    if (idx < 0 || idx >= s_ctx.total_files)
        return;

    s_ctx.selected_idx = idx;
    photo_popup_create();
}

static void photo_scroll_event_cb(lv_event_t *ev)
{
    lv_event_code_t code = lv_event_get_code(ev);
    lv_obj_t *cont = lv_event_get_current_target(ev);
    lv_coord_t sb = lv_obj_get_scroll_bottom(cont);
    lv_coord_t st = lv_obj_get_scroll_top(cont);

    if (code == LV_EVENT_SCROLL_BEGIN)
    {
        s_ctx.at_top_on_scroll_begin = (st <= 0);
        s_ctx.at_bottom_on_scroll_begin = (sb <= 0);
    }
    else if (code == LV_EVENT_SCROLL_END)
    {
        if (s_ctx.loading_page)
            return;

        if (s_ctx.at_bottom_on_scroll_begin && sb <= 0)
        {
            if (s_ctx.view_start_idx + s_ctx.cur_page_items >= s_ctx.total_files)
                return;

            s_ctx.loading_page = true;
            s_ctx.view_start_idx += LIST_PAGE_SIZE;
            if (s_ctx.view_start_idx >= s_ctx.total_files)
                s_ctx.view_start_idx = 0;
            photo_page_fill();
            photo_list_cont_create();
            photo_page_info_update();
            s_ctx.loading_page = false;
        }
        else if (s_ctx.at_top_on_scroll_begin && st <= 0 && s_ctx.view_start_idx > 0)
        {
            s_ctx.loading_page = true;
            s_ctx.view_start_idx -= LIST_PAGE_SIZE;
            if (s_ctx.view_start_idx < 0)
                s_ctx.view_start_idx = 0;
            photo_page_fill();
            photo_list_cont_create();
            lv_obj_scroll_to_y(lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), PHOTO_LIST_CONTAINER_ID), LV_COORD_MAX, LV_ANIM_OFF);
            photo_page_info_update();
            s_ctx.loading_page = false;
        }
    }
}

/************************** 左侧导航栏创建 **************************/

static void photo_sidebar_create(void)
{
    lv_common_setting_cont_create(lv_scr_act_current(), PHOTO_SIDEBAR_CONT_ID,
                                  0, 0, SIDEBAR_W, SIDEBAR_H,
                                  NULL,
                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    lv_resource_init(sidebar_icon, ICON_MEDIA_PHOTO_PNG_OFFSET, ICON_MEDIA_PHOTO_PNG_SIZE);
    lv_resource_init(sidebar_focus, ICON_MEDIA_PHOTO_FOCUS_PNG_OFFSET, ICON_MEDIA_PHOTO_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), PHOTO_SIDEBAR_ICON_ID,
                             SIDEBAR_ICON_X, SIDEBAR_ICON_Y, SIDEBAR_ICON_W, SIDEBAR_ICON_H,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &sidebar_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &sidebar_focus);

    lv_common_text_create(lv_scr_act_current(), PHOTO_SIDEBAR_TEXT_ID,
                          SIDEBAR_TEXT_X, SIDEBAR_TEXT_Y, SIDEBAR_TEXT_W, SIDEBAR_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_PHOTO), 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

/************************** 右侧列表面板创建 **************************/

static void photo_list_panel_create(void)
{
    lv_common_setting_cont_create(lv_scr_act_current(), PHOTO_LIST_CONT_ID,
                                  PANEL_X, PANEL_Y, PANEL_W, PANEL_H,
                                  NULL,
                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
}

/************************** 标题栏 + 分隔线 **************************/

static void photo_title_bar_create(void)
{
    lv_common_text_create(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), PHOTO_LIST_TITLE_ID,
                          TITLE_TEXT_X, TITLE_TEXT_Y, TITLE_TEXT_W, TITLE_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_PHOTO), 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

    lv_common_text_create(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), PHOTO_LIST_PAGE_ID,
                          PAGE_TEXT_X, PAGE_TEXT_Y, PAGE_TEXT_W, PAGE_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "000/0", 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_RIGHT_CENTER, lv_font_normal);

    /* 水平分隔线 */
    lv_common_rect_create(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), SEP_LINE_X1, SEP_LINE_Y, SEP_LINE_X2 - SEP_LINE_X1, 2, 0xffffff);
}

/************************** 页码信息更新 **************************/

static void photo_page_info_update(void)
{
    lv_obj_t *page_label = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), PHOTO_LIST_PAGE_ID);
    if (page_label)
        lv_label_set_text_fmt(page_label, "%03d/%03d",
                              s_ctx.total_files > 0 ? s_ctx.view_start_idx + 1 : 0,
                              s_ctx.total_files);
}

/************************** 分页数据填充 **************************/

static void photo_page_fill(void)
{
    int start = s_ctx.view_start_idx;
    int remaining = s_ctx.total_files - start;
    s_ctx.cur_page_items = remaining < LIST_PAGE_SIZE ? remaining : LIST_PAGE_SIZE;
    if (s_ctx.cur_page_items < 0)
        s_ctx.cur_page_items = 0;
}

/************************** 滚动列表控件创建 **************************/

static void photo_list_cont_create(void)
{
    lv_obj_t *lc = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID), PHOTO_LIST_CONTAINER_ID);
    if (lc)
        lv_obj_del(lc);

    lv_resource_init(item_icon, ICON_MEDIA_IMAGE_PNG_OFFSET, ICON_MEDIA_IMAGE_PNG_SIZE);

    lc = lv_list_create(lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_LIST_CONT_ID));
    lv_obj_set_child_id(lc, PHOTO_LIST_CONTAINER_ID);
    lv_common_style_set_common(lc, -1, LIST_X, LIST_Y, LIST_W, LIST_VISIBLE_H,
                               LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_add_flag(lc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(lc, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(lc, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_all(lc, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(lc, 0, LV_PART_MAIN);
    lv_obj_set_flex_align(lc, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_add_event_cb(lc, photo_scroll_event_cb, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(lc, photo_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);

    for (int i = 0; i < LIST_PAGE_SIZE; i++)
    {
        int y = i * LIST_ITEM_H;
        int data_idx = s_ctx.view_start_idx + i;
        bool has_data = (i < s_ctx.cur_page_items && data_idx < s_ctx.total_files);

        if (has_data)
        {
            fm_file_info_t info;
            char label[256] = "";
            if (Amy_fm_get_file(FM_DIR_PHOTO, data_idx, &info) == 0)
                snprintf(label, sizeof(label), "%s", info.name);

            lv_obj_t *row = lv_common_setting_btn_double_label_create(
                lc, PHOTO_ITEM_BASE_ID + i,
                0, y, LIST_W, LIST_ITEM_H,
                photo_item_row_click,
                LV_OPA_TRANSP, ITEM_BG_COLOR, LV_OPA_COVER, ITEM_PRESS_BG_COLOR,
                0, ITEM_BORDER_WIDTH, LV_BORDER_SIDE_BOTTOM,
                LV_OPA_COVER, ITEM_BORDER_COLOR,
                0, ITEM_BORDER_WIDTH, LV_BORDER_SIDE_BOTTOM,
                LV_OPA_COVER, ITEM_BORDER_COLOR,
                PHOTO_ITEM_LABEL1_BASE_ID + i, ITEM_NAME_X, 0, ITEM_NAME_W, LIST_ITEM_H,
                label, 0xffffff, 0xffffff,
                LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal,
                PHOTO_ITEM_LABEL2_BASE_ID + i, ITEM_NAME_X + ITEM_NAME_W + 10, 0, 0, LIST_ITEM_H,
                "", 0xffffff, 0xffffff,
                LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

            lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
            lv_obj_set_user_data(row, (void *)(uintptr_t)data_idx);
            lv_obj_set_ext_click_area(row, 8);

            lv_common_img_btn_create(row, -1,
                                     ITEM_ICON_X, ITEM_ICON_Y, ITEM_ICON_W, ITEM_ICON_H,
                                     NULL, false, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                     &item_icon, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
        }
        else
        {
            lv_common_setting_cont_create(
                lc, -1,
                0, y, LIST_W, LIST_ITEM_H,
                NULL,
                LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
        }
    }
}

/************************** 返回按键创建 **************************/

static void photo_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_obj_t *photo_back = lv_common_img_btn_create(lv_scr_act_current(), PHOTO_BACK_BTN_ID,
                                                    929, 505, 67, 67,
                                                    photo_back_obj_click, true,
                                                    LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    lv_obj_set_ext_click_area(photo_back, 21);
}

/************************** 操作弹窗 **************************/

static void photo_popup_del(void)
{
    lv_obj_t *popup = lv_obj_get_child_by_id(lv_scr_act_current(), PHOTO_POPUP_CONT_ID);
    if (popup)
        lv_obj_del(popup);
}

/* 删除后刷新文件管理器缓存并重建列表 */
static void photo_list_reload(void)
{
    Amy_fm_refresh(FM_DIR_PHOTO);
    s_ctx.total_files = Amy_fm_get_count(FM_DIR_PHOTO);
    if (s_ctx.total_files <= 0)
    {
        s_ctx.total_files = 0;
        s_ctx.view_start_idx = 0;
    }
    else if (s_ctx.view_start_idx >= s_ctx.total_files)
    {
        s_ctx.view_start_idx = 0;
    }

    photo_page_fill();
    photo_list_cont_create();
    photo_page_info_update();
}

static void photo_popup_open_click(lv_event_t *ev)
{
    photo_popup_del();

    if (s_ctx.selected_idx < 0 || s_ctx.selected_idx >= s_ctx.total_files)
        return;

    g_fm_photo_index = s_ctx.selected_idx;
    Amy_lv_page_goto(lv_page(photo_play), LV_SCR_LOAD_ANIM_NONE, );
}

static void photo_popup_delete_click(lv_event_t *ev)
{
    photo_popup_del();

    if (s_ctx.selected_idx < 0)
        return;

    Amy_fm_delete(FM_DIR_PHOTO, s_ctx.selected_idx);
    s_ctx.selected_idx = -1;
    photo_list_reload();
}

static void photo_popup_delete_all_click(lv_event_t *ev)
{
    photo_popup_del();

    Amy_fm_delete_all(FM_DIR_PHOTO);
    s_ctx.selected_idx = -1;
    photo_list_reload();
}

static void photo_popup_close_click(lv_event_t *ev)
{
    photo_popup_del();
}

static void photo_popup_btn_create(lv_obj_t *parent, int btn_index, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_common_setting_cont_create(parent, -1,
                                                  POPUP_BTN_X, POPUP_BTN_Y_START + btn_index * POPUP_BTN_GAP,
                                                  POPUP_BTN_W, POPUP_BTN_H,
                                                  cb, LV_OPA_COVER, 0x0A3340, LV_OPA_COVER, 0XFC0000,
                                                  8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    lv_obj_t *label = lv_common_text_create(btn, -1,
                                            0, 0, 0, 0,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            text, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal);
    lv_obj_center(label);
}

static void photo_popup_create(void)
{
    photo_popup_del();

    /* 全屏透明遮罩 (拦截背景点击) */
    lv_obj_t *overlay = lv_obj_create(lv_scr_act_current());
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
    lv_common_style_set_common(overlay, PHOTO_POPUP_CONT_ID, 0, 0, 1024, 600, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *cont = lv_common_setting_cont_create(overlay, -1,
                                                   POPUP_CONT_X, POPUP_CONT_Y, POPUP_CONT_W, POPUP_CONT_H,
                                                   NULL, LV_OPA_80, 0x00131D, LV_OPA_80, 0x00131D,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(cont, true, LV_PART_MAIN);

    int btn_idx = 0;
    photo_popup_btn_create(cont, btn_idx++, lang_str_get(STR_OPEN), photo_popup_open_click);
    photo_popup_btn_create(cont, btn_idx++, lang_str_get(STR_DELETE), photo_popup_delete_click);
    photo_popup_btn_create(cont, btn_idx++, lang_str_get(STR_ALL_DELETE), photo_popup_delete_all_click);
    photo_popup_btn_create(cont, btn_idx++, lang_str_get(STR_CLOSE), photo_popup_close_click);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(photo)
{
    lv_common_bg_create(lv_scr_act_current());

    photo_sidebar_create();
    photo_list_panel_create();
    photo_title_bar_create();

    s_ctx.view_start_idx = 0;
    s_ctx.cur_page_items = 0;
    s_ctx.at_top_on_scroll_begin = false;
    s_ctx.at_bottom_on_scroll_begin = false;
    s_ctx.loading_page = false;
    s_ctx.selected_idx = -1;

    Amy_fm_refresh(FM_DIR_PHOTO);
    s_ctx.total_files = Amy_fm_get_count(FM_DIR_PHOTO);

    photo_page_fill();
    photo_list_cont_create();
    photo_page_info_update();
    photo_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, photo_sd_change_cb);
}

static void Amy_lv_page_quit(photo)
{
    photo_popup_del();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(photo);
