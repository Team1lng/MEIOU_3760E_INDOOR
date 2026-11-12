#ifndef __RTSP_INTERNAL_H__
#define __RTSP_INTERNAL_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "rtsp.h"

#define RTSP_ERR_NEED_MORE_DATA           (-100)

typedef struct {
    RTSP_CODEC_E     codec;
    int              sample_rate;
    int              channels;
    char             track_uri[RTSP_URL_LEN];
    int              track_avail;
} rtsp_media_info_s;

char* rtsp_get_method_str(RTSP_METHOD_E method);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_INTERNAL_H__
