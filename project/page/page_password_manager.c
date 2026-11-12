#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "sat_user_common.h"
#include "password_common_net_proto.h"
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
#define ADD_OVERLAY_W 300
#define ADD_OVERLAY_H 400

/************************** 密码管理常量 **************************/
#define PASSW_MANAGE_MAX_ROWS 100        /* 门口机密码上限 */
#define PASSW_MANAGE_MAX_PER_ROOM 10     /* 室内机(一户)密码上限 */

/************************** 锁图标资源 **************************/
static lv_resource_init(s_lock_enable, ICON_ADMIN_ENABLE_LOCK_PNG_OFFSET, ICON_ADMIN_ENABLE_LOCK_PNG_SIZE);
static lv_resource_init(s_lock_disable, ICON_ADMIN_DISABLE_LOCK_PNG_OFFSET, ICON_ADMIN_DISABLE_LOCK_PNG_SIZE);
static lv_resource_init(s_add_icon, ICON_ADMIN_ADD_PNG_OFFSET, ICON_ADMIN_ADD_PNG_SIZE);

/************************** 页面上下文 **************************/
static struct
{
	comm_password_info_t row_data_list[PASSW_MANAGE_MAX_ROWS];
	uint16_t total_rows;

	int cur_door;
	int select_index;

	char add_pwd_buf[8];
	int add_pwd_len;
} s_passw_ctx;

/************************** 函数声明 **************************/
static void password_event_cb(void *arg);
static int passw_door_ip_get(char *ip, size_t size);

static void passw_door1_click(lv_event_t *ev);
static void passw_door2_click(lv_event_t *ev);
static void passw_back_click(lv_event_t *ev);
static void passw_add_click(lv_event_t *ev);
static void passw_row_click(lv_event_t *ev);
static void passw_lock1_click(lv_event_t *ev);
static void passw_lock2_click(lv_event_t *ev);
static void passw_popup_delete_click(lv_event_t *ev);
static void passw_popup_delete_all_click(lv_event_t *ev);
static void passw_popup_close_click(lv_event_t *ev);
static void passw_add_overlay_create(void);
static void passw_add_overlay_destroy(void);
static void passw_add_kb_cb(lv_event_t *ev);

static void passw_door_select(int door);
static void passw_door_selector_create(void);
static void passw_panel_header_create(void);
static void passw_list_create(void);
static void passw_list_clear(void);
static void passw_list_refresh(void);
static void passw_row_create(int i, const comm_password_info_t *item);
static void passw_popup_create(void);
static void passw_popup_destroy(void);
static void passw_back_create(void);
static void passw_add_btn_create(void);

/************************** 辅助: 生成门口机 IP **************************/
static int passw_door_ip_get(char *ip, size_t size)
{
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	return generate_ip(info->building, 0,
										 (s_passw_ctx.cur_door - 1) + DEVICE_TYPE_DOOR_ID1, ip);
}

/************************** 辅助: 发送查询请求 **************************/
static void passw_query_send(void)
{
	comm_password_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;

	char ip[32] = {0};
	if (passw_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_password_manage_query(&req, sizeof(req), ip, 80,
																							USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

/************************** 密码事件回调 **************************/
static void password_event_cb(void *arg)
{
	comm_password_op_res_t *res = (comm_password_op_res_t *)arg;
	if (!res)
		return;

	switch (res->op_type)
	{
	case COMM_PASSWORD_PUSH_QUERY:
	{
		s_passw_ctx.total_rows = res->password_count > PASSW_MANAGE_MAX_ROWS
																 ? PASSW_MANAGE_MAX_ROWS
																 : res->password_count;
		for (uint16_t i = 0; i < s_passw_ctx.total_rows; i++)
			s_passw_ctx.row_data_list[i] = res->passwords[i];
		passw_list_refresh();
		break;
	}
	case COMM_PASSWORD_PUSH_ADD:
	{
		/* 添加成功后重新查询 */
		passw_query_send();
		break;
	}
	case COMM_PASSWORD_PUSH_DEL:
	{
		/* 删除成功后重新查询 */
		if (res->res_code == COMM_PASSWORD_RES_SUCCESS)
			passw_query_send();
		break;
	}
	case COMM_PASSWORD_PUSH_SET_PERM:
		/* 已在锁点击时立即更新，无需处理 */
		break;
	default:
		break;
	}
}

/************************** 回调实现 **************************/

static void passw_door1_click(lv_event_t *ev)
{
	passw_door_select(1);
}

static void passw_door2_click(lv_event_t *ev)
{
	passw_door_select(2);
}

static void passw_back_click(lv_event_t *ev)
{
	if (lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_ADD_OVERLAY_ID))
		passw_add_overlay_destroy();
	else
		Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
}

static void passw_add_click(lv_event_t *ev)
{
	if (s_passw_ctx.total_rows >= PASSW_MANAGE_MAX_PER_ROOM)
	{
		lv_common_msgbox_create(329, 172, 366, 256, 42,
														lang_str_get(STR_NUM_PASS_FULL),
														NULL, NULL);
		return;
	}
	passw_add_overlay_create();
}

static void passw_row_click(lv_event_t *ev)
{
	lv_obj_t *row = lv_event_get_target(ev);
	int i = lv_obj_get_index(row);
	if (i < 0 || i >= s_passw_ctx.total_rows)
		return;
	s_passw_ctx.select_index = i;
	passw_popup_create();
}

static void passw_lock1_click(lv_event_t *ev)
{
	lv_obj_t *lock_btn = lv_event_get_target(ev);
	int i = lv_obj_get_index(lv_obj_get_parent(lock_btn));
	if (i < 0 || i >= s_passw_ctx.total_rows)
		return;

	comm_password_info_t *item = &s_passw_ctx.row_data_list[i];
	bool was_gate = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_1);
	bool was_garage = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_2);
	/* 禁止关闭最后一个已启用的锁 */
	if (was_gate && !was_garage)
		return;

	item->perm ^= COMM_PASSWORD_PERM_LOCK_1;
	lv_obj_set_style_bg_img_src(lock_btn,
															(item->perm & COMM_PASSWORD_PERM_LOCK_1) ? (void *)&s_lock_enable : (void *)&s_lock_disable,
															LV_PART_MAIN);

	/* 发送权限修改请求 */
	comm_password_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.password_count = 1;
	req.passwords[0].index = item->index;
	req.passwords[0].perm = item->perm;

	char ip[32] = {0};
	if (passw_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_password_manage_set_perm(&req, sizeof(req), ip, 80,
																								 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void passw_lock2_click(lv_event_t *ev)
{
	lv_obj_t *lock_btn = lv_event_get_target(ev);
	int i = lv_obj_get_index(lv_obj_get_parent(lock_btn));
	if (i < 0 || i >= s_passw_ctx.total_rows)
		return;

	comm_password_info_t *item = &s_passw_ctx.row_data_list[i];
	bool was_gate = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_1);
	bool was_garage = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_2);
	/* 禁止关闭最后一个已启用的锁 */
	if (was_garage && !was_gate)
		return;

	item->perm ^= COMM_PASSWORD_PERM_LOCK_2;
	lv_obj_set_style_bg_img_src(lock_btn,
															(item->perm & COMM_PASSWORD_PERM_LOCK_2) ? (void *)&s_lock_enable : (void *)&s_lock_disable,
															LV_PART_MAIN);

	/* 发送权限修改请求 */
	comm_password_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.password_count = 1;
	req.passwords[0].index = item->index;
	req.passwords[0].perm = item->perm;

	char ip[32] = {0};
	if (passw_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_password_manage_set_perm(&req, sizeof(req), ip, 80,
																								 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}
}

static void passw_popup_delete_click(lv_event_t *ev)
{
	comm_password_info_t *item = &s_passw_ctx.row_data_list[s_passw_ctx.select_index];

	comm_password_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.password_count = 1;
	req.passwords[0].index = item->index;

	char ip[32] = {0};
	if (passw_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_password_manage_del(&req, sizeof(req), ip, 80,
																						USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}

	passw_popup_destroy();
}

static void passw_popup_delete_all_click(lv_event_t *ev)
{
	comm_password_op_res_t req;
	memset(&req, 0, sizeof(req));
	device_attr_info *info = &user_data_get()->private_cfg.device.attr;
	req.host_id.building = info->building;
	req.host_id.room = info->room;
	req.host_id.device_type = info->device_type;
	req.password_count = 1;
	req.passwords[0].index = 200; /* 200 = 清空全部 */

	char ip[32] = {0};
	if (passw_door_ip_get(ip, sizeof(ip)) == 0)
	{
		sat_ipcamera_device_password_manage_del(&req, sizeof(req), ip, 80,
																						USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
	}

	passw_popup_destroy();
}

static void passw_popup_close_click(lv_event_t *ev)
{
	passw_popup_destroy();
}

/************************** 创建函数 **************************/

static void passw_door_select(int door)
{
	if (door == s_passw_ctx.cur_door)
		return;
	s_passw_ctx.cur_door = door;
	lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_DOOR1_LABEL_ID),
															lv_color_hex(door == 1 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_color(lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_DOOR2_LABEL_ID),
															lv_color_hex(door == 2 ? DOOR_SEL_COLOR : 0xffffff), LV_PART_MAIN);
	lv_label_set_text(lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_PANEL_TITLE_ID),
										lang_str_get(door == 1 ? STR_DOOR1_PASSW_MANAGE : STR_DOOR2_PASSW_MANAGE));

	/* 切换门口机时重新查询 */
	s_passw_ctx.total_rows = 0;
	passw_list_refresh();

	int door_idx = door - 1;
	if (!page_common_door_online_get(door_idx))
	{
		lv_common_msgbox_create(329, 172, 366, 256, 42,
														lang_str_get(STR_DEVICE_OFFLINE),
														NULL, NULL);
		return;
	}
	passw_query_send();
}

static void passw_door_selector_create(void)
{
	lv_resource_init(door_icon, ICON_SETTING_DOOR_PNG_OFFSET, ICON_SETTING_DOOR_PNG_SIZE);
	lv_resource_init(icon_focus, ICON_SETTING_DOOR_FOCUS_PNG_OFFSET, ICON_SETTING_DOOR_FOCUS_PNG_SIZE);

	lv_obj_t *door1 = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_DOOR1_BTN_ID,
																						 DOOR_ICON_X, DOOR1_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
																						 passw_door1_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(door1, 21);

	lv_common_text_create(lv_scr_act_current(), PASSW_MGR_DOOR1_LABEL_ID,
												0, DOOR1_LABEL_Y, SIDEBAR_W, 28,
												passw_door1_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR1), DOOR_SEL_COLOR, 0x02A9FF,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);

	lv_obj_t *door2 = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_DOOR2_BTN_ID,
																						 DOOR_ICON_X, DOOR2_ICON_Y, DOOR_ICON_W, DOOR_ICON_H,
																						 passw_door2_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						 &door_icon, LV_OPA_TRANSP, 0xF91313, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(door2, 21);

	lv_common_text_create(lv_scr_act_current(), PASSW_MGR_DOOR2_LABEL_ID,
												0, DOOR2_LABEL_Y, SIDEBAR_W, 28,
												passw_door2_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR2), 0xffffff, 0x02A9FF,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void passw_panel_header_create(void)
{
	lv_common_text_create(lv_scr_act_current(), PASSW_MGR_PANEL_TITLE_ID,
												PANEL_TITLE_X, PANEL_TITLE_Y, 300, 28,
												NULL, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_DOOR1_PASSW_MANAGE), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

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
												lang_str_get(STR_PASSWORD_NUMBER), 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_LEFT, lv_font_normal);

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

static void passw_row_create(int i, const comm_password_info_t *item)
{
	lv_obj_t *list_cont = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_LIST_CONT_ID);
	if (!list_cont)
		return;

	int y = i * LIST_ITEM_H;

	lv_obj_t *row = lv_common_setting_cont_create(list_cont, -1,
																								0, y, LIST_W, LIST_ITEM_H,
																								passw_row_click, LV_OPA_TRANSP, 0, LV_OPA_COVER, 0x800010,
																								0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x00959E,
																								0, 2, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x00959E);
	lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
	lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	char buf[64];

	/* 序号 */
	snprintf(buf, sizeof(buf), "%d", i + 1);
	lv_common_text_create(row, -1,
												COL_NO_X, 0, 100, LIST_ITEM_H,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

	/* 密码数字 */
	int pos = 0;
	for (int d = 0; d < NET_PASSWORD_CODE_LEN && pos < (int)sizeof(buf) - 1; d++)
		pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", item->password[d]);
	buf[pos] = '\0';
	lv_common_text_create(row, -1,
												COL_CARD_X, 0, 250, LIST_ITEM_H,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												buf, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT_CENTER, lv_font_normal);

	/* 锁1图标 */
	bool l1 = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_1);
	lv_common_img_btn_create(row, -1,
													 COL_LOCK_X + (80 - ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH) / 2,
													 (LIST_ITEM_H - ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT) / 2,
													 ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH, ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT,
													 passw_lock1_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 l1 ? (void *)&s_lock_enable : (void *)&s_lock_disable, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_TOP_LEFT, NULL);

	/* 锁2图标 */
	bool l2 = (bool)(item->perm & COMM_PASSWORD_PERM_LOCK_2);
	lv_common_img_btn_create(row, -1,
													 COL_GATE1_X + (80 - ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH) / 2,
													 (LIST_ITEM_H - ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT) / 2,
													 ICON_ADMIN_ENABLE_LOCK_PNG_WIDTH, ICON_ADMIN_ENABLE_LOCK_PNG_HEIGHT,
													 passw_lock2_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
													 l2 ? (void *)&s_lock_enable : (void *)&s_lock_disable, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_TOP_LEFT, NULL);
}

static void passw_list_create(void)
{
	lv_obj_t *cont = lv_common_setting_cont_create(lv_scr_act_current(), PASSW_MGR_LIST_CONT_ID,
																								 LIST_X, LIST_Y, LIST_W, LIST_H,
																								 NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
																								 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																								 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
	lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scroll_dir(cont, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

	passw_list_refresh();
}

static void passw_list_clear(void)
{
	lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_LIST_CONT_ID);
	if (!cont)
		return;
	lv_obj_t *child;
	while ((child = lv_obj_get_child(cont, 0)) != NULL)
		lv_obj_del(child);
}

static void passw_list_refresh(void)
{
	passw_list_clear();
	for (uint16_t i = 0; i < s_passw_ctx.total_rows; i++)
		passw_row_create(i, &s_passw_ctx.row_data_list[i]);
}

static void passw_popup_create(void)
{
	lv_obj_t *bg = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_POPUP_CONT_ID,
																					0, 0, SCREEN_W, SCREEN_H,
																					NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
																					0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																					0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																					NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);

	lv_obj_add_flag(bg, LV_OBJ_FLAG_CLICKABLE);
	int popup_cx = (SCREEN_W - POPUP_W) / 2;
	int popup_cy = (SCREEN_H - POPUP_H) / 2;

	lv_obj_t *popup_cont = lv_common_setting_cont_create(
			lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_POPUP_CONT_ID), -1,
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
			{PASSW_MGR_POPUP_DELETE_BTN_ID, 43, 25, 217, 48, passw_popup_delete_click, lang_str_get(STR_DELETE)},
			{PASSW_MGR_POPUP_DELETE_ALL_BTN_ID, 43, 97, 217, 48, passw_popup_delete_all_click, lang_str_get(STR_ALL_DELETE)},
			{PASSW_MGR_POPUP_CLOSE_BTN_ID, 43, 169, 217, 48, passw_popup_close_click, lang_str_get(STR_CLOSE)},
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

static void passw_popup_destroy(void)
{
	lv_obj_t *popup = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_POPUP_CONT_ID);
	if (popup)
		lv_obj_del(popup);
}

/************************** 添加密码覆盖层 **************************/

static void passw_add_kb_cb(lv_event_t *ev)
{
	lv_event_code_t code = lv_event_get_code(ev);
	if (code != LV_EVENT_VALUE_CHANGED)
		return;

	lv_obj_t *kb = lv_event_get_target(ev);
	uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);

	if (btn_id == 9) /* 确认键 */
	{
		if (s_passw_ctx.add_pwd_len != NET_PASSWORD_CODE_LEN)
		{
			lv_common_msgbox_create(329, 172, 366, 256, 42,
															lang_str_get(STR_PASSW_SHOT),
															NULL, NULL);
			return;
		}

		/* 构造添加请求 */
		comm_password_op_res_t req;
		memset(&req, 0, sizeof(req));
		device_attr_info *info = &user_data_get()->private_cfg.device.attr;
		req.host_id.building = info->building;
		req.host_id.room = info->room;
		req.host_id.device_type = info->device_type;
		req.password_count = 1;

		/* 填充密码数据 */
		for (int i = 0; i < s_passw_ctx.add_pwd_len && i < NET_PASSWORD_CODE_LEN; i++)
			req.passwords[0].password[i] = (uint8_t)(s_passw_ctx.add_pwd_buf[i] - '0');
		/* 高位补0 */
		for (int i = s_passw_ctx.add_pwd_len; i < NET_PASSWORD_CODE_LEN; i++)
			req.passwords[0].password[i] = 0;
		req.passwords[0].perm = COMM_PASSWORD_PERM_ALL; /* 默认全部权限 */

		char ip[32] = {0};
		if (passw_door_ip_get(ip, sizeof(ip)) == 0)
		{
			sat_ipcamera_device_password_manage_add(&req, sizeof(req), ip, 80,
																							USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
		}

		passw_add_overlay_destroy();
	}
	else if (btn_id == 11) /* 删除键: 有输入删一位, 无输入关闭 */
	{
		if (s_passw_ctx.add_pwd_len > 0)
		{
			s_passw_ctx.add_pwd_len--;
			s_passw_ctx.add_pwd_buf[s_passw_ctx.add_pwd_len] = '\0';
			{
				lv_obj_t *ov = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_ADD_OVERLAY_ID);
				if (ov)
					lv_label_set_text(lv_obj_get_child_by_id(ov, PASSW_MGR_ADD_PWD_LABEL_ID), s_passw_ctx.add_pwd_buf);
			}
		}
		else
		{
			passw_add_overlay_destroy();
		}
	}
	else if (btn_id == 12) /* 取消键 */
	{
		return;
	}
	else
	{
		if (s_passw_ctx.add_pwd_len < NET_PASSWORD_CODE_LEN)
		{
			const char *txt = lv_btnmatrix_get_btn_text(kb, btn_id);
			if (txt && txt[0] >= '0' && txt[0] <= '9')
			{
				s_passw_ctx.add_pwd_buf[s_passw_ctx.add_pwd_len++] = txt[0];
				s_passw_ctx.add_pwd_buf[s_passw_ctx.add_pwd_len] = '\0';
				{
					lv_obj_t *ov = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_ADD_OVERLAY_ID);
					if (ov)
						lv_label_set_text(lv_obj_get_child_by_id(ov, PASSW_MGR_ADD_PWD_LABEL_ID), s_passw_ctx.add_pwd_buf);
				}
			}
		}
	}
}

static void passw_add_overlay_create(void)
{
	standby_timer_close();

	/* 全屏透明盾牌: 屏蔽弹层外所有控件的点击与滚动 */
	lv_obj_t *shield = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_ADD_SHIELD_ID,
																							0, 0, SCREEN_W, SCREEN_H,
																							NULL, false, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
																							0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							NULL, LV_OPA_TRANSP, 0, LV_ALIGN_CENTER, NULL);
	lv_obj_add_flag(shield, LV_OBJ_FLAG_CLICKABLE);

	int panel_cx = (SCREEN_W - ADD_OVERLAY_W) / 2;
	int panel_cy = (SCREEN_H - ADD_OVERLAY_H) / 2 - 20;

	lv_obj_t *overlay = lv_common_setting_cont_create(lv_scr_act_current(), PASSW_MGR_ADD_OVERLAY_ID,
																										panel_cx, panel_cy, ADD_OVERLAY_W, ADD_OVERLAY_H,
																										NULL, LV_OPA_COVER, 0x00131D, LV_OPA_COVER, 0x00131D,
																										0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																										0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);

	/* 提示文字 */
	lv_common_text_create(overlay, -1,
												0, 20, ADD_OVERLAY_W, 24,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												lang_str_get(STR_PLEASE_INPUT_PASSWORD), 0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal);

	/* 密码显示标签 */
	lv_common_text_create(overlay, PASSW_MGR_ADD_PWD_LABEL_ID,
												0, 55, ADD_OVERLAY_W, 30,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												"", 0x02A9FF, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_large);

	/* 下划线分隔 */
	lv_common_rect_create(overlay, 30, 82, ADD_OVERLAY_W - 60, 2, 0x02A9FF);

	/* 数字键盘 */
	lv_common_number_input_keyboard_create(overlay, -1,
																				 14, 100, ADD_OVERLAY_W - 28, ADD_OVERLAY_H - 130,
																				 passw_add_kb_cb,
																				 LV_OPA_COVER, 0x0F4068, LV_OPA_COVER, 0xFC0000,
																				 8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																				 8, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																				 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER_CENTER, lv_font_normal,
																				 6, 6);

	s_passw_ctx.add_pwd_len = 0;
	memset(s_passw_ctx.add_pwd_buf, 0, sizeof(s_passw_ctx.add_pwd_buf));
}

static void passw_add_overlay_destroy(void)
{
	lv_obj_t *shield = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_ADD_SHIELD_ID);
	if (shield)
		lv_obj_del(shield);

	lv_obj_t *overlay = lv_obj_get_child_by_id(lv_scr_act_current(), PASSW_MGR_ADD_OVERLAY_ID);
	if (overlay)
		lv_obj_del(overlay);
	standby_timer_restart(true, true);
}

static void passw_back_create(void)
{
	lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
	lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
	lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_BACK_BTN_ID,
																						929, 505, 67, 67,
																						passw_back_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																						&icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
	lv_obj_set_ext_click_area(back, 21);
}

static void passw_add_btn_create(void)
{
	lv_obj_t *add_btn = lv_common_img_btn_create(lv_scr_act_current(), PASSW_MGR_ADD_BTN_ID,
																							 929, 402, ICON_ADMIN_ADD_PNG_WIDTH, ICON_ADMIN_ADD_PNG_HEIGHT,
																							 passw_add_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
																							 (void *)&s_add_icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_LEFT_MID, NULL);
	lv_obj_set_ext_click_area(add_btn, 21);
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(password_manager)
{
	memset(&s_passw_ctx, 0, sizeof(s_passw_ctx));
	s_passw_ctx.cur_door = 1;

	lv_common_bg_create(lv_scr_act_current());

	passw_door_selector_create();
	passw_panel_header_create();
	passw_list_create();
	passw_back_create();
	passw_add_btn_create();

	/* 注册密码事件回调 */
	Amy_event_cb_register(AMY_EVENT_ID_PASSWORD, password_event_cb);

	/* 查询密码列表（是否需要先进入管理模式由 PASSWORD_ADMIN_MODE_ENABLE 决定） */
	int door_idx = s_passw_ctx.cur_door - 1;
	if (!page_common_door_online_get(door_idx))
	{
		lv_common_msgbox_create(329, 172, 366, 256, 42,
														lang_str_get(STR_DEVICE_OFFLINE),
														NULL, NULL);
	}
	else
	{
		char ip[32] = {0};
		if (passw_door_ip_get(ip, sizeof(ip)) == 0)
		{
#if PASSWORD_ADMIN_MODE_ENABLE
			comm_password_op_res_t enter_req;
			memset(&enter_req, 0, sizeof(enter_req));
			device_attr_info *info = &user_data_get()->private_cfg.device.attr;
			enter_req.host_id.building = info->building;
			enter_req.host_id.room = info->room;
			enter_req.host_id.device_type = info->device_type;

			sat_ipcamera_device_password_manage_enter(&enter_req, sizeof(enter_req), ip, 80,
																								USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 1000, false);
#endif
			passw_query_send();
		}
	}

	Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(password_manager)
{
#if PASSWORD_ADMIN_MODE_ENABLE
	/* 通知门口机退出密码管理模式 */
	{
		comm_password_op_res_t exit_req;
		memset(&exit_req, 0, sizeof(exit_req));
		device_attr_info *info = &user_data_get()->private_cfg.device.attr;
		exit_req.host_id.building = info->building;
		exit_req.host_id.room = info->room;
		exit_req.host_id.device_type = info->device_type;

		char ip[32] = {0};
		if (passw_door_ip_get(ip, sizeof(ip)) == 0)
			sat_ipcamera_device_password_manage_exit(&exit_req, sizeof(exit_req), ip, 80,
																							 USER_NETWORK_AUTH_USER, USER_NETWORK_AUTH_PASSWORD, 100, false);
	}
#endif

	/* 注销事件回调 */
	Amy_event_cb_register(AMY_EVENT_ID_PASSWORD, NULL);
	Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(password_manager);
