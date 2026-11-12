/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_BOARD_H__
#define __HAL_BOARD_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* default board configuration file */
#define HAL_BOARD_CONFIG_FILE   "/etc/hal_board.json"

/** Get the path of the board configuration file.
 * @param buf buffer for saving file path.
 * @param size buffer size.
 * @returns 0 on success, error code on failure.
 */
int hal_board_get_config_file(char *buf, int size);

/*
 * Configuration Item Defination
 *
 * <File Template>
 *          src/board/hal_board.json
 */

/* Item definition */

// LED/Light
#define ITEM_LED_RED            "led_red"
#define ITEM_LED_GREEN          "led_green"
#define ITEM_LED_BLUE           "led_blue"
#define ITEM_LED_IR             "led_ir"
#define ITEM_LED_WHITE_LIGHT    "led_white_light"
// KEY (GPIO KEY)
#define ITEM_KEY_FACTORY_RESET  "key_factory_reset"
// UART
#define ITEM_UART               "uart"
// PIR
#define ITEM_PIR                "pir"
// IRCUT
#define ITEM_IRCUT              "ircut"
// audio speaker
#define ITEM_SPEAKER            "speaker_0" // format: "speaker_%d"
// 4G MODEM
#define ITEM_MODEM_4G           "modem_4g"
// IR
#define ITEM_IR                 "ir"
// RTC
#define ITEM_RTC                "rtc"
// watchdog
#define ITEM_WATCHDOG           "watchdog"
// usb storage
#define ITEM_USB_STORAGE        "usb_storage"
// sd card
#define ITEM_SDCARD             "sdcard"
// battery
#define ITEM_BATTERY		"battery"

/* Key definition */

// LED global attributes
#define KEY_LED_CONTROLLED_BY_PWM   "controlled_by_pwm"
// GPIO LED
#define KEY_LED_GPIO_NUM            "gpio_num"
#define KEY_LED_HI_ACTIVE           "high_active"
// PWM LED
#define KEY_LED_PWM_DEV             "pwm_device"
#define KEY_LED_PWM_CHN             "pwm_channel"
#define KEY_LED_PWM_POL_INV         "polarity_inversed"
// KEY (button)
#define KEY_KEY_GPIO_NUM            "gpio_num"
#define KEY_KEY_HI_ACTIVE           "high_active"
// UART
//   if not set, default: "/dev/ttyS%d"
#define KEY_UART_NAME_PATTERN       "name_pattern"
// PIR
#define KEY_PIR_GPIO_EN             "gpio_enable"               // optional
#define KEY_PIR_GPIO_EN_HI_ACTIVE   "gpio_enable_high_active"   // optional
#define KEY_PIR_GPIO_SENS_SET1      "gpio_sensitivity_set1"     // optional
#define KEY_PIR_GPIO_SENS_SET2      "gpio_sensitivity_set2"     // optional
#define KEY_PIR_GPIO_IRQ            "gpio_irq"
#define KEY_PIR_GPIO_IRQ_HI_ACTIVE  "gpio_irq_high_active"
// IRCUT
#define KEY_IRCUT_PIN1              "ircut_pin1"
#define KEY_IRCUT_PIN2              "ircut_pin2"
// audio speaker
#define KEY_SPEAKER_GPIO_NUM        "gpio_num"
#define KEY_SPEAKER_HI_ACTIVE       "high_active"
// 4G modem
#define KEY_MODEM_4G_NET_DEV        "net_dev"       // network device name, e.g. USB0
#define KEY_MODEM_4G_AT_PORT        "at_port"       // AT command port, e.g. /dev/ttyUSB0
#define KEY_MODEM_4G_AT_PORT_BAUD   "at_port_baud"  // AT command port baud rate
// IR
//   [optional] IR RX device. If not set, default: /dev/lirc0
#define KEY_IR_DEVICE_RX            "device_rx"
//   [optional] IR TX device. If not set, default: /dev/lirc1
#define KEY_IR_DEVICE_TX            "device_tx"
// RTC
//   [optional] RTC device name. If not set, default: /dev/rtc0
#define KEY_RTC_DEV                 "device"
// watchdog
//   [optional] watchdog device name. If not set, default: /dev/watchdog
#define KEY_WATCHDOG_DEV_NAME       "device_name"
// usb storage
//   If not set, default: "/sys/devices/platform/*ehci*"
#define KEY_USB_STORAGE_UEVENT_PATH "uevent_path"
//   If not set, default: "block/sd"
#define KEY_USB_STORAGE_DEVICE_NAME "device_name"
// sd card
//   If not set, default: "/sys/devices/platform/rtk-sdmmc/mmc_host/mmc0"
#define KEY_SDCARD_UEVENT_PATH      "uevent_path"
//   If not set, default: "block/mmcblk"
#define KEY_SDCARD_DEVICE_NAME      "device_name"
// battery
//   if not set, default: "/sys/class/power_supply/battery"
#define KEY_BATTERY_UEVENT_PATH	    "uevent_path"
//   if not set, default: "/power_supply/battery"
#define KEY_BATTERY_DEVICE_NAME	    "device_name"


#ifdef __cplusplus
}
#endif

#endif
