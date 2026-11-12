#include "page_common.h"
#include "lv_common.h"
#include "Amy_tfcard.h"
#include "language.h"
#include "sat_ipcamera.h"
#include "Amy_lvgl.h"
#include "user_data.h"
#include "page_tabel.h"
#include "tuya/tuya_sdk.h"
#include "sat_wifi.h"

typedef struct
{
    bool online;
    char version[32];
} door_online_status_t;

static door_online_status_t g_door_status[DOOR_DEVICE_MAX];

/* 最近一次上报涂鸦的子设备版本号（door1/door2 共用同一通道，只维护一份） */
static char g_reported_version[32] = {0};

/* 从门口机 GetVersion 原始串提取纯版本号。
   原始串形如 "ver=2026-08-20(15:52:10)ver:1.0.8"，取最后一个 "Ver:" 之后的纯版本号 */
static void door_version_extract(const char *raw, char *out, size_t out_len)
{
    const char *ver_start = raw;
    for (const char *p = raw; (p = strstr(p, "Ver:")) != NULL; p += 4)
        ver_start = p + 4;

    while (*ver_start == ' ' || *ver_start == '\t' ||
           *ver_start == '\r' || *ver_start == '\n')
        ver_start++;

    strncpy(out, ver_start, out_len - 1);
    out[out_len - 1] = '\0';

    size_t len = strlen(out);
    while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\t' ||
                       out[len - 1] == '\r' || out[len - 1] == '\n'))
        out[--len] = '\0';
}

/* 比较两个点分版本号 "x.y.z"：v1 更旧返回负值，相等返回 0，更新返回正值 */
static int door_version_cmp(const char *v1, const char *v2)
{
    if (v1[0] == '\0')
        return v2[0] == '\0' ? 0 : 1;
    if (v2[0] == '\0')
        return -1;

    while (*v1 && *v2)
    {
        unsigned int n1 = 0, n2 = 0;
        const char *s1 = v1, *s2 = v2;
        while (*v1 >= '0' && *v1 <= '9')
            n1 = n1 * 10 + (*v1++ - '0');
        while (*v2 >= '0' && *v2 <= '9')
            n2 = n2 * 10 + (*v2++ - '0');
        if (n1 != n2)
            return n1 < n2 ? -1 : 1;
        if (*v1 == '.')
            v1++;
        if (*v2 == '.')
            v2++;
        if (v1 == s1 && v2 == s2)
            break; /* 非数字非点号，避免死循环 */
    }

    if (*v1 == *v2)
        return 0;
    return *v1 == '\0' ? -1 : 1;
}

static void door_version_event_cb(void *arg)
{
    door_version_cb_attr *attr = (door_version_cb_attr *)arg;
    if (attr->index < 0 || attr->index >= DOOR_DEVICE_MAX)
        return;

    bool was_online = g_door_status[attr->index].online;
    g_door_status[attr->index].online = attr->result;

    if (!attr->result)
    {
        memset(g_door_status[attr->index].version, 0, sizeof(g_door_status[attr->index].version));
        return;
    }

    /* 存储纯版本号（也是系统信息页展示的值） */
    door_version_extract(attr->version, g_door_status[attr->index].version,
                         sizeof(g_door_status[attr->index].version));

    /* 已在线的门口机仅更新版本不上报，避免每 10s 轮询重复上报 */
    if (was_online)
        return;

    /* door1/door2 共用同一条涂鸦子设备通道，只上传在线门口机中最旧的版本号 */
    const char *oldest = NULL;
    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        if (!g_door_status[i].online || g_door_status[i].version[0] == '\0')
            continue;
        if (oldest == NULL || door_version_cmp(g_door_status[i].version, oldest) < 0)
            oldest = g_door_status[i].version;
    }

    /* 最旧版本未变化（后上线的门口机版本领先于已上报版本）时无需重复上传 */
    if (oldest != NULL && strcmp(oldest, g_reported_version) != 0)
    {
        strncpy(g_reported_version, oldest, sizeof(g_reported_version) - 1);
        g_reported_version[sizeof(g_reported_version) - 1] = '\0';
        tuya_subdev_version_report(0, oldest);
    }
}

static void door_status_check_timer_cb(lv_timer_t *timer)
{
    /* ipc_device[] 正被 CCTV 搜索/注册占用时，注入失败则跳过本轮，
       避免用搜索结果误当门口机配置去查询版本 */
    if (!sat_ipcamera_initialization_parameters(
            &user_data_get()->public_cfg.net_door_device[0], DOOR_DEVICE_MAX))
        return;

    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        if (user_data_get()->public_cfg.door_device[i].device_valid)
        {
            sat_ipcamera_device_version_get(i, 1000, false, NULL);
        }
        else
        {
            g_door_status[i].online = false;
            memset(g_door_status[i].version, 0, sizeof(g_door_status[i].version));
        }
    }
}

void page_common_door_status_init(void)
{
    Amy_event_cb_register(AMY_EVENT_ID_DOOR_VERSION, door_version_event_cb);
    lv_timer_create(door_status_check_timer_cb, 10000, NULL);
    door_status_check_timer_cb(NULL);
}

bool page_common_door_online_get(int index)
{
    if (index < 0 || index >= DOOR_DEVICE_MAX)
        return false;
    return g_door_status[index].online;
}

const char *page_common_door_version_get(int index)
{
    if (index < 0 || index >= DOOR_DEVICE_MAX)
        return "";
    return g_door_status[index].version;
}

/**
 * @brief SD卡插拔通用回调，显示插入/拔出消息框
 *
 * 处理逻辑同步自 EMOS-Indoor setting_sdcard_callback:
 * - 格式化状态守卫：SD_STATE_FORMAT 时不弹窗
 * - 插拔状态跟踪：仅 INSERT BIT 变化时弹窗，避免 MOUNT<->FULL 等冗余通知
 */
void page_common_tfcard_change_cb(void *arg)
{
    /* 拔卡即回退自定义铃声为标准铃声（铃声页的 ring_sd_change_cb 也会先做，此处兜底跨页面场景） */
    ring_mode_on_sd_changed();

    static int last_inserted = -1;
    int status = Amy_tfcard_status();

    if (status == SD_STATE_FORMAT)
        return;

    int inserted = (status & SD_STATE_INSERT) ? 1 : 0;
    if (last_inserted == inserted)
        return;

    last_inserted = inserted;
    AMY_LOG_DEBUG("创建弹窗\r\n");
    const char *msg = inserted ? lang_str_get(STR_INSET_SD_SUCCEE)
                               : lang_str_get(STR_NO_SD_CARD);
    lv_common_msgbox_create(329, 172, 366, 256, 42, msg, NULL, NULL);
}

/**
 * @brief SD卡格式化完成通用回调，显示格式化结果消息框
 */
void page_common_tfcard_format_finish_cb(void *arg)
{
    lv_common_msgbox_create(329, 172, 366, 256, 42,
                            lang_str_get(STR_FORMAT_SUCCE), NULL, NULL);
}

void page_default_device_conflict_event_cb(void *arg)
{
    lv_common_msgbox_create(329, 172, 366, 256, 42,
                            lang_str_get(STR_DEVICE_CONFLICT), NULL, NULL);
}

void page_default_network_conflict_event_cb(void *arg)
{
    ;
}

void page_default_ota_event_cb(void *arg)
{
    ota_event_t *ev = (ota_event_t *)arg;
    if (ev->state == OTA_STATE_DOWNLOAD_START)
    {
        Amy_lv_page_goto(lv_page(ota), LV_SCR_LOAD_ANIM_NONE, );
    }
}

void font_preload_all(void)
{
    lv_obj_t *tmp_label = lv_label_create(lv_scr_act());
    lv_obj_add_flag(tmp_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(tmp_label, 0, 0);
    lv_label_set_text(tmp_label, "A");
    lv_obj_set_style_text_font(tmp_label, lv_font_small, 0);
    lv_obj_set_style_text_font(tmp_label, lv_font_normal, 0);
    lv_obj_set_style_text_font(tmp_label, lv_font_large, 0);
    lv_obj_set_style_text_font(tmp_label, lv_font_large_s, 0);
    lv_obj_del(tmp_label);
}

/************************** 共享顶部状态栏（standby/home/setting 共用） **************************/

/* 布局坐标与 page_standby 原实现保持一致 */
#define STATUS_BAR_Y 36
#define STATUS_BAR_DOOR1_X 793
#define STATUS_BAR_DOOR2_X 847
#define STATUS_BAR_TUYA_CLOUD_X 898
#define STATUS_BAR_TUYA_CLOUD_Y 41
#define STATUS_BAR_WIFI_X 950
#define STATUS_BAR_WIFI_Y 39

/* WiFi 信号分档阈值（dBm），与 page_network.c / 原 standby 保持一致 */
#define STATUS_BAR_WIFI_SIG_FULL (-55)
#define STATUS_BAR_WIFI_SIG_TWO (-77)
#define STATUS_BAR_WIFI_SIG_ONE (-88)

static lv_obj_t *s_status_bar_parent;
static bool s_status_door_vis[2];
static bool s_status_tuya_vis;
static bool s_status_wifi_vis;

static void page_common_status_door_icon_update(int door_idx, int icon_id,
                                                unsigned long online_offs, unsigned long online_size,
                                                unsigned long offline_offs, unsigned long offline_size)
{
    lv_obj_t *icon = lv_obj_get_child_by_id(s_status_bar_parent, icon_id);
    if (icon == NULL)
        return;

    /* 门口机未使能时不显示图标 */
    s_status_door_vis[door_idx] = user_data_get()->public_cfg.door_device[door_idx].device_valid;
    if (!s_status_door_vis[door_idx])
    {
        lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);

    bool online = page_common_door_online_get(door_idx);
    lv_resource_init(img, online ? online_offs : offline_offs,
                     online ? online_size : offline_size);
    lv_obj_set_style_bg_img_src(icon, &img, LV_PART_MAIN);
}

static void page_common_status_wifi_icon_update(void)
{
    lv_obj_t *icon = lv_obj_get_child_by_id(s_status_bar_parent, STATUS_BAR_WIFI_ICON_ID);
    if (icon == NULL)
        return;

    bool connected = false;
    int rssi = 0;
    wifi_device_get_rssi(wifi_context_get(), &rssi, &connected);
    s_status_wifi_vis = connected;
    if (!connected)
    {
        lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    unsigned long offset, size;
    if (rssi > STATUS_BAR_WIFI_SIG_FULL)
    {
        offset = ICON_STANDBY_WIFI_4_PNG_OFFSET;
        size = ICON_STANDBY_WIFI_4_PNG_SIZE;
    }
    else if (rssi > STATUS_BAR_WIFI_SIG_TWO)
    {
        offset = ICON_STANDBY_WIFI_3_PNG_OFFSET;
        size = ICON_STANDBY_WIFI_3_PNG_SIZE;
    }
    else if (rssi > STATUS_BAR_WIFI_SIG_ONE)
    {
        offset = ICON_STANDBY_WIFI_2_PNG_OFFSET;
        size = ICON_STANDBY_WIFI_2_PNG_SIZE;
    }
    else
    {
        offset = ICON_STANDBY_WIFI_1_PNG_OFFSET;
        size = ICON_STANDBY_WIFI_1_PNG_SIZE;
    }

    lv_resource_init(img, offset, size);
    lv_obj_set_style_bg_img_src(icon, &img, LV_PART_MAIN);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
}

static void page_common_status_tuya_icon_update(void)
{
    lv_obj_t *icon = lv_obj_get_child_by_id(s_status_bar_parent, STATUS_BAR_TUYA_CLOUD_ICON_ID);
    if (icon == NULL)
        return;

    s_status_tuya_vis = tuya_mqtt_is_online();
    if (s_status_tuya_vis)
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
}

/* 顶部状态图标右对齐排布：任一图标隐藏时，其左侧图标自动右移填充空位 */
static void page_common_status_bar_layout_update(void)
{
    lv_obj_t *parent = s_status_bar_parent;
    if (parent == NULL)
        return;

    static const int icon_id[] = {
        STATUS_BAR_WIFI_ICON_ID,
        STATUS_BAR_TUYA_CLOUD_ICON_ID,
        STATUS_BAR_DOOR2_ICON_ID,
        STATUS_BAR_DOOR1_ICON_ID,
    };
    static const int width[] = {
        ICON_STANDBY_WIFI_4_PNG_WIDTH,
        ICON_STANDBY_TUYA_CLOUD_PNG_WIDTH,
        ICON_STANDBY_DOOR2_ONLINE_PNG_WIDTH,
        ICON_STANDBY_DOOR1_ONLINE_PNG_WIDTH,
    };
    static const int pos_y[] = {
        STATUS_BAR_WIFI_Y,
        STATUS_BAR_TUYA_CLOUD_Y,
        STATUS_BAR_Y,
        STATUS_BAR_Y,
    };
    /* 各图标与其左侧相邻图标的间距（从右到左：wifi-tuya/tuya-door2/door2-door1） */
    static const int left_gap[] = {
        STATUS_BAR_WIFI_X - (STATUS_BAR_TUYA_CLOUD_X + ICON_STANDBY_TUYA_CLOUD_PNG_WIDTH),
        STATUS_BAR_TUYA_CLOUD_X - (STATUS_BAR_DOOR2_X + ICON_STANDBY_DOOR2_ONLINE_PNG_WIDTH),
        STATUS_BAR_DOOR2_X - (STATUS_BAR_DOOR1_X + ICON_STANDBY_DOOR1_ONLINE_PNG_WIDTH),
        0,
    };
    const bool visible[] = {s_status_wifi_vis, s_status_tuya_vis, s_status_door_vis[1], s_status_door_vis[0]};

    /* 右对齐锚点取全部图标可见时的右边缘，保证默认布局不变 */
    int x = STATUS_BAR_WIFI_X + ICON_STANDBY_WIFI_4_PNG_WIDTH;
    for (int i = 0; i < 4; i++)
    {
        if (!visible[i])
            continue;
        lv_obj_t *icon = lv_obj_get_child_by_id(parent, icon_id[i]);
        if (icon == NULL)
            continue;
        x -= width[i];
        lv_obj_set_pos(icon, x, pos_y[i]);
        x -= left_gap[i];
    }
}

void page_common_status_bar_create(lv_obj_t *parent)
{
    s_status_bar_parent = parent;
    s_status_door_vis[0] = false;
    s_status_door_vis[1] = false;
    s_status_tuya_vis = false;
    s_status_wifi_vis = false;

    lv_resource_init(icon_door1, ICON_STANDBY_DOOR1_ONLINE_PNG_OFFSET, ICON_STANDBY_DOOR1_ONLINE_PNG_SIZE);
    lv_common_img_btn_create(parent, STATUS_BAR_DOOR1_ICON_ID,
                             STATUS_BAR_DOOR1_X, STATUS_BAR_Y,
                             ICON_STANDBY_DOOR1_ONLINE_PNG_WIDTH, ICON_STANDBY_DOOR1_ONLINE_PNG_HEIGHT,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_door1, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

    lv_resource_init(icon_door2, ICON_STANDBY_DOOR2_ONLINE_PNG_OFFSET, ICON_STANDBY_DOOR2_ONLINE_PNG_SIZE);
    lv_common_img_btn_create(parent, STATUS_BAR_DOOR2_ICON_ID,
                             STATUS_BAR_DOOR2_X, STATUS_BAR_Y,
                             ICON_STANDBY_DOOR2_ONLINE_PNG_WIDTH, ICON_STANDBY_DOOR2_ONLINE_PNG_HEIGHT,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_door2, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

    lv_resource_init(icon_tuya, ICON_STANDBY_TUYA_CLOUD_PNG_OFFSET, ICON_STANDBY_TUYA_CLOUD_PNG_SIZE);
    lv_common_img_btn_create(parent, STATUS_BAR_TUYA_CLOUD_ICON_ID,
                             STATUS_BAR_TUYA_CLOUD_X, STATUS_BAR_TUYA_CLOUD_Y,
                             ICON_STANDBY_TUYA_CLOUD_PNG_WIDTH, ICON_STANDBY_TUYA_CLOUD_PNG_HEIGHT,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_tuya, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

    lv_resource_init(icon_wifi, ICON_STANDBY_WIFI_4_PNG_OFFSET, ICON_STANDBY_WIFI_4_PNG_SIZE);
    lv_common_img_btn_create(parent, STATUS_BAR_WIFI_ICON_ID,
                             STATUS_BAR_WIFI_X, STATUS_BAR_WIFI_Y,
                             ICON_STANDBY_WIFI_4_PNG_WIDTH, ICON_STANDBY_WIFI_4_PNG_HEIGHT,
                             NULL, false,
                             LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x00,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon_wifi, LV_OPA_TRANSP, 0x00, LV_ALIGN_CENTER, NULL);

    page_common_status_bar_update();
}

void page_common_status_bar_update(void)
{
    if (s_status_bar_parent == NULL)
        return;

    page_common_status_door_icon_update(0, STATUS_BAR_DOOR1_ICON_ID,
                                        ICON_STANDBY_DOOR1_ONLINE_PNG_OFFSET, ICON_STANDBY_DOOR1_ONLINE_PNG_SIZE,
                                        ICON_STANDBY_DOOR1_OFFLINE_PNG_OFFSET, ICON_STANDBY_DOOR1_OFFLINE_PNG_SIZE);
    page_common_status_door_icon_update(1, STATUS_BAR_DOOR2_ICON_ID,
                                        ICON_STANDBY_DOOR2_ONLINE_PNG_OFFSET, ICON_STANDBY_DOOR2_ONLINE_PNG_SIZE,
                                        ICON_STANDBY_DOOR2_OFFLINE_PNG_OFFSET, ICON_STANDBY_DOOR2_OFFLINE_PNG_SIZE);
    page_common_status_wifi_icon_update();
    page_common_status_tuya_icon_update();
    page_common_status_bar_layout_update();
}

#include "user_standby.h"
static void standby_check_process_timer_cb(lv_timer_t *t)
{
    standby_timeout_check_and_process();
}
static void standby_goto_page(void *user_data)
{

    Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
}

void page_common_standby_check_init(void)
{
    standby_timer_init(standby_goto_page, NULL, 60);
    standby_timer_restart(true, false);
    lv_timer_ready(lv_timer_create(standby_check_process_timer_cb, 1000, NULL));
}

void page_default_obj_press_event_cb(lv_event_t *event)
{
    standby_timer_restart(true, true);
    if ((user_data_get()->public_cfg.global_func.scene_mode != SCENE_MODE_SLEEP) && user_data_get()->private_cfg.keytone_enable)
        Amy_audio_play(0, 80, false, NULL, NULL);
}
