#ifndef __TUYA_FBDEV_H__
#define __TUYA_FBDEV_H__
#include <stdint.h>

struct gr_dev;

struct gr_dev *gr_init(const char *dev_name);
void gr_exit(struct gr_dev *dev);
unsigned int gr_get_width(struct gr_dev *dev);
unsigned int gr_get_height(struct gr_dev *dev);
void gr_color(struct gr_dev *dev, uint8_t r,  uint8_t g,  uint8_t b,  uint8_t a);
void gr_fill(struct gr_dev *dev, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
void gr_flip(struct gr_dev *dev);
#endif
