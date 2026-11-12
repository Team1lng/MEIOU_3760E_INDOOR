#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "page_password_input.h"
#include "sat_user_common.h"
#include "user_data.h"
#include "user_network.h"
#include "call_api.h"
#include "live/testRTSPClient.h"
#include "../pwm.h"
#include "Amy_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************** 全局上下文 **************************/
data_manage_type_t g_data_manage_type = DATA_MANAGE_TYPE_NONE;

/************************** 布局常量 (匹配 EMOS-Indoor sys_setting_btn_create) **************************/
#define ROW_X 199
#define ROW_W 700
#define ROW_H 52
#define ROW1_Y 75
#define ROW_SPACING 52
#define LABEL_OFS_X 6
#define LABEL_OFS_Y 6
#define LABEL_W 250
#define ARROW_L_X 460
#define ARROW_R_X 867
#define ARROW_W 52
#define ARROW_H 52
#define SEP_X 472
#define SEP_W 422
#define SEP_OFS_Y 50

/************************** 行索引 **************************/
typedef enum
{
    ADMIN_ROW_ROOM_NUMBER = 0,
    ADMIN_ROW_ROUTER_ADDRESS,
    ADMIN_ROW_CHANGE_PASSWORD,
    ADMIN_ROW_CARD_MANAGE,
    ADMIN_ROW_FINGER_MANAGE,
    ADMIN_ROW_PASSW_MANAGE,
    ADMIN_ROW_NUM
} admin_row_t;

/************************** 行标签 ID **************************/
static const xls_str_id_t s_admin_label_ids[] = {
    STR_ROOM_NUMBER,
    STR_ROUTER_ADDRESS,
    STR_CHANGE_PASSWORD,
    STR_CARD_MANAGE,
    STR_FINGER_MANAGE,
    STR_PASSW_MANAGE,
};

/************************** 页面上下文 **************************/
static struct
{
    int cur_family_id;
    lv_timer_t *pwd_tip_timer;
} s_ctx = {.cur_family_id = 1};

/************************** 函数声明 **************************/
static void admin_back_obj_click(lv_event_t *ev);
static void admin_room_number_left_cb(lv_event_t *ev);
static void admin_room_number_right_cb(lv_event_t *ev);
static void admin_router_address_cb(lv_event_t *ev);
static void admin_change_password_cb(lv_event_t *ev);
static void admin_card_manage_cb(lv_event_t *ev);
static void admin_finger_manage_cb(lv_event_t *ev);
static void admin_passw_manage_cb(lv_event_t *ev);

static void admin_title_create(void);
static void admin_back_create(void);
static void admin_row_separator_create(admin_row_t row);
static void admin_arrow_right_create(admin_row_t row, lv_event_cb_t cb);
static void admin_arrow_left_create(admin_row_t row, lv_event_cb_t cb);
static void admin_label2_update(admin_row_t row, const char *text);
static void admin_row_create(admin_row_t row, const char *label2_text, lv_event_cb_t cb, bool with_right_arrow);

/************************** 辅助函数 **************************/

static int admin_row_y(admin_row_t row)
{
    return ROW1_Y + ROW_SPACING * row;
}

static void admin_label2_update(admin_row_t row, const char *text)
{
    int cont_id = ADMIN_ROW1_CONT_ID + row * 3;
    int label2_id = ADMIN_ROW1_LABEL2_ID + row * 3;
    lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), cont_id);
    lv_obj_t *label = lv_obj_get_child_by_id(cont, label2_id);
    if (label)
        lv_label_set_text(label, text);
}

static void admin_row_separator_create(admin_row_t row)
{
    int y = admin_row_y(row);
    lv_common_grad_sep_create(lv_scr_act_current(), SEP_X, y + SEP_OFS_Y, SEP_W);
}

static void admin_arrow_right_create(admin_row_t row, lv_event_cb_t cb)
{
    int y = admin_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_NEXT_PNG_OFFSET, ICON_COMMON_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_NEXT_FOCUS_PNG_OFFSET, ICON_COMMON_NEXT_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_R_X, y, ICON_COMMON_NEXT_PNG_WIDTH, ICON_COMMON_NEXT_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADMIN_ROW1_CONT_ID + row * 3), ADMIN_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADMIN_ROW1_CONT_ID + row * 3), ADMIN_ROW1_LABEL2_ID + row * 3));
}

static void admin_arrow_left_create(admin_row_t row, lv_event_cb_t cb)
{
    int y = admin_row_y(row) + 5;
    lv_resource_init(icon, ICON_COMMON_PREV_PNG_OFFSET, ICON_COMMON_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_PREV_FOCUS_PNG_OFFSET, ICON_COMMON_PREV_FOCUS_PNG_SIZE);
    lv_obj_t *arrow = lv_common_img_btn_create(lv_scr_act_current(), -1,
                                               ARROW_L_X, y, ICON_COMMON_PREV_PNG_WIDTH, ICON_COMMON_PREV_PNG_HEIGHT,
                                               cb, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &icon, LV_OPA_COVER, 0xff0000, LV_ALIGN_CENTER, &icon_focus);
    lv_common_arrow_link_press(arrow,
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADMIN_ROW1_CONT_ID + row * 3), ADMIN_ROW1_LABEL1_ID + row * 3),
                               lv_obj_get_child_by_id(lv_obj_get_child_by_id(lv_scr_act_current(), ADMIN_ROW1_CONT_ID + row * 3), ADMIN_ROW1_LABEL2_ID + row * 3));
}

/************************** 回调实现 **************************/

static int admin_family_id_get(void)
{
    return user_data_get()->private_cfg.device.attr.room % 100;
}

static void admin_family_id_set(int id)
{
    // 获取当前完整房号（如103）
    int cur_room_no = user_data_get()->private_cfg.device.attr.room;
    // 提取楼层
    int floor = cur_room_no / 100;
    // 组装新房号：楼层不变，房间号替换为id
    int new_room_no = floor * 100 + id;

    // 可选：增加合法性校验（和generate_ip保持一致）
    if (id < 1 || id > 15)
    {
        return; // 房间号非法，直接退出
    }

    user_data_get()->private_cfg.device.attr.room = new_room_no;
}

/* 房号 - 左键 (EMOS-Indoor: admin_apartment_number_set_left_btn_up) */
static void admin_room_number_left_cb(lv_event_t *ev)
{
    int id = s_ctx.cur_family_id;
    id = (id <= 1) ? 4 : id - 1;
    s_ctx.cur_family_id = id;
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", id);
    admin_label2_update(ADMIN_ROW_ROOM_NUMBER, buf);
}

/* 房号 - 右键 (EMOS-Indoor: admin_apartment_number_set_right_btn_up) */
static void admin_room_number_right_cb(lv_event_t *ev)
{
    int id = s_ctx.cur_family_id;
    id = (id >= 4) ? 1 : id + 1;
    s_ctx.cur_family_id = id;
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", id);
    admin_label2_update(ADMIN_ROW_ROOM_NUMBER, buf);
}

/* 路由器地址 - 点击刷新 */
static void admin_router_address_cb(lv_event_t *ev)
{
    char ip[32] = "0.0.0.0";
#if NET_SUPPORT_CABLE
    const char *iface = (user_data_get()->public_cfg.pairing_mode == PAIRING_MODE_WLAN) ? "wlan0" : "eth2";
#else
    const char *iface = "wlan0";
#endif
    sat_ip_mac_addres_get(iface, ip, NULL, NULL);
    admin_label2_update(ADMIN_ROW_ROUTER_ADDRESS, ip);
}

/* 修改密码 → password_input */
static void pwd_input_back_cb(lv_event_t *e) { Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, ); }

static void admin_pwd_tip_close_cb(lv_timer_t *t)
{
    s_ctx.pwd_tip_timer = NULL;
    lv_timer_del(t);
    lv_common_msgbox_del();
}

static void admin_pwd_tip_show(const char *msg)
{
    if (s_ctx.pwd_tip_timer != NULL)
    {
        lv_timer_del(s_ctx.pwd_tip_timer);
        s_ctx.pwd_tip_timer = NULL;
    }
    lv_common_msgbox_del();
    lv_common_msgbox_notify_create(329, 172, 366, 256, 42, msg);
    s_ctx.pwd_tip_timer = lv_timer_create(admin_pwd_tip_close_cb, 2000, NULL);
}

static void pwd_input_confirm_cb(lv_event_t *e)
{
    const char *input = page_password_input_get_text();
    if (!admin_password_validate(input))
    {
        admin_pwd_tip_show(lang_str_get(STR_PASSW_SHOT));
        return;
    }
    strncpy(user_data_get()->private_cfg.admin.password, input,
            sizeof(user_data_get()->private_cfg.admin.password) - 1);
    Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
}

static void admin_change_password_cb(lv_event_t *ev)
{
    page_password_input_set_title_text(lang_str_get(STR_CHANGE_PASSWORD));
    page_password_input_set_max_length(ADMIN_PASSWORD_LEN);
    page_password_input_set_back_cb(pwd_input_back_cb);
    page_password_input_set_confirm_cb(pwd_input_confirm_cb);
    page_password_input_set_placeholder(lang_str_get(STR_PLEASE_INPUT_PASSWORD));
    page_password_input_set_password_mode(true);
    Amy_lv_page_goto(lv_page(password_input), LV_SCR_LOAD_ANIM_NONE, );
}

/* 卡管理 → card_manager */
static void admin_card_manage_cb(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(card_manager), LV_SCR_LOAD_ANIM_NONE, );
}

/* 指纹管理 → finger_manager (TODO: 待实现) */
static void admin_finger_manage_cb(lv_event_t *ev)
{
    g_data_manage_type = DATA_MANAGE_TYPE_FINGER;
    /* TODO: Amy_lv_page_goto(lv_page(finger_manager), LV_SCR_LOAD_ANIM_NONE, ); */
    Amy_lv_page_goto(lv_page(fingerprint_manager), LV_SCR_LOAD_ANIM_NONE, );
}

/* 密码管理 → password_manager (TODO: 待实现) */
static void admin_passw_manage_cb(lv_event_t *ev)
{
    g_data_manage_type = DATA_MANAGE_TYPE_PASSWORD;
    /* TODO: Amy_lv_page_goto(lv_page(password_manager), LV_SCR_LOAD_ANIM_NONE, ); */
    Amy_lv_page_goto(lv_page(password_manager), LV_SCR_LOAD_ANIM_NONE, );
}

/* 返回 → system (EMOS-Indoor: admin_setting_back_btn_up → goto_layout(setting_sys))
 * 房号变更时：保存变更并直接应用新 IP（重启 RTSP 服务器），不重启设备 */
static void admin_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(system), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 行创建 **************************/

static void admin_row_create(admin_row_t row, const char *label2_text, lv_event_cb_t cb, bool with_right_arrow)
{
    int y = admin_row_y(row);
    int cont_id = ADMIN_ROW1_CONT_ID + row * 3;
    int label1_id = ADMIN_ROW1_LABEL1_ID + row * 3;
    int label2_id = ADMIN_ROW1_LABEL2_ID + row * 3;

    lv_common_setting_btn_double_label_create(
        lv_scr_act_current(), cont_id, ROW_X, y, ROW_W, ROW_H,
        cb, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        label1_id, LABEL_OFS_X, LABEL_OFS_Y, LABEL_W, ROW_H - LABEL_OFS_Y * 2,
        lang_str_get(s_admin_label_ids[row]), 0xffffff, 0xff0000, LV_TEXT_ALIGN_LEFT, lv_font_normal,
        label2_id, 280, 0, 380, ROW_H,
        label2_text, 0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);

    admin_row_separator_create(row);
    if (with_right_arrow)
        admin_arrow_right_create(row, cb);
}

/* 房号行: 与 EMOS-Indoor admin_apartment_number_set_btn_create 对应,
 *   左/右箭头分别处理加减, 容器本身无点击回调 */
static void admin_room_number_row_create(void)
{
    admin_row_t row = ADMIN_ROW_ROOM_NUMBER;
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", s_ctx.cur_family_id);

    admin_row_create(row, buf, NULL, false);
    admin_arrow_left_create(row, admin_room_number_left_cb);
    admin_arrow_right_create(row, admin_room_number_right_cb);
}

/* 路由器地址行: 与 EMOS-Indoor admin_router_address_set_btn_create 对应 */
static void admin_router_address_row_create(void)
{
    admin_row_t row = ADMIN_ROW_ROUTER_ADDRESS;
    char ip[32] = "0.0.0.0";
    sat_ip_mac_addres_get("wlan0", ip, NULL, NULL);
    admin_row_create(row, ip, admin_router_address_cb, false);
}

/************************** 侧栏标题 **************************/

static void admin_title_create(void)
{
    lv_resource_init(title_icon, ICON_SETTING_SYSTEM_PNG_OFFSET, ICON_SETTING_SYSTEM_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SYSTEM_FOCUS_PNG_OFFSET, ICON_SETTING_SYSTEM_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), ADMIN_TITLE_ICON_ID,
                             30, 218, 130, 130,
                             NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &title_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
    lv_common_text_create(lv_scr_act_current(), ADMIN_TITLE_ID,
                          0, 338, SIDEBAR_W, 28,
                          NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                          lang_str_get(STR_ADMINISTRATORS),
                          0xffffff, 0xff0000, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal);
}

/************************** 返回按钮 **************************/

static void admin_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *admin_back = lv_common_img_btn_create(
        lv_scr_act_current(), ADMIN_BACK_BTN_ID, 929, 505, 67, 67,
        admin_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
        &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(admin_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(admin)
{
    lv_common_bg_create(lv_scr_act_current());

    s_ctx.cur_family_id = admin_family_id_get();

    admin_title_create();

    admin_room_number_row_create();
    admin_router_address_row_create();
    admin_row_create(ADMIN_ROW_CHANGE_PASSWORD,
                     user_data_get()->private_cfg.admin.password,
                     admin_change_password_cb, true);
    admin_row_create(ADMIN_ROW_CARD_MANAGE,
                     lang_str_get(s_admin_label_ids[ADMIN_ROW_CARD_MANAGE]),
                     admin_card_manage_cb, true);
    admin_row_create(ADMIN_ROW_FINGER_MANAGE,
                     lang_str_get(s_admin_label_ids[ADMIN_ROW_FINGER_MANAGE]),
                     admin_finger_manage_cb, true);
    admin_row_create(ADMIN_ROW_PASSW_MANAGE,
                     lang_str_get(s_admin_label_ids[ADMIN_ROW_PASSW_MANAGE]),
                     admin_passw_manage_cb, true);

    admin_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(admin)
{
    if (s_ctx.pwd_tip_timer != NULL)
    {
        lv_timer_del(s_ctx.pwd_tip_timer);
        s_ctx.pwd_tip_timer = NULL;
    }

    if (admin_family_id_get() != s_ctx.cur_family_id)
    {
        admin_family_id_set(s_ctx.cur_family_id); /* 保存变更后的房号 */
        user_data_save();
        user_network_apply_device_static_ip_now(); /* 重新应用静态 IP */
        rtsp_server_request_restart();             /* 重启 RTSP 服务器以绑定新 IP */
        device_info_attr local_info = {
            .building = user_data_get()->private_cfg.device.attr.building,
            .room = user_data_get()->private_cfg.device.attr.room,
            .device_type = user_data_get()->private_cfg.device.attr.device_type,
        };
        sat_call_update_local_info(&local_info); /* 更新会话框架本地设备信息 */
    }
    else
    {
        user_data_save();
    }

    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(admin);
