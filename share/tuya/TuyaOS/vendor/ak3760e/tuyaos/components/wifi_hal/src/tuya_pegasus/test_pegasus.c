#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "tuya_pegasus.h"

static TUYA_PEGASUS_CBS_S test_pegasus = {0x0};

#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define MY_DEBUUG(format,arg ...)                       \
do{                                                     \
    printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
} while (0)


int tp_get_mac(void)
{
    int ret = 0;
    NW_MAC_S *mac = {0x0};

    ret = test_pegasus.get_mac_cb(mac);
    if(ret < 0)
    {
        MY_DEBUUG("get ap mac failed");
    }

    //MY_DEBUUG("get mac success ,mac = %d-%d-%d-%d-%d-%d",mac->mac[0],mac->mac[1],mac->mac[2],
    //                                                       mac->mac[3],mac->mac[4],mac->mac[5]);
    MY_DEBUUG("get mac success!\n");
    return 0;
}

int tp_get_ssid_passwd(void)
{
    int ret = 0;

    u_int8_t ssid[32];
    u_int16_t slen = 32,plen = 32;
    u_int8_t pwd[32];

    ret = test_pegasus.get_ssid_pwd_cb(ssid,slen,pwd, plen);
    if(ret < 0)
    {
        MY_DEBUUG("get ssid & passwd failed");
    }

    MY_DEBUUG("get pass & ssid success,ssid = %s,passwd = %s",ssid,pwd);

    return 0;
}

int tp_send_mgnt(TY_FRAME_TYPE_E type)
{
    int ret = 0;
    u_int8_t dst_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    //u_int8_t src_mac[6] = {0x00,0xE0,0x4C,0x07,0xEB,0x04};
    //u_int8_t src_mac[6] = {0x84,0xE3,0x42,0x55,0x20,0x46};
    u_int8_t src_mac[6] = {0x70,0x89,0x76,0xE7,0xF8,0x68};
    NW_MAC_S dstmac,srcmac;
    memcpy(dstmac.mac,dst_mac,6);
    memcpy(srcmac.mac,src_mac,6);
    
    char vsie_beacon[] = "this is a beacon frame";
    u_int8_t vsie_beacon_len = strlen(vsie_beacon);
    char vsie_probe_request[] = "this is a probe request frame";
    u_int8_t vsie_probe_request_len = strlen(vsie_probe_request);
    char vsie_prrobe_response[] = "this is a probe response frame";
    u_int8_t vsie_prrobe_response_len = strlen(vsie_prrobe_response);

    switch(type)
    {
        case TY_FRAME_TP_BEACON:
            ret = test_pegasus.send_frame_cb(TY_FRAME_TP_BEACON,vsie_beacon,vsie_beacon_len,&srcmac,&dstmac);
            break;
        case TY_FRAME_TP_PROBE_REQ:
            ret = test_pegasus.send_frame_cb(TY_FRAME_TP_PROBE_REQ,vsie_probe_request,vsie_probe_request_len,&srcmac,&dstmac);
            break;
        case TY_FRAME_TP_PROBE_RESP:
            ret = test_pegasus.send_frame_cb(TY_FRAME_TP_PROBE_RESP,vsie_prrobe_response,vsie_prrobe_response_len,&srcmac,&dstmac);
            break;
        default:
            break;
    }

    return 0;
}

int test_pegasus_init(void)
{
    int ret = 0;
    ret = tuya_hal_pegasus_init(&test_pegasus,500);
    if(ret < 0)
    {
        MY_DEBUUG("pegasus_init failed");
    }
}

int main(void)
{
    int i = 0;
    int ret = 0;
    MY_DEBUUG("***********tuya pegasus test ---rufan***********");
    ret = test_pegasus_init();
    while(1){
        MY_DEBUUG("***************tuya pegasus test -----rufan**************** : %d",i++);
        usleep(1000);
        //ret = tp_get_mac();
        //ret = tp_get_ssid_passwd();
        ret = tp_send_mgnt(TY_FRAME_TP_BEACON);
        MY_DEBUUG("send beacon success!\n");
        usleep(1000);
        ret = tp_send_mgnt(TY_FRAME_TP_PROBE_REQ);
        MY_DEBUUG("send probe request success!\n");
        usleep(1000);
        ret = tp_send_mgnt(TY_FRAME_TP_PROBE_RESP);
        MY_DEBUUG("send probe response success!\n");
    }

    return 0;
}


