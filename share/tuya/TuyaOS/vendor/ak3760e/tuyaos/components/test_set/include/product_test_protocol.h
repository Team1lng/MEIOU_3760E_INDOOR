/* 
 *  @file product_test_protocol.h
 *  @brief product_test_protocol module is supply protocol interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PROTOCOL_H__
#define __PRODUCT_TEST_PROTOCOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#define PARM_MAX_SIZE (128)
#define PARM_MIN_SIZE (65)

enum {
    E_TRUE =  0, //true 
    E_FALSE = -1,//false
};

enum {
    E_TYPE_PRODUCT_INFO=0x00, // product info type
    E_TYPE_CFG_INFO,   // product config type
    E_TYPE_MOTOR,      // motor 
    E_TYPE_ADC,        // adc 
    E_TYPE_PIR,        // pir 
    E_TYPE_IR,         // ir 
    E_TYPE_IRLED,      // irled
    E_TYPE_IRCUT,      // ircut 
    E_TYPE_SD,         // sd
    E_TYPE_BATTERY,    // battery
    E_TYPE_MIC,        // mic 
    E_TYPE_SPEAKER,    // speaker
    E_TYPE_LED,        // led
    E_TYPE_KEY,        // key
    E_TYPE_WIFI,       // wifi
    E_TYPE_BT,         // bt
    E_TYPE_ZIGBEE,     // zigbee
    E_TYPE_SUBG,       // sub1g
    E_TYPE_BEEP,       // beep
    E_TYPE_WDG,        // watchdog
    E_TYPE_REBOOT,     // reboot
    E_TYPE_OTA,        // ota
    E_TYPE_SLAVE_OTA,  // slave cpu ota
    E_TYPE_USB,        // usb
    E_TYPE_GDL,        // general download
    E_TYPE_LCD,        // lcd
    E_TYPE_TS,         // touchscreen
    E_TYPE_DDR,        // ddr
    E_TYPE_TEMP,       // tempture
    E_TYPE_CPU,        // cpu
    E_TYPE_FLASH,      // flash
    E_TYPE_MODEM,      // modem
    E_TYPE_ETHERENT,   // etherent
    E_TYPE_SELF_CHECK,  // self check    
    E_TYPE_SCAP,       // slave cap
    E_TYPE_FLAG_DECT,  // flag detectt
    E_TYPE_AGING,      // aging test
    E_TYPE_GPIO,       // gpio 
    E_TYPE_GEN_SWITCH, // general switch
    E_TYPE_PASSWD,     // passwd
    E_TYPE_VIDEO,      // video
    E_TYPE_BOOL_TEST,  // bool test
    E_TYPE_MAX,
};

enum {
    E_INFO_PROD_INFO, // product information
    E_INFO_MFIRM_INFO,// man firmware information
    E_INFO_SFIRM_INFO, // slave firmware information
};

enum {
    E_GEN_TEST, // general test
    };

enum {
    E_CFG_READ_CFG = 0, // read config information
    E_CFG_WRITE_CFG,    // write config information
    E_CFG_WRITE_MMAC,   // write main mac 
    E_CFG_READ_MMAC,    // read main mac
    E_CFG_WRITE_SMAC,   // write slave mac
    E_CFG_READ_SMAC,    // write slave mac
    E_CFG_WRITE_BSN,    // write bsn
    E_CFG_READ_BSN,     // read bsn
    E_CFG_WRITE_SN,     // write sn
    E_CFG_READ_SN,      // read sn
    E_CFG_WRITE_COUNTRY,// write country
    E_CFG_READ_COUNTRY, // read country
    E_CFG_MAX,          // config max
};

enum {
    E_LED_ALL_ON=0, // all led on
    E_LED_ALL_OFF,  // all led off
    E_LED_ALL_FLASH,// all led flash
};
enum{
    E_OTA_CFG,    // ota config
    E_OTA_START,  // ota start
    E_OTA_DLING,  // ota download 
    E_OTA_END,    // ota end
};
enum{
    E_MODEM_ENTER_TEST = 1, // modem enter test
    E_MODEM_EXIT_TEST,      // modem exit test
    E_MODEM_TEST_MODE1,     // modem test mode1 
    E_MODEM_TEST_MODE2,     // modem test mode2
    E_MODEM_TEST_MODE3,     // modem test mode3
    E_MODEM_READ_CCID,      // modem read ccid
    E_MODEM_READ_IMSI,      // modem read imsi
    E_MODEM_READ_IMEI,      // modem read imei
    E_MODEM_READ_RSSI,      // modem read rssi
};

enum{
    E_FLAG_DECT_WIFIRF = 0,    // flag wifirf
    E_FLAG_DECT_BOARDTEST = 1, // falg boradtest
};

enum{
    E_IR_SEND,  // ir send
    E_IR_RECV,  // ir receive
    E_IR_MODE,  // ir mode
};

enum {
    E_GDL_START, // general download start
    E_GDL_DOING, // general download doing
    E_GDL_END,   // general download end
};

typedef struct gdl_parm_s
{
    char type[32]; // download file type
    int size;      // download file size
    char md5[PARM_MIN_SIZE]; //download file md5sum
}GDL_PRAM_T;

typedef struct product_info_s {
    int cpu; // cpu num
    char mfirmname[PARM_MIN_SIZE]; // main firmware name
    char mfirmver [PARM_MIN_SIZE]; // main firmware version
    char sfirmname[PARM_MIN_SIZE]; // slave firmware name
    char sfirmver [PARM_MIN_SIZE]; // slave firmware version
    char protocol_ver[PARM_MIN_SIZE];// protocol version
    char product_type[PARM_MIN_SIZE];  // product type
    int mac_write;      // mac_write flag true/false 
    int pskkey_write;   // pskkey_write flag true/false
    int himekitType;    // himekittype flag  int 1 write
}PROD_INFO_T;

typedef struct cfg_parm_s
{
    char uuid[PARM_MAX_SIZE];    // uuid
    char auzkey[PARM_MAX_SIZE];  //auzkey
    char pid[PARM_MAX_SIZE];     // pid
    char ap_ssid[PARM_MIN_SIZE]; // ap ssid
    char ap_pwd[PARM_MIN_SIZE];  // ap passwd
    char pskKey[PARM_MIN_SIZE];  // pskkey
    char mmac[PARM_MIN_SIZE];    //master mac
    char snum;                   // slave cpu number   
    char smac[PARM_MIN_SIZE];    // slave mac 
    char bsn[PARM_MIN_SIZE];     // bsn
    char sn[PARM_MIN_SIZE];      // sn
    char country[PARM_MIN_SIZE]; // country
    int prod_test;               // product test flag 
}CFG_PARM_T;

typedef struct upgrade_cfg_s
{
    unsigned long baud;      // uart baudrate
    unsigned long size;      // size
    unsigned long crc32;     // crc32
    unsigned long packet_size; // ota packet size
}UPGRADE_CFG_T;


enum {
    E_WIFI_IPERF_TEST,     // iperf test
    E_WIFI_R_COUNTRY,      // read country
    E_WIFI_W_COUNTRY,      // write country
    E_WIFI_R_RSSI,         // read rssi
    E_WIFI_CONNECT_AP,     // connect ap 
};

typedef struct wifi_iperf_s {
    char server[32]; // iperf server ip address
    uint16_t port;   // iperf test port
    uint32_t time;   // iperf test time(seconds)
    double avr_speed;// iperf average speed
    double min_speed;// iperf min speed
    double max_speed;// iperf max speed
}WIFI_IPERF_T;


enum {
    E_SENSOR_LIGHT,  // sensor type light sensor
    E_SENSOR_ADC,    // sensor type adc
};


typedef struct adc_parm_s {
    int channel;  // adc channel
    uint32_t value;//adc value
    int type;   // adc type
}ADC_PARM_T;


enum {
        E_IR_MODE_EMISSION =  0, // ir emssion
        E_IR_MODE_RECEIVE = 1,  // ir receive
};

enum {
        E_MIC_START =  0, // mic start
        E_MIC_STOP = 1,   // mic stop
};


enum {
    E_SPEAKER_START =  0, // speaker start
    E_SPEAKER_STOP = 1,   // speaker stop
};



enum {
    E_MOTOR_SELF_CHECK, // motor self check
    E_MOTOR_AGING_TEST, // motor aging test
    E_MOTOR_MOVE_UP,    // motor move up
    E_MOTOR_MOVE_DOWN,  // motor move down
    E_MOTOR_MOVE_LEFT,  // motor move left
    E_MOTOR_MOVE_RIGHT, // motor move right
};
enum {
    E_VIDEO,  // video test 
    E_VIDEO_BLACK, //vedeo black test
};

enum {
    E_BOOL_ALARM_IN = 0, // alarm in test
    E_BOOL_ALARM_FLASH,  // alarm out flash
    E_BOOL_ALARM_OFF,    // alarm off
    E_BOOL_LIMIT_SWITCH, // limit switch test
    E_BOOL_DDR,          // ddr test
    E_BOOL_RFFLAG,       // rfflag test
    E_BOOL_LIGHT_WHITE_FLASH, // white led flash 
    E_BOOL_LIGHT_WHITE_OFF,   // white led off
    E_BOOL_AUTO_MIC_ON,       // mic auto test on 
    E_BOOL_AUTO_MIC_OFF,      // mic auto test off
	E_BOOL_AUDIO_START,		// start audio test
	E_BOOL_SWEPT_SINE_START,// start swept sine start
	E_BOOL_SWEPT_SINE_OVER, // end swept sine start
	E_BOOL_READ_SWEPT_SINE_RESULT, // read swept sine result
    E_BOOL_AUTO_LIGHT_COLOR, // light color auto
    E_BOOL_MAX,               // bool max
};

#define MAX_PARM (6)

typedef struct bool_parm_s {
    int type; //bool type
    int num;  // bool totals
    char key[MAX_PARM][PARM_MIN_SIZE]; // bool key
    char value[MAX_PARM][PARM_MIN_SIZE]; // bool value
}BOOL_PARM_T;

typedef struct scap_parm_s {
    int cpu;                    // slave cpu number: 1->zigbee; 2->bt; 4->subg
    char ssid[PARM_MIN_SIZE];   // slave ssid
    int channel;                // channel
    int num;                    // num 
    int rssi;                   // rssi 
}SCAP_PARM_T;

enum {
    E_GEN_SWITCH_RELAY, // relay
};

typedef struct gen_switch_parm_s
{
    int type;  // switch type
    int state; // switch type
}GEN_SWITCH_PARM_T;

enum {
    E_POWER_CAPACITY, // capacity
    E_POWER_VOLTAGE,  // voltage
    E_POWER_CHARGER,  //charger
};

enum{
    E_BATTERY_GET_VALUE, //batterty get value
    E_BATTERY_SET_INFO,  // battery set info
    E_BATTERY_GET_INFO,  // battery get info
};

typedef struct battery_parm_s {
    int type;                        // type
    int value;                       // value
    char b_factory[PARM_MAX_SIZE];   // battery factory
    char b_type[PARM_MIN_SIZE];      // battery type
    int b_capacity;                  // battery capacity
    char b_info[PARM_MAX_SIZE*2];    // battery information
}BATTERY_PARM_T;

typedef struct wireless_info_s {
    int8_t  rssi;                   //rssi                 
    uint8_t ssid[PARM_MIN_SIZE];    // ssid
}WIRELESS_INFO_T;

typedef struct wireless_parm_s {
    uint8_t ssid[PARM_MIN_SIZE];  //ssid
    int channel;                  // channel
    int num;                      //num
    int scan_all;                 // scan all flag
    int totals;                   // scan totals
    WIRELESS_INFO_T info[PARM_MAX_SIZE]; // scan info
}WIRELESS_PARM_T;

typedef struct pt_hbuf_s
{
    int main_type;  // main type
    int sub_type;   // sub type
    int len;        //parameter length
    int result;     // result 
    unsigned char *data; //paramter pointer
}PT_HBUF_T;

typedef struct pt_fbuf_s
{
    int len;         //datas len
    unsigned char *buf; // datas buffer
}PT_FBUF_T;

#ifdef __cplusplus
}
#endif

#endif
