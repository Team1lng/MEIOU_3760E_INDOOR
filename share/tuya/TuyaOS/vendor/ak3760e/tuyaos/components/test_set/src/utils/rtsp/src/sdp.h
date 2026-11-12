#ifndef __SDP_H__
#define __SDP_H__


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    char              server_ip[16];
    char              video_format[8];
    char              audio_format[8];
    int               video_sample_rate;
    int               audio_sample_rate;
    int               audio_channels;
    char              sps[64];
    int               sps_len;
    char              vps[64];
    int               vps_len;
    char              pps[32];
    int               pps_len;
    int               multicast_port;
    char              multicast_ip[16];
} sdp_info_t;


int sdp_encode(sdp_info_t* psdp, char* buf, int size);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SDP_H__
