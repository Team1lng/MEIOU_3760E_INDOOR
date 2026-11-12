/*!
 * @file
 * @brief hal interface to receive hw event
 */
#ifndef HAL_EVENT_H
#define HAL_EVENT_H

#include <linux/input.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "hal_sensor.h"         /* for legacy codes */
#include "hal_input_event.h"    /* for legacy codes */

/* HAL HW event(uevent) interface */

#define UEVENT_MSG_LEN  1024

/* battery status */
#define BATTERY_CHARGING    "Charging"
#define BATTERY_DISCHARGING "Discharging"
#define BATTERY_FULL        "Full"

/** hw event flag */
#define BATTERY_EVENT        (0x1)
#define USB_POWER_EVENT      (0x1 << 1)
#define ETH_EVENT            (0x1 << 2)
#define SDCARD_EVENT         (0x1 << 3)
#define USB_STORAGE_EVENT    (0x1 << 4)

/* event mask */
#define BATTERY_EVENT_MASK          (BATTERY_EVENT)
#define USB_POWER_EVENT_MASK        (USB_POWER_EVENT)
#define ETH_EVENT_MASK              (ETH_EVENT)
#define SDCARD_EVENT_MASK           (SDCARD_EVENT)
#define USB_STORAGE_EVENT_MASK      (USB_STORAGE_EVENT)

#define IS_BATTERY_EVENT(value)        (((value)&BATTERY_EVENT_MASK) == BATTERY_EVENT)
#define IS_USB_POWER_EVENT(value)      (((value)&USB_POWER_EVENT_MASK) == USB_POWER_EVENT)
#define IS_ETH_EVENT(value)            (((value)&ETH_EVENT_MASK) == ETH_EVENT)
#define IS_SDCARD_EVENT(value)         (((value)&SDCARD_EVENT_MASK) == SDCARD_EVENT)
#define IS_USB_STORAGE_EVENT(value)    (((value)&USB_STORAGE_EVENT_MASK) == USB_STORAGE_EVENT)

typedef struct hw_event_listener {
    int uevent_fd;
    int eth_fd;
    unsigned int type;
} hw_event_listener;

typedef struct hw_battery_event {
    char battery_status[25];
    int battery_present;
    int battery_capacity;
    char battery_health[25];
    int battery_temperature;
} hw_battery_event;

typedef struct hw_usb_power_event {
    int power_online;
} hw_usb_power_event;

typedef struct hw_sdcard_event {
    int sd_plug;
    char dev_name[20];
    char dev_type[20];
} hw_sdcard_event;

typedef struct hw_usb_storage_event {
    int usb_storage_plug;
    char dev_name[20];
    char dev_type[20];
} hw_usb_storage_event;

/** data for store one eth event */
typedef struct hw_eth_event {
    char device_name[20];
    int device_state;         /**< store link up/down, only vaild when type is RTM_xxxLINK */
    int type;                 /**< eth event type, current supported RTM_xxxLINK and RTM_xxxADDR */
} hw_eth_event;

/** data for store a list of eth events */
typedef struct hw_eth_event_list {
    hw_eth_event* head;      /**< list head */
    int size;                /**< the size(number of hw_eth_event) of head list which for store eth event*/
    int count;               /**< the real number of events store in head list */
} hw_eth_event_list;

/** data for store uevent/eth_event(unparsed) which recv by hal_hw_event_recv */
typedef struct hw_event {
    unsigned int type;          /**< event type */
    char msg[UEVENT_MSG_LEN+2]; /**< buffer for store event */
    int msg_len;                /**< the real stored data size */
} hw_event;

/* Note: how to add a new uevent support:
 * 1: add event flag(in this file)
 * 2: add event mask and IS_xxx_EVENT marco(in this file)
 * 3: add event struct like hw_usb_power_event(in this file)
 * 4: add event path(for coldboot) and filter keyword like USB_POWER_DEVICE_NAME(in this file)
 * 5: modify hw_event_filter function(in HAL_hw_event.c)
 * 6: modify hw_coldboot function(in HAL_hw_event.c)
 * 7: modify hal_hw_event_register function(in HAL_hw_event.c)
 * 8: add a new hal_hw_parse_XXXX_event for new event(in HAL_hw_event.c)
 * 9: then you can have a try
 */

/**
 * @brief receive hw event
 * @param listener create by hal_hw_event_register
 * @param event buffer for get the event
 * @param timeout 0 is blocked, value is seconds
 * @return success return 0, failed return -errno
 * @pre need call hal_hw_event_register get listener
 * @note block function, received only 1 event each time
 */
extern int hal_hw_event_recv(hw_event_listener* listener, hw_event* event, struct timeval* timeout);

/**
 * @brief register a hw event listener
 * @param listener which used by hal_hw_event_recv
 * @param type the event type which want to receive, use bit event flag
 * @return success return 0, failed return -errno
 * @note can receive multiple types events, but only recv 1 event each time
 */
extern int hal_hw_event_register(hw_event_listener* listener, unsigned int type);

/**
 * @brief close a hw event listener
 * @param listener which create by hal_hw_event_register
 * @return success return 0, failed return -errno
 */
extern int hal_hw_event_unregister(hw_event_listener* listener);

/**
 * @brief parse eth interface event
 * @param event use event in hw_event which received by hal_hw_event_recv
 * @param list for store parse result
 * @return success return 0, failed return -errno
 * @note result is a list of eth event
 */
extern int hal_hw_parse_eth_event(hw_event* event, hw_eth_event_list* list);

/**
 * @brief parse batter uevent
 * @param event use event in hw_event which received by hal_hw_event_recv
 * @param result for store parse result
 * @return success return 0, failed return -errno
 * @note parse 1 event each time.
 */
extern int hal_hw_parse_battery_event(hw_event* event, hw_battery_event* result);

/**
 * @brief parse usb_power uevent
 * @param event use event in hw_event which received by hal_hw_event_recv
 * @param result for store parse result
 * @return success return 0, failed return -errno
 * @note parse 1 event each time.
 */
extern int hal_hw_parse_usb_power_event(hw_event* event, hw_usb_power_event* result);

/**
 * @brief parse sd card hotplug uevent
 * @param event use event in hw_event which received by hal_hw_event_recv
 * @param result for store parse result
 * @return success return 0, failed return -errno
 * @note parse 1 event each time.
 */
extern int hal_hw_parse_sdcard_event(hw_event* event, hw_sdcard_event* result);

/**
 * @brief parse usb_storage uevent
 * @param event use event in hw_event which received by hal_hw_event_recv
 * @param result for store parse result
 * @return success return 0, failed return -errno
 * @note parse 1 event each time.
 */
extern int hal_hw_parse_usb_storage_event(hw_event* event, hw_usb_storage_event* result);

#endif
