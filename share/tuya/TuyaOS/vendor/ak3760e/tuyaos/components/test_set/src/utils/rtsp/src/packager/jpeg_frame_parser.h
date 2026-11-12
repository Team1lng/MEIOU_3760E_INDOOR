#ifndef __JPEG_FRAME_PARSER_H__
#define __JPEG_FRAME_PARSER_H__

#include <stdint.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t type;
    uint8_t precision;
    uint8_t qfactor;
    uint16_t restart_interval;
    uint8_t q_tables[128 * 2];
    uint16_t q_table_len;
    uint8_t* scan_data;
    uint32_t scan_data_len;
} jpeg_frame_t;

int jpeg_frame_parse(uint8_t* data, uint32_t size, jpeg_frame_t* pframe);

#endif