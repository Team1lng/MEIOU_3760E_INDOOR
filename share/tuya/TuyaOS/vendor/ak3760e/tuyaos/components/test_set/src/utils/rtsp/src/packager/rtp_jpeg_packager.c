#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "rtp.h"
#include "rtp_packager.h"
#include "jpeg_frame_parser.h"

extern rtp_packager_t rtp_jpeg_packager;

static int rtp_pack_jpeg_header(jpeg_frame_t* pframe, uint32_t offset, uint8_t** p_jpeg_header, uint32_t* p_jpeg_header_len)
{
    uint32_t jpeg_header_len = 8;
    if (pframe->type >= 64 && pframe->type <= 127) {
        jpeg_header_len += 4;
    }
    if (offset == 0 && pframe->qfactor >= 128) {
        jpeg_header_len += 4 + pframe->q_table_len;
    }

    uint8_t* jpeg_header = malloc(jpeg_header_len);
    if (NULL == jpeg_header){
        printf("malloc failed\n");
        return -1;
    }
    memset(jpeg_header, 0 , jpeg_header_len);
    int pos = 0;

    jpeg_header[pos++] = 0; // Type-specific
    jpeg_header[pos++] = offset >> 16;
    jpeg_header[pos++] = offset >> 8;
    jpeg_header[pos++] = offset;
    jpeg_header[pos++] = pframe->type;
    jpeg_header[pos++] = pframe->qfactor;
    jpeg_header[pos++] = pframe->width;
    jpeg_header[pos++] = pframe->height;

    if (pframe->type >= 64 && pframe->type <= 127) {
        // There is also a Restart Marker Header:
        uint16_t const restartInterval = pframe->restart_interval; // should be non-zero

        jpeg_header[pos++] = restartInterval>>8;
        jpeg_header[pos++] = restartInterval&0xFF;
        jpeg_header[pos++] = 0xFF;
        jpeg_header[pos++] = 0xFF; // F=L=1; Restart Count = 0x3FFF
    }

    if (offset == 0 && pframe->qfactor >= 128) {
        // There is also a Quantization Header:
        jpeg_header[pos++] = 0; // MBZ
        jpeg_header[pos++] = pframe->precision;
        jpeg_header[pos++] = pframe->q_table_len >> 8;
        jpeg_header[pos++] = pframe->q_table_len & 0xFF;

        int i;
        for (i = 0; i < pframe->q_table_len; ++i) {
            jpeg_header[pos++] = pframe->q_tables[i];
        }
    }

    *p_jpeg_header = jpeg_header;
    *p_jpeg_header_len = jpeg_header_len;

    return 0;
}

static int rtp_jpeg_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    jpeg_frame_t frame = {0};
    int ret = jpeg_frame_parse((uint8_t*)frm_buf, (uint32_t)frm_len, &frame);
    //printf("scan_data: %p, len: %d\n", frame.scan_data, frame.scan_data_len);
    if (ret < 0){
        printf("pack jpeg failed\n");
        return -1;
    }

    int i;
    for(i = 0; i < frame.scan_data_len; i += MAX_PAYLOAD_SIZE){
        uint8_t* jpeg_header = NULL;
        uint32_t jpeg_header_len = 0;
        int ret = rtp_pack_jpeg_header(&frame, i, &jpeg_header, &jpeg_header_len);
        //printf("offset: %d, jpeg header: %p, jpeg header len: %d\n", i, jpeg_header, jpeg_header_len);
        if (ret == 0){
            int fragment_len = MAX_PAYLOAD_SIZE;
            uint32_t marker = 0;
            if (frame.scan_data_len - i < fragment_len){
                fragment_len = frame.scan_data_len - i;
                marker = 1;
            }
            rtp_jpeg_packager.assemble_rtp_packet(rtp_param, marker,
                (char*)(frame.scan_data + i), fragment_len, (char*)jpeg_header, jpeg_header_len,
                handler, arg);

            free(jpeg_header);
            (*rtp_param->pseq)++;
        }
    }

    return 0;

}



rtp_packager_t rtp_jpeg_packager = {
    .pack = rtp_jpeg_pack,
};