#include <string.h>
#include "rtp_packager.h"

extern rtp_packager_t rtp_h264_packager;
extern rtp_packager_t rtp_g711_packager;
extern rtp_packager_t rtp_pcm_packager;
extern rtp_packager_t rtp_jpeg_packager;
extern rtp_packager_t rtp_h265_packager;
extern rtp_packager_t rtp_ps_packager;

static rtp_packager_t* g_pkgs[RTP_CODEC_MAX] = 
{
    [RTP_CODEC_H264] = &rtp_h264_packager,
    [RTP_CODEC_G711A] = &rtp_g711_packager,
    [RTP_CODEC_G711U] = &rtp_g711_packager,
    [RTP_CODEC_PCM] = &rtp_pcm_packager,
    [RTP_CODEC_PS] = &rtp_ps_packager,
    [RTP_CODEC_JPEG] = &rtp_jpeg_packager,
    [RTP_CODEC_H265] = &rtp_h265_packager,
};

static int rtp_pack_default(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    printf("default pack\n");

    return 0;
}

static int rtp_unpack_default(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm)
{
    printf("default unpack\n");

    return 0;
}

static int rtp_assemble_default(rtp_param_t* rtp_param, uint32_t marker,
            const char* pkt_buf, int pkt_len, char* fu_buf, int fu_len,
            rtp_pkt_handler handler, void* arg)
{
    rtp_header_t header = {
        .version = 2,
        .payload_type = rtp_param->payload,
        .seq = *rtp_param->pseq,
        .marker = marker,
        .timestamp = rtp_param->rtp_ts,
        .ssrc = rtp_param->ssrc,
    };

    int hdr_len = 4 * header.csrc_cnt + RTP_HDR_LEN;

    uint32_t total_len = hdr_len + pkt_len;
    if (fu_buf){
        total_len += fu_len;
    }

    char data[total_len];
    memset(data, 0, total_len);

    rtp_pack_header(&header, data, hdr_len);

    if (fu_buf){
        memcpy(data + hdr_len, fu_buf, fu_len);
    }
    memcpy(data + hdr_len + fu_len, pkt_buf, pkt_len);

    return handler(data, total_len, arg);
}

rtp_packager_t* rtp_packager_get(RTP_CODEC_E codec)
{
    if (codec <= RTP_CODEC_INVALID || codec >= RTP_CODEC_MAX)
    {
        printf("codec error: %d\n", codec);
        return NULL;
    }

    if (g_pkgs[codec] == NULL)
    {
        return NULL;
    }

    if (g_pkgs[codec]->assemble_rtp_packet == NULL)
    {
        g_pkgs[codec]->assemble_rtp_packet = rtp_assemble_default;
    }
    if (g_pkgs[codec]->pack == NULL)
    {
        g_pkgs[codec]->pack = rtp_pack_default;
    }
    if (g_pkgs[codec]->unpack == NULL)
    {
        g_pkgs[codec]->unpack = rtp_unpack_default;
    }

    return g_pkgs[codec];
}
