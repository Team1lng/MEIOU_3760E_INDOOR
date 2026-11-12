#ifndef _PAGE_PHOTO_PLAY_H_
#define _PAGE_PHOTO_PLAY_H_

#include <stdint.h>

void *jpeg_decode_file(const char *file_path, int *out_w, int *out_h);
void *scale_argb_bilinear(const uint8_t *src, int src_w, int src_h, int dst_w, int dst_h);
int jpeg_decode_scaled_tde(const char *file_path, uint8_t *dst, int dst_w, int dst_h);

#endif
