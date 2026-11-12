#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <stdint.h>
#include <stdbool.h>

// ========================= 语言ID枚举 =========================
// 注意：枚举顺序需与 s_common_lang_str 数组的列顺序一致
typedef enum
{
    LANGUAGE_ID_ENGLISH = 0, // 英文（默认 fallback 语言）
    LANGUAGE_ID_CHINESE,     // 中文
    LANGUAGE_ID_GERMAN,      // 德语
    LANGUAGE_ID_HEBREW,      // 希伯来语
    LANGUAGE_ID_POLISH,      // 波兰语
    LANGUAGE_ID_PORTUGUESE,  // 葡萄牙语
    LANGUAGE_ID_SPANISH,     // 西班牙语
    LANGUAGE_ID_FRENCH,      // 法语
    LANGUAGE_ID_JAPANESE,    // 日语
    LANGUAGE_ID_ITALIAN,     // 意大利语
    LANGUAGE_ID_CZECH,       // 捷克语
    LANGUAGE_ID_SLOVAK,      // 斯洛伐克语
    LANGUAGE_ID_HUNGARIAN,   // 匈牙利语
    LANGUAGE_ID_ROMANIAN,    // 罗马尼亚语
    LANGUAGE_ID_SLOVENIAN,   // 斯洛文尼亚语
    LANGUAGE_ID_TOTAL        // 语言总数（必须放在最后）
} LANGUAGE_ID;

// ========================= 多语言字符串ID枚举 =========================
// 与 EMOS-Indoor layout_define.h btn_string_id 保持一致
typedef enum
{
    /* language type */
    STR_LANGUAGE_TYPE,
    /* layout_home */
    STR_INTERCOM,
    STR_MONITOR,
    STR_VIDEO,
    STR_EVENT,
    STR_MEDIA,
    STR_SETTING,
    STR_AT_HOME,
    STR_NOT_AT_HOME,
    STR_DORMANT,
    STR_UNLOCK,
    STR_LOCK,
    STR_GATE,
    STR_GATE1,
    STR_GATE2,
    STR_STANDBY,
    STR_PLEASE_INSTER_SD,
    STR_NO_SD_CARD,
    STR_INSET_SD_SUCCEE,
    STR_PLEASE_FORMAT_SD,
    STR_YES,
    STR_NO,
    STR_DEVICE_CONFLICT,

    /* layout_call */
    STR_LOCAL_DEVICE_ID,
    STR_CALLED_DEVICE_ID,
    STR_CALL_TIMEOUT,
    STR_WAIT_TIMEOUT,
    STR_CALL_TIMED_OUT,

    /* layout_monitor_1 */
    STR_DOOR1,
    STR_DOOR2,
    STR_CAMERA1,
    STR_CAMERA2,

    /* layout_video_list、layout_photo_list */
    STR_OPEN,
    STR_DELETE,
    STR_ALL_DELETE,
    STR_CLOSE,
    STR_FILE_CORRUPTED,
    STR_VIDEO_LIST,

    /* layout_setting */
    STR_SYSTEM_SET,
    STR_DOOR_SET,
    STR_CAMERA_SET,
    STR_NETWORK_SET,
    STR_SCENE_SET,
    STR_SENIOR_SET,
    STR_SYSTEM_INFO,

    /* layout_monitor */
    STR_APP_PREVIEW,
    STR_REC,
    STR_SNAPSHOT,
    STR_DOOR1_CALL,
    STR_DOOR2_CALL,
    STR_DEVICE_BUSY,
    STR_SD_NO_MEMORY,

    /* layout_media */
    STR_MOVIE,
    STR_MUSIC,
    STR_PHOTO,
    STR_FILES,

    /* layout_event */
    STR_CALL_RECORD,
    STR_MESSAGE_RECORD,
    STR_MOTION_RECORD,
    STR_ALARM_RECORD,

    /* layout_setting_sys */
    STR_DEVICE_ID,
    STR_TIME,
    STR_DATE_FORMAT,
    STR_CLOCK,
    STR_LANGUAGE,
    STR_KEYTONE,
    STR_DETECTION_PREVIEW,
    STR_RINGBACK,
    STR_INDOOR_UNLOCK_TIME,
    STR_UNLOCK_HINT,
    STR_ADMIN_SETTING,
    STR_YY_MM_DD,
    STR_MM_DD_YY,
    STR_DD_MM_YY,
    STR_ON,
    STR_OFF,
    STR_S,
    STR_ONLY_DEVICE_1_SET,
    STR_SLEEP_CANNOT_SET,

    /* layout_setting_admin */
    STR_ROOM_NUMBER,
    STR_ROUTER_ADDRESS,
    STR_CHANGE_PASSWORD,
    STR_RESTOR,
    STR_ADMINISTRATORS,
    STR_RESTORE_ADMIN,
    STR_CONFIRM,
    STR_CANCEL,

    /* layout_setting_door */
    STR_STATE,
    STR_UNLOCK_TIME,
    STR_UNGATE_TIME,
    STR_RECORD_MODE,
    STR_MOTION_DETECTION,
    STR_MOTION_DETECT_SENSITIVITY,
    STR_MOTION_DETECT_RECORD_MODE,
    STR_MOTION_DETECTION_DURATION,
    STR_MESSAGE,
    STR_MESSAGE_TIME,
    STR_RING_SETTING,
    STR_CLOSE_SCENE_MODE,
    STR_IN_NOT_AT_HOME,
    STR_LOW,
    STR_MEDIUM,
    STR_HIGH,
    STR_SNAP,

    /* layout_setting_camera */
    STR_CAMERA_MODEL,
    STR_DAHUA,
    STR_HIKVISION,
    STR_CAMERA_IP_ADDRESS,
    STR_ACCOUNT_NUMBER,
    STR_PASSWORD,

    /* layout_setting_ring */
    STR_RING1,
    STR_RING2,
    STR_RING3,
    STR_RING1_DOOR1,
    STR_RING2_DOOR1,
    STR_RING3_DOOR1,
    STR_RING1_DOOR2,
    STR_RING2_DOOR2,
    STR_RING3_DOOR2,
    STR_SCHEDULE,
    STR_RING_TIME,
    STR_RING_MODE,
    STR_RING_SELECT,
    STR_RING_VOLUME,
    STR_STANDARD,
    STR_CUSTOMIZED,

    /* layout_setting_wifi */
    STR_NET_PAIRING,
    STR_WLAN_PAIRING,
    STR_WIRED_PAIRING,
    STR_STATIC_ALLOC,
    STR_UDHCPC_ALLOC,
    STR_WLAN,
    STR_NETWORK_INFO,
    STR_QR_CODE_NETWORK,
    STR_ADD_MANUALLY,
    STR_DISCONNECT_WIFI,
    STR_IP_ADDRESS,
    STR_MAC,

    /* layout_add_wifi */
    STR_WIFI_ACCOUNT,
    STR_WIFI_PASSWD,

    /* layout_setting_scene */
    STR_DIGITAL_PHOTO,
    STR_DIGITAL_PHOTO_SWITCH_TIME,
    STR_BACKGROUND_MUSIC,
    STR_BACKGROUND_MUSIC_VOLUME,
    STR_MOTION_DETECTION_CLOSE,

    /* layout_setting_senior */
    STR_FORMAT_SD,
    STR_FACTORY_RESET,
    STR_FACTORY_OUTDOOR,
    STR_SOFTWARE_UPDATE,
    STR_RESTART_SYSTEM,
    STR_APP_UNLOCK_SETTINGS,
    STR_STANDBY_MODE,
    STR_WANT_FORMAT_SD,
    STR_FORMATING,
    STR_SD_SCAN,
    STR_FORMAT_SUCCE,
    STR_WANT_FACTORY_RESET,
    STR_WANT_FACTORY_OUTDOOR,
    STR_FACTORY_RESET_ING,
    STR_WANT_SOFTWARE_UPDATE,
    STR_CHECK_OUTDOOR_STATUS,
    STR_DEVICE_OFFLINE,
    STR_NO_UPGRAD_FW,
    STR_SOFTWARE_UPDATE_ING,
    STR_UPDATE_SUCCEE,
    STR_WANT_RESTART_SYSTEM,

    /* layout_setting_info */
    STR_HARDWARE_VERSION,
    STR_SOFTWARE_VERSION,
    STR_FIRMWARE_VERSION,
    STR_DOOR_1_VERSION,
    STR_DOOR_2_VERSION,
    STR_DOOR_1_COMPILE,
    STR_DOOR_2_COMPILE,
    STR_RELEASE_DATE,
    STR_SD_SIZE,
    STR_GET_TUYA_ID,
    STR_REPLACE_IT,
    STR_NO_TUYA_FILE,
    STR_NO_UUID,
    STR_ERROR_FILE_TYPE,
    STR_FILE_RW_ERROR,

    /* layout_password_input */
    STR_PASSWORD_INPUT,
    STR_PLEASE_ENTER,
    STR_PLEASE_INPUT_PASSWORD,
    STR_PLEASE_ENTER_NEW_PASSWORD,
    STR_ENTER_CAMERA_IP_ADDR,
    STR_ENTER_CAMERA_PASSWORD,
    STR_ENTER_CAMERA_ACCOUNT,
    STR_ADD_LOCK_CARD,
    STR_ADD_GATE1_CARD,
    STR_DEL_LOCK_CARD,
    STR_DEL_GATE1_CARD,

    /* layout_add_wifi */
    STR_CONNECTING,
    STR_CONNECTION_SUCCESS,
    STR_CONNECTION_FAIL,
    STR_WIFI_NAME_EMPTY,
    STR_WIFI_PASSW_SHOT,
    STR_NOT_FIND,
    STR_PASSW_SHOT,
    STR_PASSW_LONG,
    STR_PASSW_ERROR,
    STR_INPUT_WIFI_NAME,

    /* tuya */
    STR_TUYA_CURR_DOOR1,
    STR_TUYA_DOOR1,
    STR_TUYA_CURR_DOOR2,
    STR_TUYA_DOOR2,
    STR_TUYA_CURR_CAMERA1,
    STR_TUYA_CAMERA1,
    STR_TUYA_CURR_CAMERA2,
    STR_TUYA_CAMERA2,

    /* layout_add_data & layout_data_list */
    STR_NO_NUM,
    STR_DOOR1_CARD_MANAGE,
    STR_DOOR2_CARD_MANAGE,
    STR_DOOR1_FINGER_MANAGE,
    STR_DOOR2_FINGER_MANAGE,
    STR_DOOR1_PASSW_MANAGE,
    STR_DOOR2_PASSW_MANAGE,
    STR_CARD_MANAGE,
    STR_FINGER_MANAGE,
    STR_PASSW_MANAGE,
    STR_ADD_SUCCESS,
    STR_ADD_FAILED,
    STR_PASSW_ENOUGH_BITS,
    STR_CARD_NUMBER,
    STR_FINGER_NUMBER,
    STR_PASSWORD_NUMBER,
    STR_PLEASE_ADD_FINGER,
    STR_PLEASE_DELETE_FINGER,
    STR_PLEASE_ADD_CARD,
    STR_PLEASE_DELETE_CARD,
    STR_PLEASE_ADD_PASSW,
    STR_PLEASE_DELETE_PASSW,
    STR_ILLEGAL_OPERATION,
    STR_NUM_PASS_FULL,
    STR_EXISTING_EQUIPMENT_ONLINE,

    /* weather */
    STR_REAVY_RAIN,
    STR_THUNDERSTORM,
    STR_SANDSTORM,
    STR_LIGHT_SNOW,
    STR_SNOW,
    STR_POGONIP,
    STR_RAINSTORM,
    STR_ISOLATED_SHOWERS,
    STR_FLOATING_DUST,
    STR_LIGHTNING,
    STR_LIGHT_SHOWERS,
    STR_RAIN,
    STR_RAIN_AND_SNOW,
    STR_DUST_TORNADO,
    STR_ICE_PELLET,
    STR_STRONG_SANDSTORM,
    STR_BLOWING_SAND,
    STR_LIGHT_TO_MODERATE_RAIN,
    STR_MOSTLY_CLEAR,
    STR_SUNNY,
    STR_FOG,
    STR_SHOWER,
    STR_HEAVY_SHOWERS,
    STR_HEAVY_SNOW,
    STR_TORRENTIAL_RAIN,
    STR_BLIZZARD,
    STR_HAIL,
    STR_LIGHT_TO_MODERATE_SNOW,
    STR_PARTLY_CLOUDY,
    STR_LIGHT_SNOW_SHOWERS,
    STR_MODERATE_SNOW,
    STR_OVERCAST,
    STR_HOARFROST,
    STR_HEAVY_RAIN,
    STR_THUNDERSTORMS_WITH_HAIL,
    STR_FREEZING_RAIN,
    STR_SNOW_SHOWERS,
    STR_LIGHT_RAIN,
    STR_SMOG,
    STR_MODERATE_RAIN,
    STR_CLOUDY,
    STR_THUNDER_SHOWER,
    STR_MODERATE_TO_HEAVY_RAIN,
    STR_HEAVY_TO_RAINSTORM,
    STR_SUNDAY,

    STR_WEATHER_SWITCH,
    STR_WEATHER_SCHEDULE,
    STR_PRESSURE,
    STR_HUMIDITY,
    STR_PM10,
    STR_PM25,
    STR_WEEK_SUN,
    STR_WEEK_MON,
    STR_WEEK_TUE,
    STR_WEEK_WED,
    STR_WEEK_THU,
    STR_WEEK_FRI,
    STR_WEEK_SAT,

    // #ifdef DHCP_IPCAMERA
    /******* IPCamera *******/
    STR_REGISTERED,
    STR_AVA_DEV,
    STR_REPLACE_DEV,
    STR_CONFIRM_CANCELLATION,
    STR_LINK_DEVICE_FAILED,
    STR_REG_SAME_DEVICE,
    STR_REGISTER,
    STR_CANCELLATION,
    STR_ENTER_IPC_USER,
    STR_ENTER_IPC_PWD,
    STR_WEBCAM,
    // #endif

    // #ifdef MACHINE_CHIME
    STR_CHIME_TYPE,
    STR_MECHANICAL_CHIME,
    STR_ELECTRONIC_CHIME,
    // #endif
    STR_LOAD,
    STR_ALARM_1,
    STR_ALARM_2,
    STR_LIGTH_SWITCH,
    STR_SYSYEM_BUSY,
    STR_EXIT_BUTTON,

    STR_MUTE,

    STR_SYSTEM_AMBIENT_LED,

    STR_TOTAL
} xls_str_id_t;

// ========================= 函数声明 =========================
#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************
     * @brief  设置当前语言ID
     * @param  id：目标语言ID（LANGUAGE_ID枚举）
     * @note   非法ID会被忽略，保留当前语言
     *******************************************************************/
    void language_id_set(LANGUAGE_ID id);

    /*******************************************************************
     * @brief  获取当前语言ID
     * @return 当前语言ID（默认：LANGUAGE_ID_ENGLISH）
     *******************************************************************/
    LANGUAGE_ID language_id_get(void);

    /*******************************************************************
     * @brief  获取当前语言的字符串（核心API）
     * @param  lang_str_id：字符串ID（对应XLS文件中的行号，0开始）
     * @return 字符串指针（永不返回NULL，失败返回默认值"text"）
     *******************************************************************/
    const char *lang_str_get(int lang_str_id);

    /*******************************************************************
     * @brief  按指定语言获取字符串
     * @param  index：字符串ID（xls_str_id_t类型）
     * @param  language：指定语言ID（LANGUAGE_ID枚举）
     * @return 字符串指针（永不返回NULL，失败返回默认值"text"）
     *******************************************************************/
    const char *lang_str_get_by_language(xls_str_id_t index, LANGUAGE_ID language);

    /*******************************************************************
     * @brief  获取支持的语言总数
     * @return 语言总数（XLS模块可用返回实际数量，否则返回1）
     *******************************************************************/
    int language_total_get(void);

    /*******************************************************************
     * @brief  输出所有语言文本到文件（用于校验文案）
     * @note   宏控制编译（默认关闭），输出路径：/tmp/nfs/language_default.csv
     *******************************************************************/
    void write_language_string_to_xls(void);

#ifdef __cplusplus
}
#endif

#endif // LANGUAGE_H
