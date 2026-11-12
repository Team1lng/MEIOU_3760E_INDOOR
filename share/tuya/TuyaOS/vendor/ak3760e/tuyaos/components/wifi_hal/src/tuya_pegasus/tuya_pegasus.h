#ifndef __TUYA_PEGASUS_H__
#define __TUYA_PEGASUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIFI_SUPPORT_PEGASUS
#define WIFI_SUPPORT_PEGASUS
#endif

#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif

#include "tuya_os_adapter.h"
#include "tuya_wifi_hal.h" 
#include "librawsocket.h"
#include "common.h"

#define TEST_OUI        "\x04\x8d\x38"
#define PROTO_OUI       "\x04\x88\xf0"
#define TUYA_OUI        "\x68\x57\x2d"
#define DEFAULT_OUI     TUYA_OUI

#ifndef WIFI_CONFIG
#define WIFI_CONFIG "/tuya/data/etc/wpa_supplicant.conf"
#endif

#ifndef PEGASUS_DEBUG
#define PEGASUS_DEBUG
#endif

#define MAX_PRR_BUF     (1024 * 10)
#define MAX_PRR_DEV_ID  (16)

#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define P_DEBUG(format,arg ...)                       \
do{                                                     \
    printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
} while (0)

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef TRUE
#define TRUE   1
#define FALSE  0
#endif

#ifndef WLAN_DEV
#define WLAN_DEV "wlan0"
#endif

typedef struct
{
    unsigned char   buf[MAX_PRR_BUF];
    unsigned int    sz;
    unsigned int    len;
}prr_buf_t;

typedef enum {
    TY_FRAME_TP_BEACON = 1,
    TY_FRAME_TP_PROBE_REQ = 2,
    TY_FRAME_TP_PROBE_RESP = 3,
    TY_FRAME_TP_INVALD
} TY_FRAME_TYPE_E;

#if 0
typedef enum
{
    VSIE_FRAME_TYPE_BEACON = 0,
    VSIE_FRAME_TYPE_PROBEREQ,
    VSIE_FRAME_TYPE_PROBERSP,
    VSIE_FRAME_TYPE_MAX,
}vsie_frame_type_e;
#endif

typedef struct {
    u_int8_t       it_version;
    u_int8_t       it_pad;
    u_int16_t      it_len;
    u_int32_t      it_present;
} __attribute__((__packed__)) radiotap_header;

typedef struct {
    u_int16_t        frame_control;
    u_int16_t        duration_id;
    u_int8_t         dst[ETH_ALEN];
    u_int8_t         src[ETH_ALEN];
    u_int8_t         bssid[ETH_ALEN];
    u_int16_t        seq_ctrl;
} __attribute__((__packed__)) ieee80211_header;

typedef struct {
    u_int8_t    dsap;
    u_int8_t    ssap;
    u_int8_t    control;
    u_int8_t    code[3];
    u_int16_t   type;
} __attribute__((__packed__)) llc_header;

typedef struct {
    radiotap_header         rtp;
    u_int8_t                rate;   /* on send we fill only rate field in this example */
    u_int32_t               reserved;
    ieee80211_header        wlan;
    llc_header              snap;
    char                    *payload;
} __attribute__((__packed__)) wlan_packet;

typedef enum {
    TY_PEGASUS_START = 1,
    TY_PEGASUS_STOP = 2,
    TY_PEGASUS_GET_PROBE_START = 3,
    TY_PEGASUS_EVENT_INVALD
} TY_PEGASUS_EVENT_E;

/****************************** callback functions ********************************/
typedef OPERATE_RET (*TY_PEGASUS_GET_MAC_CB)(OUT NW_MAC_S *mac);
typedef OPERATE_RET (*TY_PEGASUS_GET_SSID_PWD_CB)(OUT UINT8_T *ssid, IN INT_T slen, OUT UINT8_T *pwd, IN INT_T plen);
typedef OPERATE_RET (*TY_PEGASUS_SET_OUI_CB)(IN UINT8_T *oui, IN UINT8_T oui_len);
typedef OPERATE_RET (*TY_PEGASUS_SEND_FRAME_CB)(IN CONST TY_FRAME_TYPE_E type, IN CONST UINT8_T *vsie, IN CONST UINT_T vsie_len, IN NW_MAC_S *srcmac, IN NW_MAC_S *dstmac);
typedef void (*TY_PEGASUS_EVENT_CB)(IN CONST TY_PEGASUS_EVENT_E event);

typedef struct {
    TY_PEGASUS_EVENT_CB         event_cb;
    TY_PEGASUS_SEND_FRAME_CB    send_frame_cb;
    TY_PEGASUS_GET_SSID_PWD_CB  get_ssid_pwd_cb;
    TY_PEGASUS_GET_MAC_CB       get_mac_cb;
    TY_PEGASUS_SET_OUI_CB       set_oui_cb;
} TUYA_PEGASUS_CBS_S;

__attribute__((__packed__)) struct ieee80211_mgmt{
	le16 frame_control;
	le16 duration;
	u8 da[6];
	u8 sa[6];
	u8 bssid[6];
	le16 seq_ctrl;
	union {
		struct {
			u8 timestamp[8];
			le16 beacon_int;
			le16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params, TIM */
			u8 variable[];
		} STRUCT_PACKED beacon;
		/* probe_req: only variable items: SSID, Supported rates */
        struct {
			u8 ssid;
			u8 ssid_len;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params*/
			u8 variable[];
		} STRUCT_PACKED probe_req;
		struct {
			u8 timestamp[8];
			le16 beacon_int;
			le16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params */
			u8 variable[];
		} STRUCT_PACKED probe_resp;
	} u;
};

#ifdef WIFI_SUPPORT_PEGASUS

/**
* @brief    Initialize the pegasus function and register the required callbacks.
* 
* @param[OUT]       p_cbs
* @param[IN]        get_probe_intr_ms
* @return  0: success  Other: fail
*/
OPERATE_RET tuya_hal_pegasus_init(TUYA_PEGASUS_CBS_S *p_cbs, UINT_T get_probe_intr_ms);

/**
* @brief    get ap's mac address
* 
* @param[OUT]       mac
* @return  0: success  Other: fail
*/
int tuya_wifi_hal_pegasus_get_mac_cb(NW_MAC_S *mac);

/**
* @brief    pegasus event report
* 
* @param[OUT]       event
* @return  0: success  Other: fail
*/
void tuya_wifi_hal_event_cb(CONST TY_PEGASUS_EVENT_E event);

/**
* @brief    Set TUYA OUI
* 
* @param[IN]       oui
* @param[IN]       oui_len
* @return  0: success  Other: fail
*/
int tuya_wifi_hal_pegasus_set_oui_cb(UINT8_T *oui, UINT8_T oui_len);

/**
* @brief    Get Ap's ssid and pwd
* 
* @param[OUT]       ssid
* @param[IN]        slen   : ssid length
* @param[OUT]       pwd
* @param[IN]        plen   : pwd length
* @return  0: success  Other: fail
*/
OPERATE_RET tuya_wifi_hal_pegasus_get_ssid_pwd_cb(UINT8_T *ssid, INT_T slen, UINT8_T *pwd, INT_T plen);

/**
* @brief    send probe response,beacon,probe request frame
*
* @param[IN]        type   : frame type
* @param[IN]        vsie
* @param[IN]        vsie_len
* @param[IN]        src_mac
* @param[IN]        dst_mac
* @return  0: success  Other: fail
*/
OPERATE_RET tuya_wifi_hal_pegasus_send_frame_cb(CONST TY_FRAME_TYPE_E type, CONST UINT8_T *vsie, 
                            CONST UINT_T vsie_len, NW_MAC_S *srcmac, NW_MAC_S *dstmac);

/**
* @brief    Report the Probe Request frame of the device which will be configured to the network
* 
* @param[OUT]       vsie
* @param[IN]        vsie_len
* @param[OUT]       srcmac
* @param[IN]        dmac
* @return    0: success  Other: fail
*/
OPERATE_RET tuya_wifi_hal_pegasus_rept_probe(u_int8_t *vsie,u_int32_t *vsie_len, NW_MAC_S *smac, NW_MAC_S *dmac);

/**
* @brief    the secondary configure network
* 
* @param[IN]       time-out period
* @return       0: success  Other: fail
*/
int tuya_wifi_hal_pegasus_server_second_start(uint32_t timeout_s);

#endif


#ifdef __cplusplus
}
#endif

#endif

