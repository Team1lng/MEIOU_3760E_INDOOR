#ifndef __RTSP_H__
#define __RTSP_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RTSP_URL_LEN 256

typedef enum {
    //required
    OPTIONS = 0,
    PLAY,
    SETUP,
    TEARDOWN,
    //recommended
    DESCRIBE,
    PAUSE,
    //optional
    ANNOUNCE,
    GET_PARAMETER,
    RECORD,
    REDIRECT,
    SET_PARAMETER,

    RTSP_METHOD_NUM,
} RTSP_METHOD_E;

typedef enum {
    ACCEPT = 0,
    ACCEPT_ENCODING,
    ACCEPT_LANGUAGE,
    AUTHORIZATION,
    FROM,
    IF_MODIFIED_SINCE,
    RANGE,
    REFERER,
    USER_AGENT,
    BANDWIDTH,
    BLOCKSIZE,
    CONFERENCE,
    CSEQ,

    //general header
    CACHE_CONTROL,
    CONNECTION,
    DATE,
    VIA,
    //response header
    LOCATION,
    PROXY_AUTHENTICATE,
    PUBLIC,
    RETRY_AFTER,
    SERVER,
    VARY,
    WWW_AUTHENTICATE,
    //entity header
    CONTENT_BASE,
    ALLOW,
    CONTENT_ENCODING,
    CONTENT_LANGUAGE,
    CONTENT_LENGTH,
    CONTENT_LOCATION,
    CONTENT_TYPE,
    EXPIRES,
    LAST_MODIFIED,
    RTP_INFO,
    SESSION,
    TRANSPORT,

    RTSP_HEADER_NUM,
} RTSP_HEADER_E;

typedef enum {
    CONTINUE                      = 100,
    OK                            = 200,
    CREATED                       = 201,
    LOW_ON_STORAGE_SPACE          = 250,
    MULTIPLE_CHOICES              = 300,
    MOVED_PERMANENTLY             = 301,
    MOVED_TEMPORARILY             = 302,
    SEE_OTHER                     = 303,
    NOT_MODIFIED                  = 304,
    USE_PROXY                     = 305,
    BAD_REQUEST                   = 400,
    UNAUTHORIZED                  = 401,
    PAYMENT_REQUIRED              = 402,
    FORBIDDEN                     = 403,
    NOT_FOUND                     = 404,
    METHOD_NOT_ALLOWED            = 405,
    NOT_ACCEPTABLE                = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT               = 408,
    GONE                          = 410,
    LENGTH_REQUIRED               = 411,
    PRECONDITION_FAILED           = 412,
    REQUEST_ENTITY_TOO_LARGE      = 413,
    REQUEST_URI_TOO_LARGE         = 414,
    UNSUPPORTED_MEDIA_TYPE        = 415,
    PARAMETER_NOT_UNDERSTOOD      = 451,
    CONFERENCE_NOT_FOUND          = 452,
    NOT_ENOUGH_BANDWIDTH          = 453,
    SESSION_NOT_FOUND             = 454,
    METHOD_NOT_VALID_IN_THIS_STATE = 455,
    HEADER_FIELD_NOT_VALID_FOR_RESOURCE = 456,
    INVALID_RANGE                 = 457,
    PARAMETER_IS_READ_ONLY        = 458,
    AGGREGATE_OPERATION_NOT_ALLOWED = 459,
    ONLY_AGGREGATE_OPERATION_ALLOWED = 460,
    UNSUPPORTED_TRANSPORT         = 461,
    DESTINATION_UNREACHABLE       = 462,
    INTERNAL_SERVER_ERROR         = 500,
    NOT_IMPLEMENTED               = 501,
    BAD_GATEWAY                   = 502,
    SERVICE_UNAVAILABLE           = 503,
    GATEWAY_TIMEOUT               = 504,
    RTSP_VERSION_NOT_SUPPORTED    = 505,
    OPTION_NOT_SUPPORTED          = 551, 
 } RTSP_STATUS_CODE_E;

typedef enum {
    RTSP_TRANSPORT_MODE_UDP = 1,
    RTSP_TRANSPORT_MODE_TCP,
} RTSP_TRANSPORT_MODE_E;

typedef enum {
    RTSP_CAST_MODE_UNICAST = 1,
    RTSP_CAST_MODE_MULTICAST,
} RTSP_CAST_MODE_E;

typedef enum {
    RTSP_MSG_REQUEST = 1,
    RTSP_MSG_RESPONSE,
} RTSP_MSG_TYPE_E;

typedef enum {
    RTSP_MEDIA_TYPE_INVALID = -1,
    RTSP_MEDIA_TYPE_VIDEO = 0,
    RTSP_MEDIA_TYPE_AUDIO,
    RTSP_MEDIA_TYPE_NUM,
} RTSP_MEDIA_TYPE_E;

typedef enum {
    RTSP_CODEC_INVALID = 0,
    RTSP_CODEC_H264,
    RTSP_CODEC_H265,
    RTSP_CODEC_PCM,
    RTSP_CODEC_G711A,
    RTSP_CODEC_G711U,
} RTSP_CODEC_E;

typedef enum
{
    E_RTSP_VIDEO_PB_FRAME = 0,
    E_RTSP_VIDEO_I_FRAME,
    E_RTSP_VIDEO_SPS_FRAME,
    E_RTSP_VIDEO_PPS_FRAME,
    E_RTSP_VIDEO_VPS_FRAME,
    E_RTSP_VIDEO_SEI_FRAME,
    E_RTSP_AUDIO_FRAME,
    E_RTSP_FRAME_TYPE_MAX
}RTSP_FRAME_TYPE_E;

typedef struct
{
    RTSP_FRAME_TYPE_E  type;
    RTSP_CODEC_E       codec;
    int                frame_start;
    int                frame_end;
    char*              buf;
    uint32_t           size;
    uint64_t           timestamp;
}rtsp_data_packet_s;


typedef struct {
    RTSP_MSG_TYPE_E         type;
    union {
        RTSP_METHOD_E       method;
        RTSP_STATUS_CODE_E  status;
    };
    union {
        char                uri[1024];
        char                status_msg[32];
    };
    char                    version[4];
    char*                   headers[RTSP_HEADER_NUM];
    char*                   body;
    int                     body_len;
    int                     msg_len;
} rtsp_msg_t;

typedef struct {
    RTSP_TRANSPORT_MODE_E trans_mode;
    RTSP_CAST_MODE_E cast_mode;
    int client_port[2];
    int server_port[2];
    int interleaved[2];
} rtsp_transport_t;


int rtsp_pack_request(rtsp_msg_t* preq, char* buf, int size);
int rtsp_unpack_request(rtsp_msg_t* preq, char*buf, int size);
int rtsp_pack_response(rtsp_msg_t* presp, char* buf, int size);
int rtsp_unpack_response(rtsp_msg_t* presp, char*buf, int size);

int rtsp_pack_transport(rtsp_transport_t* ptrans, char* buf, int size);
int rtsp_unpack_transport(rtsp_transport_t* ptrans, char* buf);

int rtsp_print_msg(rtsp_msg_t* pmsg);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_H__

