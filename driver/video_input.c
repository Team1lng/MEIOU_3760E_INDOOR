#include "video_input.h"
#include "Amy_sdk.h"
#include "Amy_log.h"
#include "ak_common.h"

typedef struct video_input_context {
    char isp_conf[128];
    int crop_left;
    int crop_top;
    int crop_width;
    int crop_height;

    int main_width;
    int main_height;

    int sub_width;
    int sub_height;

    int fps;
    Amy_mutex_t mutex;

    int run;
    struct video_input_frame lastframe;

    void* cb_userdata;
    int(*frame_output_cb)(void* userdata, const unsigned char* data, int size, int w, int h);
}video_input_context;
static video_input_context video_dev = {
    .run = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

static int video_device_open(video_input_context* dev) {
    int ret = 0;
    ret = ak_vi_open(VIDEO_DEV0);
    if (ret) {
        AMY_LOG_ERROR("“video device open failed \n");
        return -1;
    }

   // RECTANGLE_S res;                //max sensor resolution
    VI_DEV_ATTR    dev_attr;
    memset(&dev_attr, 0, sizeof(VI_DEV_ATTR));
    dev_attr.dev_id = VIDEO_DEV0;
    dev_attr.crop.left = dev->crop_left;
    dev_attr.crop.top = dev->crop_top;
    dev_attr.crop.width = dev->crop_width;
    dev_attr.crop.height = dev->crop_height;
    dev_attr.max_width = dev->main_width;
    dev_attr.max_height = dev->main_height;
    dev_attr.sub_max_width = dev->sub_width;
    dev_attr.sub_max_height = dev->sub_height;
    ret = ak_vi_set_dev_attr(VIDEO_DEV0, &dev_attr);
    if (ret) {
        AMY_LOG_ERROR("vi device %d set device attribute failed!\n", VIDEO_DEV0);
        ak_vi_close(VIDEO_DEV0);
        return ret;
    }

    VI_CHN_ATTR chn_attr;
    memset(&chn_attr, 0, sizeof(VI_CHN_ATTR));
    chn_attr.chn_id = VIDEO_CHN0;
    chn_attr.res.width = dev->main_width;
    chn_attr.res.height = dev->main_height;
    chn_attr.frame_depth = 2;
    /*disable frame control*/
    chn_attr.frame_rate = 0;
    ret = ak_vi_set_chn_attr(VIDEO_CHN0, &chn_attr);
    if (ret) {
        AMY_LOG_ERROR("vi device %d set channel [%d] attribute failed!\n", VIDEO_DEV0, VIDEO_CHN0);
        ak_vi_close(VIDEO_DEV0);
        return ret;
    }
    ret = ak_vi_enable_dev(VIDEO_DEV0);
    if (ret) {
        AMY_LOG_ERROR("vi device %d enable device  failed!\n", VIDEO_DEV0);
        ak_vi_close(VIDEO_DEV0);
        return ret;
    }

    ret = ak_vi_enable_chn(VIDEO_CHN0);
    if (ret) {
        AMY_LOG_ERROR("vi channel[%d] enable failed!\n", VIDEO_CHN0);
        ak_vi_close(VIDEO_DEV0);
        return ret;
    }
    return 0;
}
static int video_device_close(video_input_context* dev) {
    ak_vi_disable_chn(VIDEO_CHN0);
    ak_vi_disable_dev(VIDEO_DEV0);
    ak_vi_close(VIDEO_DEV0);
    return 0;
}

video_input_context* video_intput_device_open(const char* isp_conf, int crop_left, int crop_top, int crop_width, int crop_height,
    int main_width, int main_height, int sub_width, int sub_height, int fps) {

    Amy_mutex_lock(&video_dev.mutex);
    if (video_dev.run) {
        AMY_LOG_ERROR("video input device awalyed open\n");
        Amy_mutex_unlock(&video_dev.mutex);
        return NULL;
    }
    memset(&video_dev.lastframe, 0, sizeof(struct video_input_frame));
    memset(video_dev.isp_conf, 0, sizeof(video_dev.isp_conf));
    strncpy(video_dev.isp_conf, isp_conf, sizeof(video_dev.isp_conf) - 1);
    video_dev.crop_left = crop_left;
    video_dev.crop_top = crop_top;
    video_dev.crop_width = crop_width;
    video_dev.crop_height = crop_height;

    video_dev.main_width = main_width;
    video_dev.main_height = main_height;
    video_dev.sub_width = sub_width;
    video_dev.sub_height = sub_height;
    video_dev.fps = fps;
    if (video_device_open(&video_dev)) {
        AMY_LOG_ERROR("video input device open failed\n");
        Amy_mutex_unlock(&video_dev.mutex);
        return NULL;
    }

    video_dev.cb_userdata = NULL;
    video_dev.frame_output_cb = NULL;
    video_dev.run = 1;
    Amy_mutex_unlock(&video_dev.mutex);
    return &video_dev;
}


int video_intput_device_close(video_input_context* dev) {
    Amy_mutex_lock(&video_dev.mutex);
    if (!dev->run) {
        AMY_LOG_ERROR("video input device awalyed close\n");
        Amy_mutex_unlock(&video_dev.mutex);
        return -1;
    }
    if (dev->lastframe.vi_frame.len) {
        ak_vi_release_frame(VIDEO_CHN0, &dev->lastframe);
    }
    video_device_close(dev);
    video_dev.run = 0;
    Amy_mutex_unlock(&video_dev.mutex);
    return 0;
}

int video_intput_get_frame(video_input_context* dev, struct video_input_frame* frame, int* w, int* h) {
    Amy_mutex_lock(&video_dev.mutex);
    if (!dev->run) {
        AMY_LOG_ERROR("video input device awalyed close\n");
        Amy_mutex_unlock(&video_dev.mutex);
        return -1;
    }
    if (w) {
        *w = video_dev.main_width;
    }
    if (h) {
        *h = video_dev.main_height;
    }
    if (ak_vi_get_frame(VIDEO_CHN0, frame) == 0) {
        if (dev->frame_output_cb) {
            dev->frame_output_cb(dev->cb_userdata, frame->vi_frame.data, frame->vi_frame.len, video_dev.main_width, video_dev.main_height);
        }

        if (dev->lastframe.vi_frame.len) {
            ak_vi_release_frame(VIDEO_CHN0, &dev->lastframe);
        }
        memcpy(&dev->lastframe, frame, sizeof(struct video_input_frame));
        Amy_mutex_unlock(&video_dev.mutex);
        return 0;
    }
    else  if (dev->lastframe.vi_frame.len) {
        memcpy(frame, &dev->lastframe, sizeof(struct video_input_frame));
        Amy_mutex_unlock(&video_dev.mutex);
        return 0;
    }
    Amy_mutex_unlock(&video_dev.mutex);
    return -1;
}


int video_input_output_frame_callback_register(video_input_context* dev,
    int(*output_frame_cb)(void* user_data, const unsigned char* data, int size, int w, int h), void* user_data) {
    Amy_mutex_lock(&video_dev.mutex);
    if (!dev->run) {
        AMY_LOG_ERROR("video input device awalyed close\n");
        Amy_mutex_unlock(&video_dev.mutex);
        return -1;
    }
    dev->cb_userdata = user_data;
    dev->frame_output_cb = output_frame_cb;
    Amy_mutex_unlock(&video_dev.mutex);
    return -1;
}