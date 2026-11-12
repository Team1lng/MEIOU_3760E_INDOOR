/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __PRODUCT_TEST_CONFIG_H__
#define __PRODUCT_TEST_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "product_test_cfg_info.h"

#define CONFIG_KEY_DEV_TYPE                 "device_type"

#define CONFIG_KEY_FIRMWARE_NAME            "firmware_name"
#define CONFIG_KEY_FIRMWARE_VERSION         "firmware_version"

#define CONFIG_KEY_SUB_FIRMWARE_NAME         "sub_firmware%d_name"
#define CONFIG_KEY_SUB_FIRMWARE_VERSION      "sub_firmware%d_version"

#define CONFIG_KEY_PROTOCOL_TYPE            "protocol_type"
#define CONFIG_KEY_PROTOCOL_VERSION         "protocol_version"
#define CONFIG_KEY_LOWPOWER_MODE            "lowpower_mode"

#define CONFIG_KEY_DEV_NAME                "dev_name"
#define CONFIG_KEY_UART_BAUD                "baud"
#define CONFIG_KEY_NET_POART                "port"
#define CONFIG_KEY_UART_LOOP                "loop"


#define CONFIG_KEY_TYPE                     "type"
#define CONFIG_KEY_MAC_WRITE                "mac_write"
#define CONFIG_KEY_MAC_LOCATION             "mac_in_fuse"
#define CONFIG_KEY_WIFI_TYPE                "wifi_type"
#define CONFIG_KEY_CFG_DYNAMIC_PASSWD       "dynamic_passwd"
#define CONFIG_KEY_CFG_TYNY_ALSA            "tiny_alsa"
#define CONFIG_KEY_CFG_SUB_MCU_KEYS         "sub_mcu_keys"
#define CONFIG_KEY_CFG_PASSWD_PATH          "passwd_path"
#define CONFIG_KEY_CFG_VERSION_PATH         "version_path"
#define CONFIG_KEY_CFG_DOWNLOAD_FILENAME   "download_filename"
#define CONFIG_KEY_CFG_PLAY_FILENAME       "play_filename"
	

#define CONFIG_KEY_PAN_SPEED                "pan_speed"
#define CONFIG_KEY_TILT_SPEED               "tilt_speed"
#define CONFIG_KEY_PAN_MAX_STEP             "pan_max_step"
#define CONFIG_KEY_TILT_MAX_STEP            "tilt_max_step"
#define CONFIG_KEY_LIMIT_SWITCH             "limit_switch"

#define CONFIG_KEY_MODE                     "mode"
#define CONFIG_KEY_DELAY                    "delay_us"
#define CONFIG_KEY_DEPTH                    "depth"

#define CONFIG_KEY_GPIO_NUM                 "gpio_num"
#define CONFIG_KEY_HI_ACTIVE                "high_active"
#define CONFIG_KEY_IS_IN_GROUP              "is_in_group"
#define CONFIG_KEY_CONTROL_BY_PWM       "controlled_by_pwm"
#define CONFIG_KEY_CONFIG_DIR               "config_dir"
#define CONFIG_KEY_OTA_FILE                  "ota_file"

#define CONFIG_KEY_SENSOR_TYPE              "sensor_type"
#define CONFIG_KEY_SENSOR_MIRROR            "sensor_mirror"
#define CONFIG_KEY_SENSOR_FLIP              "sensor_flip"
#define CONFIG_KEY_SENSOR_VERSION           "sensor_version"
#define CONFIG_KEY_SENSOR_CONFIG            "sensor_config"


#define CONFIG_KEY_IRCUT_PIN1               "ircut_pin1"
#define CONFIG_KEY_IRCUT_PIN2               "ircut_pin2"
#define CONFIG_KEY_INDEX                    "index"
#define CONFIG_KEY_ENABLE                   "enable"
#define CONFIG_KEY_PWM_DEVICE               "pwm_device"
#define CONFIG_KEY_PWM_CHANNEL              "pwm_channel" 
#define CONFIG_KEY_POLARITY_INVERSED        "polarity_inversed"

#define CONFIG_KEY_CAMERA_TEST              "camera_test"
#define CONFIG_KEY_IRCUT_TEST               "ircut_test"
#define CONFIG_KEY_IRLED_TEST               "irled_test"
#define CONFIG_KEY_LED_GROUP_TEST           "led_group_test"
#define CONFIG_KEY_AUDIO_TEST               "audio_test"
#define CONFIG_KEY_WHITE_LED_TEST           "white_led_test"
#define CONFIG_KEY_MOTOR_TEST               "motor_test" 
#define CONFIG_KEY_SLEEP                    "sleep_s"


#define CONFIG_KEY_LED_PWM_DEV             "pwm_device"
#define CONFIG_KEY_LED_PWM_CHN             "pwm_channel"
#define CONFIG_KEY_LED_PWM_FREQ             "pwm_frequency"
#define CONFIG_KEY_LED_PWM_DUTY             "pwm_duty"
#define CONFIG_KEY_LED_PWM_DEV             "pwm_device"
#define CONFIG_KEY_LED_PWM_CHN             "pwm_channel"
#define CONFIG_KEY_LED_PWM_POL_INV         "polarity_inversed"


#define CONFIG_KEY_LED_PWM_POL_INV         "polarity_inversed"
#define CONFIG_KEY_KEY_GPIO_NUM            "gpio_num"
#define CONFIG_KEY_KEY_HI_ACTIVE           "high_active"
#define CONFIG_KEY_PIR_GPIO_EN             "gpio_enable"               // optional
#define CONFIG_KEY_PIR_GPIO_EN_HI_ACTIVE   "gpio_enable_high_active"   // optional
#define CONFIG_KEY_PIR_GPIO_SENS_SET1      "gpio_sensitivity_set1"     // optional
#define CONFIG_KEY_PIR_GPIO_SENS_SET2      "gpio_sensitivity_set2"     // optional
#define CONFIG_KEY_PIR_GPIO_IRQ            "gpio_irq"
#define CONFIG_KEY_PIR_GPIO_IRQ_HI_ACTIVE  "gpio_irq_high_active"
#define CONFIG_KEY_IRCUT_PIN1              "ircut_pin1"
#define CONFIG_KEY_IRCUT_PIN2              "ircut_pin2"
#define CONFIG_KEY_SPEAKER_GPIO_NUM        "gpio_num"
#define CONFIG_KEY_SPEAKER_HI_ACTIVE       "high_active"
#define CONFIG_KEY_4G_NET_DEV              "net_dev"      // network device name, e.g. USB0
#define CONFIG_KEY_4G_AT_PORT              "at_port"      // AT command port, e.g. /dev/ttyUSB0
#define CONFIG_KEY_4G_AT_PORT_BAUD         "at_port_baud" // AT command port baud rate

#define CONFIG_KEY_VIDEO_NUM        "video_num"
#define CONFIG_KEY_VIDEO_FPS        "video_fps"
#define CONFIG_KEY_VIDEO_GOP        "video_gop"
#define CONFIG_KEY_VIDEO_BITRATE    "video_bitrate"
#define CONFIG_KEY_VIDEO_WIDTH      "video_width"
#define CONFIG_KEY_VIDEO_HEIGHT     "video_height"
#define CONFIG_KEY_VIDEO_CODECTYPE  "video_codectype"
#define CONFIG_KEY_VIDEO_MINQP      "video_minQp"
#define CONFIG_KEY_VIDEO_MAXQP      "video_maxQp"

#define CONFIG_KEY_AUDIO_NUM        "audio_num"
#define CONFIG_KEY_AUDIO_SAMPLE     "audio_sample"
#define CONFIG_KEY_AUDIO_DATABITS   "audio_databits"
#define CONFIG_KEY_AUDIO_CODECTYPE  "audio_codectype"
#define CONFIG_KEY_AUDIO_CHANNEL    "audio_channel"
#define CONFIG_KEY_AUDIO_FPS        "audio_fps"
#define CONFIG_KEY_MIC_VOLUME       "mic_volume"
#define CONFIG_KEY_SPK_VOLUME       "spk_volume"

#define CONFIG_KEY_DAY2NIGHT_THRESHOLD "day2night_threshold"
#define CONFIG_KEY_NIGHT2DAY_THRESHOLD "night2day_threshold"
#define CONFIG_KEY_DAY_NIGHT_REVERSE     "day_night_reverse"
#define CONFIG_KEY_DAY_NIGHT_SWITCH_MODE "day_night_switch_mode"


#define CONFIG_DEVICE_ZIGBEE        "zigbee"

#define CONFIG_DEVICE_UART          "com_uart"
#define CONFIG_DEVICE_TCP           "com_tcp"
#define CONFIG_DEVICE_PRODUCT_INFO  "product_info"
#define CONFIG_DEVICE_PRODUCT_CFG   "product_cfg"
#define CONFIG_DEVICE_AGING_TEST_CFG   "aging_test_cfg"
#define CONFIG_DEVICE_SINGLE_BOARD_TEST_CFG "single_board_test_cfg"

#define CONFIG_DEVICE_WIFI        "wifi"
#define CONFIG_DEVICE_MOTOR       "motor"
#define CONFIG_DEVICE_SUBG        "subg"

#define CONFIG_DEVICE_LED_RED    "led_red"
#define CONFIG_DEVICE_LED_GREEN  "led_green"
#define CONFIG_DEVICE_LED_WHITE  "led_white"
#define CONFIG_DEVICE_LED_BLUE   "led_blue"
#define CONFIG_DEVICE_LED_IDLE   "led_idle"

#define CONFIG_DEVICE_LED_RED_PWM    "led_red_pwm"
#define CONFIG_DEVICE_LED_GREEN_PWM  "led_green_pwm"
#define CONFIG_DEVICE_LED_WHITE_PWM  "led_white_pwm"
#define CONFIG_DEVICE_LED_BLUE_PWM   "led_blue_pwm"
#define CONFIG_DEVICE_LED_IDLE_PWM   "led_idle_pwm"

#define CONFIG_DEVICE_IRLED_RED       "irled_red"
#define CONFIG_DEVICE_IRLED_GREEN     "irled_green"
#define CONFIG_DEVICE_IRLED_RED_PWM   "irled_red_pwm"
#define CONFIG_DEVICE_IRLED_GREEN_PWM "irled_green_pwm"

#define CONFIG_DEVICE_SPEAKER_1   "speaker_1"
#define CONFGI_DEVICE_IRCUT_1     "ircut_1"
#define CONFIG_DEVICE_KEY_1       "key_1"
#define CONFIG_DEVICE_ADC_1       "adc_1"
#define CONFIG_DEVICE_CAMERA      "camera_1"

#define CONFIG_DEVICE_ALARM_TOOL   "alarm_tool"
#define CONFIG_DEVICE_ALARM_IN     "alarm_in"
#define CONFIG_DEVICE_ALARM_OUT    "alarm_out"
#define CONFIG_DEVICE_MEDIA_CFG    "media_cfg"

#define CONFIG_DEVICE_WATCHDOG    "watchdog"


/*
#define CONFIG_DEVICE_PIR        "pir"
#define CONFIG_DEVICE_KEY_RESET  "key_factory_reset"
#define CONFIG_DEVICE_MODEM       "modem_4g"
#define CONFIG_DEVICE_LED_IR_PWM  "led_ir_pwm"
*/


extern int product_test_config_get_file_path(char *path);
extern int product_test_config_set_file_path(char *path);
extern int product_test_config_init(char *file_name);
extern int product_test_config_uinit(void);
extern int product_test_config_read_string(char *device, char *key, char *buf, int bufsize);
extern int product_test_config_read_int(char *device, char *key, int *value);
extern int product_test_config_get_uart_info(UART_INFO_T *info);
extern int product_test_config_check_uart_loop_support(void); 
extern int product_test_config_get_tcp_info(TCP_INFO_T *info);

#ifdef __cplusplus
}
#endif

#endif
