#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "sat_user_common.h"
#include "fingerprint_common_net_proto.h"
#include "user_data.h"
#include "user_standby.h"
#include <string.h>

/************************** 布局常量 **************************/
#define DOOR_ICON_X 30
#define DOOR1_ICON_Y 151
#define DOOR2_ICON_Y 310
#define DOOR_ICON_W 130
#define DOOR_ICON_H 130
#define DOOR1_LABEL_Y 271
#define DOOR2_LABEL_Y 430
#define DOOR_SEL_COLOR 0xff0303
#define PANEL_TITLE_X 209
#define PANEL_TITLE_Y 29
#define HEADER_Y 91
#define LIST_X 199
#define LIST_Y 128
#define LIST_W 687
#define LIST_ITEM_H 52
#define LIST_H 416
#define COL_NO_X 17
#define COL_CARD_X 149
#define COL_LOCK_X 480
#define COL_GATE1_X 580
#define SEP_X 209
#define SEP_W 687
#define POPUP_W 304
#define POPUP_H 242

/************************** 指纹管理常量 **************************/
#define FINGER_MANAGE_MAX_ROWS 100
#define FINGER_ADD_IDLE_TIMEOUT_MS 20000

/************************** 锁图标资源 **************************/
static lv_resource_init(s_lock_enable, ICON_ADMIN_ENABLE_LOCK_PNG_OFFSET, ICON_ADMIN_ENABLE_LOCK_PNG_SIZE);
static lv_resource_init(s_lock_disable, ICON_ADMIN_DISABLE_LOCK_PNG_OFFSET, ICON_ADMIN_DISABLE_LOCK_PNG_SIZE);
static lv_resource_init(s_add_icon, ICON_ADMIN_ADD_PNG_OFFSET, ICON_ADMIN_ADD_PNG_SIZE);

/************************** 页面上下文 **************************/
static struct
{
	comm_finger_info_t row_data_list[FINGER_MANAGE_MAX_ROWS];
	uint16_t total_rows;

	int cur_door;
	int select_index;

	lv_anim_t add_success_anim;
	lv_timer_t *add_idle_timer;
} s_finger_ctx;

/************************** 函数声明 **************************/
static void fingerprint_event_cb(void *arg);
static int finger_door_ip_get(char *ip, size_t size);

static void finger_door1_click(lv_event_t *ev);
static void finger_door2_click(lv_event_t *ev);
static void finger_back_click(lv_event_t *ev);
static void finger_add_click(lv_event_t *ev);
static void finger_row_click(lv_event_t *ev);
static void finger_lock1_click(lv_event_t *ev);
static void finger_lock2_click(lv_event_t *ev);
static void finger_popup_delete_click(lv_event_t *ev);
static void finger_popup_delete_all_click(lv_event_t *ev);
static void finger_popup_close_click(lv_event_t *ev);

static void finger_door_select(int door);
static void finger_door_selector_create(void);
static void finger_panel_header_create(void);
static void finger_list_create(void);
static void finger_list_clear(void);
static void finger_list_refresh(void);
static void finger_row_create(int i, const comm_finger_info_t *item);
static void finger_popup_create(void);
static void finger_popup_destroy(void);
static void finger_back_create(void);
static void finger_add_btn_create(void);
static void finger_add_success_anim_cb(void *var, int32_t value);
static void finger_add_overlay_back_click(lv_event_t *ev);
static void finger_add_overlay_create(void);
static void finger_add_overlay_destroy(void);
static void finger_add_idle_timeout_cb(lv_timer_t *t);
static void finger_add_idle_timer_stop(void);
static void finger_manage_enter_send(void);
static void finger_manage_exit_send(void);

/************************** 辅助: 生成门口机 IP **************************/
static int finger_door_ip_get(char *ip, size_t size)
{
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	return generate_ip(info->building, 0,
										 (s_finger_ctx.cur_door - 1) + DEVICE_TYPE_DOOR_ID1, ip);
}

/************************** 辅助: 发送查询请求 **************************/
static void finger_query_send(void)
{
	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_query(&req, sizeof(req), ip, 80,
																								 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

/************************** 指纹事件回调 **************************/
static void fingerprint_event_cb(void *arg)
{
	comm_finger_op_res_t *res = (comm_finger_op_res_t *)arg;
	if (!res)
		return;

	switch (res->op_type)
	{
	case COMM_FINGER_PUSH_QUERY:
	{
		s_finger_ctx.total_rows = res->finger_count > FINGER_MANAGE_MAX_ROWS
																	? FINGER_MANAGE_MAX_ROWS
																	: res->finger_count;
		for (uint16_t i = 0; i < s_finger_ctx.total_rows; i++)
			s_finger_ctx.row_data_list[i] = res->fingers[i];
		finger_list_refresh();
		break;
	}
	case COMM_FINGER_PUSH_ADD:
	{
		if (s_finger_ctx.add_idle_timer)
			lv_timer_reset(s_finger_ctx.add_idle_timer);

		lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_ADD_OVERLAY_ID);
		if (overlay)
		{
			lv_obj_t *label = lv_obj_get_child_by_id(overlay, FINGER_MGR_ADD_SUCCESS_LABEL_ID);
			if (label)
			{
				lv_label_set_text(label, lang_str_get(res->res_code == COMM_FINGER_RES_SUCCESS ? STR_ADD_SUCCESS : STR_ADD_FAILED));
				lv_anim_start(&s_finger_ctx.add_success_anim);
			}
		}
		finger_query_send();
		break;
	}
	case COMM_FINGER_PUSH_DEL:
	{
		if (res->res_code == COMM_FINGER_RES_SUCCESS)
			finger_query_send();
		break;
	}
	case COMM_FINGER_PUSH_CLEAR:
	{
		if (res->res_code == COMM_FINGER_RES_SUCCESS)
		{
			s_finger_ctx.total_rows = 0;
			finger_list_refresh();
		}
		break;
	}
	case COMM_FINGER_PUSH_SET_PERM:
	case COMM_FINGER_PUSH_SCAN:
		break;
	default:
		break;
	}
}

/************************** 回调实现 **************************/

static void finger_door1_click(lv_event_t *ev)
{
	finger_door_select(1);
}

static void finger_door2_click(lv_event_t *ev)
{
	finger_door_select(2);
}

static void finger_back_click(lv_event_t *ev)
{
	Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
}

static void finger_add_click(lv_event_t *ev)
{
	finger_add_overlay_create();

	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_add(&req, sizeof(req), ip, 80,
																							 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void finger_row_click(lv_event_t *ev)
{
	lv_obj_t *row = lv_event_get_target(ev);
	int i = lv_obj_get_index(row);
	if (i < 0 || i >= s_finger_ctx.total_rows)
		return;
	s_finger_ctx.select_index = i;
	finger_popup_create();
}

static void finger_lock1_click(lv_event_t *ev)
{
	lv_obj_t *lock_btn = lv_event_get_target(ev);
	int i = lv_obj_get_index(lv_obj_get_parent(lock_btn));
	if (i < 0 || i >= s_finger_ctx.total_rows)
		return;

	comm_finger_info_t *item = &s_finger_ctx.row_data_list[i];
	bool was_lock = (bool)(item->perm & COMM_FINGER_PERM_LOCK);
	bool was_gate = (bool)(item->perm & COMM_FINGER_PERM_GATE);
	if (was_lock && !was_gate)
		return;

	item->perm ^= COMM_FINGER_PERM_LOCK;
	lv_obj_set_style_bg_img_src(lock_btn,
															(item->perm & COMM_FINGER_PERM_LOCK) ? (void *)&s_lock_enable : (void *)&s_lock_disable,
															LV_PART_MAIN);

	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.finger_count = 1;
	req.fingers[0].index = item->index;
	req.fingers[0].perm = item->perm;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_set_perm(&req, sizeof(req), ip, 80,
																										USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void finger_lock2_click(lv_event_t *ev)
{
	lv_obj_t *lock_btn = lv_event_get_target(ev);
	int i = lv_obj_get_index(lv_obj_get_parent(lock_btn));
	if (i < 0 || i >= s_finger_ctx.total_rows)
		return;

	comm_finger_info_t *item = &s_finger_ctx.row_data_list[i];
	bool was_lock = (bool)(item->perm & COMM_FINGER_PERM_LOCK);
	bool was_gate = (bool)(item->perm & COMM_FINGER_PERM_GATE);
	if (was_gate && !was_lock)
		return;

	item->perm ^= COMM_FINGER_PERM_GATE;
	lv_obj_set_style_bg_img_src(lock_btn,
															(item->perm & COMM_FINGER_PERM_GATE) ? (void *)&s_lock_enable : (void *)&s_lock_disable,
															LV_PART_MAIN);

	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.finger_count = 1;
	req.fingers[0].index = item->index;
	req.fingers[0].perm = item->perm;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_set_perm(&req, sizeof(req), ip, 80,
																										USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void finger_popup_delete_click(lv_event_t *ev)
{
	comm_finger_info_t *item = &s_finger_ctx.row_data_list[s_finger_ctx.select_index];

	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.finger_count = 1;
	req.fingers[0].index = item->index;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_del(&req, sizeof(req), ip, 80,
																							 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}

	finger_popup_destroy();
}

static void finger_popup_delete_all_click(lv_event_t *ev)
{
	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.finger_count = 1;
	req.fingers[0].index = 200;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_clear(&req, sizeof(req), ip, 80,
																								 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}

	finger_popup_destroy();
}

static void finger_popup_close_click(lv_event_t *ev)
{
	finger_popup_destroy();
}

/************************** 创建函数 **************************/

static void finger_door_select(int door)
{
	if (door == s_finger_ctx.cur_door)
		return;
	s_finger_ctx.cur_door = door;
	lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_DOOR1_LABEL_ID),
															lv_color_hex(door == 1 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_DOOR2_LABEL_ID),
															lv_color_hex(door == 2 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
	lv_label_set_text(lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_PANEL_TITLE_ID),
										lang_str_get(door == 1 ? STR_DOOR1_FINGER_MANAGE : STR_DOOR2_FINGER_MANAGE));

	/* 切换门口机时重新查询 */
	s_finger_ctx.total_rows = 0;
	finger_list_refresh();

	int door_idx = door - 1;
	if (!page_common_door_online_get(door_idx))
	{
		lv_common_msgbox_create(329, 172, 366, 256, 42,
														lang_str_get(STR_DEVICE_OFFLINE),
														NULL, NULL);
		return;
	}
	finger_query_send();
}

static void finger_door_selector_create(void)
{
	lv_resource_init(door_icon, ICON_SETTING_DOOR_PNG_OFFSET, ICON_SETTING_DOOR_PNG_SIZE);
	lv_resource_init(icon_focus, ICON_SETTING_DOOR_FOCUS_PNG_OFFSET, ICON_SETTING_DOOR_FOCUS_PNG_SIZE);

	lv_obj_t *door1 = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_DOOR1_BTN_ID,
																						 DOOR_ICON_X, DOOR1_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
																						 finger_door1_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(door1, 21);

	lv_common_text_create(lv_scr_act_current(), FINGER_MGR_DOOR1_LABEL_ID,
												0, DOOR1_LABEL_Y, SIDEBAR_W, 28,
												finger_door1_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR1), DOOR_SEL_COLOR, 0x02A9FF,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);

	lv_obj_t *door2 = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_DOOR2_BTN_ID,
																						 DOOR_ICON_X, DOOR2_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
																						 finger_door2_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(door2, 21);

	lv_common_text_create(lv_scr_act_current(), FINGER_MGR_DOOR2_LABEL_ID,
												0, DOOR2_LABEL_Y, SIDEBAR_W, 28,
												finger_door2_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR2), 0xffffff, 0x02A9FF,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void finger_panel_header_create(void)
{
	lv_common_text_create(lv_scr_act_current(), FINGER_MGR_PANEL_TITLE_ID,
												PANEL_TITLE_X, PANEL_TITLE_Y, 300, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR1_FINGER_MANAGE), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

	lv_common_text_create(lv_scr_act_current(), -1,
												LIST_X + COL_NO_X, HEADER_Y, 100, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_NO_NUM), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

	lv_common_text_create(lv_scr_act_current(), -1,
												LIST_X + COL_CARD_X, HEADER_Y, 250, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_FINGER_NUMBER), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

	lv_common_text_create(lv_scr_act_current(), -1,
												LIST_X + COL_LOCK_X, HEADER_Y, 80, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_LOCK), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);

	lv_common_text_create(lv_scr_act_current(), -1,
												LIST_X + COL_GATE1_X, HEADER_Y, 80, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_GATE1), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);

	lv_common_rect_create(lv_scr_act_current(), LIST_X, HEADER_Y + 28, LIST_W, 2, 0x00959E);
}

static void finger_row_create(int i, const comm_finger_info_t *item)
{
	lv_obj_t *list_cont = lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_LIST_CONT_ID);
	if (!list_cont)
		return;

	int y = i * LIST_ITEM_H;

	lv_obj_t *row = lv_common_setting_cont_create(list_cont, -1,
																								0, y, LIST_W, LIST_ITEM_H,
																								finger_row_click, LV_OPA_TRANSP, 0, LV_OPA_COVER, 0x800010,
																								0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x00959E,
																								0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x00959E);
	lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
	lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	char buf[64];

	snprintf(buf, sizeof(buf), "%d", i + 1);
	lv_common_text_create(row, -1,
												COL_NO_X, 0, 100, LIST_ITEM_H,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

	snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x",
					 item->data[0], item->data[1], item->data[2],
					 item->data[3], item->data[4]);
	lv_common_text_create(row, -1,
												COL_CARD_X, 0, 250, LIST_ITEM_H,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

	bool l1 = (bool)(item->perm & COMM_FINGER_PERM_LOCK);
	lv_common_img_btn_create(row, -1,
													 COL_LOCK_X + (80 - ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH) / 2,
													 (LIST_ITEM_H - ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT) / 2,
													 ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH, ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT,
													 finger_lock1_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 l1 ? (void *)&s_lock_enable : (void *)&s_lock_disable, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_TOP_LEFT, NULL);

	bool l2 = (bool)(item->perm & COMM_FINGER_PERM_GATE);
	lv_common_img_btn_create(row, -1,
													 COL_GATE1_X + (80 - ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH) / 2,
													 (LIST_ITEM_H - ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT) / 2,
													 ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH, ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT,
													 finger_lock2_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 l2 ? (void *)&s_lock_enable : (void *)&s_lock_disable, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_TOP_LEFT, NULL);
}

static void finger_list_create(void)
{
	lv_obj_t *cont = lv_common_setting_cont_create(lv_scr_act_current(), FINGER_MGR_LIST_CONT_ID,
																								 LIST_X, LIST_Y, LIST_W, LIST_H,
																								 NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
																								 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																								 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
	lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scroll_dir(cont, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

	finger_list_refresh();
}

static void finger_list_clear(void)
{
	lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_LIST_CONT_ID);
	if (!cont)
		return;
	lv_obj_t *child;
	while ((child = lv_obj_get_child(cont, 0)) != NULL)
		lv_obj_del(child);
}

static void finger_list_refresh(void)
{
	finger_list_clear();
	for (uint16_t i = 0; i < s_finger_ctx.total_rows; i++)
		finger_row_create(i, &s_finger_ctx.row_data_list[i]);
}

static void finger_popup_create(void)
{
	lv_obj_t *bg = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_POPUP_CONT_ID,
																					0, 0, SCREEN_W, SCREEN_H,
																					NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
																					0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																					0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																					NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_CLICKABLE);

	int popup_cx = (SCREEN_W - POPUP_W) / 2;
	int popup_cy = (SCREEN_H - POPUP_H) / 2;

	lv_obj_t *popup_cont = lv_common_setting_cont_create(
			lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_POPUP_CONT_ID), -1,
			popup_cx, popup_cy, POPUP_W, POPUP_H,
			NULL, LV_OPA_80, 0x00131D, LV_OPA_80, 0x00131D,
			20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
			20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);

	struct
	{
		int id;
		int x, y, w, h;
		lv_event_cb_t cb;
		const char *text;
	} btns[] = {
			{FINGER_MGR_POPUP_DELETE_BTN_ID, 43, 25, 217, 48, finger_popup_delete_click, lang_str_get(STR_DELETE)},
			{FINGER_MGR_POPUP_DELETE_ALL_BTN_ID, 43, 97, 217, 48, finger_popup_delete_all_click, lang_str_get(STR_ALL_DELETE)},
			{FINGER_MGR_POPUP_CLOSE_BTN_ID, 43, 169, 217, 48, finger_popup_close_click, lang_str_get(STR_CLOSE)},
	};

	for (int i = 0; i < 3; i++)
	{
		lv_common_img_text_btn_create(popup_cont, btns[i].id,
																	btns[i].x, btns[i].y, btns[i].w, btns[i].h,
																	btns[i].cb, LV_OPA_TRANSP, 0x00, LV_OPA_20, 0x02A9FF,
																	8, 1, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0x02A9FF,
																	8, 1, LV_BORDER_SIDE_FULL, LV_OPA_COVER, 0x02A9FF,
																	0, 0, btns[i].w, btns[i].h, 0,
																	btns[i].text, 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER_CENTER, (lv_font_t *)lv_font_normal,
																	0, 0, 0, 0, 0,
																	NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
	}
}

static void finger_popup_destroy(void)
{
	lv_obj_t *popup = lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_POPUP_CONT_ID);
	if (popup)
		lv_obj_del(popup);
}

/************************** 添加指纹 Overlay **************************/
static void finger_add_success_anim_cb(void *var, int32_t value)
{
	lv_obj_set_style_text_opa((lv_obj_t *)var, value, LV_PART_MAIN);
}

static void finger_add_overlay_back_click(lv_event_t *ev)
{
	finger_add_overlay_destroy();
}

static void finger_manage_enter_send(void)
{
	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_fingerprint_manage_enter(&req, sizeof(req), ip, 80,
																								 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void finger_manage_exit_send(void)
{
	comm_finger_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;

	char ip[32] = {0};
	if (finger_door_ip_get(ip, sizeof(ip)) == 0)
		sat_ipcamera_device_fingerprint_manage_exit(&req, sizeof(req), ip, 80,
																								USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 100, false);
}

static void finger_add_idle_timer_stop(void)
{
	if (s_finger_ctx.add_idle_timer)
	{
		lv_timer_del(s_finger_ctx.add_idle_timer);
		s_finger_ctx.add_idle_timer = NULL;
	}
}

static void finger_add_idle_timeout_cb(lv_timer_t *t)
{
	s_finger_ctx.add_idle_timer = NULL;
	lv_timer_del(t);
	finger_add_overlay_destroy();
}

static void finger_add_overlay_create(void)
{
	standby_timer_close();

	/* 进入指纹管理模式 */
	finger_manage_enter_send();

	finger_add_idle_timer_stop();
	s_finger_ctx.add_idle_timer = lv_timer_create(finger_add_idle_timeout_cb, FINGER_ADD_IDLE_TIMEOUT_MS, NULL);

	lv_obj_t *overlay = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_ADD_OVERLAY_ID,
																							 0, 0, SCREEN_W, SCREEN_H,
																							 NULL, false, LV_OPA_COVER, 0x00131D, LV_OPA_COVER, 0x00131D,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
	lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
	lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

	lv_resource_init(add_finger_icon, ICON_ADMIN_ADD_FINGERPRINT_PNG_OFFSET, ICON_ADMIN_ADD_FINGERPRINT_PNG_SIZE);
	lv_obj_t *icon = lv_img_create(overlay);
	lv_img_set_src(icon, &add_finger_icon);
	lv_obj_set_size(icon, ICON_ADMIN_ADD_FINGERPRINT_PNG_WIDTH, ICON_ADMIN_ADD_FINGERPRINT_PNG_HEIGHT);
	lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *success_label = lv_label_create(overlay);
	lv_obj_set_child_id(success_label, FINGER_MGR_ADD_SUCCESS_LABEL_ID);
	lv_label_set_text(success_label, lang_str_get(STR_ADD_SUCCESS));
	lv_obj_set_style_text_color(success_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
	lv_obj_set_style_text_font(success_label, lv_font_normal, LV_PART_MAIN);
	lv_obj_set_style_text_align(success_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_text_opa(success_label, LV_OPA_0, LV_PART_MAIN);
	lv_obj_align_to(success_label, icon, LV_ALIGN_OUT_TOP_MID, 0, -30);

	lv_anim_init(&s_finger_ctx.add_success_anim);
	lv_anim_set_var(&s_finger_ctx.add_success_anim, success_label);
	lv_anim_set_exec_cb(&s_finger_ctx.add_success_anim, finger_add_success_anim_cb);
	lv_anim_set_time(&s_finger_ctx.add_success_anim, 1000);
	lv_anim_set_values(&s_finger_ctx.add_success_anim, LV_OPA_0, LV_OPA_100);
	lv_anim_set_playback_time(&s_finger_ctx.add_success_anim, 1000);
	lv_anim_set_playback_delay(&s_finger_ctx.add_success_anim, 200);
	lv_anim_set_repeat_count(&s_finger_ctx.add_success_anim, 1);

	lv_obj_t *prompt = lv_label_create(overlay);
	lv_obj_set_style_text_color(prompt, lv_color_hex(0xBDBDBD), LV_PART_MAIN);
	lv_obj_set_style_text_font(prompt, lv_font_normal, LV_PART_MAIN);
	lv_label_set_text(prompt, lang_str_get(STR_PLEASE_ADD_FINGER));
	lv_obj_align(prompt, LV_ALIGN_BOTTOM_MID, 0, -70);

	lv_resource_init(back_icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
	lv_obj_t *back = lv_common_img_btn_create(overlay, -1,
																						929, 505, 67, 67,
																						finger_add_overlay_back_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						&back_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
	lv_obj_set_ext_click_area(back, 21);
}

static void finger_add_overlay_destroy(void)
{
	finger_add_idle_timer_stop();
	finger_manage_exit_send();

	lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), FINGER_MGR_ADD_OVERLAY_ID);
	if (overlay)
		lv_obj_del(overlay);
	standby_timer_restart(true, true);
}

static void finger_back_create(void)
{
	lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
	lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
	lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_BACK_BTN_ID,
																						929, 505, 67, 67,
																						finger_back_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						&icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(back, 21);
}

static void finger_add_btn_create(void)
{
	lv_obj_t *add_btn = lv_common_img_btn_create(lv_scr_act_current(), FINGER_MGR_ADD_BTN_ID,
																							 929, 402, ICON_ADMIN_ADD_PNG_WIDTH, ICON_ADMIN_ADD_PNG_HEIGHT,
																							 finger_add_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 (void *)&s_add_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_LEFT_MID, NULL);
	lv_obj_set_ext_click_area(add_btn, 21);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(fingerprint_manager)
{
	memset(&s_finger_ctx, 0, sizeof(s_finger_ctx));
	s_finger_ctx.cur_door = 1;

	lv_common_bg_create(lv_scr_act_current());

	finger_door_selector_create();
	finger_panel_header_create();
	finger_list_create();
	finger_back_create();
	finger_add_btn_create();

	Amy_event_cb_register(AMY_EVENT_ID_FINGERPRINT, fingerprint_event_cb);

	int door_idx = s_finger_ctx.cur_door - 1;
	if (!page_common_door_online_get(door_idx))
	{
		lv_common_msgbox_create(329, 172, 366, 256, 42,
														lang_str_get(STR_DEVICE_OFFLINE),
														NULL, NULL);
	}
	else
	{
		/* 列表浏览不进入管理模式，仅查询；管理模式由添加态发起 */
		finger_query_send();
	}

	Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(fingerprint_manager)
{
	/* 通知门口机退出指纹管理模式 */
	finger_manage_exit_send();

	finger_add_idle_timer_stop();

	Amy_event_cb_register(AMY_EVENT_ID_FINGERPRINT, NULL);
	Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(fingerprint_manager);
