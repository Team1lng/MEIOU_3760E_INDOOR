#include "user_data.h"
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "stdlib.h"
#include "sat_ipcamera.h"
#include "user_network.h"
#include "../business/device/device_business.h"
#define USER_DATA_PATH "/home/data/user_data.cfg"
#define MIN_VOLUME 1
#define MAX_VOLUME 6
#define MIN_LOCK_CLOSE_TIME 0
#define MAX_LOCK_CLOSE_TIME 120
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 100
#include "device_types.h"
static user_data_info user_data;

static user_data_info user_data_default =
    {
        // ===================== 公共配置 =====================
        .public_cfg =
            {
                .time = {
                    .tm_year = 2025,
                    .tm_mon = 12,
                    .tm_mday = 31,
                    .tm_hour = 12,
                    .tm_min = 12,
                    .tm_sec = 30,
                },
                .auto_sync_network_time = true,
                .pairing_mode = PAIRING_MODE_WLAN,
                .allocation_mode = IP_ALLOC_STATIC,

                .global_func = {
                    .reserved = false,
                    .scene_mode = SCENE_MODE_HOME,
                },

                .atmosphere = false,

                .door_device = {
                    [0] = {.door_cfg = {.doorbell_ring_volume = 3, .global_record_mode = RECORD_MODE_PHOTO, .gate_lock_close_time = 3, .garage_lock_close_time = 3, .access_lock_select = LOCK_SELECT_GATE, .access_card_count = 0, .human_detect = {
                                                                                                                                                                                                                                        .detect_level = DETECT_LEVEL_OFF,
                                                                                                                                                                                                                                        .alarm_interval = 10,
                                                                                                                                                                                                                                        .record_mode = RECORD_MODE_PHOTO,
                                                                                                                                                                                                                                    },
                                        .ringtone[0] = {.start_h = 6, .start_m = 0, .end_h = 12, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .ringtone[1] = {.start_h = 12, .start_m = 0, .end_h = 18, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .ringtone[2] = {.start_h = 18, .start_m = 0, .end_h = 6, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .indoor_ringback_volume = 3,
                                        .message_sw = false,
                                        .message_time = 60,
                                        .record_time = 60,
                                        .indoor_talk_volume = 5,
                                        .outdoor_talk_volume = 5,
                                        .display_brightness = 50},
                           .device_valid = true},
                    [1] = {.door_cfg = {.doorbell_ring_volume = 3, .global_record_mode = RECORD_MODE_PHOTO, .gate_lock_close_time = 3, .garage_lock_close_time = 3, .access_lock_select = LOCK_SELECT_GATE, .access_card_count = 0, .human_detect = {
                                                                                                                                                                                                                                        .detect_level = DETECT_LEVEL_OFF,
                                                                                                                                                                                                                                        .alarm_interval = 10,
                                                                                                                                                                                                                                        .record_mode = RECORD_MODE_PHOTO,
                                                                                                                                                                                                                                    },
                                        .ringtone[0] = {.start_h = 6, .start_m = 0, .end_h = 12, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .ringtone[1] = {.start_h = 12, .start_m = 0, .end_h = 18, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .ringtone[2] = {.start_h = 18, .start_m = 0, .end_h = 6, .end_m = 0, .type = RINGTONE_TYPE_1, .duration = RING_DURATION_30S, .volume = 3, .mode = 0, .select = 1},
                                        .indoor_ringback_volume = 3,
                                        .message_sw = false,
                                        .message_time = 60,
                                        .record_time = 60,
                                        .indoor_talk_volume = 5,
                                        .outdoor_talk_volume = 5,
                                        .display_brightness = 50},
                           .device_valid = true},
                },

                .net_door_device = {
                    [0] = {
                        .ipaddr = "10.1.0.9",
                        .port = 80,
                        .username = USER_NETWORK_AUTH_USER,
                        .password = USER_NETWORK_AUTH_PASSWORD,
                        .auther_flag = 0,
                        .door_name = "Door 1",
                        .rtsp = {
                            [0] = {.rtsp_url = "rtsp://10.1.0.9:8554/video", .rtsp_url_type = RTSP_URL_TYPE_BASE},
                        },
                        .profile_token_num = 1,
                    },
                    [1] = {
                        .ipaddr = "10.1.0.10",
                        .port = 80,
                        .username = USER_NETWORK_AUTH_USER,
                        .password = USER_NETWORK_AUTH_PASSWORD,
                        .auther_flag = 0,
                        .door_name = "Door 2",
                        .rtsp = {
                            [0] = {.rtsp_url = "rtsp://10.1.0.10:8554/video", .rtsp_url_type = RTSP_URL_TYPE_BASE},
                        },
                        .profile_token_num = 1,
                    },
                },
                // .net_cctv_device[0] = {0},
                // .net_cctv_device[1] = {0},

                .cctv_display_brightness = {50, 50},
            },

        // ===================== 私有配置 =====================
        .private_cfg = {
            .admin = {.password = ADMIN_DEFAULT_PASSWORD},
            .device = {.auth = {.number = "", .password = ""}, .attr = {.device_type = 1, .building = 1, .room = 101}},
            // .tuya_info = {0},
            .lang = LANG_ENGLISH,
            .display_brightness = 50,
            .screen_saver_en = SCREEN_SAVER_DISABLE,
            .mute_enable = false,
            .date_format = 0,
            .keytone_enable = true,
            .detection_preview_enable = false,
            .unlock_delay = 3,
            .unlock_voice_enable = true,

            .scene = {
                .digital_photo_frame_sw = false,
                .digital_photo_sw_time = 5,
                .bg_music_sw = false,
                .bg_music_vol = 50,
            },
        }};

#define USER_DATA_CHECK_RANGE(field, default_field, min, max)                               \
    do                                                                                      \
    {                                                                                       \
        if ((field) < (min) || (field) > (max))                                             \
        {                                                                                   \
            printf("[USER_DATA] Invalid %s: %d, reset to default\n", #field, (int)(field)); \
            (field) = (default_field);                                                      \
        }                                                                                   \
    } while (0)

#define USER_DATA_CHECK_ENUM(field, default_field, max)                                          \
    do                                                                                           \
    {                                                                                            \
        if ((field) >= (max))                                                                    \
        {                                                                                        \
            printf("[USER_DATA] Invalid enum %s: %d, reset to default\n", #field, (int)(field)); \
            (field) = (default_field);                                                           \
        }                                                                                        \
    } while (0)

#define USER_DATA_CHECK_STR_LEN(field, max_len) \
    do                                          \
    {                                           \
        if (strlen(field) >= (max_len))         \
        {                                       \
            field[max_len - 1] = 0;             \
        }                                       \
    } while (0)

static void user_data_default_door_devices_sync(void)
{
    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        if (user_data.public_cfg.net_door_device[i].rtsp[0].rtsp_url[0] != '\0')
            continue;

        memcpy(&user_data.public_cfg.net_door_device[i],
               &user_data_default.public_cfg.net_door_device[i],
               sizeof(struct ipcamera_info));
    }
}

bool admin_password_validate(const char *pwd)
{
    return strlen(pwd) == ADMIN_PASSWORD_LEN;
}

static void user_data_check_valid(void)
{
    // ===================== 公共配置校验 =====================
    USER_DATA_CHECK_ENUM(user_data.private_cfg.lang, user_data_default.private_cfg.lang, LANG_MAX);
    USER_DATA_CHECK_ENUM(user_data.public_cfg.pairing_mode, PAIRING_MODE_WLAN, PAIRING_MODE_TOTAL);
#if !NET_SUPPORT_CABLE
    /* 硬件已移除有线网卡：存量配置中的 Cable 模式回落到 WLAN */
    if (user_data.public_cfg.pairing_mode == PAIRING_MODE_WIRED)
        user_data.public_cfg.pairing_mode = PAIRING_MODE_WLAN;
#endif
    USER_DATA_CHECK_ENUM(user_data.public_cfg.allocation_mode, IP_ALLOC_STATIC, IP_ALLOC_TOTAL);
    USER_DATA_CHECK_RANGE(user_data.public_cfg.auto_sync_network_time, false, 0, 1);

    USER_DATA_CHECK_ENUM(user_data.public_cfg.global_func.scene_mode, user_data_default.public_cfg.global_func.scene_mode, SCENE_MODE_MAX);

    user_data_default_door_devices_sync();

    // 门口机配置校验
    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        device_single_cfg *dev = &user_data.public_cfg.door_device[i];
        const device_single_cfg *def = &user_data_default.public_cfg.door_device[i];

        USER_DATA_CHECK_RANGE(dev->door_cfg.doorbell_ring_volume, def->door_cfg.doorbell_ring_volume, 1, 6);
        USER_DATA_CHECK_RANGE(dev->door_cfg.indoor_ringback_volume, def->door_cfg.indoor_ringback_volume, 1, 6);
        USER_DATA_CHECK_RANGE(dev->door_cfg.message_sw, def->door_cfg.message_sw, 0, 1);
        USER_DATA_CHECK_RANGE(dev->door_cfg.message_time, def->door_cfg.message_time, 30, 120);
        USER_DATA_CHECK_RANGE(dev->door_cfg.record_time, def->door_cfg.record_time, 30, 120);
        USER_DATA_CHECK_RANGE(dev->door_cfg.indoor_talk_volume, def->door_cfg.indoor_talk_volume, 1, 10);
        USER_DATA_CHECK_RANGE(dev->door_cfg.outdoor_talk_volume, def->door_cfg.outdoor_talk_volume, 1, 10);
        USER_DATA_CHECK_RANGE(dev->door_cfg.display_brightness, def->door_cfg.display_brightness, 0, 100);
        USER_DATA_CHECK_RANGE(dev->door_cfg.gate_lock_close_time, def->door_cfg.gate_lock_close_time, 1, 10);
        USER_DATA_CHECK_RANGE(dev->door_cfg.garage_lock_close_time, def->door_cfg.garage_lock_close_time, 1, 10);
        USER_DATA_CHECK_ENUM(dev->door_cfg.global_record_mode, def->door_cfg.global_record_mode, RECORD_MODE_MAX);

        for (int j = 0; j < 3; j++)
        {
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].start_h, 0, 0, 23);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].start_m, 0, 0, 59);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].end_h, 0, 0, 23);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].end_m, 0, 0, 59);
            USER_DATA_CHECK_ENUM(dev->door_cfg.ringtone[j].duration, RING_DURATION_30S, RING_DURATION_MAX);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].volume, 3, 0, 6);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].mode, 0, 0, 1);
            USER_DATA_CHECK_RANGE(dev->door_cfg.ringtone[j].select, 1, 1, 100);
        }

        USER_DATA_CHECK_ENUM(dev->door_cfg.access_lock_select, def->door_cfg.access_lock_select, LOCK_SELECT_MAX);
        USER_DATA_CHECK_RANGE(dev->door_cfg.access_card_count, 0, 0, 64);

        for (int k = 0; k < 64; k++)
        {
            USER_DATA_CHECK_RANGE(dev->door_cfg.access_cards[k].valid, false, 0, 1);
            USER_DATA_CHECK_STR_LEN(dev->door_cfg.access_cards[k].card_id, 16);
        }

        USER_DATA_CHECK_ENUM(dev->door_cfg.human_detect.detect_level, def->door_cfg.human_detect.detect_level, DETECT_LEVEL_MAX);
        USER_DATA_CHECK_RANGE(dev->door_cfg.human_detect.alarm_interval, def->door_cfg.human_detect.alarm_interval, 10, 300);
        USER_DATA_CHECK_ENUM(dev->door_cfg.human_detect.record_mode, def->door_cfg.human_detect.record_mode, RECORD_MODE_MAX);
    }

    // CCTV 通道亮度校验
    for (int i = 0; i < DOOR_DEVICE_MAX; i++)
    {
        USER_DATA_CHECK_RANGE(user_data.public_cfg.cctv_display_brightness[i],
                              user_data_default.public_cfg.cctv_display_brightness[i], 0, 100);
    }

    // ===================== 私有配置校验 =====================
    if (!admin_password_validate(user_data.private_cfg.admin.password))
    {
        strncpy(user_data.private_cfg.admin.password, ADMIN_DEFAULT_PASSWORD,
                sizeof(user_data.private_cfg.admin.password) - 1);
    }
    USER_DATA_CHECK_RANGE(user_data.private_cfg.display_brightness, 50, 0, 100);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.mute_enable, false, 0, 1);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.date_format, 0, 0, 2);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.keytone_enable, true, 0, 1);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.detection_preview_enable, false, 0, 1);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.unlock_delay, 3, 1, 10);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.unlock_voice_enable, true, 0, 1);

    USER_DATA_CHECK_ENUM(user_data.private_cfg.screen_saver_en, SCREEN_SAVER_DISABLE, SCREEN_SAVER_MAX);

    USER_DATA_CHECK_RANGE(user_data.private_cfg.device.attr.device_type, 0, 0, 15);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.device.attr.building, 1, 1, 1); // 0, 0, 254
    USER_DATA_CHECK_RANGE(user_data.private_cfg.device.attr.room, 0, 0, 2559);
    USER_DATA_CHECK_STR_LEN(user_data.private_cfg.device.auth.number, 128);
    USER_DATA_CHECK_STR_LEN(user_data.private_cfg.device.auth.password, 10);

    // scene 参数校验
    USER_DATA_CHECK_RANGE(user_data.private_cfg.scene.digital_photo_frame_sw, false, 0, 1);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.scene.digital_photo_sw_time, 5, 1, 30);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.scene.bg_music_sw, false, 0, 1);
    USER_DATA_CHECK_RANGE(user_data.private_cfg.scene.bg_music_vol, 50, 1, 100);
}

bool user_data_init(void)
{

    int fd = open(USER_DATA_PATH, O_RDONLY);

    if (fd < 0)
    {
        memcpy(&user_data, &user_data_default, sizeof(user_data_info));
        return false;
    }

    ssize_t rlen = read(fd, &user_data, sizeof(user_data_info));
    if (rlen != sizeof(user_data_info))
    {
        memcpy(&user_data, &user_data_default, sizeof(user_data_info));
        close(fd);
        return false;
    }
    close(fd);
    user_data_check_valid();
    return true;
}

bool user_data_save(void)
{
    int fd = open(USER_DATA_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return false;

    ssize_t wlen = write(fd, &user_data, sizeof(user_data_info));
    bool ok = (wlen == sizeof(user_data_info));
    close(fd);
    system("sync");

    if (ok)
        device_notify_config_changed();

    return ok;
}

user_data_info *user_data_get(void)
{
    return &user_data;
}

int ringtone_schedule_active_index(const door_config_type_def *cfg, int hour, int min)
{
    if (cfg == NULL)
        return -1;

    int now = hour * 60 + min;
    for (int i = 0; i < 3; i++)
    {
        const ringtone_config_type_def *rt = &cfg->ringtone[i];
        int start = rt->start_h * 60 + rt->start_m;
        int end = rt->end_h * 60 + rt->end_m;
        printf("rt->start_h is %d\r\n", rt->start_h);
        printf("rt->start_m is %d\r\n", rt->start_m);
        printf("rt->end_h is %d\r\n", rt->end_h);
        printf("rt->end_m is %d\r\n", rt->end_m);
        if (start <= end)
        {
            if (now >= start && now < end)
                return i;
        }
        else /* 跨零点时段，如 18:00 - 6:00 */
        {
            if (now >= start || now < end)
                return i;
        }
    }
    return -1;
}

bool ringtone_schedule_active(const door_config_type_def *cfg, int hour, int min)
{
    return ringtone_schedule_active_index(cfg, hour, min) >= 0;
}

void user_data_reset(void)
{
    memcpy(&user_data, &user_data_default, sizeof(user_data_info));
    user_data_save();
}

void scene_mode_switch_to(scene_mode_type_def new_mode)
{
    scene_mode_type_def old_mode = user_data.public_cfg.global_func.scene_mode;
    if (old_mode == new_mode)
        return;

    static bool away_saved;
    static bool away_msg_sw[DOOR_DEVICE_MAX];
    static record_mode_type_def away_rec_mode[DOOR_DEVICE_MAX];

    static bool sleep_saved;
    static bool sleep_mute;

    /* 离开离家模式：恢复原来的 message_sw 和 record_mode */
    if (old_mode == SCENE_MODE_AWAY && away_saved)
    {
        for (int i = 0; i < DOOR_DEVICE_MAX; i++)
        {
            user_data.public_cfg.door_device[i].door_cfg.message_sw = away_msg_sw[i];
            user_data.public_cfg.door_device[i].door_cfg.global_record_mode = away_rec_mode[i];
        }
        away_saved = false;
    }

    /* 离开睡眠模式：恢复原来的 mute_enable */
    if (old_mode == SCENE_MODE_SLEEP && sleep_saved)
    {
        user_data.private_cfg.mute_enable = sleep_mute;
        sleep_saved = false;
    }

    /* 进入离家模式：保存并强制开启 message_sw 和 record_mode */
    if (new_mode == SCENE_MODE_AWAY)
    {
        for (int i = 0; i < DOOR_DEVICE_MAX; i++)
        {
            away_msg_sw[i] = user_data.public_cfg.door_device[i].door_cfg.message_sw;
            away_rec_mode[i] = user_data.public_cfg.door_device[i].door_cfg.global_record_mode;
            user_data.public_cfg.door_device[i].door_cfg.message_sw = true;
            user_data.public_cfg.door_device[i].door_cfg.global_record_mode = RECORD_MODE_VIDEO;
        }
        away_saved = true;
    }

    /* 进入睡眠模式：保存并强制开启静音 */
    if (new_mode == SCENE_MODE_SLEEP)
    {
        sleep_mute = user_data.private_cfg.mute_enable;
        user_data.private_cfg.mute_enable = true;
        sleep_saved = true;
    }

    user_data.public_cfg.global_func.scene_mode = new_mode;
}