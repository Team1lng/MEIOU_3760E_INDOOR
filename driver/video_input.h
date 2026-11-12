#ifndef _AMY_VIDEO_INPUT_H_
#define _AMY_VIDEO_INPUT_H_
#include "ak_common.h"
#include "ak_vi.h"

typedef struct video_input_context video_input_context;
video_input_context* video_intput_device_open(const char* isp_conf, int crop_left, int crop_top, int crop_width, int crop_height,
    int main_width, int main_height, int sub_width, int sub_height, int fps);

int video_intput_device_close(video_input_context* dev);

int video_intput_get_frame(video_input_context* dev, struct video_input_frame* frame, int* w, int* h);

int video_input_output_frame_callback_register(video_input_context* dev,
    int(*output_frame_cb)(void* user_data, const unsigned char* data, int size, int w, int h), void* user_data);
#endif