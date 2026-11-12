#include <stdio.h>
#include <string.h>

#include "sdp.h"
#include "byte_array_buf.h"
#include "uni_base64.h"

typedef enum {
    AVCODEC_H264,
    AVCODEC_H265,
    AVCODEC_ALAW,
    AVCODEC_ULAW,
    AVCODEC_NUM,
} avcodec_e;

int remove_h264or5_emulation_bytes(char* pout, char* pin, int in_len)
{
    int i = 0, idx = 0;
    while (i < in_len){
        if (i + 2 < in_len && pin[i] == 0 && pin[i + 1] == 0 && pin[i + 2] == 3){
            pout[idx] = pout[idx + 1] = 0;
            idx += 2;
            i += 3;
        }
        else {
            pout[idx++] = pin[i++];
        }
    }

    return idx;
}

int sdp_encode(sdp_info_t* psdp, char* buf, int size)
{
    char tmp_buf[1024];
    byte_array_buf_t byte_array_buf;
    byte_array_buf_init(&byte_array_buf, buf, size, 0);

    //version
    byte_array_buf_write_str(&byte_array_buf, "v=0\r\n");
    //origin
    snprintf(tmp_buf, sizeof(tmp_buf), "o=- 14665860 345678943 IN IP4 %s\r\n", psdp->server_ip);
    byte_array_buf_write_str(&byte_array_buf, tmp_buf);
    //session name
    byte_array_buf_write_str(&byte_array_buf, "s=Session streamed by \"Tuya RTSP Server\"\r\n");
    //time
    byte_array_buf_write_str(&byte_array_buf, "t=0 0\r\n");

    //m
    int video_rtp_type = 0;
    avcodec_e codec = AVCODEC_NUM;
    if (psdp->video_format[0] != '\0'){

        if (0 == strcmp(psdp->video_format, "H264")){
            codec = AVCODEC_H264;
            video_rtp_type = 96;
        }
        else if (0 == strcmp(psdp->video_format, "H265")){
            codec = AVCODEC_H265;
            video_rtp_type = 96;
        }
        snprintf(tmp_buf, sizeof(tmp_buf), "m=video %d RTP/AVP %d\r\n", psdp->multicast_port, video_rtp_type);
        byte_array_buf_write_str(&byte_array_buf, tmp_buf);

        //connect
        if (psdp->multicast_port == 0){
            strcpy(psdp->multicast_ip, "0.0.0.0");
        }
        snprintf(tmp_buf, sizeof(tmp_buf), "c=IN IP4 %s\r\n", psdp->multicast_ip);
        byte_array_buf_write_str(&byte_array_buf, tmp_buf);
        //bandwidth
        byte_array_buf_write_str(&byte_array_buf, "b=AS:4096\r\n");
        //a
        byte_array_buf_write_str(&byte_array_buf, "a=range:npt=0-\r\n");
        byte_array_buf_write_str(&byte_array_buf, "a=control:track0\r\n");
        snprintf(tmp_buf, sizeof(tmp_buf), "a=rtpmap:%d %s/%d\r\n",
            video_rtp_type, psdp->video_format, psdp->video_sample_rate);
        byte_array_buf_write_str(&byte_array_buf, tmp_buf);

        if (psdp->multicast_port > 0){
            byte_array_buf_write_str(&byte_array_buf, "a=rtcp-mux\r\n");
        }

        if (codec == AVCODEC_H264){
            // WEB is short for Without Emulation Bytes, namely '0x03'.
            // Refer to ITU-T-H.264[2003] Page 5 for details.
            char sps_web[64];
            int web_len = remove_h264or5_emulation_bytes(sps_web, psdp->sps, psdp->sps_len);
            if (web_len < 4){
                printf("sdp encode: encounter bad sps length\n");
                return -1;
            }
            snprintf(tmp_buf, sizeof(tmp_buf), "a=fmtp:96 packetization-mode=1; profile-level-id="
                "%02X%02X%02X; sprop-parameter-sets=", sps_web[1], sps_web[2], sps_web[3]);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            tuya_base64_encode((unsigned char*)psdp->sps, tmp_buf, psdp->sps_len);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            byte_array_buf_write_str(&byte_array_buf, ",");
            tuya_base64_encode((unsigned char*)psdp->pps, tmp_buf, psdp->pps_len);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            byte_array_buf_write_str(&byte_array_buf, "\r\n");
        }
        else if (codec == AVCODEC_H265){
            char vps_web[64];
            int web_len = remove_h264or5_emulation_bytes(vps_web, psdp->vps, psdp->vps_len);
            if (web_len < 6 + 12){
                printf("sdp encode: encounter bad vps length\n");
                return -1;
            }
            snprintf(tmp_buf, sizeof(tmp_buf), "a=fmtp:96 profile-space=%u;profile-id=%u;"
                     "tier-flag=%u;level-id=%u;interop-constraints=%02X%02X%02X%02X%02X%02X;sprop-vps=",
                     vps_web[6] >> 6,       /* general_profile_space */
                     vps_web[6] & 0x1F,     /* general_profile_idc */
                     vps_web[6] >> 5 & 0x1, /* general_tier_flag */
                     vps_web[17],           /* general_level_idc */
                     vps_web[11], vps_web[12], vps_web[13],
                     vps_web[14], vps_web[15], vps_web[16]); /* interop_constraints */
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            //vps
            tuya_base64_encode((unsigned char*)psdp->vps, tmp_buf, psdp->vps_len);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            byte_array_buf_write_str(&byte_array_buf, ";sprop-sps=");
            //sps
            tuya_base64_encode((unsigned char*)psdp->sps, tmp_buf, psdp->sps_len);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            byte_array_buf_write_str(&byte_array_buf, ";sprop-pps=");
            //pps
            tuya_base64_encode((unsigned char*)psdp->pps, tmp_buf, psdp->pps_len);
            byte_array_buf_write_str(&byte_array_buf, tmp_buf);
            byte_array_buf_write_str(&byte_array_buf, "\r\n");
        }

    }

    //audio
    int audio_rtp_type = 0;
    if (psdp->audio_format[0] != '\0'){
        if (0 == strcmp(psdp->audio_format, "PCMA")){
            audio_rtp_type = 8;
        }
        else if (0 == strcmp(psdp->audio_format, "PCMU")){
            audio_rtp_type = 0;
        }
        else {
            audio_rtp_type = 98;
        }

        if (psdp->multicast_port > 0){
            snprintf(tmp_buf, sizeof(tmp_buf), "m=audio %d RTP/AVP %d\r\n", psdp->multicast_port + 2, audio_rtp_type);
        }
        else {
            snprintf(tmp_buf, sizeof(tmp_buf), "m=audio 0 RTP/AVP %d\r\n", audio_rtp_type);
        }
        byte_array_buf_write_str(&byte_array_buf, tmp_buf);

        snprintf(tmp_buf, sizeof(tmp_buf), "a=rtpmap:%d %s/%d/%d\r\n",
            audio_rtp_type, psdp->audio_format, psdp->audio_sample_rate, psdp->audio_channels);
        byte_array_buf_write_str(&byte_array_buf, tmp_buf);

        if (psdp->multicast_port > 0){
            byte_array_buf_write_str(&byte_array_buf, "a=rtcp-mux\r\n");
        }
        byte_array_buf_write_str(&byte_array_buf, "a=control:track1\r\n");
    }

    return byte_array_buf_data_len(&byte_array_buf);
}