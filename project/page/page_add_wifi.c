#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "sat_wifi.h"
#include "sat_user_common.h"
#include "page_password_input.h"
#include <stdint.h>

extern void network_menu_select_wlan(void);

/************************** 常量 **************************/
#define WIFI_INPUT_BUF_SIZE 64
#define WIFI_CONNECT_TIMEOUT_S 20
#define WIFI_PWD_MIN_LEN 8

/************************** 页面上下文 **************************/
static struct
{
    wifi_context *wifi_ctx;
    char ssid_buf[WIFI_INPUT_BUF_SIZE];
    char pwd_buf[WIFI_INPUT_BUF_SIZE];
    char prefill_ssid[WIFI_INPUT_BUF_SIZE];
    int editing_field; /* 0=SSID, 1=password */
    bool enter_from_kb;
    bool has_prefill;
    bool direct_pwd;
    lv_timer_t *connect_timer;
    int connect_tick;              /* 连接超时计数 */
    volatile bool connect_running; /* 后台连接线程运行中 */
    volatile bool connect_ok;      /* 后台连接线程结果 */
} s_ctx;

void setting_wifi_input_name_set(char *name)
{
    if (name)
    {
        strncpy(s_ctx.prefill_ssid, name, sizeof(s_ctx.prefill_ssid) - 1);
        s_ctx.prefill_ssid[sizeof(s_ctx.prefill_ssid) - 1] = '\0';
        s_ctx.has_prefill = true;
    }
}

void setting_wifi_direct_pwd(void)
{
    s_ctx.direct_pwd = true;
}

/************************** 函数声明 **************************/
static void add_wifi_back_click(lv_event_t *ev);
static void add_wifi_ssid_click(lv_event_t *ev);
static void add_wifi_pwd_click(lv_event_t *ev);
static void add_wifi_ok_click(lv_event_t *ev);
static void add_wifi_cancel_click(lv_event_t *ev);
static void add_wifi_kb_back(lv_event_t *ev);
static void add_wifi_kb_confirm(lv_event_t *ev);

static void add_wifi_title_create(void);
static void add_wifi_form_create(void);
static void add_wifi_ui_create(void);

typedef enum
{
    ADD_WIFI_CONN_TEMP = 0,    /* 后台启动临时配置(wpa_supplicant) */
    ADD_WIFI_CONN_PERSIST = 1, /* 失败后后台恢复持久化旧配置 */
} add_wifi_conn_kind;

static void *add_wifi_connect_thread(void *arg);
static int add_wifi_connect_thread_start(add_wifi_conn_kind kind);

static void add_wifi_connect_start(void);
static void add_wifi_connect_timeout_cb(lv_timer_t *timer);
static void add_wifi_connect_failed(void);
static void add_wifi_show_msgbox(const char *msg);
static void add_wifi_spinner_show(const char *msg);
static void add_wifi_spinner_hide(void);

/************************** 回调实现 **************************/

static void add_wifi_back_click(lv_event_t *ev)
{
    network_menu_select_wlan();
    Amy_lv_page_goto(lv_page(network), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_cancel_click(lv_event_t *ev)
{
    network_menu_select_wlan();
    Amy_lv_page_goto(lv_page(network), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_kb_back(lv_event_t *ev)
{
    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(add_wifi), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_kb_confirm(lv_event_t *ev)
{
    const char *text = page_password_input_get_text();

    if (s_ctx.editing_field == 0)
    {
        strncpy(s_ctx.ssid_buf, text, sizeof(s_ctx.ssid_buf) - 1);
        s_ctx.ssid_buf[sizeof(s_ctx.ssid_buf) - 1] = '\0';
    }
    else
    {
        strncpy(s_ctx.pwd_buf, text, sizeof(s_ctx.pwd_buf) - 1);
        s_ctx.pwd_buf[sizeof(s_ctx.pwd_buf) - 1] = '\0';
    }

    s_ctx.enter_from_kb = true;
    Amy_lv_page_goto(lv_page(add_wifi), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_direct_pwd_confirm(lv_event_t *ev)
{
    const char *text = page_password_input_get_text();
    strncpy(s_ctx.pwd_buf, text, sizeof(s_ctx.pwd_buf) - 1);
    s_ctx.pwd_buf[sizeof(s_ctx.pwd_buf) - 1] = '\0';

    /* 直接创建连接 UI 并开始连接，超时/失败统一提示 */
    add_wifi_connect_start();
}

static void add_wifi_ssid_click(lv_event_t *ev)
{
    s_ctx.editing_field = 0;

    page_password_input_set_title_visible(true);
    page_password_input_set_title_text(lang_str_get(STR_WIFI_ACCOUNT));
    page_password_input_set_placeholder(lang_str_get(STR_INPUT_WIFI_NAME));
    page_password_input_set_password_mode(false);
    page_password_input_set_pwd_toggle(false);
    page_password_input_set_max_length(32);
    page_password_input_set_text(s_ctx.ssid_buf);
    page_password_input_set_back_cb(add_wifi_kb_back);
    page_password_input_set_confirm_cb(add_wifi_kb_confirm);

    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_pwd_click(lv_event_t *ev)
{
    s_ctx.editing_field = 1;

    page_password_input_set_title_visible(true);
    page_password_input_set_title_text(lang_str_get(STR_WIFI_PASSWD));
    page_password_input_set_placeholder(lang_str_get(STR_PLEASE_INPUT_PASSWORD));
    page_password_input_set_password_mode(true);
    page_password_input_set_pwd_toggle(true);
    page_password_input_set_max_length(32);
    page_password_input_set_text(s_ctx.pwd_buf);
    page_password_input_set_back_cb(add_wifi_kb_back);
    page_password_input_set_confirm_cb(add_wifi_kb_confirm);

    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

static void add_wifi_ok_click(lv_event_t *ev)
{
    add_wifi_connect_start();
}

/************************** 表单 UI **************************/

static void add_wifi_title_create(void)
{
    lv_common_text_create(lv_scr_act_current(), ADD_WIFI_TITLE_ID,
                          312, 30, 400, 40,
                          NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_ADD_MANUALLY), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void add_wifi_form_create(void)
{
    /* WiFi Account 字段 */
    {
        lv_common_text_create(lv_scr_act_current(), ADD_WIFI_NAME_LABEL_ID,
                              120, 120, 170, 40,
                              NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                              lang_str_get(STR_WIFI_ACCOUNT), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

        lv_obj_t *ssid_val = lv_common_text_create(lv_scr_act_current(), ADD_WIFI_NAME_VAL_LABEL_ID,
                                                   310, 120, 500, 40,
                                                   add_wifi_ssid_click, LV_OPA_TRANSP, 0, LV_OPA_COVER, 0x1A3549,
                                                   0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x878787,
                                                   0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x878787,
                                                   s_ctx.ssid_buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal);
        lv_obj_set_ext_click_area(ssid_val, 10);
        lv_common_grad_sep_create(lv_scr_act_current(), 310, 164, 500);
    }

    /* Password 字段 */
    {
        lv_common_text_create(lv_scr_act_current(), ADD_WIFI_PWD_LABEL_ID,
                              120, 200, 170, 40,
                              NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                              lang_str_get(STR_WIFI_PASSWD), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

        lv_obj_t *pwd_val = lv_common_text_create(lv_scr_act_current(), ADD_WIFI_PWD_VAL_LABEL_ID,
                                                  310, 200, 500, 40,
                                                  add_wifi_pwd_click, LV_OPA_TRANSP, 0, LV_OPA_COVER, 0x1A3549,
                                                  0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x878787,
                                                  0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x878787,
                                                  s_ctx.pwd_buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal);
        lv_obj_set_ext_click_area(pwd_val, 10);
        lv_common_grad_sep_create(lv_scr_act_current(), 310, 244, 500);
    }

    /* OK 按钮 */
    {
        lv_obj_t *ok_btn = lv_common_text_create(lv_scr_act_current(), ADD_WIFI_CONFIRM_BTN_ID,
                                                 236, 461, 182, 63,
                                                 add_wifi_ok_click, LV_OPA_COVER, 0x1A3549, LV_OPA_COVER, 0x2D85FB,
                                                 8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 lang_str_get(STR_CONFIRM), 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
        lv_obj_set_ext_click_area(ok_btn, 10);
        lv_common_grad_sep_create(lv_scr_act_current(), 236, 526, 182);
    }

    /* Cancel 按钮 */
    {
        lv_obj_t *cancel_btn = lv_common_text_create(lv_scr_act_current(), ADD_WIFI_CANCEL_BTN_ID,
                                                     606, 461, 182, 63,
                                                     add_wifi_cancel_click, LV_OPA_COVER, 0x1A3549, LV_OPA_COVER, 0x2D85FB,
                                                     8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     lang_str_get(STR_CANCEL), 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
        lv_obj_set_ext_click_area(cancel_btn, 10);
        lv_common_grad_sep_create(lv_scr_act_current(), 606, 526, 182);
    }
}

static void add_wifi_ui_create(void)
{
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_PANEL_COLOR);
    add_wifi_title_create();
    add_wifi_form_create();

    /* 返回按钮 */
    lv_resource_init(back_icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(back_icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), ADD_WIFI_BACK_BTN_ID,
                                              929, 505, 67, 67,
                                              add_wifi_back_click, true,
                                              LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &back_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &back_icon_focus);
    lv_obj_set_ext_click_area(back, 21);
}

/************************** 连接流程 **************************/

static void add_wifi_connect_timeout_cb(lv_timer_t *timer)
{
    s_ctx.connect_tick++;

    if (s_ctx.connect_tick >= WIFI_CONNECT_TIMEOUT_S)
    {
        s_ctx.connect_tick = 0;
        add_wifi_connect_failed();
        return;
    }

    /* 后台连接线程未完成：wpa_supplicant 还在启动，继续等待 */
    if (s_ctx.connect_running)
    {
        return;
    }

    /* 后台连接线程已结束但启动失败 */
    if (!s_ctx.connect_ok)
    {
        s_ctx.connect_tick = 0;
        add_wifi_connect_failed();
        return;
    }

    bool connected = false;
    unsigned char ip[64] = {0};

    /* 关联成功(wpa_state=COMPLETED)不代表数据面可用，必须拿到有效 IP 才算连接成功 */
    if (s_ctx.wifi_ctx &&
        wifi_device_connection_status(s_ctx.wifi_ctx, NULL, NULL, ip, &connected, NULL) &&
        connected &&
        ip[0] != '\0' && strcmp((char *)ip, "0.0.0.0") != 0)
    {
        s_ctx.connect_tick = 0;
        wifi_device_tmp_sync(s_ctx.wifi_ctx);
        add_wifi_spinner_hide();
        if (s_ctx.connect_timer)
        {
            lv_timer_del(s_ctx.connect_timer);
            s_ctx.connect_timer = NULL;
        }
        add_wifi_show_msgbox(lang_str_get(STR_CONNECTION_SUCCESS));
        network_menu_select_wlan();
        Amy_lv_page_goto(lv_page(network), LV_SCR_LOAD_ANIM_NONE, );
    }
}

static void add_wifi_toast_timer_cb(lv_timer_t *t)
{
    lv_obj_t *cont = (lv_obj_t *)t->user_data;
    lv_timer_del(t);
    if (cont)
        lv_obj_del(cont);
}

static void add_wifi_show_msgbox(const char *msg)
{
    if (msg == NULL)
        return;

    lv_obj_t *cont = lv_common_setting_cont_create(lv_scr_act_current(), -1,
                                                   0, 0, 498, 299,
                                                   NULL, LV_OPA_90, 0x1A1A1A, LV_OPA_90, 0x1A1A1A,
                                                   12, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                                   12, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_common_text_create(cont, -1,
                                            0, 0, 0, 0,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            msg, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal);
    lv_obj_center(label);

    lv_timer_create_recycle(add_wifi_toast_timer_cb, 2500, cont);
}

/************************** Loading Spinner **************************/

static void add_wifi_spinner_show(const char *msg)
{
    add_wifi_spinner_hide();

    lv_obj_t *overlay = lv_common_img_btn_create(lv_scr_act_current(), ADD_WIFI_SPINNER_OVERLAY_ID,
                                                      0, 0, SCREEN_W, SCREEN_H,
                                                      NULL, false, LV_OPA_70, 0x000000, LV_OPA_70, 0x000000,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *box = lv_common_setting_cont_create(overlay, -1,
                                                  0, 0, 498, 299,
                                                  NULL, LV_OPA_COVER, 0x1A1A1A, LV_OPA_COVER, 0x1A1A1A,
                                                  0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C,
                                                  0, 2, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0xEFCC8C);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *spinner = lv_common_spinner_create(box, -1,
                                                 0, 0, 120, 120,
                                                 0x2183EF, 0xFFFFFF, 1000, 45, 12);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -30);

    if (msg != NULL)
    {
        lv_obj_t *label = lv_common_text_create(box, -1,
                                                0, 0, 0, 0,
                                                NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                msg, 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_normal);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
    }
}

static void add_wifi_spinner_hide(void)
{
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), ADD_WIFI_SPINNER_OVERLAY_ID);
    if (overlay)
    {
        lv_obj_del(overlay);
    }
}

static void add_wifi_connect_failed(void)
{
    /* 后台恢复持久化旧配置，避免阻塞 UI */
    add_wifi_connect_thread_start(ADD_WIFI_CONN_PERSIST);

    add_wifi_spinner_hide();
    if (s_ctx.connect_timer)
    {
        lv_timer_del(s_ctx.connect_timer);
        s_ctx.connect_timer = NULL;
    }
    add_wifi_show_msgbox(lang_str_get(STR_CONNECTION_FAIL));
}

static void *add_wifi_connect_thread(void *arg)
{
    add_wifi_conn_kind kind = (add_wifi_conn_kind)(uintptr_t)arg;

    pthread_detach(pthread_self());

    if (kind == ADD_WIFI_CONN_TEMP)
    {
        s_ctx.connect_ok = wifi_device_temp_connecting(s_ctx.wifi_ctx);
    }
    else
    {
        wifi_device_connecting(s_ctx.wifi_ctx);
    }
    s_ctx.connect_running = false;

    return NULL;
}

static int add_wifi_connect_thread_start(add_wifi_conn_kind kind)
{
    pthread_t tid;

    /* 唯一创建入口：确保任意时刻最多一个 wifi 后台线程 */
    if (s_ctx.connect_running)
    {
        return -1;
    }

    s_ctx.connect_running = true;
    if (pthread_create(&tid, sat_pthread_attr_get(), add_wifi_connect_thread,
                       (void *)(uintptr_t)kind) != 0)
    {
        s_ctx.connect_running = false;
        return -1;
    }
    return 0;
}

static void add_wifi_connect_start(void)
{
    /* 校验 SSID 不能为空 */
    if (strlen(s_ctx.ssid_buf) == 0)
    {
        add_wifi_show_msgbox(lang_str_get(STR_WIFI_NAME_EMPTY));
        return;
    }

    /* 校验密码：非空时长度至少 8 位 */
    if (strlen(s_ctx.pwd_buf) > 0 && strlen(s_ctx.pwd_buf) < WIFI_PWD_MIN_LEN)
    {
        add_wifi_show_msgbox(lang_str_get(STR_WIFI_PASSW_SHOT));
        return;
    }

    /* 上一个连接/恢复线程还在运行，忽略本次点击 */
    if (s_ctx.connect_running)
    {
        return;
    }

    if (s_ctx.wifi_ctx == NULL ||
        !wifi_device_user_to_temp(s_ctx.wifi_ctx, s_ctx.ssid_buf, s_ctx.pwd_buf))
    {
        add_wifi_connect_failed();
        return;
    }

    add_wifi_spinner_show(lang_str_get(STR_CONNECTING));

    /* 耗时连接操作放后台线程，UI 线程只轮询连接状态 */
    if (add_wifi_connect_thread_start(ADD_WIFI_CONN_TEMP) != 0)
    {
        add_wifi_connect_failed();
        return;
    }

    s_ctx.connect_tick = 0;
    s_ctx.connect_timer = lv_timer_create(add_wifi_connect_timeout_cb, 1000, NULL);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(add_wifi)
{
    if (s_ctx.enter_from_kb)
    {
        s_ctx.enter_from_kb = false;
        add_wifi_ui_create();
        return;
    }

    /* 首次进入：初始化 */
    if (s_ctx.wifi_ctx == NULL)
        s_ctx.wifi_ctx = wifi_context_get();

    memset(s_ctx.ssid_buf, 0, sizeof(s_ctx.ssid_buf));
    memset(s_ctx.pwd_buf, 0, sizeof(s_ctx.pwd_buf));

    /* 从扫描列表点击时预填 SSID */
    if (s_ctx.has_prefill)
    {
        strncpy(s_ctx.ssid_buf, s_ctx.prefill_ssid, sizeof(s_ctx.ssid_buf) - 1);
        s_ctx.ssid_buf[sizeof(s_ctx.ssid_buf) - 1] = '\0';
        s_ctx.has_prefill = false;
    }

    /* 直接密码模式：从扫描列表点击，跳过表单直接进入密码输入 */
    if (s_ctx.direct_pwd)
    {
        s_ctx.direct_pwd = false;
        page_password_input_set_title_visible(true);
        page_password_input_set_title_text(lang_str_get(STR_WIFI_PASSWD));
        page_password_input_set_placeholder(lang_str_get(STR_PLEASE_INPUT_PASSWORD));
        page_password_input_set_password_mode(true);
        page_password_input_set_pwd_toggle(true);
        page_password_input_set_max_length(32);
        page_password_input_set_text(s_ctx.pwd_buf);
        page_password_input_set_back_cb(add_wifi_back_click);
        page_password_input_set_confirm_cb(add_wifi_direct_pwd_confirm);
        Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
        return;
    }

    add_wifi_ui_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(add_wifi)
{
    if (s_ctx.connect_timer)
    {
        lv_timer_del(s_ctx.connect_timer);
        s_ctx.connect_timer = NULL;
    }
    add_wifi_spinner_hide();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(add_wifi);
