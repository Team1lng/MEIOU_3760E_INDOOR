#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/time.h>
#include "common.h"
#include "tuya_pegasus.h"

static const u_int8_t rfc1042_header[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

char G_TUYA_OUI[3] = {0x68,0x57,0x2d};         //default tuya_oui
static volatile BOOL_T G_PEGASUS_START = FALSE;
static volatile TY_PEGASUS_EVENT_E G_PEGASUS_EVENT = 0;

static pthread_t report_probe_request;

/************************Register the callback function*********************/
static TUYA_PEGASUS_CBS_S s_pegasus_cbs = {
        .event_cb = tuya_wifi_hal_event_cb,
        .send_frame_cb = tuya_wifi_hal_pegasus_send_frame_cb,
        .get_ssid_pwd_cb = tuya_wifi_hal_pegasus_get_ssid_pwd_cb,
        .get_mac_cb = tuya_wifi_hal_pegasus_get_mac_cb,
        .set_oui_cb = tuya_wifi_hal_pegasus_set_oui_cb,
};

#ifdef WIFI_SUPPORT_PEGASUS
/*********************************pegasus***********************************/
void hexdump (char *desc, void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

#if 0
void __dump_80211_packet(char* pkt, size_t len)
{
    radiotap_header *rtp = (radiotap_header *)pkt;
    ieee80211_header *wlan = (ieee80211_header *)(pkt+rtp->it_len);
    llc_header *llc = NULL;
    if (wlan->frame_control == 0x0080 || wlan->frame_control == 0x0050 || wlan->frame_control ==0x0040)
    {
	llc = (llc_header *)(pkt+rtp->it_len+sizeof(ieee80211_header));
	if( htons(llc->type) == 0x1234 ) {
		size_t headers_size = rtp->it_len +
			sizeof(ieee80211_header) +
			sizeof(llc_header);
		printf("\nRADIOTAP: vers:0x%02X, len: %d, flags: 0x%04X\n",
		rtp->it_version, rtp->it_len, rtp->it_present);
		printf("IEEE802.11 src=%02x:%02x:%02x:%02x:%02x:%02x, "
		"dst=%02x:%02x:%02x:%02x:%02x:%02x bssid==%02x:%02x:%02x:%02x:%02x:%02x\n",
		wlan->src[0],wlan->src[1],wlan->src[2],wlan->src[3],wlan->src[4],wlan->src[5],
		wlan->dst[0],wlan->dst[1],wlan->dst[2],wlan->dst[3],wlan->dst[4],wlan->dst[5],
		wlan->bssid[0],wlan->bssid[1],wlan->bssid[2],wlan->bssid[3],wlan->bssid[4],wlan->bssid[5]);
		printf("LLC: type 0x%04X\n", htons(llc->type));
		hexdump("data:", pkt+headers_size, len-headers_size);
		hexdump("full packet:", pkt, len);
		return;
    }
    /* Data with not our link type - simple show asterisk */
    printf("*");
    } else {
    /* No data (management) frame - simple show dot */
    printf("!");
    }
    fflush(stdout);
}
#endif

void dump_80211_packet(char* pkt, size_t len)
{
    struct ieee80211_mgmt *tmp_buf = pkt;
    if (tmp_buf->frame_control == 0x0080 || tmp_buf->frame_control == 0x0050 || tmp_buf->frame_control ==0x0040)
    {
		size_t headers_size = len;

		printf("IEEE802.11 src=%02x:%02x:%02x:%02x:%02x:%02x, "
		"dst=%02x:%02x:%02x:%02x:%02x:%02x bssid==%02x:%02x:%02x:%02x:%02x:%02x\n",
		tmp_buf->sa[0],tmp_buf->sa[1],tmp_buf->sa[2],tmp_buf->sa[3],tmp_buf->sa[4],tmp_buf->sa[5],
		tmp_buf->da[0],tmp_buf->da[1],tmp_buf->da[2],tmp_buf->da[3],tmp_buf->da[4],tmp_buf->da[5],
		tmp_buf->bssid[0],tmp_buf->bssid[1],tmp_buf->bssid[2],tmp_buf->bssid[3],tmp_buf->bssid[4],tmp_buf->bssid[5]);

		//hexdump("data:", pkt+headers_size, len-headers_size);
		hexdump("full packet:", pkt, len);
		return;
    }else {
        /* No data (management) frame - simple show dot */
        printf("!");
    }
    fflush(stdout);
}

#if 0
int ty_net_hal_pegasus_send_mgnt(TY_FRAME_TYPE_E frame_type,unsigned char srcmac[6],unsigned char dstmac[6],
                             unsigned char bssid[6],CONST UINT8_T *vsiebuf, CONST UINT_T vsie_len)
{
    int ret = 0;
    int socket_raw;
    char buffer[1024] = {0x0}; // Max packet length
    wlan_packet *send_packet = (wlan_packet *)buffer;
    char if_name[16] = WLAN_DEV;

    if(srcmac == NULL || dstmac == NULL || bssid == NULL || vsiebuf == NULL)
    {
        printf("Error : Lack of enough information\n");
        return -1;
    }
    /* -------------- create socket --------------------------*/
    if((socket_raw = create_rawsocket(ETH_P_ALL)) == -1)
        return socket_raw;

    if((ret = bind_rawsocket(if_name, socket_raw,ETH_P_ALL)) == -1)
        return ret;

    /*-----------------Construct management frame------------------*/
    /* RADIOTAP */
    send_packet->rtp.it_version = 0;
    send_packet->rtp.it_pad = 0;
    send_packet->rtp.it_len = sizeof(radiotap_header) + 1 + 4; /* only rate append, se next line */
    send_packet->rtp.it_present = 0x00028004; /* we need only rate flag here */
    send_packet->rate = 0x02; /* 1Mbit/s - need to be preprocessor macros */
    send_packet->reserved = 0x00; /* 1Mbit/s - need to be preprocessor macros */

    /*----------------- IEEE802.11 frame body --------------------*/
    switch(frame_type)
    {
        case TY_FRAME_TP_BEACON:
            send_packet->wlan.frame_control = host_to_le16(0 << 2 | 8 << 4); // beacon frame type
            send_packet->wlan.duration_id = host_to_le16(0);
            break;
        case TY_FRAME_TP_PROBE_REQ:
            send_packet->wlan.frame_control = host_to_le16(0 << 2 | 4 << 4); // probe request frame type
            break;
        case TY_FRAME_TP_PROBE_RESP:
            //send_packet->wlan.frame_control = host_to_le16(0 << 2 | 5 << 4); // probe response frame type
            send_packet->wlan.frame_control = htons(0x0050);
            break;
        default:
            break;
    }
    //send_packet->wlan.duration_id = htons(0x2005); /* 520 microseconds  */
    memcpy(send_packet->wlan.dst, dstmac, ETH_ALEN);
    memcpy(send_packet->wlan.src, srcmac, ETH_ALEN);
    memcpy(send_packet->wlan.bssid, bssid, ETH_ALEN);
    //send_packet->wlan.seq_ctrl = 0;       //hardware or lowlevel driver will set

    /* ---------------------------LLC rfc1042_header---------------------------*/
    send_packet->snap.dsap = 0xaa; /* CSAP */
    send_packet->snap.ssap = 0xaa; /* SSAP */
    send_packet->snap.control = 0x03; /* unnumbered frame */
    send_packet->snap.code[0] = 0x00; /* OUI */
    send_packet->snap.code[1] = 0x00; /* OUI */
    send_packet->snap.code[2] = 0x00; /* OUI */
    send_packet->snap.type = htons(0x1234);  /*frame type*/

    memcpy(buffer + sizeof(radiotap_header) + 1 + 4 
            + sizeof(ieee80211_header) 
            + sizeof(llc_header),vsiebuf,vsie_len);
    //ret = send_rawpacket(socket_raw, buffer, sizeof(buffer));
    ret = send_rawpacket(socket_raw, buffer,8 + 5 + 24 + 8 + vsie_len);
    dump_80211_packet(buffer, 45 + vsie_len);

    //debug
    printf("%s , %d : Send %d bytes data to network\n", __FUNCTION__,__LINE__,ret);

    return 0;
}
#endif

int ty_net_hal_pegasus_send_mgnt(TY_FRAME_TYPE_E frame_type,unsigned char srcmac[6],unsigned char dstmac[6],
                                         unsigned char bssid[6],CONST UINT8_T *vsiebuf, CONST UINT_T vsie_len)
{
    int t_size;
    u_int8_t t_i;
    u_int64_t t_timestamp;
    static u_int16_t seq = 0;
    struct ifreq t_ifr;
    struct packet_mreq t_mr;
    struct sockaddr_ll t_sll;
    int socket_raw;
    struct timeval t_time;          //timestamp
    int packet_len = 0;
    char ssid[] = "sjm112211112";
    u_int8_t* tmpBuf = NULL;

    struct ieee80211_mgmt *mgnt_packet;
    mgnt_packet = malloc(128);
    if(mgnt_packet == NULL)
    {
        printf("malloc failed \n");
        return -1;
    }
    memset(mgnt_packet,0,sizeof(mgnt_packet));

    u_int8_t* t_radiotap = (u_int8_t*)"\x00\x00\x0d\x00\x04\x80\x02\x00\x02\x00\x00\x00\x00";
    //u_int8_t rfc1042[6] = {0xaa,0xaa,0x03,0x00,0x00,0x00};

    /*-----------------------Create socket-----------------------------------*/
    socket_raw = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    if(socket_raw < 0) {
        printf("socket failed\n");
        return -1;
    }

    memset(&t_ifr,0,sizeof(t_ifr));
    strncpy(t_ifr.ifr_name,WLAN_DEV,sizeof(t_ifr.ifr_name) - 1);
    if(ioctl(socket_raw,SIOCGIFINDEX,&t_ifr) < 0) {
        printf("ioctl failed\n");
        close(socket_raw);
        return -1;
    }

    memset(&t_sll,0,sizeof(t_sll));
    t_sll.sll_family=AF_PACKET;
    t_sll.sll_ifindex=t_ifr.ifr_ifindex;
    t_sll.sll_protocol=htons(ETH_P_ALL);
    //t_sll.sll_protocol=htons(ETH_P_PAE);
    //t_sll.sll_halen = ETH_ALEN;
    //memcpy(t_sll.sll_addr,dstmac,ETH_ALEN);
    if (bind(socket_raw,(struct sockaddr*)&t_sll,sizeof(t_sll)) < 0) {
        perror("bind socket failed\n");
        close(socket_raw);
        return -1;
    }

    memset(&t_mr,0,sizeof(t_mr));
    t_mr.mr_ifindex = t_sll.sll_ifindex;
    //t_mr.mr_type=PACKET_MR_PROMISC;     // 混杂模式
    t_mr.mr_type = PACKET_HOST | PACKET_MULTICAST;
    if(setsockopt(socket_raw,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&t_mr,sizeof(t_mr)) < 0)
    {
        perror("setsockoptfailed\n");
        close(socket_raw);
        return -1;
    }

    /*-------------------------------Construct packet--------------------------------*/
    memcpy(mgnt_packet->bssid,bssid,ETH_ALEN);  // bssid
    memcpy(mgnt_packet->sa,srcmac,ETH_ALEN);    // src mac
    memcpy(mgnt_packet->da,dstmac,ETH_ALEN);    // dst mac

    switch(frame_type)
    {
        case TY_FRAME_TP_BEACON:
            mgnt_packet->frame_control = htons(0x8000); // beacon frame type
            mgnt_packet->duration = htons(0);
            mgnt_packet->seq_ctrl = htons(seq);
            gettimeofday(&t_time,0);
            t_timestamp = ((u_int64_t)t_time.tv_sec)*1000000 + t_time.tv_usec;
            for(t_i = 0;t_i < 8;t_i++){
                mgnt_packet->u.beacon.timestamp[t_i] = (u_int8_t)((t_timestamp >>  (t_i  << 3)) & 0xFF);
            }
            mgnt_packet->u.beacon.beacon_int = htons(0x6400);
            mgnt_packet->u.beacon.capab_info = htons(0x100);
            u_int8_t *Beacon_pos = &mgnt_packet->u.beacon.variable[0];

            /*-------------------------information element---------------------*/
            *Beacon_pos++ = 0;                       // element ID : ssid
            *Beacon_pos++ = strlen(ssid);            // ssid length
            memcpy(Beacon_pos,ssid,strlen(ssid));    // ssid value
            Beacon_pos += strlen(ssid);

            /*-------------------------------vsie-----------------------------*/
            *Beacon_pos++ = 221;                    // vender_specicfic
            *Beacon_pos++ = vsie_len + 3;
            *Beacon_pos++ = G_TUYA_OUI[0];          // OUI
            *Beacon_pos++ = G_TUYA_OUI[1];
            *Beacon_pos++ = G_TUYA_OUI[2];
            memcpy(Beacon_pos,vsiebuf,vsie_len);    // vsie value
            Beacon_pos += vsie_len + 3;
            packet_len = 24 + 8 + 2 + 2 + 1 + strlen(ssid) + 1 + 3 + vsie_len; //frame_head + timestamp + beacon_int + capab_info + information len + vsie_len
            break;

        case TY_FRAME_TP_PROBE_REQ:
            mgnt_packet->frame_control = htons(0x4000); // probe request frame type
            mgnt_packet->duration = 0;
            mgnt_packet->seq_ctrl = htons(seq);
            //gettimeofday(&t_time,0);
            //t_timestamp=((u_int64_t)t_time.tv_sec)*1000000 + t_time.tv_usec;
            //for(t_i = 0;t_i < 8;t_i++){
            //    mgnt_packet->u.probe_req.timestamp[t_i] = (u_int8_t)((t_timestamp >>  (t_i  << 3)) & 0xFF);
            //}
            //mgnt_packet->u.probe_req.beacon_int = htons(0x6400);
            //mgnt_packet->u.probe_req.capab_info = htons(0x100);

            /*-------------------------information element---------------------*/
            mgnt_packet->u.probe_req.ssid = 0;                     // element ID : ssid
            mgnt_packet->u.probe_req.ssid_len = 0;                     // length
            //memcpy(Req_pos,ssid,strlen(ssid));
            //Req_pos += strlen(ssid);

            u_int8_t *Req_pos = &mgnt_packet->u.probe_req.variable[0];
            /*-------------------------------vsie-----------------------------*/
            *Req_pos++ = 221;                    // vender_specicfic
            *Req_pos++ = vsie_len + 3;
            *Req_pos++ = G_TUYA_OUI[0];          // OUI
            *Req_pos++ = G_TUYA_OUI[1];
            *Req_pos++ = G_TUYA_OUI[2];
            memcpy(Req_pos,vsiebuf,vsie_len);    // vsie value
            Req_pos += vsie_len + 3;
            packet_len = 24 + 2 + 1 + 1 + 3 + vsie_len;     //frame_head + ssid + ssid_len + vsie_len + oui[3] + vsie_len
            break;

        case TY_FRAME_TP_PROBE_RESP:
            mgnt_packet->frame_control = htons(0x5000); // probe response frame type
            mgnt_packet->duration = 0;
            mgnt_packet->seq_ctrl = htons(seq);
            gettimeofday(&t_time,0);
            t_timestamp=((u_int64_t)t_time.tv_sec)*1000000 + t_time.tv_usec;
            for(t_i = 0;t_i < 8;t_i++){
                mgnt_packet->u.probe_resp.timestamp[t_i] = (u_int8_t)((t_timestamp >>  (t_i  << 3)) & 0xFF);
            }
            mgnt_packet->u.probe_resp.beacon_int = htons(0x6400);
            mgnt_packet->u.probe_resp.capab_info = htons(0x100);
            u_int8_t *Rsp_pos = &mgnt_packet->u.probe_resp.variable[0];

            /*-------------------------information element---------------------*/
            *Rsp_pos++ = 0;                     // element ID : ssid
            *Rsp_pos++ = strlen(ssid);
            memcpy(Rsp_pos,ssid,strlen(ssid));
            Rsp_pos += strlen(ssid);

            /*-------------------------------vsie-----------------------------*/
            *Rsp_pos++ = 221;                    // element ID : vender_specicfic
            *Rsp_pos++ = vsie_len + 3;
            *Rsp_pos++ = G_TUYA_OUI[0];          // OUI
            *Rsp_pos++ = G_TUYA_OUI[1];
            *Rsp_pos++ = G_TUYA_OUI[2];
            memcpy(Rsp_pos,vsiebuf,vsie_len);    // vsie value
            Rsp_pos += vsie_len + 3;
            packet_len = 24 + 12 + 1 + strlen(ssid) + 1 + 3 + vsie_len;
            break;

        default:
            break;
    }

#if 0
    tmpBuf = malloc(13 + packet_len);
    if(tmpBuf == NULL) {
        printf("malloc failed\n");
        return 0;
    }
    memset(tmpBuf,0,13 + packet_len);
    memcpy(tmpBuf,t_radiotap,13);
    memcpy(tmpBuf+13,mgnt_packet,packet_len);
#endif

    /*-----------------------------------------send packet---------------------------------------*/
    t_size=write(socket_raw,mgnt_packet,packet_len + 2);
    if(t_size < 0) {
        printf("write() failed : seng packet failed\n");
        free(tmpBuf);
        close(socket_raw);
        return -1;
    }
    printf("*****%d byte data have been sent to network by rufan*****\n",t_size);
    // ret = send_rawpacket(socket_raw, tmpBuf,13 + packet_len);

    /*--------------------------------------print the packet------------------------------------*/
    dump_80211_packet(mgnt_packet, 50);

    //free(tmpBuf);
    free(mgnt_packet);
    close(socket_raw);
    return 0;
}

int ty_net_hal_pegasus_get_ssid_pwd(UINT8_T *ssid,INT_T slen,UINT8_T *pwd,INT_T plen)
{
    int chn,ret;
    FILE *fd = NULL;
    char buffer[1024];
    char tmp_ssid[32] = {0x0};
    char tmp_pwd[32] = {0x0};
    char *opstr = NULL;
    printf("enter ty_net_hal_pegasus_get_ssid_pwd \n");
    fd = fopen(WIFI_CONFIG,"r");
    while(fgets(buffer, sizeof(buffer), fd) != NULL)  //parse wpa_supplicant.conf and get ssid and psk
    {
        //printf("%s , %d : parse wpa_supplciant.conf success \n",__FUNCTION__,__LINE__);
        opstr = strstr(buffer, "ssid=");
        if(opstr)
        {
            sscanf(opstr+strlen("ssid="),"%s",tmp_ssid);
            //printf("%s , %d : ssid = %s\n",__FUNCTION__,__LINE__,tmp_ssid);
        }
        
        opstr = strstr(buffer, "psk=");
        if(opstr)
        {
            sscanf(opstr+strlen("psk="),"%s",tmp_pwd);
            //printf("%s , %d : pwd = %s\n",__FUNCTION__,__LINE__,tmp_pwd);
        }
    }
    fclose(fd);

    //strcpy(ssid,tmp_ssid);
    //strcpy(pwd,tmp_pwd);

    memcpy(ssid,tmp_ssid,strlen(tmp_ssid)+1);
    memcpy(pwd,tmp_pwd,strlen(tmp_pwd)+1);

    return 0;
}


int ty_net_hal_pegasus_get_ap_mac(NW_MAC_S *mac)
{
    int ret;
    //ret = wifi_get_current_connect_ap_bssid(mac->mac);
    if(ret  < 0)
    {
        P_DEBUG("get bssid failed");
        //return -1;
    }
#ifdef PEGASUS_DEBUG
    P_DEBUG("get bssid success");
#endif

    return 0;
}

int ty_net_hal_pegasus_send_frame(CONST TY_FRAME_TYPE_E type, CONST UINT8_T *vsie, 
                            CONST UINT_T vsie_len, NW_MAC_S *srcmac, NW_MAC_S *dstmac)
{
    int ret = 0;
    //u_int8_t bssid[6] = {0x00,0xE0,0x4C,0x07,0xEB,0x04};
    //u_int8_t bssid[6] = {0x84,0xE3,0x42,0x55,0x20,0x46};
    u_int8_t bssid[6] = {0x70,0x89,0x76,0xE7,0xF8,0x68};
    u_int8_t src_mac[6];
    u_int8_t dst_mac[6];

    memcpy(src_mac,srcmac->mac,TY_MAC_ADDR_LEN);
    memcpy(dst_mac,dstmac->mac,TY_MAC_ADDR_LEN);

    //ret = tuya_wifi_hal_get_bssid(bssid);
    if(ret < 0)
    {
        //TN_LOGD("get bssid failed!");
        return -1;
    }

    TY_FRAME_TYPE_E frame_type_s = type;

    switch(frame_type_s)
    {
        case TY_FRAME_TP_BEACON :
            ret = ty_net_hal_pegasus_send_mgnt(TY_FRAME_TP_BEACON,src_mac,dst_mac,bssid,vsie,vsie_len);
            if(ret < 0){
                //TN_LOGD("send beacon frame failed");
                return -1;
            }
            break;

        case TY_FRAME_TP_PROBE_REQ:
            ret = ty_net_hal_pegasus_send_mgnt(TY_FRAME_TP_PROBE_REQ,src_mac,dst_mac,bssid,vsie,vsie_len);
            if(ret < 0){
                //TN_LOGD("send probe_request frame failed");
                return -1;
            }
            break;

        case TY_FRAME_TP_PROBE_RESP:
            ret = ty_net_hal_pegasus_send_mgnt(TY_FRAME_TP_PROBE_RESP,src_mac,dst_mac,bssid,vsie,vsie_len);
            if(ret < 0){
                //TN_LOGD("send probe_response frame failed");
                return -1;
            }
            break;

        default:
            break;
    }

    if(ret < 0){
        P_DEBUG("send_mgnt failed");
        return -1;
    }

    return 0;
}

OPERATE_RET tuya_hal_pegasus_init(TUYA_PEGASUS_CBS_S *p_cbs, UINT_T get_probe_intr_ms)
{
    if(p_cbs == NULL)
    {
        P_DEBUG("p_cbs is NULL");
        return -1;
    }

    memcpy(p_cbs, &s_pegasus_cbs, sizeof(TUYA_PEGASUS_CBS_S));
        
    return 0;
}

int tuya_wifi_hal_pegasus_get_mac_cb(NW_MAC_S *mac)
{
    int ret;

    printf("%s ,%d \n",__FUNCTION__,__LINE__);

    ret = ty_net_hal_pegasus_get_ap_mac(mac);
    if(ret < 0)
    {
        P_DEBUG("pagasus : get ap's mac failed");
        return -1;
    }

    return ret;
}

void tuya_wifi_hal_event_cb(CONST TY_PEGASUS_EVENT_E event)
{
    switch(event)
    {
        case TY_PEGASUS_START:
            G_PEGASUS_EVENT = TY_PEGASUS_START;
            G_PEGASUS_START = TRUE;
            break;
        case TY_PEGASUS_STOP:
            G_PEGASUS_EVENT = TY_PEGASUS_STOP;
            G_PEGASUS_START = FALSE;
            break;
        case TY_PEGASUS_GET_PROBE_START:
            G_PEGASUS_EVENT = TY_PEGASUS_GET_PROBE_START;       //get probe request
            break;
        case TY_PEGASUS_EVENT_INVALD:
            P_DEBUG("invalid event");
            break;
        default:
            P_DEBUG("Wrong Event");
            exit(-1);
    }
}

int tuya_wifi_hal_pegasus_set_oui_cb(UINT8_T *oui, UINT8_T oui_len)
{
    int ret = 0;
    if(oui == NULL)
    {
        printf("WRONG parameter !\n");
        return -1;
    }

    memcpy(G_TUYA_OUI,oui,oui_len);

    return ret;
}

OPERATE_RET tuya_wifi_hal_pegasus_get_ssid_pwd_cb(UINT8_T *ssid, INT_T slen, UINT8_T *pwd, INT_T plen)
{
    int ret = 0;

    ret = ty_net_hal_pegasus_get_ssid_pwd(ssid,slen,pwd,plen);
    if(ret < 0)
    {
        printf("get ssid and pwd failed \n");
        return -1;
    }

    return ret;
}

OPERATE_RET tuya_wifi_hal_pegasus_send_frame_cb(CONST TY_FRAME_TYPE_E type, CONST UINT8_T *vsie, 
                            CONST UINT_T vsie_len, NW_MAC_S *srcmac, NW_MAC_S *dstmac)
{
    int ret;

    ret = ty_net_hal_pegasus_send_frame(type,vsie,vsie_len,srcmac,dstmac);
    if(ret < 0)
    {
        //TN_LOGD("");
    }
}
void process_80211_packet(char *pkt,u_int32_t pkt_len)
{
    radiotap_header *rtp = (radiotap_header *)pkt;
    ieee80211_header *wlan = (ieee80211_header *)(pkt+rtp->it_len);
    llc_header *llc = NULL;
    if (wlan->frame_control == 0x0088)
    {
        llc = (llc_header *)(pkt+rtp->it_len+sizeof(ieee80211_header));
        if( htons(llc->type) == 0x1234 ) {
        size_t headers_size = rtp->it_len +
            sizeof(ieee80211_header) +
            sizeof(llc_header);

        printf("\nRADIOTAP: vers:0x%02X, len: %d, flags: 0x%04X\n",
        rtp->it_version, rtp->it_len, rtp->it_present);

        printf("IEEE802.11 src=%02x:%02x:%02x:%02x:%02x:%02x, "
        "dst=%02x:%02x:%02x:%02x:%02x:%02x bssid==%02x:%02x:%02x:%02x:%02x:%02x\n",
        wlan->src[0],wlan->src[1],wlan->src[2],wlan->src[3],wlan->src[4],wlan->src[5],
        wlan->dst[0],wlan->dst[1],wlan->dst[2],wlan->dst[3],wlan->dst[4],wlan->dst[5],
        wlan->bssid[0],wlan->bssid[1],wlan->bssid[2],wlan->bssid[3],wlan->bssid[4],wlan->bssid[5]);

        printf("LLC: type 0x%04X\n", htons(llc->type));
        hexdump("data:", pkt+headers_size, pkt_len-headers_size);
        hexdump("full packet:", pkt, pkt_len);

        return;
    }
        /* Data with not our link type - simple show asterisk */
        printf("*");
    } else {
        /* No data (management) frame - simple show dot */
        printf("!");
    }
    
    fflush(stdout);
}

int ty_net_hal_pegasus_receive_frame(UINT8_T *vsie,UINT_T *vsie_len, NW_MAC_S *smac, NW_MAC_S *dmac)
{
    int sockraw;
    int ret = 0;
    char pkt[1024];
    char *if_name = WLAN_DEV;
    u_int8_t *pos = NULL;
    u_int16_t l_vsie;
    struct ieee80211_mgmt *probe_req_frame;
    if((sockraw = create_rawsocket(ETH_P_ALL)) == -1)
        return sockraw;

    if((ret = bind_rawsocket(if_name, sockraw,ETH_P_ALL)) == -1)
        return ret;

    while(1){
        if ((ret = recv_rawpacket(sockraw, pkt, sizeof(pkt))) > -1){
            probe_req_frame = (struct ieee80211_mgmt *)pkt;
            if(probe_req_frame->frame_control == 0x0040){
                memcpy(smac->mac,probe_req_frame->sa,ETH_ALEN);
                memcpy(dmac->mac,probe_req_frame->bssid,ETH_ALEN);
                pos = (u_int8_t *)&probe_req_frame->u.probe_req.variable[0];
                pos++;
                *vsie_len = *pos;    //vsie length
                l_vsie = *pos;
                memcpy(vsie,pos,l_vsie);

                return 0;
            }
        }
        process_80211_packet(pkt, ret);
    }
}

#if 0
static void *recev_frame(void *arg)
{
    int ret;
    pthread_detach(pthread_self());

    ret = ty_net_hal_pegasus_receive_frame();
    if(ret < 0){
        P_DEBUG("recev frame failed");
        exit(-1);
    }

    return (void *)0;
}
#endif

/*----------------------上报待配网设备的 probe request --------------------------------*/
OPERATE_RET tuya_wifi_hal_pegasus_rept_probe(u_int8_t *vsie,u_int32_t *vsie_len, NW_MAC_S *smac, NW_MAC_S *dmac)
{
    int ret;
    if(vsie == NULL || smac == NULL || dmac == NULL)
    {
        P_DEBUG("WRONG parameter !");
        return -1;
    }

    if(G_PEGASUS_EVENT == TY_PEGASUS_GET_PROBE_START && G_PEGASUS_START == TRUE)
    {
#if 0
        ret = pthread_create(&report_probe_request, NULL, recev_frame, NULL);
        if(ret < 0){
            P_DEBUG("create thread failed !");
            return -1;
        }
#endif
        ret = ty_net_hal_pegasus_receive_frame(vsie,vsie_len,smac,dmac);
        if(ret < 0)
        {
            P_DEBUG("recv frame failed");
            return -1;
        }
    }

    return 0;
}

int tuya_wifi_hal_pegasus_server_second_start(uint32_t timeout_s)
{
    int ret;
    
    usleep(timeout_s);
    
    return ret;
}

#endif

