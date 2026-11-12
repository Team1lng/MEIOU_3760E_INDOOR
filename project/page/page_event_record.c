#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_tfcard.h"
#include "../monitor.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/************************** 布局常量 **************************/
/*
 * 布局参考 EMOS-Indoor layout_file_list.c 的事件记录列表风格：
 *   左侧导航栏：图标垂直居中 + 底部文字标签
 *   右侧面板：标题栏 + 分隔线 + 滚动列表
 *   列表条目：52px 行高、半透明底、底部 2px teal 分割线
 */

/* 左侧导航栏 */
#define SIDEBAR_H 600

/* 图标垂直居中: (600 - 130) / 2 = 235, 略向上偏移以容纳文字 */
#define SIDEBAR_ICON_X 30
#define SIDEBAR_ICON_Y 190
#define SIDEBAR_ICON_W 130
#define SIDEBAR_ICON_H 130

/* 图标下方 15px 偏移（参考 EMOS-Indoor value_ofs_y = 15） */
#define SIDEBAR_TEXT_X 0
#define SIDEBAR_TEXT_Y (SIDEBAR_ICON_Y + SIDEBAR_ICON_H + 15)
#define SIDEBAR_TEXT_W SIDEBAR_W
#define SIDEBAR_TEXT_H 28

/* 右侧列表面板：对应 EMOS-Indoor 列表区域 (199, 76) ，标题在上方 */
#define PANEL_X 190
#define PANEL_Y 35
#define PANEL_W 690
#define PANEL_H 565

/* 标题栏（面板内，对应 EMOS-Indoor 标题 (200,38) + 分隔线 (202,76) ） */
#define TITLE_TEXT_X 10
#define TITLE_TEXT_Y 0
#define TITLE_TEXT_W 200
#define TITLE_TEXT_H 32

/* 页面指示器：EMOS-Indoor 风格 "001/001" ，右对齐 */
#define PAGE_TEXT_X (PANEL_W - 120)
#define PAGE_TEXT_Y 0
#define PAGE_TEXT_W 110
#define PAGE_TEXT_H 32

/* 分隔线（面板内 Y=40 → 屏幕 Y=75，接近 EMOS-Indoor 的 76） */
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

/* DB 分页：每页最多 50 条（标准） */
#define LIST_PAGE_SIZE 50

/* 列表条目三部分布局（参考 EMOS-Indoor: 16,13 34×27 图标 + 文件名 + 通道号） */
#define ITEM_ICON_X 16
#define ITEM_ICON_Y 11
#define ITEM_ICON_W 38
#define ITEM_ICON_H 29

#define ITEM_NAME_X 66 /* icon右边缘 16+38=54，间距12px */
#define ITEM_NAME_W 370

#define ITEM_CHANNEL_X 500
#define ITEM_CHANNEL_W 150

/* EMOS-Indoor 风格：半透明底 + teal 底线 + 按下高亮 */
#define ITEM_BG_OPA LV_OPA_30
#define ITEM_BG_COLOR 0x202020
#define ITEM_BORDER_COLOR 0x00959E
#define ITEM_BORDER_WIDTH 2
#define ITEM_PRESS_BG_COLOR 0x800010

/* 操作弹窗（参考 EMOS-Indoor record_list_window_create） */
#define POPUP_CONT_W 304
#define POPUP_CONT_H 313
#define POPUP_CONT_X ((1024 - POPUP_CONT_W) / 2)
#define POPUP_CONT_Y ((600 - POPUP_CONT_H) / 2)
#define POPUP_BTN_W 217
#define POPUP_BTN_H 48
#define POPUP_BTN_X 43
#define POPUP_BTN_Y_START 25
#define POPUP_BTN_GAP (POPUP_BTN_H + 25)

/************************** 记录类型配置（event 页面入口设置 g_media_record_type） **************************/

static struct
{
    unsigned int icon_offset;
    unsigned int icon_size;
    xls_str_id_t title_id;
    unsigned int media_mask;
} s_type_cfg[3] = {
    [MEDIA_RECORD_TYPE_CALL] = {
        .icon_offset = ICON_RECORD_CALL_RECORD_PNG_OFFSET,
        .icon_size = ICON_RECORD_CALL_RECORD_PNG_SIZE,
        .title_id = STR_CALL_RECORD,
        .media_mask = MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO |
                      MEDIA_MODE_MANUAL | MEDIA_MODE_AUTO |
                      MEDIA_CHANNEL_ALL | MEDIA_ATTR_ALL,
    },
    [MEDIA_RECORD_TYPE_MESSAGE] = {
        .icon_offset = ICON_RECORD_MESSAGE_RECORD_PNG_OFFSET,
        .icon_size = ICON_RECORD_MESSAGE_RECORD_PNG_SIZE,
        .title_id = STR_MESSAGE_RECORD,
        .media_mask = MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO | MEDIA_MODE_MESSAGE | MEDIA_CHANNEL_ALL | MEDIA_ATTR_ALL,
    },
    [MEDIA_RECORD_TYPE_DETECTION] = {
        .icon_offset = ICON_RECORD_DETECTION_RECORD_PNG_OFFSET,
        .icon_size = ICON_RECORD_DETECTION_RECORD_PNG_SIZE,
        .title_id = STR_MOTION_RECORD,
        .media_mask = MEDIA_TYPE_VIDEO | MEDIA_TYPE_PHOTO | MEDIA_MODE_MOTION | MEDIA_CHANNEL_ALL | MEDIA_ATTR_ALL,
    },
};

/************************** 录制模式图标**************************/
static firamware_resource s_icon_manual = {{'S', '@'}, ICON_RECORD_MANUAL_PNG_OFFSET, ICON_RECORD_MANUAL_PNG_SIZE};
static firamware_resource s_icon_auto = {{'S', '@'}, ICON_RECORD_AUTO_PNG_OFFSET, ICON_RECORD_AUTO_PNG_SIZE};
static firamware_resource s_icon_detection = {{'S', '@'}, ICON_RECORD_DETECTION_PNG_OFFSET, ICON_RECORD_DETECTION_PNG_SIZE};
static firamware_resource s_icon_message = {{'S', '@'}, ICON_RECORD_MESSAGE_PNG_OFFSET, ICON_RECORD_MESSAGE_PNG_SIZE};

static const char *event_record_file_ext(unsigned long mask)
{
    if (mask & MEDIA_TYPE_VIDEO)
        return "avi";
    if (mask & MEDIA_TYPE_PHOTO)
        return "jpg";
    return "";
}

static void *event_record_icon_src(unsigned long mask)
{
    if (mask & MEDIA_MODE_MANUAL)
        return &s_icon_manual;
    if (mask & MEDIA_MODE_AUTO)
        return &s_icon_auto;
    if (mask & MEDIA_MODE_MOTION)
        return &s_icon_detection;
    if (mask & MEDIA_MODE_MESSAGE)
        return &s_icon_message;
    return NULL;
}

static const char *event_record_channel_name(int ch)
{
    switch (ch)
    {
    case 0:
        return "DOOR1";
    case 1:
        return "DOOR2";
    case 2:
        return "CCTV1";
    case 3:
        return "CCTV2";
    default:
        return NULL;
    }
}

/************************** 页面上下文 **************************/
static struct
{
    Amy_media_context media_ctx;         /* 媒体数据库句柄（enter 打开，按 g_media_record_type 设过滤 mask，每页 50） */
    int total_files;                     /* 匹配到的媒体总数，即全局总数 all_total */
    int cur_page_items;                  /* 当前列表页实际填充的行数（可能不足 LIST_PAGE_SIZE） */
    int view_start_idx;                  /* 当前列表页首行在全局序列中的序号（0 为最新，跨天连续） */
    bool reuse_db_chunk_on_next;         /* 下次取“下一页”时是否复用当前 DB 块，不重新查库 */
    bool reuse_db_chunk_on_prev;         /* 下次取“上一页”时是否复用当前 DB 块，不重新查库 */
    unsigned long db_chunk_start_idx;    /* 最近一次从 DB 取回的块首行对应的全局序号（= ctx->all_index） */
    media_file_info_t page_row_buf[LIST_PAGE_SIZE]; /* 当前页行缓存，每行含 year/month/day */
    bool at_top_on_scroll_begin;         /* 本次滚动开始时是否已在顶部 */
    bool at_bottom_on_scroll_begin;      /* 本次滚动开始时是否已在底部 */
    bool loading_page;                   /* 正在加载/重建列表页，期间忽略点击与滚动事件 */
    int selected_global_idx;             /* 弹窗中选中的媒体全局序号，-1 表示未选 */
} s_ctx = {.selected_global_idx = -1};

/************************** 函数声明 **************************/
static void event_record_back_obj_click(lv_event_t *ev);
static void event_record_item_row_click(lv_event_t *ev);
static void event_record_scroll_event_cb(lv_event_t *ev);

static void event_record_sidebar_create(void);
static void event_record_list_panel_create(void);
static void event_record_title_bar_create(void);
static void event_record_page_info_update(void);
static void event_record_back_create(void);
static void event_record_list_cont_create(void);
static void event_record_popup_create(void);
static void event_record_popup_del(void);

/* 数据处理（参考 page_demo.txt） */
static void event_record_merge_commit(int visible_rows);
static bool event_record_next_trim_chunk_from_index(int *chunk_size, const media_file_info_t **chunk,
                                                    unsigned long db_start, unsigned long global_index);
static bool event_record_prev_trim_chunk_from_index(int *chunk_size, const media_file_info_t **chunk,
                                                    unsigned long db_start, unsigned long global_index);
static void event_record_merge_mark_reuse_on_prev(void);
static void event_record_merge_fill_next(bool first);
static void event_record_merge_fill_prev(void);

/************************** 回调实现 **************************/

static void event_record_back_obj_click(lv_event_t *ev)
{
    if (s_ctx.media_ctx)
    {
        Amy_media_close(s_ctx.media_ctx);
        s_ctx.media_ctx = NULL;
    }
    Amy_lv_page_goto(lv_page(event), LV_SCR_LOAD_ANIM_NONE, );
}

static void event_record_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        event_record_back_obj_click(NULL);
    page_common_tfcard_change_cb(arg);
}

static void event_record_item_row_click(lv_event_t *ev)
{
    if (s_ctx.loading_page)
        return;

    lv_obj_t *target = lv_event_get_current_target(ev);
    int idx = (int)(uintptr_t)lv_obj_get_user_data(target);

    if (!s_ctx.media_ctx || idx < 0 || idx >= s_ctx.cur_page_items)
        return;

    s_ctx.selected_global_idx = s_ctx.view_start_idx + idx;
    event_record_popup_create();
}

static void event_record_scroll_event_cb(lv_event_t *ev)
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
            /* 已到末尾则不再翻页 */
            if (s_ctx.view_start_idx + s_ctx.cur_page_items >= s_ctx.total_files)
                return;

            s_ctx.loading_page = true;
            event_record_merge_fill_next(false);
            event_record_list_cont_create();
            event_record_page_info_update();
            s_ctx.loading_page = false;
        }
        else if (s_ctx.at_top_on_scroll_begin && st <= 0 && s_ctx.view_start_idx > 0)
        {
            s_ctx.loading_page = true;
            event_record_merge_fill_prev();
            event_record_list_cont_create();
            lv_obj_scroll_to_y(lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), EVENT_RECORD_LIST_CONTAINER_ID), LV_COORD_MAX, LV_ANIM_OFF);
            event_record_page_info_update();
            s_ctx.loading_page = false;
        }
    }
}

/************************** 左侧导航栏创建 **************************/

static void event_record_sidebar_create(void)
{
    lv_common_setting_cont_create(lv_scr_act_current(), EVENT_RECORD_SIDEBAR_CONT_ID,
                                  0, 0, SIDEBAR_W, SIDEBAR_H,
                                  NULL,
                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    lv_resource_init(sidebar_icon, s_type_cfg[g_media_record_type].icon_offset,
                     s_type_cfg[g_media_record_type].icon_size);
    unsigned long focus_offset = 0;
    unsigned long focus_size = 0;
    if (g_media_record_type == MEDIA_RECORD_TYPE_CALL)
    {
        focus_offset = ICON_RECORD_CALL_RECORD_FOCUS_PNG_OFFSET;
        focus_size = ICON_RECORD_CALL_RECORD_FOCUS_PNG_SIZE;
    }
    else if (g_media_record_type == MEDIA_RECORD_TYPE_MESSAGE)
    {
        focus_offset = ICON_RECORD_MESSAGE_RECORD_FOCUS_PNG_OFFSET;
        focus_size = ICON_RECORD_MESSAGE_RECORD_FOCUS_PNG_SIZE;
    }
    else if (g_media_record_type == MEDIA_RECORD_TYPE_DETECTION)
    {
        focus_offset = ICON_RECORD_DETECTION_RECORD_FOCUS_PNG_OFFSET;
        focus_size = ICON_RECORD_DETECTION_RECORD_FOCUS_PNG_SIZE;
    }
    lv_resource_init(sidebar_icon_focus, focus_offset, focus_size);
    lv_common_img_btn_create(lv_scr_act_current(), EVENT_RECORD_SIDEBAR_ICON_ID,
                             SIDEBAR_ICON_X, SIDEBAR_ICON_Y, SIDEBAR_ICON_W, SIDEBAR_ICON_H,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &sidebar_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &sidebar_icon_focus);

    lv_common_text_create(lv_scr_act_current(), EVENT_RECORD_SIDEBAR_TEXT_ID,
                          SIDEBAR_TEXT_X, SIDEBAR_TEXT_Y, SIDEBAR_TEXT_W, SIDEBAR_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(s_type_cfg[g_media_record_type].title_id), 0xffffff, 0xF91313,
                          LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

/************************** 右侧列表面板创建 **************************/

static void event_record_list_panel_create(void)
{
    lv_common_setting_cont_create(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID,
                                  PANEL_X, PANEL_Y, PANEL_W, PANEL_H,
                                  NULL,
                                  LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
}

/************************** 标题栏 + 分隔线 **************************/

static void event_record_title_bar_create(void)
{
    /* 标题文字（参考 EMOS-Indoor 标题位置） */
    lv_common_text_create(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), EVENT_RECORD_LIST_TITLE_ID,
                          TITLE_TEXT_X, TITLE_TEXT_Y, TITLE_TEXT_W, TITLE_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(s_type_cfg[g_media_record_type].title_id), 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

    /* 页面指示器（EMOS-Indoor 风格 "001/001"） */
    lv_common_text_create(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), EVENT_RECORD_LIST_PAGE_ID,
                          PAGE_TEXT_X, PAGE_TEXT_Y, PAGE_TEXT_W, PAGE_TEXT_H,
                          NULL,
                          LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          "000/0", 0xffffff, 0x02A9FF,
                          LV_TEXT_ALIGN_RIGHT_CENTER, lv_font_normal);

    /* 水平分隔线（参考 EMOS-Indoor lv_line at y=76） */
    lv_common_rect_create(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), SEP_LINE_X1, SEP_LINE_Y, SEP_LINE_X2 - SEP_LINE_X1, 2, 0xffffff);
}

/************************** 页码信息更新 **************************/

static void event_record_page_info_update(void)
{
    lv_obj_t *page_label = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), EVENT_RECORD_LIST_PAGE_ID);
    if (page_label)
        lv_label_set_text_fmt(page_label, "%03d/%03d",
                              s_ctx.total_files > 0 ? s_ctx.view_start_idx + 1 : 0,
                              s_ctx.total_files);
}

/************************** 数据处理（参考 page_demo.txt） **************************/

/**
 * @brief 将合并结果写回页面上下文
 */
static void event_record_merge_commit(int visible_rows)
{
    s_ctx.cur_page_items = visible_rows;
}

/**
 * @brief 从 global_index 起裁剪 DB 块（丢弃更早的记录，用于下一页）
 */
static bool event_record_next_trim_chunk_from_index(int *chunk_size, const media_file_info_t **chunk,
                                                    unsigned long db_start, unsigned long global_index)
{
    if (db_start >= global_index)
    {
        return (*chunk_size > 0);
    }

    int skip = (int)(global_index - db_start);
    if (*chunk_size > skip)
    {
        *chunk_size -= skip;
        *chunk += skip;
        return true;
    }

    *chunk_size = 0;
    return false;
}

/**
 * @brief 从 global_index 起裁剪 DB 块（丢弃最晚的记录，用于上一页）
 */
static bool event_record_prev_trim_chunk_from_index(int *chunk_size, const media_file_info_t **chunk,
                                                    unsigned long db_start, unsigned long global_index)
{
    int skip = *chunk_size - ((int)(global_index - db_start));
    if (*chunk_size > skip)
    {
        *chunk_size -= skip;
        return true;
    }

    *chunk_size = 0;
    return false;
}

/**
 * @brief 若 DB 块起点落后于 UI 页起点，标记「上一页」需复用当前 DB 块
 */
static void event_record_merge_mark_reuse_on_prev(void)
{
    if (s_ctx.db_chunk_start_idx < (unsigned long)s_ctx.view_start_idx)
    {
        s_ctx.reuse_db_chunk_on_prev = true;
    }
}

/**
 * @brief 下一页：从当前 DB 块起向后合并，填满一页（最多 LIST_PAGE_SIZE 行）
 */
static void event_record_merge_fill_next(bool first)
{
    Amy_media_context ctx = s_ctx.media_ctx;
    if (!ctx)
    {
        return;
    }

    int target_rows = 0;
    unsigned long view_start = (unsigned long)s_ctx.view_start_idx;
    unsigned long temp_view_start = view_start + LIST_PAGE_SIZE;
    unsigned int remain = (s_ctx.total_files % LIST_PAGE_SIZE);
    if (temp_view_start == (unsigned long)(s_ctx.total_files - (int)remain) && remain != 0 && !first)
    {
        target_rows = remain;
    }
    else if (s_ctx.total_files != 0)
    {
        target_rows = s_ctx.total_files > LIST_PAGE_SIZE ? LIST_PAGE_SIZE : s_ctx.total_files;
    }

    if (s_ctx.view_start_idx + LIST_PAGE_SIZE >= s_ctx.total_files)
    {
        s_ctx.view_start_idx = 0;
    }
    else
    {
        s_ctx.view_start_idx += LIST_PAGE_SIZE;
    }
    if (first)
    {
        s_ctx.view_start_idx = 0;
    }

    int head_slots_needed = target_rows;
    int head_rows_filled = 0;
    while (head_rows_filled < head_slots_needed)
    {
        int db_chunk_size = 0;
        const media_file_info_t *next_chunk = NULL;
        if (s_ctx.reuse_db_chunk_on_next)
        {
            next_chunk = Amy_media_current_page(ctx, &db_chunk_size);
        }
        else
        {
            next_chunk = Amy_media_next_page(ctx, &db_chunk_size);
        }
        s_ctx.reuse_db_chunk_on_next = false;
        s_ctx.reuse_db_chunk_on_prev = false;
        Amy_media_all_index_total_get(ctx, &s_ctx.db_chunk_start_idx, NULL);
        if (!next_chunk)
        {
            break;
        }
        if ((s_ctx.view_start_idx == 0) && (s_ctx.db_chunk_start_idx != 0) &&
            s_ctx.db_chunk_start_idx >= (unsigned long)head_slots_needed)
        {
            continue;
        }
        if (!event_record_next_trim_chunk_from_index(&db_chunk_size, &next_chunk,
                                                     s_ctx.db_chunk_start_idx,
                                                     (unsigned long)s_ctx.view_start_idx))
        {
            continue;
        }
        if (db_chunk_size + head_rows_filled <= head_slots_needed)
        {
            memcpy(&s_ctx.page_row_buf[head_rows_filled], next_chunk,
                   (size_t)db_chunk_size * sizeof(media_file_info_t));
            head_rows_filled += db_chunk_size;
            if (s_ctx.db_chunk_start_idx + (unsigned long)db_chunk_size >=
                (unsigned long)s_ctx.total_files)
            {
                break;
            }
        }
        else
        {
            s_ctx.reuse_db_chunk_on_next = true;
            int copy_count = head_slots_needed - head_rows_filled;
            memcpy(&s_ctx.page_row_buf[head_rows_filled], next_chunk,
                   (size_t)copy_count * sizeof(media_file_info_t));
            head_rows_filled = head_slots_needed;
            break;
        }
    }

    event_record_merge_commit(head_rows_filled);
    event_record_merge_mark_reuse_on_prev();
}

/**
 * @brief 上一页：从当前 DB 块起向前合并，填满一页
 */
static void event_record_merge_fill_prev(void)
{
    Amy_media_context ctx = s_ctx.media_ctx;
    if (!ctx)
    {
        return;
    }

    int target_rows = 0;
    unsigned long view_start = (unsigned long)s_ctx.view_start_idx;
    unsigned int remain = (s_ctx.total_files % LIST_PAGE_SIZE);
    if (view_start == 0 && remain)
    {
        target_rows = remain;
    }
    else
    {
        target_rows = s_ctx.total_files > LIST_PAGE_SIZE ? LIST_PAGE_SIZE : s_ctx.total_files;
    }

    int head_slots_needed = target_rows;
    int head_rows_filled = 0;
    while (head_rows_filled < head_slots_needed)
    {
        int prev_chunk_size = 0;
        const media_file_info_t *prev_chunk = NULL;
        if (s_ctx.reuse_db_chunk_on_prev)
        {
            prev_chunk = Amy_media_current_page(ctx, &prev_chunk_size);
        }
        else
        {
            prev_chunk = Amy_media_prev_page(ctx, &prev_chunk_size);
        }
        s_ctx.reuse_db_chunk_on_next = false;
        s_ctx.reuse_db_chunk_on_prev = false;
        Amy_media_all_index_total_get(ctx, &s_ctx.db_chunk_start_idx, NULL);
        if (!prev_chunk)
        {
            break;
        }
        if (s_ctx.db_chunk_start_idx >= view_start &&
            ((view_start != 0) ||
             (s_ctx.db_chunk_start_idx + prev_chunk_size <= (unsigned long)LIST_PAGE_SIZE)))
        {
            continue;
        }
        if ((s_ctx.db_chunk_start_idx + prev_chunk_size > (unsigned long)s_ctx.view_start_idx) &&
            s_ctx.view_start_idx != 0)
        {
            if (!event_record_prev_trim_chunk_from_index(&prev_chunk_size, &prev_chunk,
                                                         s_ctx.db_chunk_start_idx,
                                                         (unsigned long)s_ctx.view_start_idx))
            {
                continue;
            }
        }
        if (prev_chunk_size + head_rows_filled <= head_slots_needed)
        {
            int dst_offset = head_slots_needed - head_rows_filled - prev_chunk_size;
            memcpy(&s_ctx.page_row_buf[dst_offset], prev_chunk,
                   (size_t)prev_chunk_size * sizeof(media_file_info_t));
            head_rows_filled += prev_chunk_size;
        }
        else
        {
            int copy_count = head_slots_needed - head_rows_filled;
            prev_chunk += (prev_chunk_size - copy_count);
            memcpy(s_ctx.page_row_buf, prev_chunk,
                   (size_t)copy_count * sizeof(media_file_info_t));
            head_rows_filled += copy_count;
            break;
        }
    }

    if (s_ctx.view_start_idx == 0)
    {
        if (remain != 0)
        {
            s_ctx.view_start_idx = s_ctx.total_files - (s_ctx.total_files % LIST_PAGE_SIZE);
        }
        else
        {
            s_ctx.view_start_idx = s_ctx.total_files - LIST_PAGE_SIZE;
        }
    }
    else
    {
        s_ctx.view_start_idx -= LIST_PAGE_SIZE;
    }

    event_record_merge_commit(head_rows_filled);
    event_record_merge_mark_reuse_on_prev();
}

/************************** 滚动列表控件创建 **************************/

static void event_record_list_cont_create(void)
{
    lv_obj_t *lc = lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID), EVENT_RECORD_LIST_CONTAINER_ID);
    if (lc)
        lv_obj_del(lc);

    int num = s_ctx.cur_page_items;
    const media_file_info_t *items = (num > 0) ? s_ctx.page_row_buf : NULL;

    /* 使用 lv_list_create 创建滚动列表容器 */
    lc = lv_list_create(lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_LIST_CONT_ID));
    lv_obj_set_child_id(lc, EVENT_RECORD_LIST_CONTAINER_ID);
    lv_common_style_set_common(lc, -1, LIST_X, LIST_Y, LIST_W, LIST_VISIBLE_H,
                               LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_add_flag(lc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(lc, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(lc, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_all(lc, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(lc, 0, LV_PART_MAIN);
    lv_obj_set_flex_align(lc, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_add_event_cb(lc, event_record_scroll_event_cb, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(lc, event_record_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);

    for (int i = 0; i < num; i++)
    {
        int y = i * LIST_ITEM_H;
        const media_file_info_t *info = &items[i];

        char label1[64];
        char label2[64];

        unsigned long long ts = 0;
        sscanf(info->timestamp, "%llu", &ts);
        struct tm local_tm;
        Amy_time2localtime(ts, local_tm);
        snprintf(label1, sizeof(label1), "%04d-%02d-%02d  %02d:%02d:%02d  .%s",
                 local_tm.tm_year, local_tm.tm_mon, local_tm.tm_mday,
                 local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
                 event_record_file_ext(info->mask));

        int ch = Amy_media_mask_to_channel(info->mask);
        const char *ch_name = event_record_channel_name(ch);
        if (ch_name)
            snprintf(label2, sizeof(label2), "%s", ch_name);

        bool is_new = (info->mask & MEDIA_ATTR_NEW) != 0;
        unsigned int label1_color = is_new ? 0xff1111 : 0xffffff;

        lv_obj_t *row = lv_common_setting_btn_double_label_create(
            lc, EVENT_RECORD_ITEM_BASE_ID + i,
            0, y, LIST_W, LIST_ITEM_H,
            event_record_item_row_click,
            LV_OPA_TRANSP, ITEM_BG_COLOR, LV_OPA_COVER, ITEM_PRESS_BG_COLOR,
            0, ITEM_BORDER_WIDTH, LV_BORDER_SIDE_BOTTOM,
            LV_OPA_COVER, ITEM_BORDER_COLOR,
            0, ITEM_BORDER_WIDTH, LV_BORDER_SIDE_BOTTOM,
            LV_OPA_COVER, ITEM_BORDER_COLOR,
            EVENT_RECORD_ITEM_LABEL1_BASE_ID + i, ITEM_NAME_X, 0, ITEM_NAME_W, LIST_ITEM_H,
            label1, label1_color, 0xffffff,
            LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal,
            EVENT_RECORD_ITEM_LABEL2_BASE_ID + i, ITEM_CHANNEL_X, 0, ITEM_CHANNEL_W, LIST_ITEM_H,
            label2, 0xffffff, 0xffffff,
            LV_TEXT_ALIGN_RIGHT_CENTER, lv_font_normal);

        lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
        lv_obj_set_user_data(row, (void *)(uintptr_t)i);
        lv_obj_set_ext_click_area(row, 8);

        lv_common_img_btn_create(row, -1,
                                 ITEM_ICON_X, ITEM_ICON_Y, ITEM_ICON_W, ITEM_ICON_H,
                                 NULL, false, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                 event_record_icon_src(info->mask), LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    }
}

/************************** 返回按键创建 **************************/

static void event_record_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *event_record_back = lv_common_img_btn_create(lv_scr_act_current(), EVENT_RECORD_BACK_BTN_ID,
                                                           929, 505, 67, 67,
                                                           event_record_back_obj_click, true,
                                                           LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                           &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(event_record_back, 21);
}

/************************** 操作弹窗 **************************/

static void event_record_popup_del(void)
{
    lv_obj_t *popup = lv_obj_get_child_by_id(lv_scr_act_current(), EVENT_RECORD_POPUP_CONT_ID);
    if (popup)
        lv_obj_del(popup);
}

static void event_record_popup_open_click(lv_event_t *ev)
{
    event_record_popup_del();

    if (!s_ctx.media_ctx || s_ctx.selected_global_idx < 0)
        return;

    /* 从 s_ctx.page_row_buf 直接获取文件信息，避免 ctx->tm 跨天错位导致数据错误 */
    int buf_idx = s_ctx.selected_global_idx - s_ctx.view_start_idx;
    if (buf_idx < 0 || buf_idx >= s_ctx.cur_page_items)
        return;
    const media_file_info_t *selected = &s_ctx.page_row_buf[buf_idx];

    Amy_media_all_index_set(s_ctx.media_ctx, s_ctx.selected_global_idx);

    g_media_browse.ctx = s_ctx.media_ctx;
    memcpy(&g_media_browse.info, selected, sizeof(media_file_info_t));
    Amy_media_info_conver_path(s_ctx.media_ctx, selected, g_media_browse.path, sizeof(g_media_browse.path));

    Amy_lv_page_goto(lv_page(record_play), LV_SCR_LOAD_ANIM_NONE, );
}

static void event_record_popup_delete_click(lv_event_t *ev)
{
    event_record_popup_del();

    if (!s_ctx.media_ctx || s_ctx.selected_global_idx < 0)
        return;

    Amy_media_all_index_set(s_ctx.media_ctx, s_ctx.selected_global_idx);
    Amy_media_delete_file(s_ctx.media_ctx);

    unsigned long total = 0;
    Amy_media_all_index_total_get(s_ctx.media_ctx, NULL, &total);
    s_ctx.total_files = (int)total;

    s_ctx.view_start_idx = 0;
    s_ctx.reuse_db_chunk_on_next = false;
    s_ctx.reuse_db_chunk_on_prev = false;
    Amy_media_first_page(s_ctx.media_ctx, NULL);
    s_ctx.reuse_db_chunk_on_next = true;
    event_record_merge_fill_next(true);
    s_ctx.view_start_idx = 0;
    s_ctx.reuse_db_chunk_on_prev = false;

    event_record_list_cont_create();
    event_record_page_info_update();
}

static void event_record_popup_delete_all_click(lv_event_t *ev)
{
    event_record_popup_del();

    if (!s_ctx.media_ctx)
        return;

    Amy_media_delete_all_file(s_ctx.media_ctx);

    s_ctx.total_files = 0;
    s_ctx.cur_page_items = 0;
    s_ctx.view_start_idx = 0;
    s_ctx.reuse_db_chunk_on_next = false;
    s_ctx.reuse_db_chunk_on_prev = false;
    s_ctx.db_chunk_start_idx = 0;

    event_record_list_cont_create();
    event_record_page_info_update();
}

static void event_record_popup_close_click(lv_event_t *ev)
{
    event_record_popup_del();
}

static void event_record_popup_btn_create(lv_obj_t *parent, int btn_index, const char *text, lv_event_cb_t cb)
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

static void event_record_popup_create(void)
{
    event_record_popup_del();

    /* 全屏透明遮罩 (拦截背景点击, 不关闭弹窗，参考 lv_common_msgbox_body_create) */
    lv_obj_t *overlay = lv_obj_create(lv_scr_act_current());
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
    lv_common_style_set_common(overlay, EVENT_RECORD_POPUP_CONT_ID, 0, 0, 1024, 600, LV_ALIGN_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    /* 弹窗容器（参考 EMOS-Indoor: #00131D bg, 80% opacity, radius 20） */
    lv_obj_t *cont = lv_common_setting_cont_create(overlay, -1,
                                                   POPUP_CONT_X, POPUP_CONT_Y, POPUP_CONT_W, POPUP_CONT_H,
                                                   NULL, LV_OPA_80, 0x00131D, LV_OPA_80, 0x00131D,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(cont, true, LV_PART_MAIN);

    int btn_idx = 0;
    event_record_popup_btn_create(cont, btn_idx++, lang_str_get(STR_OPEN), event_record_popup_open_click);
    event_record_popup_btn_create(cont, btn_idx++, lang_str_get(STR_DELETE), event_record_popup_delete_click);
    event_record_popup_btn_create(cont, btn_idx++, lang_str_get(STR_ALL_DELETE), event_record_popup_delete_all_click);
    event_record_popup_btn_create(cont, btn_idx++, lang_str_get(STR_CLOSE), event_record_popup_close_click);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(event_record)
{
    lv_common_bg_create(lv_scr_act_current());

    event_record_sidebar_create();
    event_record_list_panel_create();
    event_record_title_bar_create();

    char base_dir[64];
    if (Amy_tfcard_status() == SD_STATE_MOUNT)
        snprintf(base_dir, sizeof(base_dir), "%s", TFCARD_MOUNT_PATH);
    else
        snprintf(base_dir, sizeof(base_dir), "%s", LOCAL_MEDIA_DIR);

    unsigned int mask = s_type_cfg[g_media_record_type].media_mask;
    s_ctx.media_ctx = Amy_media_open(base_dir, mask, LIST_PAGE_SIZE);
    s_ctx.view_start_idx = 0;
    s_ctx.total_files = 0;
    s_ctx.cur_page_items = 0;
    s_ctx.reuse_db_chunk_on_next = false;
    s_ctx.reuse_db_chunk_on_prev = false;
    s_ctx.db_chunk_start_idx = 0;
    s_ctx.at_top_on_scroll_begin = false;
    s_ctx.at_bottom_on_scroll_begin = false;
    s_ctx.loading_page = false;
    s_ctx.selected_global_idx = -1;

    if (s_ctx.media_ctx)
    {
        /* 参考 page_demo.txt: first_page → 标记复用 → 获取 total → fill_next(true) */
        int num = 0;
        Amy_media_first_page(s_ctx.media_ctx, &num);
        s_ctx.reuse_db_chunk_on_next = true;

        unsigned long total = 0;
        Amy_media_all_index_total_get(s_ctx.media_ctx, &s_ctx.db_chunk_start_idx, &total);
        s_ctx.total_files = (int)total;
        s_ctx.reuse_db_chunk_on_next = true;

        event_record_merge_fill_next(true);
        s_ctx.view_start_idx = 0;
        s_ctx.reuse_db_chunk_on_prev = false;
    }

    event_record_list_cont_create();
    event_record_page_info_update();
    event_record_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, event_record_sd_change_cb);
}

static void Amy_lv_page_quit(event_record)
{
    event_record_popup_del();
    if (s_ctx.media_ctx && lv_next_page_get() != lv_page(record_play))
    {
        Amy_media_close(s_ctx.media_ctx);
        s_ctx.media_ctx = NULL;
    }
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(event_record);
