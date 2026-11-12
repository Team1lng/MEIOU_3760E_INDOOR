#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>

#include "tkl_display_drm.h"

#define FB_DEV_PATH "/dev/fb0"

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

struct drm_buffer {
	uint32_t fb_id;
	uint32_t handle;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t size;
	void *data;
};

struct drm_property_info {
	const char *name;
	uint32_t prop_id;
};

enum {
	TKL_PLANE_PROP_FB_ID = 0,
	TKL_PLANE_PROP_CRTC_ID,
	TKL_PLANE_PROP_SRC_X,
	TKL_PLANE_PROP_SRC_Y,
	TKL_PLANE_PROP_SRC_W,
	TKL_PLANE_PROP_SRC_H,
	TKL_PLANE_PROP_CRTC_X,
	TKL_PLANE_PROP_CRTC_Y,
	TKL_PLANE_PROP_CRTC_W,
	TKL_PLANE_PROP_CRTC_H,
	TKL_PLANE_PROP_END,
};

struct drm_dev {
	int fd, fb_fd;
	uint32_t conn_id, crtc_id, plane_id, crtc_idx;
	uint32_t width, height;
	uint32_t mmWidth, mmHeight;
	uint32_t format;
	uint32_t dpi;
	drmModeCrtcPtr crtc;
	drmModeConnectorPtr conn;
	drmModePlanePtr plane;
	drmModeModeInfoPtr mode;
	drmModeAtomicReqPtr req;
	uint32_t plane_props[TKL_PLANE_PROP_END];
	TKL_DISP_POWER_MODE_E power;
	TKL_DISP_ROTATION_E rotation;
	TKL_DISP_EVENT_HANDLER_S *event_cb;
	struct drm_buffer *bufs[2];
	struct drm_buffer *cur_buf, *draw_buf;
};

static struct drm_property_info tkl_plane_props[] = {
    [TKL_PLANE_PROP_FB_ID] = { .name = "FB_ID", },
    [TKL_PLANE_PROP_CRTC_ID] = { .name = "CRTC_ID", },
    [TKL_PLANE_PROP_SRC_X] = { .name = "SRC_X", },
    [TKL_PLANE_PROP_SRC_Y] = { .name = "SRC_Y", },
    [TKL_PLANE_PROP_SRC_W] = { .name = "SRC_W", },
    [TKL_PLANE_PROP_SRC_H] = { .name = "SRC_H", },
    [TKL_PLANE_PROP_CRTC_X] = { .name = "CRTC_X", },
    [TKL_PLANE_PROP_CRTC_Y] = { .name = "CRTC_Y", },
    [TKL_PLANE_PROP_CRTC_W] = { .name = "CRTC_W", },
    [TKL_PLANE_PROP_CRTC_H] = { .name = "CRTC_H", },
};

static int drm_format_to_bpp(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
		return 32;
	case DRM_FORMAT_RGB565:
		return 16;
	default:
		printf("Unknown format %d\n", format);
		return 32;
	}
}

static int drm_format_to_tkl(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_ABGR8888:
		return TKL_DISP_PIXEL_FMT_ABGR;
	case DRM_FORMAT_RGBX8888:
		return TKL_DISP_PIXEL_FMT_RGBX;
	case DRM_FORMAT_RGBA8888:
		return TKL_DISP_PIXEL_FMT_RGBA;
	case DRM_FORMAT_ARGB8888:
		return TKL_DISP_PIXEL_FMT_ARGB;
	case DRM_FORMAT_RGB565:
		return TKL_DISP_PIXEL_FMT_RGB565;
	default:
		break;
	}

	return 0;
}

char *drm_get_format_name(uint32_t format)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "%c%c%c%c %s-endian (0x%08x)", (format & 0xff), ((format >> 8) & 0xff),
		 ((format >> 16) & 0xff), ((format >> 24) & 0x7f), format & DRM_FORMAT_BIG_ENDIAN ? "big" : "little",
		 format);

	return buf;
}

static drmModeConnectorPtr find_connector_by_type(int fd, drmModeResPtr res, TKL_DISP_PORT_E type, int index)
{
	int i;

	for (i = 0; i < res->count_connectors; i++) {
		drmModeConnectorPtr connector = drmModeGetConnector(fd, res->connectors[i]);
		if (connector) {
			if (connector->connector_type_id != (index + 1))
				continue;
			if ((type == TKL_DISP_LCD) &&
			    (connector->connector_type == DRM_MODE_CONNECTOR_DPI ||
			     connector->connector_type == DRM_MODE_CONNECTOR_DSI ||
			     connector->connector_type == DRM_MODE_CONNECTOR_eDP ||
			     connector->connector_type == DRM_MODE_CONNECTOR_LVDS) &&
			    (connector->connection == DRM_MODE_CONNECTED)) {
				return connector;
			} else if ((type == TKL_DISP_HDMI) && (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA) &&
				   (connector->connection == DRM_MODE_CONNECTED)) {
				return connector;
			} else if ((type == TKL_DISP_DP) &&
				   (connector->connector_type == DRM_MODE_CONNECTOR_DisplayPort) &&
				   (connector->connection == DRM_MODE_CONNECTED)) {
				return connector;
			} else if ((type == TKL_DISP_VGA) && (connector->connector_type == DRM_MODE_CONNECTOR_VGA) &&
				   (connector->connection == DRM_MODE_CONNECTED)) {
				return connector;
			}
		}
	}

	return NULL;
}

static drmModeCrtc *find_crtc_for_connector(int fd, drmModeRes *resources, drmModeConnector *connector)
{
	// Find the encoder. If we already have one, just use it.
	drmModeEncoder *encoder;
	if (connector->encoder_id) {
		encoder = drmModeGetEncoder(fd, connector->encoder_id);
	} else {
		encoder = NULL;
	}

	int32_t crtc;
	if (encoder && encoder->crtc_id) {
		crtc = encoder->crtc_id;
		drmModeFreeEncoder(encoder);
		return drmModeGetCrtc(fd, crtc);
	}

	// Didn't find anything, try to find a crtc and encoder combo.
	crtc = -1;
	for (int i = 0; i < connector->count_encoders; i++) {
		encoder = drmModeGetEncoder(fd, connector->encoders[i]);

		if (encoder) {
			for (int j = 0; j < resources->count_crtcs; j++) {
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

static drmModePlane *find_plane_for_crtc(int fd, uint32_t crtc_idx)
{
	drmModePlaneResPtr plane_res;
	drmModePlanePtr plane;

	plane_res = drmModeGetPlaneResources(fd);
	if (!plane_res) {
		return NULL;
	}

	for (int i = 0; i < plane_res->count_planes; i++) {
		plane = drmModeGetPlane(fd, plane_res->planes[i]);
		if (plane->possible_crtcs & (1 << crtc_idx)) {
			drmModeObjectPropertiesPtr plane_props =
				drmModeObjectGetProperties(fd, plane_res->planes[i], DRM_MODE_OBJECT_PLANE);
			if (!plane_props) {
				return NULL;
			}

			for (i = 0; i < plane_props->count_props; i++) {
				drmModePropertyPtr prop = drmModeGetProperty(fd, plane_props->props[i]);
				for (int j = 0; j < TKL_PLANE_PROP_END; j++) {
					if (!strcmp(prop->name, tkl_plane_props[j].name)) {
						tkl_plane_props[j].prop_id = prop->prop_id;
						break;
					}
				}
				drmModeFreeProperty(prop);
			}
			drmModeFreeObjectProperties(plane_props);
			drmModeFreePlaneResources(plane_res);
			return plane;
		}
	}

	drmModeFreePlaneResources(plane_res);

	return NULL;
}

static struct drm_buffer *drm_create_fb(int fd, uint32_t width, uint32_t height, uint32_t format)
{
	struct drm_mode_create_dumb create_dumb = {};
	struct drm_mode_map_dumb map_dumb = {};
	struct drm_buffer *buf;
	uint32_t handles[4] = { 0 }, pitches[4] = { 0 }, offsets[4] = { 0 };
	int ret;

	buf = malloc(sizeof(struct drm_buffer));
	if (!buf) {
		printf("No free drm buffer found\n");
		return NULL;
	}

	create_dumb.height = height;
	create_dumb.width = width;
	create_dumb.bpp = drm_format_to_bpp(format);
	create_dumb.flags = 0;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
	if (ret) {
		printf("DRM_IOCTL_MODE_CREATE_DUMB failed ret=%d\n", ret);
		return NULL;
	}

	buf->handle = create_dumb.handle;
	buf->pitch = create_dumb.pitch;
	buf->width = create_dumb.width;
	buf->height = create_dumb.height;
	buf->bpp = create_dumb.bpp;
	buf->size = create_dumb.size;
	map_dumb.handle = create_dumb.handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
	if (ret) {
		printf("DRM_IOCTL_MODE_MAP_DUMB failed ret=%d\n", ret);
		return NULL;
	}

	buf->data = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_dumb.offset);
	if (buf->data == MAP_FAILED) {
		printf("mmap failed\n");
		return NULL;
	}

	memset(buf->data, 0, create_dumb.size);

	handles[0] = create_dumb.handle;
	pitches[0] = create_dumb.pitch;
	offsets[0] = 0;
	ret = drmModeAddFB2(fd, width, height, format, handles, pitches, offsets, &buf->fb_id, 0);
	if (ret) {
		printf("drmModeAddFB2 failed ret=%d\n", ret);
		return NULL;
	}

	return buf;
}

static void drm_destroy_fb(int fd, struct drm_buffer *buf)
{
	struct drm_mode_destroy_dumb dreq;

	/* unmap buffer */
	munmap(buf->data, buf->size);

	/* delete framebuffer */
	drmModeRmFB(fd, buf->fb_id);

	/* delete dumb buffer */
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = buf->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	free(buf);
}

OPERATE_RET drm_init(TKL_DISP_DEVICE_S *tdev, TKL_DISP_EVENT_HANDLER_S *event_cb)
{
	drmModeResPtr res = NULL;
	drmModeConnectorPtr conn = NULL;
	struct drm_dev *drm_dev = NULL;
	int drm_fd;
	int i = 0;

	printf("Display device init...\n");

	for (i = 0; i < DRM_MAX_MINOR; i++) {
		char *dev_name;
		int ret = asprintf(&dev_name, DRM_DEV_NAME, DRM_DIR_NAME, i);
		if (ret < 0)
			continue;

		drm_fd = open(dev_name, O_RDWR|O_CLOEXEC, 0);
		free(dev_name);
		if (drm_fd < 0)
			continue;

		drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1);

		res = drmModeGetResources(drm_fd);
		if (!res) {
			close(drm_fd);
			continue;
		}

		if (res->count_crtcs > 0 && res->count_connectors > 0) {
			conn = find_connector_by_type(drm_fd, res, tdev->device_port, tdev->device_id);
			if (conn)
				break;
		}

		drmModeFreeResources(res);
		close(drm_fd);
		res = NULL;
	}

	if (conn) {
		drm_dev = malloc(sizeof(struct drm_dev));
		if (!drm_dev) {
			drmModeFreeResources(res);
			close(drm_fd);
			return OPRT_NOT_FOUND;
		}

		drm_dev->fb_fd = open(FB_DEV_PATH, O_RDWR|O_CLOEXEC, 0);
		if (drm_dev->fb_fd < 0) {
			drmModeFreeResources(res);
			close(drm_fd);
			return OPRT_NOT_FOUND;
		}
		drm_dev->fd = drm_fd;
		drm_dev->conn = conn;
		drm_dev->conn_id = conn->connector_id;
		drm_dev->crtc = find_crtc_for_connector(drm_fd, res, conn);
		if (!drm_dev->crtc) {
			drmModeFreeResources(res);
			close(drm_dev->fb_fd);
			close(drm_fd);
			free(drm_dev);
			return OPRT_NOT_FOUND;
		}
		drm_dev->crtc_id = drm_dev->crtc->crtc_id;
		for (i = 0; i < res->count_crtcs; i++) {
			if (drm_dev->crtc_id == res->crtcs[i])
				drm_dev->crtc_idx = i;
		}
		/* Get preferred mode */
		for (int mode = 0; mode < conn->count_modes; mode++) {
			if (conn->modes[mode].type & DRM_MODE_TYPE_PREFERRED) {
				drm_dev->mode = &conn->modes[mode];
				break;
			}
		}
		/* Get Primary plane */
		drm_dev->plane = find_plane_for_crtc(drm_dev->fd, drm_dev->crtc_idx);
		if (!drm_dev->plane) {
		}
		drm_dev->plane_id = drm_dev->plane->plane_id;
		drm_dev->width = drm_dev->mode->hdisplay;
		drm_dev->height = drm_dev->mode->vdisplay;
		drm_dev->mmWidth = conn->mmWidth;
		drm_dev->mmHeight = conn->mmHeight;
		if (drm_dev->mmWidth)
			drm_dev->dpi = DIV_ROUND_UP(drm_dev->width * 25400, drm_dev->mmWidth * 1000);
		drm_dev->format = drm_dev->plane ? drm_dev->plane->formats[0] : DRM_FORMAT_ARGB8888;
		drm_dev->power = TKL_DISP_POWER_ON;
		drm_dev->rotation = TKL_DISP_ROTATION_0;
		drm_dev->bufs[0] = drm_create_fb(drm_dev->fd, drm_dev->width, drm_dev->height, drm_dev->format);
		drm_dev->bufs[1] = drm_create_fb(drm_dev->fd, drm_dev->width, drm_dev->height, drm_dev->format);
		drm_dev->req = NULL;
		drm_dev->cur_buf = NULL;
		drm_dev->draw_buf = drm_dev->bufs[0];
		drm_dev->event_cb = event_cb;
		tdev->device_info = (PVOID_T)drm_dev;

		drmModeFreeResources(res);

		drmModeSetCrtc(drm_dev->fd, drm_dev->crtc_id, drm_dev->bufs[1]->fb_id, 0, 0, &drm_dev->conn_id, 1,
			       drm_dev->mode);

		printf("Found display device, resolution %s, default pixel format %s \n",
			drm_dev->mode->name, drm_get_format_name(drm_dev->format));

		return OPRT_OK;
	}

	printf("No display device found\n");

	return OPRT_NOT_FOUND;
}

OPERATE_RET drm_deinit(TKL_DISP_DEVICE_S *tdev)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	drmModeSetCrtc(drm_dev->fd, drm_dev->crtc_id, 0, 0, 0, NULL, 0, NULL);
	drmModeFreeCrtc(drm_dev->crtc);
	drmModeFreeConnector(drm_dev->conn);
	close(drm_dev->fb_fd);
	close(drm_dev->fd);
	free(drm_dev);
	tdev->device_info = NULL;

	return 0;
}

OPERATE_RET drm_set_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;

	return OPRT_OK;
}

OPERATE_RET drm_get_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;

	if (!info)
		return OPRT_INVALID_PARM;

	if (drm_dev->rotation == TKL_DISP_ROTATION_90 || drm_dev->rotation == TKL_DISP_ROTATION_270) {
		info->width = drm_dev->height;
		info->height = drm_dev->width;
	} else {
		info->width = drm_dev->width;
		info->height = drm_dev->height;
	}
	info->format = drm_format_to_tkl(drm_dev->format);
	info->bpp = drm_format_to_bpp(drm_dev->format);
	info->fps = drm_dev->mode->vrefresh;
	info->dpi = drm_dev->dpi;

	return 0;
}

static void page_flip_complete(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec,
			       void *user_data)
{
	TKL_DISP_DEVICE_S *tdev = (TKL_DISP_DEVICE_S *)user_data;
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;

	if (drm_dev->event_cb && drm_dev->event_cb->vsync_cb)
		drm_dev->event_cb->vsync_cb(tdev->device_port, tv_sec * 1000000000 + tv_usec);
}

OPERATE_RET drm_wait_vsync(TKL_DISP_DEVICE_S *tdev)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	int ret;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(drm_dev->fd, &fds);

	do {
		ret = select(drm_dev->fd + 1, &fds, NULL, NULL, NULL);
	} while (ret == -1 && errno == EINTR);

	if (ret < 0) {
		printf("select failed: %s", strerror(errno));
		drmModeAtomicFree(drm_dev->req);
		drm_dev->req = NULL;
		return OPRT_COM_ERROR;
	}

	if (FD_ISSET(drm_dev->fd, &fds)) {
		drmEventContext evctx = {
			.version = DRM_EVENT_CONTEXT_VERSION,
			.page_flip_handler = page_flip_complete,
		};
		drmHandleEvent(drm_dev->fd, &evctx);
	}

	drmModeAtomicFree(drm_dev->req);
	drm_dev->req = NULL;

	return OPRT_OK;
}

OPERATE_RET drm_flush(TKL_DISP_DEVICE_S *tdev)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	struct drm_buffer *fbuf = drm_dev->draw_buf;
	OPERATE_RET ret = OPRT_OK;

	if (drm_dev->req)
		drm_wait_vsync(tdev);

	drm_dev->req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_FB_ID].prop_id,
				 fbuf->fb_id);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_CRTC_ID].prop_id,
				 drm_dev->crtc_id);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_SRC_X].prop_id, 0);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_SRC_Y].prop_id, 0);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_SRC_W].prop_id,
				 drm_dev->width << 16);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_SRC_H].prop_id,
				 drm_dev->height << 16);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_CRTC_X].prop_id, 0);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_CRTC_Y].prop_id, 0);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_CRTC_W].prop_id,
				 drm_dev->width);
	drmModeAtomicAddProperty(drm_dev->req, drm_dev->plane_id, tkl_plane_props[TKL_PLANE_PROP_CRTC_H].prop_id,
				 drm_dev->height);
	ret = drmModeAtomicCommit(drm_dev->fd, drm_dev->req, DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, tdev);
	if (ret) {
		printf("drmModeAtomicCommit failed: %s\n", strerror(errno));
		drmModeAtomicFree(drm_dev->req);
		return ret;
	}

	if (!drm_dev->cur_buf)
		drm_dev->draw_buf = drm_dev->bufs[1];
	else
		drm_dev->draw_buf = drm_dev->cur_buf;

	drm_dev->cur_buf = fbuf;

	return OPRT_OK;
}

static uint32_t *pixel_at(struct drm_buffer *fb, TKL_DISP_ROTATION_E rotation, int x, int y)
{
	int row_pixels = fb->pitch / (fb->bpp / 8);

	switch (rotation) {
	case TKL_DISP_ROTATION_0:
		return (uint32_t *)(fb->data) + y * row_pixels + x;
	case TKL_DISP_ROTATION_90:
		return (uint32_t *)(fb->data) + x * row_pixels + (fb->width - y - 1);
	case TKL_DISP_ROTATION_180:
		return (uint32_t *)(fb->data) + (fb->height - 1 - y) * row_pixels + (fb->width - 1 - x);
	case TKL_DISP_ROTATION_270:
		return (uint32_t *)(fb->data) + (fb->height - 1 - x) * row_pixels + y;
	default:
		printf("invalid rotation %d", rotation);
	}
	return NULL;
}

static void incr_x(TKL_DISP_ROTATION_E rotation, uint32_t **p, int row_pixels)
{
	switch (rotation) {
	case TKL_DISP_ROTATION_90:
		*p = *p + row_pixels;
		break;
	case TKL_DISP_ROTATION_180:
		*p = *p - 1;
		break;
	case TKL_DISP_ROTATION_270:
		*p = *p - row_pixels;
		break;
	default:
		*p = *p + 1;
		break;
	}
}

static void incr_y(TKL_DISP_ROTATION_E rotation, uint32_t **p, int row_pixels)
{
	switch (rotation) {
	case TKL_DISP_ROTATION_90:
		*p = *p - 1;
		break;
	case TKL_DISP_ROTATION_180:
		*p = *p - row_pixels;
		break;
	case TKL_DISP_ROTATION_270:
		*p = *p + 1;
		break;
	default:
		*p = *p + row_pixels;
		break;
	}
}

OPERATE_RET drm_blit(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *fb, TKL_DISP_RECT_S *rect)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	struct drm_buffer *fbuf = drm_dev->draw_buf;
	int pixel_bytes = fbuf->bpp / 8;
	int src_row_bytes = fb->rect.width * pixel_bytes;
	int dst_row_bytes = fbuf->pitch;
	int rotation = drm_dev->rotation;

	/* Partial update */
	if ((rect->width != drm_dev->width || rect->height != drm_dev->height) && drm_dev->cur_buf)
		memcpy(fbuf->data, drm_dev->cur_buf->data, fbuf->size);

	if (rotation) {
		int src_row_pixels = fb->rect.width;
		int row_pixels = fbuf->pitch / pixel_bytes;
		const uint32_t *src_py = (uint32_t *)fb->buffer + fb->rect.y * src_row_bytes / 4 + fb->rect.x;
		uint32_t *dst_py = pixel_at(fbuf, rotation, rect->x, rect->y);
		for (int y = 0; y < rect->height; y += 1) {
			const uint32_t *src_px = src_py;
			uint32_t *dst_px = dst_py;
			for (int x = 0; x < rect->width; x += 1) {
				*dst_px = *src_px++;
				incr_x(rotation, &dst_px, row_pixels);
			}
			src_py += src_row_pixels;
			incr_y(rotation, &dst_py, row_pixels);
		}
	} else {
		uint8_t *src_p = (uint8_t *)fb->buffer + (fb->rect.y * src_row_bytes) + (fb->rect.x * pixel_bytes);
		uint8_t *dst_p = (uint8_t *)fbuf->data + (rect->y * dst_row_bytes) + (rect->x * pixel_bytes);
		for (int i = 0; i < rect->height; ++i) {
			memcpy(dst_p, src_p, src_row_bytes);
			src_p += src_row_bytes;
			dst_p += dst_row_bytes;
		}
	}

	return OPRT_OK;
}

OPERATE_RET drm_fill(TKL_DISP_DEVICE_S *tdev, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	struct drm_buffer *fbuf = drm_dev->draw_buf;
	int row_pixels = fbuf->pitch / (fbuf->bpp / 8);

	uint32_t *p = pixel_at(fbuf, drm_dev->rotation, rect->x, rect->y);
	for (int y = rect->y; y < rect->y + rect->height; ++y) {
		uint32_t *px = p;
		for (int x = rect->x; x < rect->x + rect->width; ++x) {
			*px = color.full;
			incr_x(drm_dev->rotation, &px, row_pixels);
		}
		incr_y(drm_dev->rotation, &p, row_pixels);
	}

	return OPRT_OK;
}

TKL_DISP_FRAMEBUFFER_S *drm_alloc_fb(TKL_DISP_DEVICE_S *tdev)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	struct drm_buffer *dbuf = drm_create_fb(drm_dev->fd, drm_dev->width, drm_dev->height, drm_dev->format);
	TKL_DISP_FRAMEBUFFER_S *buf = malloc(sizeof(TKL_DISP_FRAMEBUFFER_S));

	if (!dbuf || !buf)
		return NULL;

	buf->buffer = dbuf->data;
	buf->rect.x = 0;
	buf->rect.y = 0;
	buf->rect.width = drm_dev->width;
	buf->rect.height = drm_dev->height;
	buf->format = drm_format_to_tkl(drm_dev->format);
	buf->priority = 0;

	return buf;
}

void drm_free_fb(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *buf)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	struct drm_buffer *dbuf = CNTR_OF(&buf->buffer, struct drm_buffer, data);

	if (dbuf) {
		drm_destroy_fb(drm_dev->fd, dbuf);
	}
}

OPERATE_RET drm_set_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E mode)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;
	int fb_blank = 0;

	if (!drm_dev || !drm_dev->crtc) {
		return OPRT_NOT_FOUND;
	}

	if (mode == TKL_DISP_POWER_OFF) {
		fb_blank = FB_BLANK_POWERDOWN;
	} else {
		fb_blank = FB_BLANK_UNBLANK;
	}

	int err = ioctl(drm_dev->fb_fd, FBIOBLANK, fb_blank);
	if (err < 0) {
		printf("Set power mode %d failed\n", mode);
		return OPRT_COM_ERROR;
	} else {
		drm_dev->power = mode;
	}

	return OPRT_OK;
}

OPERATE_RET drm_get_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E *power_mode)
{
	struct drm_dev *drm_dev = (struct drm_dev *)tdev->device_info;

	*power_mode = drm_dev->power;

	return OPRT_OK;
}
