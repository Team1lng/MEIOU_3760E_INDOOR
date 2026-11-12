#ifdef USE_DRM
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

struct gr_surface {
    uint32_t fb_id;
    uint32_t handle;
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
    int displayed_buffer;
    drmModeCrtc *crtc;
    drmModeConnector *connector;
    struct gr_surface *surfaces[2];
    struct gr_color color;
};

static drmModeConnector *find_connector(int fd, drmModeRes *resources, int *mode)
{
    drmModeConnector *connector;
    int i, modes;

    /* find first connected connector */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector) {
            if ((connector->count_modes > 0) &&
                (connector->connection == DRM_MODE_CONNECTED)) {
                *mode = 0;
                for (modes = 0; modes < connector->count_modes; modes++) {
                    if (connector->modes[modes].type &
                        DRM_MODE_TYPE_PREFERRED) {
                        *mode = modes;
                        break;
                    }
                }
                return connector;
            }
            drmModeFreeConnector(connector);
        }
    }

    return NULL;
}

static drmModeCrtc *find_crtc_for_connector(int fd,
                        drmModeRes *resources,
                        drmModeConnector *connector) {
    int i, j;
    drmModeEncoder *encoder;
    int32_t crtc;

    /*
     * Find the encoder. If we already have one, just use it.
     */
    if (connector->encoder_id)
        encoder = drmModeGetEncoder(fd, connector->encoder_id);
    else
        encoder = NULL;

    if (encoder && encoder->crtc_id) {
        crtc = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
        return drmModeGetCrtc(fd, crtc);
    }

    /*
     * Didn't find anything, try to find a crtc and encoder combo.
     */
    crtc = -1;
    for (i = 0; i < connector->count_encoders; i++) {
        encoder = drmModeGetEncoder(fd, connector->encoders[i]);

        if (encoder) {
            for (j = 0; j < resources->count_crtcs; j++) {
                if (!(encoder->possible_crtcs & (1 << j)))
                    continue;
                crtc = resources->crtcs[j];
                break;
            }
            if (crtc >= 0) {
                drmModeFreeEncoder(encoder);
                return drmModeGetCrtc(fd, crtc);
            }
        }
    }

    return NULL;
}

static void drm_destroy_surface(int fd, struct gr_surface *surface) {
    struct drm_gem_close gem_close;
    int ret;

    if(!surface)
        return;

    if (surface->data)
        munmap(surface->data,
               surface->row_bytes * surface->height);

    if (surface->fb_id) {
        ret = drmModeRmFB(fd, surface->fb_id);
        if (ret)
            printf("drmModeRmFB failed ret=%d\n", ret);
    }

    if (surface->handle) {
        memset(&gem_close, 0, sizeof(gem_close));
        gem_close.handle = surface->handle;

        ret = drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
        if (ret)
            printf("DRM_IOCTL_GEM_CLOSE failed ret=%d\n", ret);
    }

    free(surface);
}

static struct gr_surface *drm_create_surface(int fd, int width, int height)
{
    struct gr_surface *surface;
    struct drm_mode_create_dumb create_dumb;
    int ret;

    surface = (struct gr_surface*)calloc(1, sizeof(*surface));
    if (!surface) {
        perror("Can't allocate memory");
        return NULL;
    }

    memset(&create_dumb, 0, sizeof(create_dumb));
    create_dumb.height = height;
    create_dumb.width = width;
    create_dumb.bpp = 32;
    create_dumb.flags = 0;

    ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
    if (ret) {
        printf("DRM_IOCTL_MODE_CREATE_DUMB failed ret=%d\n",ret);
        drm_destroy_surface(fd, surface);
        return NULL;
    }
    surface->handle = create_dumb.handle;

    uint32_t handles[4], pitches[4], offsets[4];

    handles[0] = surface->handle;
    pitches[0] = create_dumb.pitch;
    offsets[0] = 0;

    ret = drmModeAddFB2(fd, width, height,
                DRM_FORMAT_ARGB8888, handles, pitches, offsets,
                &(surface->fb_id), 0);
    if (ret) {
        printf("drmModeAddFB2 failed ret=%d\n", ret);
        drm_destroy_surface(fd, surface);
        return NULL;
    }

    struct drm_mode_map_dumb map_dumb;
    memset(&map_dumb, 0, sizeof(map_dumb));
    map_dumb.handle = create_dumb.handle;
    ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
    if (ret) {
        printf("DRM_IOCTL_MODE_MAP_DUMB failed ret=%d\n",ret);
        drm_destroy_surface(fd, surface);
        return NULL;;
    }

    surface->height = height;
    surface->width = width;
    surface->row_bytes = create_dumb.pitch;
    surface->pixel_bytes = create_dumb.bpp / 8;
    surface->data = (unsigned char*)mmap(NULL,
             surface->height * surface->row_bytes,
             PROT_READ | PROT_WRITE, MAP_SHARED,
             fd, map_dumb.offset);
    if (surface->data == MAP_FAILED) {
        perror("mmap() failed");
        drm_destroy_surface(fd, surface);
        return NULL;
    }

    return surface;
}

static int drm_disable_crtc(struct gr_dev *dev)
{
    return drmModeSetCrtc(dev->fd, dev->crtc->crtc_id,
                  0, 0, 0, NULL, 0, NULL);
}

static int drm_enable_crtc(struct gr_dev *dev)
{
    if (!dev)
        return -EINVAL;

    return drmModeSetCrtc(dev->fd, dev->crtc->crtc_id,
                  dev->surfaces[dev->displayed_buffer]->fb_id,
                  0, 0,
                  &dev->connector->connector_id, 1,
                  &dev->crtc->mode);
}

static void gr_blank(struct gr_dev *dev, bool blank)
{
    if (blank)
        drm_disable_crtc(dev);
    else
        drm_enable_crtc(dev);
}

struct gr_dev *gr_init(const char *dev_name)
{
    struct gr_dev *dev;
    drmModeRes *res = NULL;
    int mode;

    dev = malloc(sizeof(struct gr_dev));
    if (!dev)
        return NULL;

    dev->fd = open(dev_name, O_RDWR);
    if (dev->fd < 0) {
        perror("cannot open card0");
        goto err_fd;
    }

    res = drmModeGetResources(dev->fd);
    if (!res) {
        perror("cannot get drm resources");
        goto err_res;
    }

    dev->connector = find_connector(dev->fd, res, &mode);
    if (!dev->connector) {
        perror("cannot find connector");
        goto err_conn;
    }

    dev->crtc = find_crtc_for_connector(dev->fd, res, dev->connector);
    if (!dev->crtc) {
        goto err_crtc;
    }
    dev->crtc->mode = dev->connector->modes[mode];

    drmModeFreeResources(res);

    dev->surfaces[0] = drm_create_surface(dev->fd, dev->crtc->mode.hdisplay, dev->crtc->mode.vdisplay);
    dev->surfaces[1] = drm_create_surface(dev->fd, dev->crtc->mode.hdisplay, dev->crtc->mode.vdisplay);
    if (!dev->surfaces[0] || !dev->surfaces[1]) {
        drm_destroy_surface(dev->fd, dev->surfaces[0]);
        drm_destroy_surface(dev->fd, dev->surfaces[1]);
        goto err_crtc;
    }

    dev->displayed_buffer = 0;

    drm_enable_crtc(dev);

    return dev;

err_crtc:
    drmModeFreeConnector(dev->connector);
err_conn:
    drmModeFreeResources(res);
err_res:
    close(dev->fd);
err_fd:
    free(dev);
    return NULL;
}

void gr_exit(struct gr_dev *dev)
{
    if (dev) {
        drm_disable_crtc(dev);
        drm_destroy_surface(dev->fd, dev->surfaces[0]);
        drm_destroy_surface(dev->fd, dev->surfaces[1]);
        drmModeFreeCrtc(dev->crtc);
        drmModeFreeConnector(dev->connector);
        close(dev->fd);
        free(dev);
    }
}

void gr_flip(struct gr_dev *dev)
{
    dev->displayed_buffer = 1 - dev->displayed_buffer;
    drmModePageFlip(dev->fd, dev->crtc->crtc_id,
            dev->surfaces[dev->displayed_buffer]->fb_id, 0, NULL);
}

unsigned int gr_get_width(struct gr_dev *dev)
{
    return dev->crtc->mode.hdisplay;
}

unsigned int gr_get_height(struct gr_dev *dev)
{
    return dev->crtc->mode.vdisplay;
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

    if (x1 > dev->crtc->mode.hdisplay)
        x1 = dev->crtc->mode.hdisplay;
    if (x2 > dev->crtc->mode.hdisplay)
        x2 = dev->crtc->mode.hdisplay;
    if (y1 > dev->crtc->mode.vdisplay)
        y1 = dev->crtc->mode.vdisplay;
    if (y2 > dev->crtc->mode.vdisplay)
        y2 = dev->crtc->mode.vdisplay;

    sf = dev->surfaces[1 - dev->displayed_buffer];
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
