#ifndef __RTSP_INTERNAL_H__
#define __RTSP_INTERNAL_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "rtsp.h"

#define RTSP_ERR_NEED_MORE_DATA           (-100)

char* rtsp_get_method_str(RTSP_METHOD_E method);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_INTERNAL_H__
