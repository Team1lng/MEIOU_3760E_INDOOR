#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "sat_wifi.h"
#include "user_data.h"
#include "sat_user_common.h"
#include "tuya/tuya_sdk.h"
#include "../pwm.h"
#include <stdlib.h>

/************************** 布局常量 **************************/
#define PANEL_X 468
#define PANEL_W 509
#define TITLE_ICON_X 30
#define TITLE_ICON_Y 241
#define TITLE_ICON_W 130
#define TITLE_ICON_H 130
#define TITLE_Y 358
#define MENU_X SIDEBAR_W
#define MENU_W (PANEL_X - SIDEBAR_W)
#define MENU_SEL_H 52
#define MENU_SEL_COLOR 0x25679c
#define ROW_H 56

/* Net pairing Mode 面板 */
#define PAIR_VALUE_X 12
#define PAIR_VALUE_Y 38
#define PAIR_VALUE_W 394
#define PAIR_ARROW_L_X 21
#define PAIR_ARROW_R_X 392
#define PAIR_SEP_Y 75

/* WLAN 面板 */
#define WLAN_ROW_W 412
#define WLAN_ROW_H 52
#define WLAN_ROW_X 19
#define WLAN_SEP_X 23
#define WLAN_SEP_W 358

/* 信号阈值 (参考 EMOS-Indoor) */
#define WIFI_SIG_FULL -55
#define WIFI_SIG_TWO -77
#define WIFI_SIG_ONE -88

/* IP allocation 面板 (仅在 CABLE 模式下显示) */
#define ALLOC_VALUE_Y 90
#define ALLOC_SEP_Y 127

/* Network information 面板 */
#define INFO_TEXT_X 59
#define INFO_TEXT_W 350
#define INFO_SEP_X 20
#define INFO_SEP_W 394

/* QR code 面板 */
#define QR_X 105
#define QR_Y 139
#define QR_SIZE 229

/************************** 菜单项 **************************/
#define NET_MENU_PAIRING 0
#define NET_MENU_WLAN 1
#define NET_MENU_INFO 2
#define NET_MENU_QR 3

static const struct
{
    int str_id;
    int text_y;
    int sel_y;
} s_net_menus[] = {
    {STR_NET_PAIRING, 44, 33},
    {STR_WLAN, 101, 88},
    {STR_NETWORK_INFO, 158, 146},
    {STR_QR_CODE_NETWORK, 216, 204},
};
#define NET_MENU_NUM (int)(sizeof(s_net_menus) / sizeof(s_net_menus[0]))

/************************** 页面上下文 **************************/
static struct
{
    lv_obj_t *sel_rect;
    lv_obj_t *panel;
    int cur_menu;
    wifi_context *wifi_ctx;
    wifi_info *scan_info;
    int scan_total;
    lv_timer_t *scan_timer;
    char connected_ssid[64];
    bool wifi_connected;
    bool wifi_free;
    int connected_rssi;
    lv_obj_t *alloc_cont;
    pairing_mode_t prev_pairing_mode;
    bool select_wlan;
} s_net_ctx;

/************************** 函数声明 **************************/
static void network_menu_pairing_obj_click(lv_event_t *ev);
static void network_menu_wlan_obj_click(lv_event_t *ev);
static void network_menu_info_obj_click(lv_event_t *ev);
static void network_menu_qr_obj_click(lv_event_t *ev);
static void network_back_obj_click(lv_event_t *ev);

static void network_menu_select(int idx);
static void network_menu_create(void);
static void network_panel_container_create(void);
static void network_panel_pairing_create(void);
static void network_panel_wlan_create(void);
static void network_panel_info_create(void);
static void network_panel_qr_create(void);
static void network_back_create(void);

static void wlan_add_manual_click(lv_event_t *ev);
static void wlan_wifi_row_click(lv_event_t *ev);
static void wlan_connected_click(lv_event_t *ev);
static void wlan_scan_timer_cb(lv_timer_t *timer);
static void wlan_forget_confirm_cb(lv_event_t *ev);
static void wlan_forget_cancel_cb(lv_event_t *ev);
static void network_reboot_confirm_cb(lv_event_t *ev);
static void network_reboot_cancel_cb(lv_event_t *ev);

static const firamware_resource *wlan_sig_icon_select(int db, bool free, bool connected);
static void wlan_row_create(lv_obj_t *parent, int y, const char *ssid,
                            const firamware_resource *icon, lv_event_cb_t cb, void *user_data);

/************************** 回调实现 **************************/

static void network_menu_pairing_obj_click(lv_event_t *ev)
{
    network_menu_select(NET_MENU_PAIRING);
}

static void network_menu_wlan_obj_click(lv_event_t *ev)
{
    network_menu_select(NET_MENU_WLAN);
}

static void network_menu_info_obj_click(lv_event_t *ev)
{
    network_menu_select(NET_MENU_INFO);
}

static void network_menu_qr_obj_click(lv_event_t *ev)
{
    network_menu_select(NET_MENU_QR);
}

static void network_back_obj_click(lv_event_t *ev)
{
    if (s_net_ctx.prev_pairing_mode != user_data_get()->public_cfg.pairing_mode)
    {
        lv_common_msgbox_create(251, 106, 586, 388, 62,
                                lang_str_get(STR_WANT_RESTART_SYSTEM),
                                network_reboot_confirm_cb, network_reboot_cancel_cb);
    }
    else
    {
        Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
    }
}

/* 联网模式改动后重启确认/取消 */
static void network_reboot_confirm_cb(lv_event_t *ev)
{
    user_data_save();
    backlight_enable(0);
    Amy_msleep(100);
    system("reboot");
}

static void network_reboot_cancel_cb(lv_event_t *ev)
{
    user_data_get()->public_cfg.pairing_mode = s_net_ctx.prev_pairing_mode;
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 创建函数 **************************/

static void network_menu_select(int idx)
{
    if (idx == s_net_ctx.cur_menu)
    {
        return;
    }
    s_net_ctx.cur_menu = idx;
    lv_obj_set_y(s_net_ctx.sel_rect, s_net_menus[idx].sel_y);

    lv_obj_clean(s_net_ctx.panel);
    switch (idx)
    {
    case NET_MENU_PAIRING:
        network_panel_pairing_create();
        break;
    case NET_MENU_WLAN:
        network_panel_wlan_create();
        break;
    case NET_MENU_INFO:
        network_panel_info_create();
        break;
    case NET_MENU_QR:
        network_panel_qr_create();
        break;
    default:
        break;
    }
}

/* 从添加 WiFi 流程返回时，导航栏定位到 WLAN 菜单 */
void network_menu_select_wlan(void)
{
    s_net_ctx.select_wlan = true;
}

static void network_menu_create(void)
{
    static const lv_event_cb_t menu_cbs[] = {
        network_menu_pairing_obj_click,
        network_menu_wlan_obj_click,
        network_menu_info_obj_click,
        network_menu_qr_obj_click,
    };

    s_net_ctx.sel_rect = lv_common_rect_create(lv_scr_act_current(), MENU_X, s_net_menus[s_net_ctx.cur_menu].sel_y,
                                               MENU_W, MENU_SEL_H, MENU_SEL_COLOR);
    for (int i = 0; i < NET_MENU_NUM; i++)
    {
        lv_obj_t *item = lv_common_text_create(lv_scr_act_current(), NETWORK_ROW1_LABEL1_ID + i * 3,
                                               MENU_X, s_net_menus[i].text_y, MENU_W, ROW_H,
                                               menu_cbs[i], LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               lang_str_get(s_net_menus[i].str_id), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);
        lv_obj_set_ext_click_area(item, (MENU_SEL_H - ROW_H) / 2);
    }
}

static void network_panel_container_create(void)
{
    lv_obj_t *panel = lv_common_setting_cont_create(lv_scr_act_current(), -1,
                                                    PANEL_X, 0, PANEL_W, SCREEN_H,
                                                    NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    s_net_ctx.panel = panel;
}

#if NET_SUPPORT_CABLE
static void pairing_mode_left_cb(lv_event_t *ev);
static void pairing_mode_right_cb(lv_event_t *ev);
#endif
static void alloction_mode_left_cb(lv_event_t *ev);
static void alloction_mode_right_cb(lv_event_t *ev);

static void alloction_mode_update_label(lv_obj_t *mode_label, int mode)
{
    lv_label_set_text(mode_label, lang_str_get(STR_STATIC_ALLOC + mode));
}

static void alloction_mode_create(void)
{
    user_data_info *info = user_data_get();
    int mode = info->public_cfg.allocation_mode;

    int cont_y = PAIR_SEP_Y + 5;
    int label_y = ALLOC_VALUE_Y - cont_y;
    int sep_y = ALLOC_SEP_Y - cont_y;

    s_net_ctx.alloc_cont = lv_common_setting_cont_create(s_net_ctx.panel, -1,
                                                         0, cont_y, PANEL_W, ALLOC_SEP_Y - cont_y + 10,
                                                         NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(s_net_ctx.alloc_cont, 0, LV_PART_MAIN);

    lv_obj_t *mode_label = lv_common_text_create(s_net_ctx.alloc_cont, NETWORK_PANEL_ITEM3_ID,
                                                 PAIR_VALUE_X, label_y, PAIR_VALUE_W, ROW_H,
                                                 NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 lang_str_get(STR_STATIC_ALLOC + mode), 0xffffff, 0x02A9FF,
                                                 LV_TEXT_ALIGN_CENTER, lv_font_normal);

    lv_resource_init(icon_prev, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_prev_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *left_btn = lv_common_img_btn_create(s_net_ctx.alloc_cont, -1,
                                                  PAIR_ARROW_L_X, label_y + 5, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                                  alloction_mode_left_cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_prev, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_prev_focus);
    lv_obj_set_user_data(left_btn, mode_label);
    lv_common_arrow_link_press(left_btn, NULL, mode_label);

    lv_resource_init(icon_next, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_next_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *right_btn = lv_common_img_btn_create(s_net_ctx.alloc_cont, -1,
                                                   PAIR_ARROW_R_X, label_y + 5, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                                   alloction_mode_right_cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &icon_next, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_next_focus);
    lv_obj_set_user_data(right_btn, mode_label);
    lv_common_arrow_link_press(right_btn, NULL, mode_label);

    lv_common_grad_sep_create(s_net_ctx.alloc_cont, PAIR_VALUE_X, sep_y, PAIR_VALUE_W);
}

static void network_panel_pairing_create(void)
{
    user_data_info *info = user_data_get();
    int mode = info->public_cfg.pairing_mode;

    /* Mode label */
    lv_obj_t *mode_label = lv_common_text_create(s_net_ctx.panel, NETWORK_PANEL_ITEM1_ID,
                                                 PAIR_VALUE_X, PAIR_VALUE_Y, PAIR_VALUE_W, ROW_H,
                                                 NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 lang_str_get(STR_WLAN_PAIRING + mode), 0xffffff, 0x02A9FF,
                                                 LV_TEXT_ALIGN_CENTER, lv_font_normal);

#if NET_SUPPORT_CABLE
    /* Left arrow button */
    lv_resource_init(icon_prev, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_prev_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *left_btn = lv_common_img_btn_create(s_net_ctx.panel, -1,
                                                  PAIR_ARROW_L_X, PAIR_VALUE_Y + 5, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                                  pairing_mode_left_cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &icon_prev, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_prev_focus);
    lv_obj_set_user_data(left_btn, mode_label);
    lv_common_arrow_link_press(left_btn, NULL, mode_label);

    /* Right arrow button */
    lv_resource_init(icon_next, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_next_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *right_btn = lv_common_img_btn_create(s_net_ctx.panel, -1,
                                                   PAIR_ARROW_R_X, PAIR_VALUE_Y + 5, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                                   pairing_mode_right_cb, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &icon_next, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_next_focus);
    lv_obj_set_user_data(right_btn, mode_label);
    lv_common_arrow_link_press(right_btn, NULL, mode_label);
#else
    /* 硬件已移除有线网卡，配对模式固定为 WLAN，不再提供切换 */
    LV_UNUSED(mode_label);
#endif

    lv_common_grad_sep_create(s_net_ctx.panel, PAIR_VALUE_X, PAIR_SEP_Y, PAIR_VALUE_W);

    /* IP allocation mode switch (仅在 CABLE 模式下可见) */
    alloction_mode_create();
    if (mode != PAIRING_MODE_WIRED)
        lv_obj_add_flag(s_net_ctx.alloc_cont, LV_OBJ_FLAG_HIDDEN);
}

#if NET_SUPPORT_CABLE
static void pairing_mode_update_label(lv_obj_t *mode_label, int mode)
{
    lv_label_set_text(mode_label, lang_str_get(STR_WLAN_PAIRING + mode));
}

static void alloc_cont_set_visible(bool visible)
{
    if (s_net_ctx.alloc_cont == NULL)
        return;
    if (visible)
        lv_obj_clear_flag(s_net_ctx.alloc_cont, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(s_net_ctx.alloc_cont, LV_OBJ_FLAG_HIDDEN);
}

static void pairing_mode_left_cb(lv_event_t *ev)
{
    user_data_info *info = user_data_get();
    if (info->public_cfg.pairing_mode == 0)
        info->public_cfg.pairing_mode = PAIRING_MODE_TOTAL - 1;
    else
        info->public_cfg.pairing_mode--;

    lv_obj_t *mode_label = lv_obj_get_user_data(lv_event_get_current_target(ev));
    if (mode_label)
        pairing_mode_update_label(mode_label, info->public_cfg.pairing_mode);

    alloc_cont_set_visible(info->public_cfg.pairing_mode == PAIRING_MODE_WIRED);
}

static void pairing_mode_right_cb(lv_event_t *ev)
{
    user_data_info *info = user_data_get();
    if (info->public_cfg.pairing_mode >= PAIRING_MODE_TOTAL - 1)
        info->public_cfg.pairing_mode = 0;
    else
        info->public_cfg.pairing_mode++;

    lv_obj_t *mode_label = lv_obj_get_user_data(lv_event_get_current_target(ev));
    if (mode_label)
        pairing_mode_update_label(mode_label, info->public_cfg.pairing_mode);

    alloc_cont_set_visible(info->public_cfg.pairing_mode == PAIRING_MODE_WIRED);
}
#endif

static void alloction_mode_left_cb(lv_event_t *ev)
{
    user_data_info *info = user_data_get();
    if (info->public_cfg.allocation_mode == 0)
        info->public_cfg.allocation_mode = IP_ALLOC_TOTAL - 1;
    else
        info->public_cfg.allocation_mode--;

    user_data_save();
    lv_obj_t *mode_label = lv_obj_get_user_data(lv_event_get_current_target(ev));
    if (mode_label)
        alloction_mode_update_label(mode_label, info->public_cfg.allocation_mode);
}

static void alloction_mode_right_cb(lv_event_t *ev)
{
    user_data_info *info = user_data_get();
    if (info->public_cfg.allocation_mode >= IP_ALLOC_TOTAL - 1)
        info->public_cfg.allocation_mode = 0;
    else
        info->public_cfg.allocation_mode++;

    user_data_save();
    lv_obj_t *mode_label = lv_obj_get_user_data(lv_event_get_current_target(ev));
    if (mode_label)
        alloction_mode_update_label(mode_label, info->public_cfg.allocation_mode);
}

/************************** WLAN 面板 **************************/

static const firamware_resource *wlan_sig_icon_select(int db, bool free, bool connected)
{
    static firamware_resource icon_0 = {{'S', '@'}, ICON_WIFI_WIFI_0_PNG_OFFSET, ICON_WIFI_WIFI_0_PNG_SIZE};
    static firamware_resource icon_1 = {{'S', '@'}, ICON_WIFI_WIFI_1_PNG_OFFSET, ICON_WIFI_WIFI_1_PNG_SIZE};
    static firamware_resource icon_2 = {{'S', '@'}, ICON_WIFI_WIFI_2_PNG_OFFSET, ICON_WIFI_WIFI_2_PNG_SIZE};
    static firamware_resource icon_3 = {{'S', '@'}, ICON_WIFI_WIFI_3_PNG_OFFSET, ICON_WIFI_WIFI_3_PNG_SIZE};
    static firamware_resource icon_0_lock = {{'S', '@'}, ICON_WIFI_WIFI_0_LOCK_PNG_OFFSET, ICON_WIFI_WIFI_0_LOCK_PNG_SIZE};
    static firamware_resource icon_1_lock = {{'S', '@'}, ICON_WIFI_WIFI_1_LOCK_PNG_OFFSET, ICON_WIFI_WIFI_1_LOCK_PNG_SIZE};
    static firamware_resource icon_2_lock = {{'S', '@'}, ICON_WIFI_WIFI_2_LOCK_PNG_OFFSET, ICON_WIFI_WIFI_2_LOCK_PNG_SIZE};
    static firamware_resource icon_3_lock = {{'S', '@'}, ICON_WIFI_WIFI_3_LOCK_PNG_OFFSET, ICON_WIFI_WIFI_3_LOCK_PNG_SIZE};

    int level;
    if (db > WIFI_SIG_FULL)
        level = 3;
    else if (db > WIFI_SIG_TWO)
        level = 2;
    else if (db > WIFI_SIG_ONE)
        level = 1;
    else
        level = 0;

    const firamware_resource *icons_free[] = {&icon_0, &icon_1, &icon_2, &icon_3};
    const firamware_resource *icons_sec[] = {&icon_0_lock, &icon_1_lock, &icon_2_lock, &icon_3_lock};
    return free ? icons_free[level] : icons_sec[level];
}

static void wlan_row_create(lv_obj_t *parent, int y, const char *ssid,
                            const firamware_resource *icon, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_common_setting_cont_create(parent, -1,
                                                  WLAN_ROW_X, y, WLAN_ROW_W, WLAN_ROW_H,
                                                  NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    /* SSID 文本 (左对齐，可滚动) */
    lv_obj_t *label = lv_common_text_create(btn, -1,
                                            0, 0, WLAN_ROW_W - 120, WLAN_ROW_H,
                                            NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            ssid, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /* 信号图标 (右对齐) */
    lv_obj_t *img = lv_img_create(btn);
    lv_img_set_src(img, (void *)icon);
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -10, 0);

    /* 底部分隔线 */
    lv_common_grad_sep_create(parent, WLAN_SEP_X, y + WLAN_ROW_H, WLAN_SEP_W);
}

static void wlan_add_manual_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(add_wifi), LV_SCR_LOAD_ANIM_NONE, );
}

static void wlan_wifi_row_click(lv_event_t *ev)
{
    int index = (int)(uintptr_t)lv_event_get_user_data(ev);
    if (index < 0 || index >= s_net_ctx.scan_total)
        return;

    extern void setting_wifi_input_name_set(char *name);
    extern void setting_wifi_direct_pwd(void);
    setting_wifi_input_name_set((char *)s_net_ctx.scan_info[index].name);
    setting_wifi_direct_pwd();
    Amy_lv_page_goto(lv_page(add_wifi), LV_SCR_LOAD_ANIM_NONE, );
}

static void wlan_forget_confirm_cb(lv_event_t *ev)
{
    lv_common_msgbox_del();
    wifi_api_reset_default(s_net_ctx.wifi_ctx);
    /* 彻底断开当前热点：关闭接口清除连接与旧 IP，再以重置配置重新拉起（不关联任何热点） */
    wifi_device_close(s_net_ctx.wifi_ctx);
    wifi_device_open(s_net_ctx.wifi_ctx);
    s_net_ctx.wifi_connected = false;
    memset(s_net_ctx.connected_ssid, 0, sizeof(s_net_ctx.connected_ssid));

    lv_obj_clean(s_net_ctx.panel);
    network_panel_wlan_create();
}

static void wlan_forget_cancel_cb(lv_event_t *ev)
{
    lv_common_msgbox_del();
}

static void wlan_connected_click(lv_event_t *ev)
{
    lv_common_msgbox_create(263, 150, 498, 299, 30,
                            lang_str_get(STR_DISCONNECT_WIFI),
                            wlan_forget_confirm_cb,
                            wlan_forget_cancel_cb);
}

static void network_panel_wlan_create(void)
{
    int y = s_net_menus[NET_MENU_PAIRING].sel_y;

    /* 已连接 WiFi (固定在列表上方，不随列表滚动) */
    if (s_net_ctx.wifi_connected && s_net_ctx.connected_ssid[0] != '\0')
    {
        const firamware_resource *icon = wlan_sig_icon_select(
            s_net_ctx.connected_rssi, s_net_ctx.wifi_free, true);
        wlan_row_create(s_net_ctx.panel, y, s_net_ctx.connected_ssid, icon,
                        wlan_connected_click, NULL);
        y += WLAN_ROW_H;
    }

    /* Add manually 行 (固定在列表上方，不随列表滚动) */
    {
        static firamware_resource icon_add = {{'S', '@'}, ICON_WIFI_ADD_WIFI_PNG_OFFSET, ICON_WIFI_ADD_WIFI_PNG_SIZE};
        wlan_row_create(s_net_ctx.panel, y, lang_str_get(STR_ADD_MANUALLY), &icon_add,
                        wlan_add_manual_click, NULL);
        y += WLAN_ROW_H;
    }

    /* 扫描结果列表 (可滚动) */
    lv_obj_t *list = lv_common_setting_cont_create(s_net_ctx.panel, -1,
                                                   0, y, PANEL_W, SCREEN_H - y,
                                                   NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    int row_y = 0;
    for (int i = 0; i < s_net_ctx.scan_total; i++)
    {
        wifi_info *info = &s_net_ctx.scan_info[i];

        if (s_net_ctx.wifi_connected &&
            strcmp((const char *)info->name, s_net_ctx.connected_ssid) == 0)
            continue;

        const firamware_resource *icon = wlan_sig_icon_select(
            info->db, info->free, false);

        wlan_row_create(list, row_y, (const char *)info->name, icon,
                        wlan_wifi_row_click, (void *)(uintptr_t)i);
        row_y += WLAN_ROW_H;
    }
}

/************************** 信息面板 **************************/

static void network_panel_info_create(void)
{
#if NET_SUPPORT_CABLE
    user_data_info *info = user_data_get();
    bool is_wlan = (info->public_cfg.pairing_mode == PAIRING_MODE_WLAN);
    const char *iface = is_wlan ? "wlan0" : "eth2";
#else
    const char *iface = "wlan0";
#endif

    char ip[32] = "---";
    char mac[32] = "---";
    sat_ip_mac_addres_get(iface, ip, mac, NULL);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s : %s", lang_str_get(STR_IP_ADDRESS), ip);
    lv_common_text_create(s_net_ctx.panel, NETWORK_PANEL_ITEM1_ID, INFO_TEXT_X, 109, INFO_TEXT_W, ROW_H,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          buf, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);
    lv_common_grad_sep_create(s_net_ctx.panel, INFO_SEP_X, 146, INFO_SEP_W);

    snprintf(buf, sizeof(buf), "%s : %s", lang_str_get(STR_MAC), mac);
    lv_common_text_create(s_net_ctx.panel, NETWORK_PANEL_ITEM2_ID, INFO_TEXT_X, 161, INFO_TEXT_W, ROW_H,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          buf, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);
    lv_common_grad_sep_create(s_net_ctx.panel, INFO_SEP_X, 198, INFO_SEP_W);
}

static void network_panel_qr_create(void)
{
    char qrcode[128] = {0};
    if (tuya_api_get_qrcode(qrcode, sizeof(qrcode)) && qrcode[0] != '\0')
    {
        lv_obj_t *qr = lv_qrcode_create(s_net_ctx.panel, QR_SIZE, lv_color_black(), lv_color_white());
        if (qr != NULL)
        {
            lv_obj_set_pos(qr, QR_X, QR_Y);
            lv_qrcode_update(qr, qrcode, strlen(qrcode));
        }
    }
}

static void network_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *network_back = lv_common_img_btn_create(lv_scr_act_current(), NETWORK_BACK_BTN_ID, 929, 505, 67, 67,
                                                      network_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(network_back, 21);
}

/************************** WiFi 扫描定时器 **************************/

static void wlan_scan_timer_cb(lv_timer_t *timer)
{
    if (s_net_ctx.wifi_ctx == NULL)
    {
        lv_timer_del(timer);
        s_net_ctx.scan_timer = NULL;
        return;
    }
    if (wifi_device_scan(s_net_ctx.wifi_ctx))
    {
        wifi_device_scan_info_get(s_net_ctx.wifi_ctx, &s_net_ctx.scan_info, &s_net_ctx.scan_total);

        if (s_net_ctx.cur_menu == NET_MENU_WLAN)
        {
            lv_obj_clean(s_net_ctx.panel);
            network_panel_wlan_create();
        }
        lv_timer_del(timer);
        s_net_ctx.scan_timer = NULL;
    }
}

/************************** 页面生命周期 **************************/
static void network_ui_create(void)
{
    lv_common_bg_create(lv_scr_act_current());

    lv_resource_init(title_icon, ICON_SETTING_NETWORK_PNG_OFFSET, ICON_SETTING_NETWORK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_NETWORK_FOCUS_PNG_OFFSET, ICON_SETTING_NETWORK_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), NETWORK_TITLE_ICON_ID, TITLE_ICON_X, TITLE_ICON_Y, TITLE_ICON_W, TITLE_ICON_H,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), NETWORK_TITLE_ID, 0, TITLE_Y, SIDEBAR_W, ROW_H,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_NETWORK_INFO), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);
    network_menu_create();
    network_panel_container_create();
    network_back_create();
}

static void Amy_lv_page_enter(network)
{
    /* 正常进入：重置状态 */
    s_net_ctx.cur_menu = NET_MENU_PAIRING;
    s_net_ctx.scan_info = NULL;
    s_net_ctx.scan_total = 0;
    s_net_ctx.wifi_connected = false;
    s_net_ctx.wifi_free = false;
    s_net_ctx.connected_ssid[0] = '\0';
    s_net_ctx.connected_rssi = 0;
    s_net_ctx.alloc_cont = NULL;
    s_net_ctx.prev_pairing_mode = user_data_get()->public_cfg.pairing_mode;

    /* 从添加 WiFi 流程返回时，导航栏定位到 WLAN */
    bool to_wlan = s_net_ctx.select_wlan;
    s_net_ctx.select_wlan = false;
    if (to_wlan)
        s_net_ctx.cur_menu = NET_MENU_WLAN;

    network_ui_create();

    /* 初始化 WiFi */
    if (s_net_ctx.wifi_ctx == NULL)
    {
        s_net_ctx.wifi_ctx = wifi_context_get();
    }

    /* 先获取连接状态与扫描数据，再绘制 WLAN 面板，保证热点信息实时显示 */
    bool need_scan_retry = false;
    if (s_net_ctx.wifi_ctx)
    {
        bool free = false;
        wifi_device_connection_status(s_net_ctx.wifi_ctx,
                                      (unsigned char *)s_net_ctx.connected_ssid,
                                      &free, NULL, &s_net_ctx.wifi_connected, NULL);
        s_net_ctx.wifi_free = free;
        wifi_device_get_rssi(s_net_ctx.wifi_ctx, &s_net_ctx.connected_rssi, &s_net_ctx.wifi_connected);

        /* 优先使用缓存的扫描数据 */
        if (!wifi_device_scan_info_get(s_net_ctx.wifi_ctx, &s_net_ctx.scan_info, &s_net_ctx.scan_total))
        {
            if (wifi_device_scan(s_net_ctx.wifi_ctx))
            {
                wifi_device_scan_info_get(s_net_ctx.wifi_ctx, &s_net_ctx.scan_info, &s_net_ctx.scan_total);
            }
            else
            {
                need_scan_retry = true;
            }
        }
    }

    if (to_wlan)
        network_panel_wlan_create();
    else
        network_panel_pairing_create();

    if (need_scan_retry)
        s_net_ctx.scan_timer = lv_timer_create(wlan_scan_timer_cb, 3000, NULL);

    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(network)
{
    s_net_ctx.sel_rect = NULL;
    s_net_ctx.panel = NULL;

    if (s_net_ctx.scan_timer)
    {
        lv_timer_del(s_net_ctx.scan_timer);
        s_net_ctx.scan_timer = NULL;
    }
    s_net_ctx.scan_info = NULL;
    s_net_ctx.scan_total = 0;
    s_net_ctx.wifi_ctx = NULL;
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(network);
