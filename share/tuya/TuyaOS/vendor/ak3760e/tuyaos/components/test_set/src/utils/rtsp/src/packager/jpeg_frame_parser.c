#include <string.h>
#include <stdio.h>

#include "jpeg_frame_parser.h"


typedef enum
{
    START_MARKER = 0xFF,
    SOI_MARKER   = 0xD8,
    JFIF_MARKER  = 0xE0,
    CMT_MARKER   = 0xFE,
    DQT_MARKER   = 0xDB,
    SOF_MARKER   = 0xC0,
    DHT_MARKER   = 0xC4,
    SOS_MARKER   = 0xDA,
    EOI_MARKER   = 0xD9,
    DRI_MARKER   = 0xDD
} JPEG_HEADER_MARKER_E;

typedef struct
{
    unsigned char id;
    unsigned char samp;
    unsigned char qt;
} comp_info;

static unsigned int scan_jpeg_marker(const unsigned char* data,
                                             unsigned int size,
                                             unsigned int* offset)
{
    while ((data[(*offset)++] != START_MARKER) && ((*offset) < size));

    if ((*offset) >= size) {
        return EOI_MARKER;
    } else {
        unsigned int marker;

        marker = data[*offset];
        if (marker != 0xFF){
            (*offset)++;
        }

        return marker;
    }
}

static unsigned int __jpeg_header_size(const unsigned char* data, unsigned int offset)
{
    return data[offset] << 8 | data[offset + 1];
}

static int readSOF(const unsigned char* data, unsigned int size,
                             unsigned int* offset, jpeg_frame_t* pframe)
{
    int i, j;
    comp_info elem;
    comp_info info[3] = { {0,}, };
    unsigned int sof_size, off;
    unsigned int width, height, infolen;

    off = *offset;

    /* we need at least 17 bytes for the SOF */
    if (off + 17 > size) goto wrong_size;

    sof_size = __jpeg_header_size(data, off);
    if (sof_size < 17) goto wrong_length;

    *offset += sof_size;

    /* skip size */
    off += 2;

    /* precision should be 8 */
    if (data[off++] != 8) goto bad_precision;

    /* read dimensions */
    height = data[off] << 8 | data[off + 1];
    width = data[off + 2] << 8 | data[off + 3];
    off += 4;

    if (height == 0 || height > 2040) goto invalid_dimension;
    if (width == 0 || width > 2040) goto invalid_dimension;

    pframe->width = width / 8;
    pframe->height = height / 8;

    /* we only support 3 components */
    if (data[off++] != 3) goto bad_components;

    infolen = 0;
    for (i = 0; i < 3; i++) {
        elem.id = data[off++];
        elem.samp = data[off++];
        elem.qt = data[off++];

        /* insertion sort from the last element to the first */
        for (j = infolen; j > 1; j--) {
            if (info[j - 1].id < elem.id) break;
            info[j] = info[j - 1];
        }
        info[j] = elem;
        infolen++;
    }

    /* see that the components are supported */
    if (info[0].samp == 0x21) {
        pframe->type = 0;
    } else if (info[0].samp == 0x22) {
        pframe->type = 1;
    } else {
        goto invalid_comp;
    }

    if (!(info[1].samp == 0x11)) goto invalid_comp;
    if (!(info[2].samp == 0x11)) goto invalid_comp;
    if (info[1].qt != info[2].qt) goto invalid_comp;

    return 0;

    /* ERRORS */
wrong_size:
    printf("Wrong SOF size\n");
    return -1;

wrong_length:
    printf("Wrong SOF length\n");
    return -1;

bad_precision:
    printf("Bad precision\n");
    return -1;

invalid_dimension:
    printf("Invalid dimension\n");
    return -1;

bad_components:
    printf("Bad component\n");
    return -1;

invalid_comp:
    printf("Invalid component\n");
    return -1;
}

static unsigned int readDQT(const unsigned char* data,
                                      unsigned int size,
                                      unsigned int offset, jpeg_frame_t* pframe)
{
    unsigned int quant_size, tab_size;
    unsigned char prec;
    unsigned char id;

    if (offset + 2 > size) goto too_small;

    quant_size = __jpeg_header_size(data, offset);
    if (quant_size < 2) goto small_quant_size;

    /* clamp to available data */
    if (offset + quant_size > size) {
        quant_size = size - offset;
    }

    offset += 2;
    quant_size -= 2;

    while (quant_size > 0) {
        /* not enough to read the id */
        if (offset + 1 > size) break;

        id = data[offset] & 0x0f;
        if (id == 15) goto invalid_id;

        prec = (data[offset] & 0xf0) >> 4;
        if (prec) {
            tab_size = 128;
            pframe->q_table_len = 128 * 2;
        } else {
            tab_size = 64;
            pframe->q_table_len = 64 * 2;
        }

        /* there is not enough for the table */
        if (quant_size < tab_size + 1) goto no_table;

        //printf("Copy quantization table: %u, %u\n", id, tab_size);
        memcpy(&pframe->q_tables[id * tab_size], &data[offset + 1], tab_size);

        tab_size += 1;
        quant_size -= tab_size;
        offset += tab_size;
    }

done:
    return offset + quant_size;

    /* ERRORS */
too_small:
    printf("DQT is too small\n");
    return size;

small_quant_size:
    printf("Quantization table is too small\n");
    return size;

invalid_id:
    printf("Invalid table ID\n");
    goto done;

no_table:
    printf("table doesn't exist\n");
    goto done;
}

static int readDRI(const unsigned char* data,
                             unsigned int size, unsigned int* offset, jpeg_frame_t* pframe)
{
    unsigned int dri_size, off;

    off = *offset;

    /* we need at least 4 bytes for the DRI */
    if (off + 4 > size) goto wrong_size;

    dri_size = __jpeg_header_size(data, off);
    if (dri_size < 4) goto wrong_length;

    *offset += dri_size;
    off += 2;

    pframe->restart_interval = (data[off] << 8) | data[off + 1];

    return 0;

wrong_size:
    return -1;

wrong_length:
    *offset += dri_size;
    return -1;
}

int jpeg_frame_parse(uint8_t* data, uint32_t size, jpeg_frame_t* pframe)
{
    memset(pframe, 0, sizeof(jpeg_frame_t));
    pframe->qfactor = 255;

    unsigned int offset = 0;
    unsigned int dqtFound = 0;
    unsigned int sosFound = 0;
    unsigned int sofFound = 0;
    unsigned int driFound = 0;
    unsigned int jpeg_header_size = 0;

    while ((sosFound == 0) && (offset < size)) {
        switch (scan_jpeg_marker(data, size, &offset)) {
        case JFIF_MARKER:
        case CMT_MARKER:
        case DHT_MARKER:
            offset += __jpeg_header_size(data, offset);
            break;
        case SOF_MARKER:
            if (readSOF(data, size, &offset, pframe) != 0) {
                goto invalid_format;
            }
            sofFound = 1;
            break;
        case DQT_MARKER:
            offset = readDQT(data, size, offset, pframe);
            dqtFound = 1;
            break;
        case SOS_MARKER:
            sosFound = 1;
            jpeg_header_size = offset + __jpeg_header_size(data, offset);
            break;
        case EOI_MARKER:
            /* EOI reached before SOS!? */
            printf("EOI reached before SOS!?\n");
            break;
        case SOI_MARKER:
            //printf("SOI found\n");
            break;
        case DRI_MARKER:
            printf("DRI found\n");
            if (readDRI(data, size, &offset, pframe) == 0) {
                driFound = 1;
            }
            break;
        default:
            break;
        }
    }
    if ((dqtFound == 0) || (sofFound == 0)) {
        goto unsupported_jpeg;
    }

    if (pframe->width == 0 || pframe->height == 0) {
        goto no_dimension;
    }

    pframe->scan_data = data + jpeg_header_size;
    pframe->scan_data_len = size - jpeg_header_size;

    if (driFound == 1) {
        pframe->type += 64;
    }

    return 0;

    /* ERRORS */
unsupported_jpeg:
    return -1;

no_dimension:
    return -1;

invalid_format:
    return -1;
}