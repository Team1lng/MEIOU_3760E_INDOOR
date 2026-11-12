#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "page_password_input.h"
#include "user_data.h"
#include "language.h"
#include <stdio.h>
#include <string.h>
#include "sat_ipcamera.h"
#include "live/testRTSPClient.h"
#include "device/lv_disp_dev.h"
#include "Amy_lvgl.h"

/************************** 常量 **************************/
#define MANUAL_LINK_TIMEOUT_MS 5000
#define SCAN_TIMEOUT_MS 10000
#define ONVIF_REG_TIMEOUT_MS 15000

/************************** 子视图枚举 **************************/
typedef enum
{
    CCTV_VIEW_SCAN,
    CCTV_VIEW_MANUAL,
} cctv_view_t;

/************************** 页面上下文 **************************/
static struct
{
    cctv_view_t cur_view;
    int model[2];
    int online_total;
    int cur_search_idx;
    int credential_step; /* 0=输入账号, 1=输入密码 */
    struct
    {
        int model;
        char ip[32];
        char account[64];
        char password[64];
        char rtsp_url[128];
    } temp_dev;
    bool enter_from_kb;
    bool quit_to_kb;
    bool saved_from_manual;
    bool show_onvif_spinner;
    rtsp_client_context *rtsp_ctx;
    lv_timer_t *manual_link_timer;
    lv_timer_t *scan_timer;
    lv_timer_t *searching_timer;
    bool is_searching;
    bool navigating_to_cam_display;
} s_ctx = {.cur_view = CCTV_VIEW_SCAN};

bool g_cctv_enter_from_cam_display = false;

void cctv_set_enter_from_cam_display(bool val)
{
    g_cctv_enter_from_cam_display = val;
}

/************************** UI 辅助：获取视图容器 **************************/
static lv_obj_t *cctv_view_container_get(void)
{
    return lv_obj_get_child_by_id(lv_scr_act_current(), CCTV_VIEW_CONTAINER_ID);
}

/* 品牌列表 (与 EMOS-Indoor 一致) */
static const int s_brand_ids[] = {STR_DAHUA, STR_HIKVISION};
#define BRAND_COUNT (int)(sizeof(s_brand_ids) / sizeof(s_brand_ids[0]))

/************************** 前向声明 **************************/
static void cctv_view_switch(cctv_view_t view);
static void cctv_scan_view_create(void);
static void cctv_manual_view_create(void);
static void cctv_goto_cam_display(void);
static void cctv_video_open(const char *url);
static void cctv_video_close(void);
static void cctv_spinner_show(const char *msg);
static void cctv_spinner_hide(void);
static void cctv_toast(const char *msg);
static void cctv_manual_link_timeout_cb(lv_timer_t *t);
static void cctv_scan_timeout_cb(lv_timer_t *t);
static void cctv_scan_timer_start(void);
static void cctv_scan_timer_stop(void);
static void cctv_search_timer_cb(lv_timer_t *t);
static void cctv_search_loading_start(void);
static bool cctv_searched_result_has_new_camera(void);
static void cctv_back_to_setting(lv_event_t *e);

/* password_input 回调 */
static void cctv_kb_back_to_cctv(lv_event_t *e);
static void cctv_kb_confirm_ip(lv_event_t *e);
static void cctv_kb_confirm_account(lv_event_t *e);
static void cctv_kb_confirm_password(lv_event_t *e);
static void cctv_kb_confirm_onvif_account(lv_event_t *e);
static void cctv_kb_confirm_onvif_password(lv_event_t *e);

/************************** 辅助函数 **************************/

static struct ipcamera_info *cctv_device(int idx)
{
    return &user_data_get()->public_cfg.net_cctv_device[idx];
}

static bool cctv_device_has_url(int idx)
{
    struct ipcamera_info *dev = cctv_device(idx);
    return dev->rtsp[0].rtsp_url[0] != 0;
}

static void cctv_model_detect(int idx)
{
    struct ipcamera_info *dev = cctv_device(idx);
    if (strstr(dev->rtsp[0].rtsp_url, "Streaming/Channels"))
        s_ctx.model[idx] = 1; /* Hikvision */
    else if (strstr(dev->rtsp[0].rtsp_url, "cam/realmonitor"))
        s_ctx.model[idx] = 0; /* Dahua */
}

static void cctv_rtsp_url_build(void)
{
    memset(s_ctx.temp_dev.rtsp_url, 0, sizeof(s_ctx.temp_dev.rtsp_url));
    if (s_ctx.temp_dev.model) /* Hikvision */
        snprintf(s_ctx.temp_dev.rtsp_url, sizeof(s_ctx.temp_dev.rtsp_url),
                 "rtsp://%s:%s@%s:554/Streaming/Channels/1",
                 s_ctx.temp_dev.account, s_ctx.temp_dev.password, s_ctx.temp_dev.ip);
    else /* Dahua */
        snprintf(s_ctx.temp_dev.rtsp_url, sizeof(s_ctx.temp_dev.rtsp_url),
                 "rtsp://%s:%s@%s:554/cam/realmonitor?channel=1&subtype=1",
                 s_ctx.temp_dev.account, s_ctx.temp_dev.password, s_ctx.temp_dev.ip);
}

static void cctv_rtsp_url_build_from(int idx)
{
    struct ipcamera_info *dev = cctv_device(idx);
    char url[128];
    memset(url, 0, sizeof(url));
    if (s_ctx.model[idx])
        snprintf(url, sizeof(url), "rtsp://%s:%s@%s:554/Streaming/Channels/1",
                 dev->username, dev->password, dev->ipaddr);
    else
        snprintf(url, sizeof(url), "rtsp://%s:%s@%s:554/cam/realmonitor?channel=1&subtype=1",
                 dev->username, dev->password, dev->ipaddr);
    /* 同时写入 sip_url 和 rtsp[0].rtsp_url */
    strncpy(dev->sip_url, url, sizeof(dev->sip_url) - 1);
    strncpy(dev->rtsp[0].rtsp_url, url, sizeof(dev->rtsp[0].rtsp_url) - 1);
    dev->rtsp[0].rtsp_url_type = RTSP_URL_TYPE_FULL;
}

static void cctv_temp_init_defaults(void)
{
    memset(&s_ctx.temp_dev, 0, sizeof(s_ctx.temp_dev));
}

/************************** Toast 消息 **************************/

static void cctv_toast_timer_cb(lv_timer_t *t)
{
    lv_obj_t *cont = (lv_obj_t *)t->user_data;
    lv_timer_del(t);
    if (cont)
        lv_obj_del(cont);
}

static void cctv_toast(const char *msg)
{
    if (msg == NULL)
        return;

    lv_obj_t *cont = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                              0, 0, 498, 299,
                                              NULL, false, LV_OPA_90, 0x1A1A1A, LV_OPA_90, 0x1A1A1A,
                                              12, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                              12, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                              NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_common_text_create(cont, -1,
                          0, 0, 498, 299,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          msg, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    lv_timer_create_recycle(cctv_toast_timer_cb, 2500, cont);
}

/************************** Loading Spinner **************************/

static void cctv_spinner_show(const char *msg)
{
    cctv_spinner_hide();

    lv_obj_t *overlay = lv_common_img_btn_create(lv_scr_act_current(), CCTV_SPINNER_OVERLAY_ID,
                                                 0, 0, SCREEN_W, SCREEN_H,
                                                 NULL, false, LV_OPA_70, 0x000000, LV_OPA_70, 0x000000,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);

    lv_obj_t *box = lv_common_img_btn_create(overlay, -1,
                                             0, 0, 498, 299,
                                             NULL, false, LV_OPA_COVER, 0x1A1A1A, LV_OPA_COVER, 0x1A1A1A,
                                             0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                             0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                             NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *spinner = lv_common_spinner_create(box, -1,
                                                 0, 0, 120, 120,
                                                 0x2183EF, 0xFFFFFF,
                                                 1000, 45, 12);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -30);

    if (msg != NULL)
    {
        lv_obj_t *label = lv_common_text_create(box, -1,
                                                0, 0, 498, 40,
                                                NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                msg, 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
    }
}

static void cctv_spinner_hide(void)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), CCTV_SPINNER_OVERLAY_ID);
    if (overlay)
        lv_obj_del(overlay);
}

/************************** password_input 回调 **************************/

static void cctv_kb_back_to_cctv(lv_event_t *e)
{
    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_kb_confirm_ip(lv_event_t *e)
{
    const char *text = page_password_input_get_text();
    memset(s_ctx.temp_dev.ip, 0, sizeof(s_ctx.temp_dev.ip));
    strncpy(s_ctx.temp_dev.ip, text, sizeof(s_ctx.temp_dev.ip) - 1);

    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_kb_confirm_account(lv_event_t *e)
{
    const char *text = page_password_input_get_text();
    memset(s_ctx.temp_dev.account, 0, sizeof(s_ctx.temp_dev.account));
    strncpy(s_ctx.temp_dev.account, text, sizeof(s_ctx.temp_dev.account) - 1);

    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_kb_confirm_password(lv_event_t *e)
{
    const char *text = page_password_input_get_text();
    memset(s_ctx.temp_dev.password, 0, sizeof(s_ctx.temp_dev.password));
    strncpy(s_ctx.temp_dev.password, text, sizeof(s_ctx.temp_dev.password) - 1);

    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

/* ONVIF 两步认证：账号 → 密码 */
static void cctv_kb_confirm_onvif_account(lv_event_t *e)
{
    const char *text = page_password_input_get_text();
    memset(s_ctx.temp_dev.account, 0, sizeof(s_ctx.temp_dev.account));
    strncpy(s_ctx.temp_dev.account, text, sizeof(s_ctx.temp_dev.account) - 1);

    s_ctx.credential_step = 1;
    s_ctx.quit_to_kb = 1;
    page_password_input_set_title_visible(false);
    page_password_input_set_placeholder(lang_str_get(STR_ENTER_IPC_PWD));
    page_password_input_set_password_mode(true);
    page_password_input_set_pwd_toggle(true);
    page_password_input_set_max_length(32);
    page_password_input_set_text("");
    page_password_input_set_back_cb(cctv_kb_back_to_cctv);
    page_password_input_set_confirm_cb(cctv_kb_confirm_onvif_password);

    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_kb_confirm_onvif_password(lv_event_t *e)
{
    const char *text = page_password_input_get_text();
    memset(s_ctx.temp_dev.password, 0, sizeof(s_ctx.temp_dev.password));
    strncpy(s_ctx.temp_dev.password, text, sizeof(s_ctx.temp_dev.password) - 1);

    const char *ip = sat_ipcamera_ipaddr_get(s_ctx.cur_search_idx);
    if (ip)
    {
        memset(s_ctx.temp_dev.ip, 0, sizeof(s_ctx.temp_dev.ip));
        strncpy(s_ctx.temp_dev.ip, ip, sizeof(s_ctx.temp_dev.ip) - 1);
    }

    sat_ipcamera_user_password_set(s_ctx.cur_search_idx, s_ctx.temp_dev.account, s_ctx.temp_dev.password);
    sat_ipcamera_rtsp_url_get(s_ctx.cur_search_idx);

    /* 注册超时定时器：若 RTSP_DONE 一直未返回，则取消旋转并提示失败 */
    s_ctx.manual_link_timer = lv_timer_create((lv_timer_cb_t)cctv_manual_link_timeout_cb,
                                              ONVIF_REG_TIMEOUT_MS, NULL);

    s_ctx.enter_from_kb = true;
    s_ctx.saved_from_manual = false;
    s_ctx.show_onvif_spinner = true;
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 视图切换 **************************/

static void cctv_view_switch(cctv_view_t view)
{
    lv_obj_t *old = lv_obj_get_child_by_id(lv_scr_act_current(), CCTV_VIEW_CONTAINER_ID);
    lv_obj_t *parent = old ? lv_obj_get_parent(old) : lv_scr_act_current();
    if (old)
        lv_obj_del(old);

    lv_obj_t *vc = lv_common_setting_cont_create(parent, CCTV_VIEW_CONTAINER_ID,
                                                 SIDEBAR_W, 0, SCREEN_W - SIDEBAR_W, SCREEN_H,
                                                 NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(vc, LV_SCROLLBAR_MODE_OFF);

    s_ctx.cur_view = view;

    switch (view)
    {
    case CCTV_VIEW_SCAN:
        cctv_scan_view_create();
        break;
    case CCTV_VIEW_MANUAL:
        cctv_manual_view_create();
        break;
    }
}

/************************** 侧栏 **************************/

static void cctv_sidebar_create(void)
{
    lv_resource_init(icon, ICON_SETTING_CAMERA_PNG_OFFSET, ICON_SETTING_CAMERA_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), CCTV_TITLE_ICON_ID,
                             44, 218, 102, 102,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);

    lv_common_text_create(lv_scr_act_current(), CCTV_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_CAMERA_SET), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 扫描视图 **************************/

/* 布局常量 (cctv_view_container_get() 坐标系, 原点 x=0 即屏幕 x=SIDEBAR_W=190) */
#define WEBCAM_X 0
#define WEBCAM_W 278
#define WEBCAM_Y 24
#define IPC_ROW_X WEBCAM_W /* 紧挨 WEBCAM 右侧 */
#define IPC_ROW_W 466      /* EMOS-Indoor ipc_page 宽度 */
#define IPC_ITEM_W 422     /* EMOS-Indoor 列表项宽度 */
#define IPC_SEP_X IPC_ROW_X
#define IPC_SEP_W IPC_ITEM_W
#define IPC_BTN_H 52

static void cctv_scan_registered_click(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int dev_id = (int)(intptr_t)lv_obj_get_user_data(btn);

    struct ipcamera_info *dev = cctv_device(dev_id);
    cctv_rtsp_url_build_from(dev_id);

    /* 设置 camera_display 上下文 */
    cam_display_ctx_fill(dev->ipaddr, dev->rtsp[0].rtsp_url, dev->username, dev->password,
                         s_ctx.model[dev_id], dev_id, false);

    cctv_goto_cam_display();
}

static void cctv_scan_discovered_click(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);

    s_ctx.cur_search_idx = index;
    s_ctx.credential_step = 0;

    page_password_input_set_title_visible(false);
    page_password_input_set_placeholder(lang_str_get(STR_ENTER_IPC_USER));
    page_password_input_set_password_mode(false);
    page_password_input_set_pwd_toggle(false);
    page_password_input_set_max_length(32);
    page_password_input_set_text("");
    page_password_input_set_back_cb(cctv_kb_back_to_cctv);
    page_password_input_set_confirm_cb(cctv_kb_confirm_onvif_account);

    s_ctx.saved_from_manual = false;
    s_ctx.quit_to_kb = 1;
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_scan_add_manually_click(lv_event_t *e)
{
    cctv_temp_init_defaults();
    cctv_view_switch(CCTV_VIEW_MANUAL);
}

static lv_obj_t *cctv_scan_btn_create(lv_obj_t *parent, const char *text, int id, lv_event_cb_t cb,
                                      intptr_t user_data, bool has_add_icon)
{
    lv_obj_t *cont = lv_common_setting_cont_create(parent, id,
                                                   0, 0, IPC_ITEM_W, IPC_BTN_H,
                                                   cb, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)user_data);

    lv_obj_t *label = lv_common_text_create(cont, -1,
                                            0, 0, 0, 0,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            text, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    if (has_add_icon)
    {
        lv_resource_init(add_icon, ICON_CCTV_C_ADD_PNG_OFFSET,
                         ICON_CCTV_C_ADD_PNG_SIZE);
        lv_obj_t *img = lv_common_img_btn_create(cont, -1,
                                                 0, 0, ICON_CCTV_C_ADD_PNG_WIDTH, ICON_CCTV_C_ADD_PNG_HEIGHT,
                                                 NULL, false, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 &add_icon, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
        lv_obj_align(img, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    return cont;
}

/* 创建可滚动的列表容器，子对象坐标相对容器左上角 */
static lv_obj_t *cctv_scan_list_cont_create(lv_obj_t *parent, int id, int x, int y, int w, int h)
{
    lv_obj_t *list = lv_common_setting_cont_create(parent, id,
                                                   x, y, w, h,
                                                   NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    return list;
}

static void cctv_scan_list_create(void)
{
    lv_obj_t *view = cctv_view_container_get();
    int reg_cnt = (cctv_device_has_url(0) ? 1 : 0) + (cctv_device_has_url(1) ? 1 : 0);

    /* 列表1：已注册摄像头（高度按内容自适应） */
    if (reg_cnt > 0)
    {
        lv_obj_t *reg_list = cctv_scan_list_cont_create(view, CCTV_REGISTER_LIST_CONT_ID,
                                                        IPC_ROW_X, WEBCAM_Y,
                                                        IPC_ITEM_W, reg_cnt * IPC_BTN_H);
        int y = 0;

        if (cctv_device_has_url(0))
        {
            lv_obj_t *btn = cctv_scan_btn_create(reg_list, lang_str_get(STR_CAMERA1),
                                                 CCTV_ROW1_CONT_ID,
                                                 cctv_scan_registered_click, 0, false);
            lv_obj_set_pos(btn, 0, y);
            y += IPC_BTN_H;
            lv_common_grad_sep_create(reg_list, 0, y, IPC_SEP_W);
        }

        if (cctv_device_has_url(1))
        {
            lv_obj_t *btn = cctv_scan_btn_create(reg_list, lang_str_get(STR_CAMERA2),
                                                 CCTV_ROW2_CONT_ID,
                                                 cctv_scan_registered_click, 1, false);
            lv_obj_set_pos(btn, 0, y);
            y += IPC_BTN_H;
            lv_common_grad_sep_create(reg_list, 0, y, IPC_SEP_W);
        }
    }

    /* 手动添加按钮（固定，不随列表滚动） */
    int add_y = WEBCAM_Y + reg_cnt * IPC_BTN_H;
    lv_obj_t *add_btn = cctv_scan_btn_create(view, lang_str_get(STR_ADD_MANUALLY),
                                             CCTV_SCAN_ADD_MANUAL_ID,
                                             cctv_scan_add_manually_click, 0, true);
    lv_obj_set_pos(add_btn, IPC_ROW_X, add_y);
    lv_common_grad_sep_create(view, IPC_SEP_X, add_y + IPC_BTN_H + 10, IPC_SEP_W);

    /* 列表2：发现的 ONVIF 设备（可滚动到底部） */
    int disc_y = add_y + IPC_BTN_H + 10;
    lv_obj_t *disc_list = cctv_scan_list_cont_create(view, CCTV_SCAN_LIST_CONT_ID,
                                                     IPC_ROW_X, disc_y,
                                                     IPC_ITEM_W, SCREEN_H - disc_y);
    if (s_ctx.online_total > 0)
    {
        int valid_idx = 0;
        char buf[128];
        int y = 0;
        for (int i = 0; i < s_ctx.online_total; i++)
        {
            const char *ip = sat_ipcamera_ipaddr_get(i);
            ;
            if (ip == NULL || ip[0] == 0)
                continue;
            /* 跳过已经注册的 */
            if (cctv_device_has_url(0) && strcmp(ip, cctv_device(0)->ipaddr) == 0)
                continue;
            if (cctv_device_has_url(1) && strcmp(ip, cctv_device(1)->ipaddr) == 0)
                continue;
            valid_idx++;
            snprintf(buf, sizeof(buf), "%d.%s",
                     valid_idx, ip);

            lv_obj_t *btn = cctv_scan_btn_create(disc_list, buf, i,
                                                 cctv_scan_discovered_click, (intptr_t)i, false);
            lv_obj_set_pos(btn, 0, y);
            y += IPC_BTN_H;
            lv_common_grad_sep_create(disc_list, 0, y, IPC_SEP_W);
        }
    }
}

static void cctv_scan_view_create(void)
{
    /* WEBCAM 标签 (中间区域, 默认选中) */
    lv_common_text_create(cctv_view_container_get(), CCTV_TITLE_ID,
                          WEBCAM_X, WEBCAM_Y, WEBCAM_W, 52,
                          NULL, LV_OPA_COVER, 0x25679c, LV_OPA_COVER, 0x25679c,
                          6, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          6, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_WEBCAM),
                          0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    cctv_scan_list_create();

    /* 返回按钮 (屏幕坐标 929,505 → 容器坐标 739,505) */
    lv_resource_init(back_icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(back_icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *back = lv_common_img_btn_create(cctv_view_container_get(), CCTV_BACK_BTN_ID,
                                              739, 505, 67, 67,
                                              cctv_back_to_setting,
                                              true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &back_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &back_icon_focus);
    lv_obj_set_ext_click_area(back, 21);
}

/************************** 手动添加视图 **************************/

#define MANUAL_ROW_X 0
#define MANUAL_ROW_Y0 75
#define MANUAL_ROW_W 700
#define MANUAL_ROW_H 52
#define MANUAL_ROW_SPACING 52
#define MANUAL_ROW(i) (MANUAL_ROW_Y0 + MANUAL_ROW_SPACING * (i))

static void cctv_manual_model_left_cb(lv_event_t *e)
{
    s_ctx.temp_dev.model = (s_ctx.temp_dev.model - 1 + BRAND_COUNT) % BRAND_COUNT;
    lv_obj_t *cont = lv_obj_get_child_by_id(cctv_view_container_get(), CCTV_ADD_CONT_ID);
    if (cont)
    {
        lv_obj_t *label = lv_obj_get_child_by_id(cont, CCTV_MODE_LABEL2_ID);
        if (label)
            lv_label_set_text(label, lang_str_get(s_brand_ids[s_ctx.temp_dev.model]));
    }
}

static void cctv_manual_model_right_cb(lv_event_t *e)
{
    s_ctx.temp_dev.model = (s_ctx.temp_dev.model + 1) % BRAND_COUNT;
    lv_obj_t *cont = lv_obj_get_child_by_id(cctv_view_container_get(), CCTV_ADD_CONT_ID);
    if (cont)
    {
        lv_obj_t *label = lv_obj_get_child_by_id(cont, CCTV_MODE_LABEL2_ID);
        if (label)
            lv_label_set_text(label, lang_str_get(s_brand_ids[s_ctx.temp_dev.model]));
    }
}

static void cctv_manual_field_click_ip(lv_event_t *e)
{
    page_password_input_set_title_visible(false);
    page_password_input_set_placeholder(lang_str_get(STR_ENTER_CAMERA_IP_ADDR));
    page_password_input_set_password_mode(false);
    page_password_input_set_pwd_toggle(false);
    page_password_input_set_max_length(32);
    page_password_input_set_text(s_ctx.temp_dev.ip);
    page_password_input_set_back_cb(cctv_kb_back_to_cctv);
    page_password_input_set_confirm_cb(cctv_kb_confirm_ip);

    s_ctx.saved_from_manual = true;
    s_ctx.quit_to_kb = 1;
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_manual_field_click_account(lv_event_t *e)
{
    page_password_input_set_title_visible(false);
    page_password_input_set_placeholder(lang_str_get(STR_ENTER_CAMERA_ACCOUNT));
    page_password_input_set_password_mode(false);
    page_password_input_set_pwd_toggle(false);
    page_password_input_set_max_length(32);
    page_password_input_set_text(s_ctx.temp_dev.account);
    page_password_input_set_back_cb(cctv_kb_back_to_cctv);
    page_password_input_set_confirm_cb(cctv_kb_confirm_account);

    s_ctx.saved_from_manual = true;
    s_ctx.quit_to_kb = 1;
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void cctv_manual_field_click_password(lv_event_t *e)
{
    page_password_input_set_title_visible(false);
    page_password_input_set_placeholder(lang_str_get(STR_ENTER_CAMERA_PASSWORD));
    page_password_input_set_password_mode(false);
    page_password_input_set_pwd_toggle(true);
    page_password_input_set_max_length(32);
    page_password_input_set_text(s_ctx.temp_dev.password);
    page_password_input_set_back_cb(cctv_kb_back_to_cctv);
    page_password_input_set_confirm_cb(cctv_kb_confirm_password);

    s_ctx.saved_from_manual = true;
    s_ctx.quit_to_kb = 1;
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static lv_obj_t *cctv_manual_arrow_left_create(int y, lv_event_cb_t cb)
{
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    return lv_common_img_btn_create(cctv_view_container_get(), -1,
                                    270, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                    cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                    &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
}

static lv_obj_t *cctv_manual_arrow_right_create(int y, lv_event_cb_t cb)
{
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    return lv_common_img_btn_create(cctv_view_container_get(), -1,
                                    677, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                    cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                    &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
}

static void cctv_manual_row_create(int row_idx, const char *label_text,
                                   const char *value_text, int label2_id,
                                   lv_event_cb_t click_cb,
                                   bool has_arrows, lv_event_cb_t left_cb, lv_event_cb_t right_cb)
{
    int y = MANUAL_ROW(row_idx);

    lv_obj_t *btn = lv_common_setting_btn_double_label_create(
        cctv_view_container_get(), CCTV_ADD_CONT_ID + row_idx,
        MANUAL_ROW_X, y, MANUAL_ROW_W, MANUAL_ROW_H,
        click_cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        CCTV_ADD_LABEL1_ID + row_idx * 2, 6, 6, 250, 40,
        label_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 420, MANUAL_ROW_H,
        value_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    lv_obj_t *label1 = lv_obj_get_child_by_id(btn, CCTV_ADD_LABEL1_ID + row_idx * 2);
    lv_obj_t *label2 = lv_obj_get_child_by_id(btn, label2_id);

    if (has_arrows)
    {
        lv_obj_t *left = cctv_manual_arrow_left_create(y, left_cb);
        lv_obj_t *right = cctv_manual_arrow_right_create(y, right_cb);
        lv_common_arrow_link_press(left, label1, label2);
        lv_common_arrow_link_press(right, label1, label2);
    }
    else if (click_cb)
    {
        lv_obj_t *right = cctv_manual_arrow_right_create(y, click_cb);
        lv_common_arrow_link_press(right, label1, label2);
    }

    lv_common_grad_sep_create(cctv_view_container_get(), 282, y + 50, 422);
}

static void cctv_manual_confirm_click(lv_event_t *e)
{
    if (s_ctx.temp_dev.ip[0] == 0 || s_ctx.temp_dev.account[0] == 0 || s_ctx.temp_dev.password[0] == 0)
    {
        cctv_toast(lang_str_get(STR_LINK_DEVICE_FAILED));
        return;
    }

    cctv_rtsp_url_build();

    /* 设置 camera_display 上下文 */
    cam_display_ctx_fill(s_ctx.temp_dev.ip, s_ctx.temp_dev.rtsp_url,
                         s_ctx.temp_dev.account, s_ctx.temp_dev.password,
                         s_ctx.temp_dev.model, -1, true);

    cctv_goto_cam_display();
}

static void cctv_manual_back_click(lv_event_t *e)
{
    cctv_view_switch(CCTV_VIEW_SCAN);
}

static void cctv_manual_view_create(void)
{
    lv_common_text_create(cctv_view_container_get(), CCTV_MANUAL_TITLE_ID,
                          280, 28, MANUAL_ROW_W, 38,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_ADD_MANUALLY),
                          0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    /* Model row */
    cctv_manual_row_create(0, lang_str_get(STR_CAMERA_MODEL),
                           lang_str_get(s_brand_ids[s_ctx.temp_dev.model]),
                           CCTV_MODE_LABEL2_ID, NULL, true,
                           cctv_manual_model_left_cb, cctv_manual_model_right_cb);

    /* IP row */
    cctv_manual_row_create(1, lang_str_get(STR_CAMERA_IP_ADDRESS),
                           s_ctx.temp_dev.ip[0] ? s_ctx.temp_dev.ip : "",
                           CCTV_IP_LABEL2_ID, cctv_manual_field_click_ip,
                           false, NULL, NULL);

    /* Account row */
    cctv_manual_row_create(2, lang_str_get(STR_ACCOUNT_NUMBER),
                           s_ctx.temp_dev.account,
                           CCTV_ACCOUNT_LABEL2_ID, cctv_manual_field_click_account,
                           false, NULL, NULL);

    /* Password row */
    cctv_manual_row_create(3, lang_str_get(STR_PASSWORD),
                           s_ctx.temp_dev.password,
                           CCTV_PWD_LABEL2_ID, cctv_manual_field_click_password,
                           false, NULL, NULL);

    /* 确认按钮 (屏幕坐标 929,402 → 容器坐标 739,402) */
    lv_resource_init(confirm_icon, ICON_CCTV_C_CONFIRM_PNG_OFFSET, ICON_CCTV_C_CONFIRM_PNG_SIZE);
    lv_resource_init(confirm_icon_focus, ICON_CCTV_C_CONFIRM_FOCUS_PNG_OFFSET, ICON_CCTV_C_CONFIRM_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(cctv_view_container_get(), CCTV_MANUAL_CONFIRM_BTN_ID,
                             739, 402, 67, 67,
                             cctv_manual_confirm_click, true,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &confirm_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &confirm_icon_focus);

    /* 返回按钮 (屏幕坐标 929,505 → 容器坐标 739,505) */
    lv_resource_init(back_icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(back_icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *manual_back = lv_common_img_btn_create(cctv_view_container_get(), CCTV_MANUAL_BACK_BTN_ID,
                                                     739, 505, 67, 67,
                                                     cctv_manual_back_click, true,
                                                     LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     &back_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &back_icon_focus);
    lv_obj_set_ext_click_area(manual_back, 21);
}

/************************** 跳转 camera_display **************************/

static void cctv_goto_cam_display(void)
{
    /* 保持 keep_state：不释放 discovery、跳转时不重建/保存已注册 URL */
    s_ctx.navigating_to_cam_display = true;
    Amy_lv_page_goto(lv_page(camera_display), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 视频辅助 **************************/

static void cctv_video_open(const char *url)
{
    if (url == NULL || url[0] == 0)
        return;

    cctv_video_close();

    s_ctx.rtsp_ctx = live555_rtsp_client_start(url, "wlan0", 0, 0, SCREEN_W, SCREEN_H);
}

static void cctv_video_close(void)
{
    if (s_ctx.rtsp_ctx)
    {
        rtsp_client_context *ctx = s_ctx.rtsp_ctx;
        s_ctx.rtsp_ctx = NULL;
        lv_video_mode_enable(0);
        live555_rtsp_client_close(ctx);
    }

    if (s_ctx.manual_link_timer)
    {
        lv_timer_del(s_ctx.manual_link_timer);
        s_ctx.manual_link_timer = NULL;
    }
}

static void cctv_manual_link_timeout_cb(lv_timer_t *t)
{
    if (s_ctx.manual_link_timer)
    {
        lv_timer_del(s_ctx.manual_link_timer);
        s_ctx.manual_link_timer = NULL;
    }
    cctv_spinner_hide();
    cctv_video_close();
    cctv_toast(lang_str_get(STR_LINK_DEVICE_FAILED));
}

static void cctv_scan_timeout_cb(lv_timer_t *t)
{
    if (s_ctx.scan_timer)
    {
        lv_timer_del(s_ctx.scan_timer);
        s_ctx.scan_timer = NULL;
    }
    cctv_spinner_hide();

    if (s_ctx.searching_timer)
    {
        lv_timer_del(s_ctx.searching_timer);
        s_ctx.searching_timer = NULL;
    }
    s_ctx.is_searching = false;
}

static void cctv_scan_timer_start(void)
{
    cctv_scan_timer_stop();
    s_ctx.scan_timer = lv_timer_create((lv_timer_cb_t)cctv_scan_timeout_cb, SCAN_TIMEOUT_MS, NULL);
}

static void cctv_scan_timer_stop(void)
{
    if (s_ctx.scan_timer)
    {
        lv_timer_del(s_ctx.scan_timer);
        s_ctx.scan_timer = NULL;
    }
}

/************************** IPC 搜索 **************************/

static void cctv_search_timer_cb(lv_timer_t *t)
{
#if NET_SUPPORT_CABLE
    /* 按配对链路选搜索网卡：WiFi 配对搜 wlan0，有线配对搜 eth2。
     * 参数是网卡名而非“有线/无线”，有线摄像头接在 br0 等其它网卡时改这里即可。 */
    const char *ifname = (user_data_get()->public_cfg.pairing_mode == PAIRING_MODE_WLAN) ? "wlan0" : "eth2";
#else
    const char *ifname = "wlan0";
#endif
    sat_ipcamera_device_discover_search(0, ifname);
    if (s_ctx.searching_timer)
    {
        lv_timer_del(s_ctx.searching_timer);
        s_ctx.searching_timer = NULL;
    }
}

static void cctv_search_loading_start(void)
{
    if (s_ctx.is_searching)
        return;
    s_ctx.online_total = 0;
    sat_ipcamera_device_status_reset();
    sat_ipcamera_discovery_reserve();

    s_ctx.is_searching = true;

    cctv_spinner_show(lang_str_get(STR_LOAD));
    cctv_scan_timer_start();

    s_ctx.searching_timer = lv_timer_create(cctv_search_timer_cb, 5000, NULL);
    lv_timer_ready(s_ctx.searching_timer);
}

/************************** 事件回调 **************************/

static bool cctv_searched_result_has_new_camera(void)
{
    int online_cnt = sat_ipcamera_online_num_get();
    if (online_cnt <= 0)
        return false;

    for (int i = 0; i < online_cnt; i++)
    {
        const char *ip = sat_ipcamera_ipaddr_get(i);
        if (ip == NULL || ip[0] == 0)
            continue;

        bool is_registered = false;
        for (int j = 0; j < 2; j++)
        {
            if (!cctv_device_has_url(j))
                continue;
            if (cctv_device(j)->ipaddr[0] == 0)
                continue;
            if (strcmp(ip, cctv_device(j)->ipaddr) == 0)
            {
                is_registered = true;
                break;
            }
        }
        if (!is_registered)
            return true;
    }
    return false;
}

static void cctv_event_ipcamera_cb(void *arg)
{
    ipcamera_cb_attr *attr = (ipcamera_cb_attr *)arg;
    if (attr == NULL)
        return;
    if (attr->type == IPCAMERA_EVENT_TYPE_SEARCH_DONE)
    {
        if (attr->num <= 0)
            return;
        if (!cctv_searched_result_has_new_camera())
            return;
        cctv_scan_timer_stop();
        if (s_ctx.searching_timer)
        {
            lv_timer_del(s_ctx.searching_timer);
            s_ctx.searching_timer = NULL;
        }
        cctv_spinner_hide();
        s_ctx.is_searching = false;
        s_ctx.online_total = attr->num;
        if (s_ctx.cur_view == CCTV_VIEW_SCAN)
            cctv_view_switch(CCTV_VIEW_SCAN);
    }
    else if (attr->type == IPCAMERA_EVENT_TYPE_RTSP_DONE)
    {

        /* 注册完成，取消注册超时定时器 */
        if (s_ctx.manual_link_timer)
        {
            lv_timer_del(s_ctx.manual_link_timer);
            s_ctx.manual_link_timer = NULL;
        }

        int nstream = sat_ipcamera_profile_token_num_get(s_ctx.cur_search_idx);
        if (nstream > 0)
        {
            const char *pstr = NULL;
            for (int i = 0; i < nstream; i++)
            {
                const char *addr = sat_ipcamera_rtsp_addr_get(s_ctx.cur_search_idx, i);
                pstr = strstr(addr, "//");
                if (pstr != NULL)
                    break;
            }
            if (pstr) // 等待视频流进来直接跳转
            {
                pstr += 2;
                memset(s_ctx.temp_dev.rtsp_url, 0, sizeof(s_ctx.temp_dev.rtsp_url));
                snprintf(s_ctx.temp_dev.rtsp_url, sizeof(s_ctx.temp_dev.rtsp_url),
                         "rtsp://%s:%s@%s",
                         s_ctx.temp_dev.account, s_ctx.temp_dev.password, pstr);

                /* 设置 camera_display 上下文 */
                cam_display_ctx_fill(sat_ipcamera_ipaddr_get(s_ctx.cur_search_idx),
                                     s_ctx.temp_dev.rtsp_url, s_ctx.temp_dev.account,
                                     s_ctx.temp_dev.password, s_ctx.temp_dev.model, -1, false);

                cctv_video_open(s_ctx.temp_dev.rtsp_url);
                /* 视频连接超时定时器 */
                s_ctx.manual_link_timer = lv_timer_create((lv_timer_cb_t)cctv_manual_link_timeout_cb,
                                                          MANUAL_LINK_TIMEOUT_MS, NULL);
            }
            else
            {
                cctv_spinner_hide();
                cctv_toast(lang_str_get(STR_LINK_DEVICE_FAILED));
            }
        }
        else
        {
            cctv_spinner_hide();
            cctv_toast(lang_str_get(STR_LINK_DEVICE_FAILED));
        }
    }
}

static void cctv_event_video_mode_cb(void *arg)
{
    video_mode_cb_attr *attr = (video_mode_cb_attr *)arg;
    if (attr == NULL)
        return;

    if (s_ctx.rtsp_ctx == NULL)
        return;

    cctv_spinner_hide();

    if (s_ctx.manual_link_timer)
    {
        lv_timer_del(s_ctx.manual_link_timer);
        s_ctx.manual_link_timer = NULL;
    }

    Amy_video_mode_register_device(attr->func, attr->user_data);
    lv_video_mode_enable(1);

    /* 转移 rtsp_ctx 所有权给 camera_display */
    cam_display_ctx_set_rtsp_ctx(s_ctx.rtsp_ctx);
    s_ctx.rtsp_ctx = NULL;

    s_ctx.navigating_to_cam_display = true;
    Amy_lv_page_goto(lv_page(camera_display), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 返回回调辅助 **************************/

static void cctv_back_to_setting(lv_event_t *e)
{
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(cctv)
{
    s_ctx.quit_to_kb = false;
    Amy_event_cb_register(AMY_EVENT_ID_IPCAMERA, cctv_event_ipcamera_cb);
    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, cctv_event_video_mode_cb);
    /* 从存储的 URL 推导 model */
    for (int i = 0; i < 2; i++)
        cctv_model_detect(i);

    /* 从 password_input 返回时，不重新初始化 */
    if (s_ctx.enter_from_kb)
    {
        lv_common_bg_create(lv_scr_act_current());
        cctv_sidebar_create();

        if (s_ctx.saved_from_manual)
            cctv_view_switch(CCTV_VIEW_MANUAL);
        else
            cctv_view_switch(CCTV_VIEW_SCAN);

        if (s_ctx.show_onvif_spinner)
        {
            cctv_spinner_show(lang_str_get(STR_LOAD));
            s_ctx.show_onvif_spinner = false;
        }

        s_ctx.enter_from_kb = false;
        return;
    }

    /* 从 camera_display 返回时，显示扫描视图 */
    if (g_cctv_enter_from_cam_display)
    {
        g_cctv_enter_from_cam_display = false;
        lv_common_bg_create(lv_scr_act_current());
        cctv_sidebar_create();
        cctv_view_switch(CCTV_VIEW_SCAN);
        return;
    }

    /* 初始化临时数据 */
    cctv_temp_init_defaults();

    /* 背景 + 侧栏 */
    lv_common_bg_create(lv_scr_act_current());
    cctv_sidebar_create();

    /* 扫描视图 + 开始 ONVIF 搜索 */
    cctv_view_switch(CCTV_VIEW_SCAN);
    cctv_search_loading_start(); /* 注册事件 */
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(cctv)
{
    /* 导航到 camera_display 时不关闭视频（rtsp_ctx 所有权已转移） */
    bool navigating = s_ctx.navigating_to_cam_display;
    /* 临时离开（去键盘页继续注册 / 去 camera_display 预览）时保留搜索状态与占用 */
    bool keep_state = s_ctx.quit_to_kb || s_ctx.navigating_to_cam_display;
    s_ctx.navigating_to_cam_display = false;

    if (!navigating)
        cctv_video_close();

    cctv_spinner_hide();

    /* 前往 password_input 继续注册或跳 camera_display 预览时保留占用；真正离开页面时释放 */
    if (!keep_state)
        sat_ipcamera_discovery_release();

    if (s_ctx.manual_link_timer)
    {
        lv_timer_del(s_ctx.manual_link_timer);
        s_ctx.manual_link_timer = NULL;
    }
    cctv_scan_timer_stop();

    if (s_ctx.searching_timer)
    {
        lv_timer_del(s_ctx.searching_timer);
        s_ctx.searching_timer = NULL;
    }
    s_ctx.is_searching = false;

    /* 注销事件 */
    Amy_event_cb_register(AMY_EVENT_ID_IPCAMERA, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);

    /* 为已注册的摄像头构建 RTSP URL 并持久化 */
    if (!navigating)
    {
        for (int i = 0; i < 2; i++)
        {
            struct ipcamera_info *dev = cctv_device(i);
            if (dev->ipaddr[0] != 0 && dev->ipaddr[0] != '0')
            {
                cctv_rtsp_url_build_from(i);
            }
            else
            {
                memset(dev->sip_url, 0, sizeof(dev->sip_url));
                memset(dev->rtsp[0].rtsp_url, 0, sizeof(dev->rtsp[0].rtsp_url));
            }
        }

        user_data_save();
    }

    /* 重置状态（临时离开时保留搜索列表，便于返回后继续/重试） */
    s_ctx.cur_view = CCTV_VIEW_SCAN;
    if (!keep_state)
        s_ctx.online_total = 0;
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(cctv);
