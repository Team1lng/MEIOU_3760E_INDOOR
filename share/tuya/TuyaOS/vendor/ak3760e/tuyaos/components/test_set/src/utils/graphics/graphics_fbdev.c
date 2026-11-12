#ifdef USE_FBDEV

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>

struct gr_surface {
    int width;
    int height;
    int row_bytes;
    int pixel_bytes;
    unsigned char* data;
};

struct gr_color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct gr_dev {
    int fd;
    bool double_buffered;
    int displayed_buffer;
    struct fb_fix_screeninfo fi;
    struct fb_var_screeninfo vi;
    struct gr_surface surfaces[2];
    struct gr_color color;
};

static void gr_blank(struct gr_dev *dev, bool blank)
{
    int ret;

    if (!dev)
        return;

    ret = ioctl(dev->fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (ret < 0)
        perror("ioctl(): blank");
}

static void set_displayed_framebuffer(struct gr_dev *dev, unsigned n)
{
    if (n > 1 || !dev || !dev->double_buffered)
        return;

    dev->vi.yres_virtual = dev->surfaces[0].height * 2;
    dev->vi.yoffset = n * dev->surfaces[0].height;
    dev->vi.bits_per_pixel = dev->surfaces[0].pixel_bytes * 8;
    if (ioctl(dev->fd, FBIOPUT_VSCREENINFO, &dev->vi) < 0) {
        perror("active fb swap failed");
    }
    dev->displayed_buffer = n;
}

struct gr_dev *gr_init(const char *dev_name)
{
    struct gr_dev *dev;
    void *bits;

    dev = malloc(sizeof(struct gr_dev));
    if (!dev)
        return NULL;

    dev->fd = open(dev_name, O_RDWR);
    if (dev->fd < 0) {
        perror("cannot open fb0");
        goto err_fd;
    }

    if (ioctl(dev->fd, FBIOGET_FSCREENINFO, &dev->fi) < 0) {
        perror("failed to get fb0 info");
        goto err_info;
    }

    if (ioctl(dev->fd, FBIOGET_VSCREENINFO, &dev->vi) < 0) {
        perror("failed to get fb0 info");
        goto err_info;
    }

    printf("fb0 reports (possibly inaccurate):\n"
           "  vi.bits_per_pixel = %d\n"
           "  vi.red.offset   = %3d   .length = %3d\n"
           "  vi.green.offset = %3d   .length = %3d\n"
           "  vi.blue.offset  = %3d   .length = %3d\n",
           dev->vi.bits_per_pixel,
           dev->vi.red.offset, dev->vi.red.length,
           dev->vi.green.offset, dev->vi.green.length,
           dev->vi.blue.offset, dev->vi.blue.length);

    bits = mmap(0, dev->fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        goto err_info;
    }

    memset(bits, 0, dev->fi.smem_len);

    dev->surfaces[0].width = dev->vi.xres;
    dev->surfaces[0].height = dev->vi.yres;
    dev->surfaces[0].row_bytes = dev->fi.line_length;
    dev->surfaces[0].pixel_bytes = dev->vi.bits_per_pixel / 8;
    dev->surfaces[0].data = bits;

    /* check if we can use double buffering */
    if (dev->vi.yres * dev->fi.line_length * 2 <= dev->fi.smem_len) {
        dev->double_buffered = true;

        memcpy(dev->surfaces+1, dev->surfaces, sizeof(struct gr_surface));
        dev->surfaces[1].data = dev->surfaces[0].data +
            dev->surfaces[0].height * dev->surfaces[0].row_bytes;
    } else {
        dev->double_buffered = false;
    }

    set_displayed_framebuffer(dev, 0);

    gr_blank(dev, true);
    gr_blank(dev, false);

    return dev;

err_info:
    close(dev->fd);
err_fd:
    free(dev);
    return NULL;
}

void gr_exit(struct gr_dev *dev)
{
    if (dev) {
        close(dev->fd);
        free(dev);
    }
}

void gr_flip(struct gr_dev *dev)
{
    if (dev->double_buffered) {
        set_displayed_framebuffer(dev, 1 - dev->displayed_buffer);
    }
}

unsigned int gr_get_width(struct gr_dev *dev)
{
    return dev->vi.xres;
}

unsigned int gr_get_height(struct gr_dev *dev)
{
    return dev->vi.yres;
}

void gr_color(struct gr_dev *dev, uint8_t r,  uint8_t g,  uint8_t b,  uint8_t a)
{
    if (!dev)
        return;

    dev->color.r = r;
    dev->color.g = g;
    dev->color.b = b;
    dev->color.a = a;
}

void gr_fill(struct gr_dev *dev, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
    struct gr_surface *sf;
    uint8_t *p, *px;
    int x, y;

    if (!dev)
        return;

    if (x1 > dev->vi.xres)
        x1 = dev->vi.xres;
    if (x2 > dev->vi.xres)
        x2 = dev->vi.xres;
    if (y1 > dev->vi.yres)
        y1 = dev->vi.yres;
    if (y2 > dev->vi.yres)
        y2 = dev->vi.yres;

    sf = &dev->surfaces[1 - dev->displayed_buffer];
    p = sf->data  + y1 * sf->row_bytes + x1 * sf->pixel_bytes;
    if (dev->color.a == 255) {
        for (y = y1; y < y2; ++y) {
            px = p;
            for (x = x1; x < x2; ++x) {
                *px++ = dev->color.r;
                *px++ = dev->color.g;
                *px++ = dev->color.b;
                px++;
            }
            p += sf->row_bytes;
        }
    } else {
        for (y = y1; y < y2; ++y) {
            px = p;
            for (x = x1; x < x2; ++x) {
                *px = (*px * (255-dev->color.a) + dev->color.r * dev->color.a) / 255;
                ++px;
                *px = (*px * (255-dev->color.a) + dev->color.g * dev->color.a) / 255;
                ++px;
                *px = (*px * (255-dev->color.a) + dev->color.b * dev->color.a) / 255;
                ++px;
                ++px;
            }
            p += sf->row_bytes;
        }
    }
}

#endif
