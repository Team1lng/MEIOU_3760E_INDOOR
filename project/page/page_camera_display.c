/**
 * @file page_camera_display.c
 * @brief CCTV 摄像头预览与注册页面
 *
 * 从 page_cctv 搜索/手动添加流程中进入，展示 RTSP 视频流并支持将设备注册为 Camera1/2。
 */
#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "language.h"
#include <stdio.h>
#include <string.h>
#include "live/testRTSPClient.h"
#include "device/lv_disp_dev.h"
#include "Amy_lvgl.h"

/************************** 页面上下文（page_cctv 导航前设置） **************************/

typedef struct
{
    char ip[32];
    char rtsp_url[128];
    char account[64];
    char password[64];
    int model;  /* 0=Dahua, 1=Hikvision */
    int dev_id; /* -1=未注册, 0=Camera1, 1=Camera2 */
    bool from_manual;
    rtsp_client_context *rtsp_ctx;
    int confirm_target_idx;
} camera_display_ctx_t;

camera_display_ctx_t g_cam_display_ctx;

void cam_display_ctx_fill(const char *ip, const char *rtsp_url, const char *account,
                          const char *password, int model, int dev_id, bool from_manual)
{
    memset(&g_cam_display_ctx, 0, sizeof(g_cam_display_ctx));
    if (ip)
        strncpy(g_cam_display_ctx.ip, ip, sizeof(g_cam_display_ctx.ip) - 1);
    if (rtsp_url)
        strncpy(g_cam_display_ctx.rtsp_url, rtsp_url, sizeof(g_cam_display_ctx.rtsp_url) - 1);
    if (account)
        strncpy(g_cam_display_ctx.account, account, sizeof(g_cam_display_ctx.account) - 1);
    if (password)
        strncpy(g_cam_display_ctx.password, password, sizeof(g_cam_display_ctx.password) - 1);
    g_cam_display_ctx.model = model;
    g_cam_display_ctx.dev_id = dev_id;
    g_cam_display_ctx.from_manual = from_manual;
}

void cam_display_ctx_set_rtsp_ctx(rtsp_client_context *ctx)
{
    g_cam_display_ctx.rtsp_ctx = ctx;
}

/************************** 前向声明 **************************/

static void cam_display_back_to_cctv(void);
static void cam_display_video_close(void);
static void cam_display_video_open(void);
static void cam_display_event_video_mode_cb(void *arg);
static void cam_display_register_popup_create(void);
static void cam_display_register_do(int idx);
static void cam_display_confirm_replace_show(int idx);
static void cam_display_confirm_logout_show(int idx);

/************************** 辅助函数 **************************/

static struct ipcamera_info *cam_display_device(int idx)
{
    return &user_data_get()->public_cfg.net_cctv_device[idx];
}

static bool cam_display_device_has_url(int idx)
{
    return cam_display_device(idx)->rtsp[0].rtsp_url[0] != 0;
}

static void cam_display_save_to_device(int idx)
{
    struct ipcamera_info *dev = cam_display_device(idx);
    memset(dev->ipaddr, 0, sizeof(dev->ipaddr));
    memset(dev->username, 0, sizeof(dev->username));
    memset(dev->password, 0, sizeof(dev->password));
    strncpy(dev->ipaddr, g_cam_display_ctx.ip, sizeof(dev->ipaddr) - 1);
    strncpy(dev->username, g_cam_display_ctx.account, sizeof(dev->username) - 1);
    strncpy(dev->password, g_cam_display_ctx.password, sizeof(dev->password) - 1);

    char url[128];
    memset(url, 0, sizeof(url));
    if (g_cam_display_ctx.model)
        snprintf(url, sizeof(url), "rtsp://%s:%s@%s:554/Streaming/Channels/1",
                 g_cam_display_ctx.account, g_cam_display_ctx.password, g_cam_display_ctx.ip);
    else
        snprintf(url, sizeof(url), "rtsp://%s:%s@%s:554/cam/realmonitor?channel=1&subtype=1",
                 g_cam_display_ctx.account, g_cam_display_ctx.password, g_cam_display_ctx.ip);

    strncpy(dev->sip_url, url, sizeof(dev->sip_url) - 1);
    strncpy(dev->rtsp[0].rtsp_url, url, sizeof(dev->rtsp[0].rtsp_url) - 1);
    dev->rtsp[0].rtsp_url_type = RTSP_URL_TYPE_FULL;
}

/************************** Toast **************************/

static void cam_display_toast_timer_cb(lv_timer_t *t)
{
    lv_obj_t *cont = (lv_obj_t *)t->user_data;
    lv_timer_del(t);
    if (cont)
        lv_obj_del(cont);
}

static void cam_display_toast(const char *msg)
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

    lv_timer_create_recycle(cam_display_toast_timer_cb, 2500, cont);
}

/************************** 视频 **************************/

static void cam_display_video_close(void)
{
    if (g_cam_display_ctx.rtsp_ctx)
    {
        rtsp_client_context *ctx = g_cam_display_ctx.rtsp_ctx;
        g_cam_display_ctx.rtsp_ctx = NULL;
        lv_video_mode_enable(0);
        live555_rtsp_client_close(ctx);
    }
}

static void cam_display_video_open(void)
{
    cam_display_video_close();
    if (g_cam_display_ctx.rtsp_url[0] == 0)
        return;
    g_cam_display_ctx.rtsp_ctx =
        live555_rtsp_client_start(g_cam_display_ctx.rtsp_url, "wlan0", 0, 0, SCREEN_W, SCREEN_H);
}

/************************** 视频模式 **************************/

static void cam_display_event_video_mode_cb(void *arg)
{
    video_mode_cb_attr *attr = (video_mode_cb_attr *)arg;
    if (attr == NULL || g_cam_display_ctx.rtsp_ctx == NULL)
        return;

    Amy_video_mode_register_device(attr->func, attr->user_data);
    lv_video_mode_enable(1);
}

/************************** 确认对话框 **************************/

static void cam_display_confirm_replace_yes_cb(lv_event_t *e)
{
    cam_display_save_to_device(g_cam_display_ctx.confirm_target_idx);
    cam_display_video_close();
    user_data_save();
    lv_common_screen_click_unlock();
    lv_common_msgbox_del();
    cam_display_back_to_cctv();
}

static void cam_display_confirm_replace_no_cb(lv_event_t *e)
{
    lv_common_screen_click_unlock();
    lv_common_msgbox_del();
}

static void cam_display_confirm_replace_show(int idx)
{
    lv_common_screen_click_lock();
    g_cam_display_ctx.confirm_target_idx = idx;
    lv_common_msgbox_create_nobg(251, 106, 586, 388, 62,
                                 lang_str_get(STR_REPLACE_DEV),
                                 cam_display_confirm_replace_yes_cb,
                                 cam_display_confirm_replace_no_cb);
}

static void cam_display_confirm_logout_yes_cb(lv_event_t *e)
{
    struct ipcamera_info *dev = cam_display_device(g_cam_display_ctx.confirm_target_idx);
    memset(dev->ipaddr, 0, sizeof(dev->ipaddr));
    memset(dev->username, 0, sizeof(dev->username));
    memset(dev->password, 0, sizeof(dev->password));
    memset(dev->sip_url, 0, sizeof(dev->sip_url));
    memset(dev->rtsp[0].rtsp_url, 0, sizeof(dev->rtsp[0].rtsp_url));

    cam_display_video_close();
    user_data_save();
    lv_common_screen_click_unlock();
    lv_common_msgbox_del();
    cam_display_back_to_cctv();
}

static void cam_display_confirm_logout_no_cb(lv_event_t *e)
{
    lv_common_screen_click_unlock();
    lv_common_msgbox_del();
}

static void cam_display_confirm_logout_show(int idx)
{
    lv_common_screen_click_lock();
    g_cam_display_ctx.confirm_target_idx = idx;
    lv_common_msgbox_create_nobg(251, 106, 586, 388, 62,
                                 lang_str_get(STR_CONFIRM_CANCELLATION),
                                 cam_display_confirm_logout_yes_cb,
                                 cam_display_confirm_logout_no_cb);
}

/************************** 返回 **************************/

static void cam_display_back_to_cctv(void)
{
    cctv_set_enter_from_cam_display(true);
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 顶部栏 **************************/

static void cam_display_close_click(lv_event_t *e)
{
    cam_display_video_close();
    cam_display_back_to_cctv();
}

static void cam_display_header_create(void)
{
    /* 顶部栏: 0,0, 1024x80, 黑色 50% 不透明度 */
    lv_obj_t *header = lv_common_setting_cont_create(lv_scr_act_current(), CAMERA_DISPLAY_HEADER_ID,
                                                     0, 0, SCREEN_W, 80,
                                                     NULL, LV_OPA_50, 0x000000, LV_OPA_50, 0x000000,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    /* IP 文字居中 */
    lv_obj_t *ip_label = lv_common_text_create(header, -1,
                                               0, 0, 0, 0,
                                               NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               g_cam_display_ctx.ip, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal);
    lv_obj_center(ip_label);

    /* 关闭按钮 icon/common/cancel.png, (30,0), 80x80 */
    lv_resource_init(cancel_icon, ICON_COMMON_CANCEL_PNG_OFFSET, ICON_COMMON_CANCEL_PNG_SIZE);
    lv_obj_t *close_btn = lv_common_img_btn_create(lv_scr_act_current(),
                                                   CAMERA_DISPLAY_CLOSE_BTN_ID,
                                                   24, 0, 80, 80,
                                                   cam_display_close_click,
                                                   true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   &cancel_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);

    (void)close_btn;
}

/************************** 底部按钮 **************************/

static void cam_display_bottom_click(lv_event_t *e)
{
    if (g_cam_display_ctx.dev_id >= 0)
        cam_display_confirm_logout_show(g_cam_display_ctx.dev_id);
    else
        cam_display_register_popup_create();
}

static void cam_display_bottom_create(void)
{
    lv_obj_t *btn = lv_common_setting_cont_create(lv_scr_act_current(), CAMERA_DISPLAY_BOTTOM_BTN_ID,
                                                  0, 528, SCREEN_W, 72,
                                                  cam_display_bottom_click, LV_OPA_100, 0x000000, LV_OPA_100, 0x000000,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

    const char *text = g_cam_display_ctx.dev_id >= 0
                           ? lang_str_get(STR_CANCELLATION)
                           : lang_str_get(STR_REGISTER);

    lv_obj_t *label = lv_common_text_create(btn, -1,
                                            0, 0, 0, 0,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            text, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal);
    lv_obj_center(label);
}

/************************** 注册弹窗（参考 page_event_record 操作弹窗风格） **************************/

#define REG_POPUP_CONT_W 304
#define REG_POPUP_CONT_H 244
#define REG_POPUP_CONT_X ((SCREEN_W - REG_POPUP_CONT_W) / 2)
#define REG_POPUP_CONT_Y ((SCREEN_H - REG_POPUP_CONT_H) / 2)
#define REG_POPUP_BTN_W 217
#define REG_POPUP_BTN_H 48
#define REG_POPUP_BTN_X 43
#define REG_POPUP_BTN_Y_START 25
#define REG_POPUP_BTN_GAP (REG_POPUP_BTN_H + 25)

static void cam_display_popup_del(void)
{
    lv_common_screen_click_unlock();
    lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), CAMERA_DISPLAY_REG_POPUP_CONT_ID);
    if (overlay)
        lv_obj_del(overlay);
}

static void cam_display_popup_cam_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    cam_display_popup_del();
    cam_display_register_do(idx);
}

static void cam_display_popup_close_cb(lv_event_t *e)
{
    cam_display_popup_del();
}

static void cam_display_popup_btn_create(lv_obj_t *parent, int btn_index, const char *text,
                                         lv_event_cb_t cb, int idx)
{
    lv_obj_t *btn = lv_common_setting_cont_create(parent, -1,
                                                  REG_POPUP_BTN_X, REG_POPUP_BTN_Y_START + btn_index * REG_POPUP_BTN_GAP,
                                                  REG_POPUP_BTN_W, REG_POPUP_BTN_H,
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
    lv_obj_set_user_data(btn, (void *)(intptr_t)idx);
}

static void cam_display_register_popup_create(void)
{
    lv_common_screen_click_lock();

    /* 弹窗容器 */
    lv_obj_t *cont = lv_common_setting_cont_create(lv_scr_act_current(), CAMERA_DISPLAY_REG_POPUP_CONT_ID,
                                                   REG_POPUP_CONT_X, REG_POPUP_CONT_Y, REG_POPUP_CONT_W, REG_POPUP_CONT_H,
                                                   NULL, LV_OPA_80, 0x00131D, LV_OPA_80, 0x00131D,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                   20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(cont, true, LV_PART_MAIN);

    cam_display_popup_btn_create(cont, 0, lang_str_get(STR_CAMERA1), cam_display_popup_cam_cb, 0);
    cam_display_popup_btn_create(cont, 1, lang_str_get(STR_CAMERA2), cam_display_popup_cam_cb, 1);
    cam_display_popup_btn_create(cont, 2, lang_str_get(STR_CLOSE), cam_display_popup_close_cb, 0);
}

/************************** 注册逻辑 **************************/

static void cam_display_register_do(int idx)
{
    /* check duplicate */
    if (cam_display_device_has_url(0) &&
        strcmp(cam_display_device(0)->rtsp[0].rtsp_url, g_cam_display_ctx.rtsp_url) == 0)
    {
        cam_display_toast(lang_str_get(STR_REG_SAME_DEVICE));
        return;
    }
    if (cam_display_device_has_url(1) &&
        strcmp(cam_display_device(1)->rtsp[0].rtsp_url, g_cam_display_ctx.rtsp_url) == 0)
    {
        cam_display_toast(lang_str_get(STR_REG_SAME_DEVICE));
        return;
    }

    if (!cam_display_device_has_url(idx))
    {
        cam_display_save_to_device(idx);
        cam_display_video_close();
        user_data_save();
        cam_display_back_to_cctv();
    }
    else
    {
        cam_display_confirm_replace_show(idx);
    }
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(camera_display)
{
    lv_area_t area[] = {
        {0, 0, 1023, 79},
        {0, 528, 1023, 528 + 71},
        {360, 178, 360 + 303, 178 + 243},
        {251, 106, 251 + 585, 106 + 387},
    };
    for (int i = 0; i < sizeof(area) / sizeof(lv_area_t); i++)
    {
        Amy_rendering_area(&area[i]);
    }
    cam_display_header_create();
    cam_display_bottom_create();

    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, cam_display_event_video_mode_cb);

    /* ONVIF 搜索路径的流已在 cctv 启动并转移所有权，无需在此拉流；
       已注册/手动添加路径在此开始拉流 */
    if (g_cam_display_ctx.rtsp_ctx == NULL)
    {
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

        cam_display_video_open();
    }
}

static void Amy_lv_page_quit(camera_display)
{
    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_MODE, NULL);

    /* video close is handled explicitly by caller (register/logout/close) */
    /* If leaving without explicit close (e.g. system navigation), clean up */
    if (g_cam_display_ctx.rtsp_ctx)
    {
        rtsp_client_context *ctx = g_cam_display_ctx.rtsp_ctx;
        g_cam_display_ctx.rtsp_ctx = NULL;
        lv_video_mode_enable(0);
        live555_rtsp_client_close(ctx);
    }

    memset(&g_cam_display_ctx, 0, sizeof(g_cam_display_ctx));
    g_cam_display_ctx.dev_id = -1;
}

Amy_lv_page_create(camera_display);
