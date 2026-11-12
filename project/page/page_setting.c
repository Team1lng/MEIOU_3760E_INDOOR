#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"

/************************** 布局常量（2行 x 4列网格，与page_home.c一致） **************************/
#define BTN_W 180
#define BTN_H 190
#define START_X 87
#define START_Y 113
#define GAP_H 43
#define GAP_V 64
#define ROW1_Y START_Y
#define ROW2_Y (START_Y + BTN_H + GAP_V)
#define COL1_X START_X
#define COL2_X (START_X + BTN_W + GAP_H)
#define COL3_X (START_X + 2 * (BTN_W + GAP_H))
#define COL4_X (START_X + 3 * (BTN_W + GAP_H))

/*顶部状态栏刷新定时器*/
static lv_timer_t *s_setting_status_timer;

static void setting_status_refresh_cb(lv_timer_t *timer)
{
    page_common_status_bar_update();
}

/************************** 函数声明 **************************/
static void setting_door_obj_click(lv_event_t *ev);
static void setting_network_obj_click(lv_event_t *ev);
static void setting_camera_obj_click(lv_event_t *ev);
static void setting_scene_obj_click(lv_event_t *ev);
static void setting_system_obj_click(lv_event_t *ev);
static void setting_system_info_obj_click(lv_event_t *ev);
static void setting_advanced_obj_click(lv_event_t *ev);
static void setting_back_obj_click(lv_event_t *ev);

static void setting_door_create(void);
static void setting_network_create(void);
static void setting_camera_create(void);
static void setting_scene_create(void);
static void setting_system_create(void);
static void setting_system_info_create(void);
static void setting_advanced_create(void);
static void setting_back_create(void);

/************************** 回调实现 **************************/

static void setting_door_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(door), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_network_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(network), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_camera_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(cctv), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_scene_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(scene), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_system_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(system), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_system_info_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(system_info), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_advanced_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(advanced), LV_SCR_LOAD_ANIM_NONE, );
}

static void setting_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 按钮创建 **************************/

static void setting_system_create(void)
{
    lv_resource_init(icon, ICON_SETTING_SYSTEM_PNG_OFFSET, ICON_SETTING_SYSTEM_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SYSTEM_FOCUS_PNG_OFFSET, ICON_SETTING_SYSTEM_FOCUS_PNG_SIZE);
    lv_obj_t *setting_system = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_SYSTEM_CONT_ID, COL1_X, ROW1_Y, BTN_W, BTN_H,
                                                             setting_system_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                             0, 130, BTN_W, 60, SETTING_SYSTEM_TEXT_ID,
                                                             lang_str_get(STR_SYSTEM_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                             25, 0, 130, 130, SETTING_SYSTEM_BTN_ID,
                                                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_system, 21);
}

static void setting_door_create(void)
{
    lv_resource_init(icon, ICON_SETTING_DOOR_PNG_OFFSET, ICON_SETTING_DOOR_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_DOOR_FOCUS_PNG_OFFSET, ICON_SETTING_DOOR_FOCUS_PNG_SIZE);
    lv_obj_t *setting_door = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_DOOR_CONT_ID, COL2_X, ROW1_Y, BTN_W, BTN_H,
                                                           setting_door_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                           0, 130, BTN_W, 60, SETTING_DOOR_TEXT_ID,
                                                           lang_str_get(STR_DOOR_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                           25, 0, 130, 130, SETTING_DOOR_BTN_ID,
                                                           &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_door, 21);
}

static void setting_camera_create(void)
{
    lv_resource_init(icon, ICON_SETTING_CAMERA_PNG_OFFSET, ICON_SETTING_CAMERA_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_CAMERA_FOCUS_PNG_OFFSET, ICON_SETTING_CAMERA_FOCUS_PNG_SIZE);
    lv_obj_t *setting_camera = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_CAMERA_CONT_ID, COL3_X, ROW1_Y, BTN_W, BTN_H,
                                                             setting_camera_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                             0, 130, BTN_W, 60, SETTING_CAMERA_TEXT_ID,
                                                             lang_str_get(STR_CAMERA_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                             25, 0, 130, 130, SETTING_CAMERA_BTN_ID,
                                                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_camera, 21);
}

static void setting_network_create(void)
{
    lv_resource_init(icon, ICON_SETTING_NETWORK_PNG_OFFSET, ICON_SETTING_NETWORK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_NETWORK_FOCUS_PNG_OFFSET, ICON_SETTING_NETWORK_FOCUS_PNG_SIZE);
    lv_obj_t *setting_network = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_NETWORK_CONT_ID, COL4_X, ROW1_Y, BTN_W, BTN_H,
                                                              setting_network_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                              0, 130, BTN_W, 60, SETTING_NETWORK_TEXT_ID,
                                                              lang_str_get(STR_NETWORK_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                              25, 0, 130, 130, SETTING_NETWORK_BTN_ID,
                                                              &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_network, 21);
}

static void setting_scene_create(void)
{
    lv_resource_init(icon, ICON_SETTING_SCENE_PNG_OFFSET, ICON_SETTING_SCENE_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SCENE_FOCUS_PNG_OFFSET, ICON_SETTING_SCENE_FOCUS_PNG_SIZE);
    lv_obj_t *setting_scene = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_SCENE_CONT_ID, COL1_X, ROW2_Y, BTN_W, BTN_H,
                                                            setting_scene_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 130, BTN_W, 60, SETTING_SCENE_TEXT_ID,
                                                            lang_str_get(STR_SCENE_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                            25, 0, 130, 130, SETTING_SCENE_BTN_ID,
                                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_scene, 21);
}

static void setting_advanced_create(void)
{
    lv_resource_init(icon, ICON_SETTING_ADVANCED_PNG_OFFSET, ICON_SETTING_ADVANCED_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_ADVANCED_FOCUS_PNG_OFFSET, ICON_SETTING_ADVANCED_FOCUS_PNG_SIZE);
    lv_obj_t *setting_advanced = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_ADVANCED_CONT_ID, COL2_X, ROW2_Y, BTN_W, BTN_H,
                                                               setting_advanced_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                               0, 130, BTN_W, 60, SETTING_ADVANCED_TEXT_ID,
                                                               lang_str_get(STR_SENIOR_SET), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                               25, 0, 130, 130, SETTING_ADVANCED_BTN_ID,
                                                               &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_advanced, 21);
}

static void setting_system_info_create(void)
{
    lv_resource_init(icon, ICON_SETTING_SYSTEM_INFO_PNG_OFFSET, ICON_SETTING_SYSTEM_INFO_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_SETTING_SYSTEM_INFO_FOCUS_PNG_OFFSET, ICON_SETTING_SYSTEM_INFO_FOCUS_PNG_SIZE);
    lv_obj_t *setting_system_info = lv_common_img_text_btn_create(lv_scr_act_current(), SETTING_SYSTEM_INFO_CONT_ID, COL3_X, ROW2_Y, BTN_W, BTN_H,
                                                                  setting_system_info_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                  0, 130, BTN_W, 60, SETTING_SYSTEM_INFO_TEXT_ID,
                                                                  lang_str_get(STR_SYSTEM_INFO), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                                  25, 0, 130, 130, SETTING_SYSTEM_INFO_BTN_ID,
                                                                  &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_system_info, 21);
}

static void setting_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *setting_back = lv_common_img_btn_create(lv_scr_act_current(), SETTING_BACK_BTN_ID, 929, 505, 67, 67,
                                                      setting_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(setting_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(setting)
{
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    /*顶部状态栏：门口机/WiFi/涂鸦云在线状态*/
    page_common_status_bar_create(lv_scr_act_current());
    s_setting_status_timer = lv_timer_create(setting_status_refresh_cb, 1000, NULL);
    /*第一行：system 系统设置 / door 门口机设置 / camera 摄像头设置 / network 网络设置*/
    setting_system_create();
    setting_door_create();
    setting_camera_create();
    setting_network_create();
    /*第二行：scene 场景设置 / advanced 高级设置 / system info 系统信息*/
    setting_scene_create();
    setting_advanced_create();
    setting_system_info_create();
    /*back 返回*/
    setting_back_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(setting)
{
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);

    if (s_setting_status_timer != NULL)
    {
        lv_timer_del(s_setting_status_timer);
        s_setting_status_timer = NULL;
    }
}

Amy_lv_page_create(setting);
