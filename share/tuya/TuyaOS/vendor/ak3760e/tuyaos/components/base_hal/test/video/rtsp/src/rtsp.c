#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include "rtsp.h"
#include "rtsp_internal.h"

char* g_rtsp_methods[] = {
    [DESCRIBE]       = "DESCRIBE",
    [ANNOUNCE]       = "ANNOUNCE",
    [GET_PARAMETER]  = "GET_PARAMETER",
    [OPTIONS]        = "OPTIONS",
    [PAUSE]          = "PAUSE",
    [PLAY]           = "PLAY",
    [RECORD]         = "RECORD",
    [REDIRECT]       = "REDIRECT",
    [SETUP]          = "SETUP",
    [SET_PARAMETER]  = "SET_PARAMETER",
    [TEARDOWN]       = "TEARDOWN",
};

char* g_rtsp_headers[] = {
    [ACCEPT]               = "Accept",
    [ACCEPT_ENCODING]      = "Accept-Encoding",
    [ACCEPT_LANGUAGE]      = "Accept-Language",
    [AUTHORIZATION]        = "Authorization",
    [FROM]                 = "From",
    [IF_MODIFIED_SINCE]    = "If-Modified-Since",
    [RANGE]                = "Range",
    [REFERER]              = "Referer",
    [USER_AGENT]           = "User-Agent",
    [BANDWIDTH]            = "BandWidth",
    [BLOCKSIZE]            = "BlockSize",
    [CONFERENCE]           = "Conference",
    [CSEQ]                 = "CSeq",

    //general header
    [CACHE_CONTROL]        = "Cache-Control",
    [CONNECTION]           = "Connection",
    [DATE]                 = "Date",
    [VIA]                  = "Via",
    //response header
    [LOCATION]             = "Location",
    [PROXY_AUTHENTICATE]   = "Proxy-Authenticate",
    [PUBLIC]               = "Public",
    [RETRY_AFTER]          = "Retry-After",
    [SERVER]               = "Server",
    [VARY]                 = "Vary",
    [WWW_AUTHENTICATE]     = "WWW-Authenticate",
    //entity header
    [CONTENT_BASE]         = "Content-Base",
    [ALLOW]                = "Allow",
    [CONTENT_ENCODING]     = "Content-Encoding",
    [CONTENT_LANGUAGE]     = "Content-Language",
    [CONTENT_LENGTH]       = "Content-Length",
    [CONTENT_LOCATION]     = "Content-Location",
    [CONTENT_TYPE]         = "Content-Type",
    [EXPIRES]              = "Expires",
    [LAST_MODIFIED]        = "Last-Modified",
    [RTP_INFO]             = "RTP-Info",
    [SESSION]              = "Session",
    [TRANSPORT]            = "Transport",
};


/*返回写入buf的字节数*/
int rtsp_pack_request(rtsp_msg_t* preq, char* buf, int size)
{
    char* pos = buf;
    int left = size;
    //request line
    char line_buf[1024];
    int len = 0;
    snprintf(line_buf, sizeof(line_buf), "%s %s RTSP/%s\r\n", g_rtsp_methods[preq->method], preq->uri, preq->version);
    len = strlen(line_buf);
    if (len > left){
        return 0;
    }
    memcpy(pos, line_buf, len);
    pos += len;
    left -= len;
    //header
    int i;
    for(i = 0; i < RTSP_HEADER_NUM; i++){
        if (preq->headers[i] != NULL){
            snprintf(line_buf, sizeof(line_buf), "%s: %s\r\n", g_rtsp_headers[i], preq->headers[i]);
            len = strlen(line_buf);
            if (len > left)
            {
                return 0;
            }
            memcpy(pos, line_buf, len);
            pos += len;
            left -= len;
        }
    }
    len = strlen("\r\n");
    if (len > left){
        return 0;
    }
    memcpy(pos, "\r\n", len);
    pos += len;
    left -= len;
    //body
    if (preq->body){
        len = strlen(preq->body);
        if (len > left){
            return 0;
        }
        memcpy(pos, preq->body, len);
        pos += len;
        left -= len;
    }

    return size - left;
}


int rtsp_unpack_request(rtsp_msg_t* preq, char*buf, int size)
{
    char* saveptr = NULL;
    int got_request_line = 0;
    char* pos = strtok_r(buf, "\r\n", &saveptr);
    while(pos != NULL){
        if (!got_request_line){ // parse request line
            char* p = pos, *q = pos;
            //parse method
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != '\0') q++;
            int i;
            for(i = 0; i < RTSP_METHOD_NUM; i++){
                if (0 == strncmp(g_rtsp_methods[i], p, q - p)){
                    preq->method = i;
                    break;
                }
            }
            if (i == RTSP_METHOD_NUM){
                char bak = *q;
                *q = '\0';
                printf("unknown method: %s", p);
                *q = bak;
                return -1;
            }
            //parse uri
            p = q;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != '\0') q++;
            if (q - p > sizeof(preq->uri)){
                printf("uri too large: %d\n", (int)(q - p));
                return -1;
            }
            memcpy(preq->uri, p, q - p);
            //parse version
            p = q;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != '\0') q++;
            if (q - p != strlen("RTSP/1.0") || 0 != strncmp("RTSP/", p, 5)){
                char bak = *q;
                *q = '\0';
                printf("unknown version: %s", p);
                *q = bak;
                return -1;
            }
            memcpy(preq->version, p + 5, 3);
            preq->version[3] = '\0';

            got_request_line = 1;
        }
        else{ //parse header
            char* p = pos, *q = pos;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != ':' && *q != '\0') q++;

            int i;
            for(i = 0; i < RTSP_HEADER_NUM; i++){
                if (0 == strncmp(g_rtsp_headers[i], p, q - p)){
                    break;
                }
            }
            if (i != RTSP_HEADER_NUM){
                p = q;
                while (*p == ' ' || *p == '\t' || *p == ':') p++;
                preq->headers[i] = p;
            }
        }
        pos = strtok_r(NULL, "\r\n", &saveptr);
    }
    if (NULL == preq->headers[CSEQ]) {
        printf("wrong cseq\n");
        return -1;
    }

    preq->type = RTSP_MSG_REQUEST;

    return 0;
}

/*返回写入buf的字节数*/
int rtsp_pack_response(rtsp_msg_t* presp, char* buf, int size)
{
    char* pos = buf;
    int left = size;
    //status line
    char line_buf[1024];
    int len = 0;
    snprintf(line_buf, sizeof(line_buf), "RTSP/%s %d %s\r\n", presp->version, presp->status, presp->status_msg);
    len = strlen(line_buf);
    if (len > left){
        return 0;
    }
    memcpy(pos, line_buf, len);
    pos += len;
    left -= len;
    //header
    int i;
    for(i = 0; i < RTSP_HEADER_NUM; i++){
        if (presp->headers[i] != NULL){
            snprintf(line_buf, sizeof(line_buf), "%s: %s\r\n", g_rtsp_headers[i], presp->headers[i]);
            len = strlen(line_buf);
            if (len > left)
            {
                return 0;
            }
            memcpy(pos, line_buf, len);
            pos += len;
            left -= len;
        }
    }
    len = strlen("\r\n");
    if (len > left){
        return 0;
    }
    memcpy(pos, "\r\n", len);
    pos += len;
    left -= len;
    //body
    if (presp->body){
        len = strlen(presp->body);
        if (len > left){
            return 0;
        }
        memcpy(pos, presp->body, len);
        pos += len;
        left -= len;
    }

    return size - left;
}

int rtsp_unpack_response(rtsp_msg_t* presp, char*buf, int size)
{
    char* saveptr = NULL;
    int got_status_line = 0;
    char* pos = strtok_r(buf, "\r\n", &saveptr);
    while(pos != NULL){
        if (!got_status_line){ // parse status line
            char* p = pos, *q = pos;
            //parse version
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != '\0') q++;
            if (q - p != strlen("RTSP/1.0") || 0 != strncmp("RTSP/", p, 5)){
                char bak = *q;
                *q = '\0';
                printf("unknown version: %s", p);
                *q = bak;
                return -1;
            }
            memcpy(presp->version, p + 5, 3);
            presp->version[3] = '\0';
            //parse status code
            p = q;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != '\0') q++;
            char bak = *q;
            *q = '\0';
            int code = atoi(p);
            if (code <= 0){
                printf("unknown version: %s", p);
                return -1;
            }
            *q = bak;
            presp->status = code;

            //parse status msg
            p = q;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != '\r' && *q != '\0') q++;
            if (q - p > sizeof(presp->status_msg)){
                printf("status msg too large: %d\n", (int)(q - p));
                return -1;
            }
            memcpy(presp->status_msg, p, q - p);

            got_status_line = 1;
        }
        else{ //parse header
            char* p = pos, *q = pos;
            while(*p == ' ' || *p == '\t') p++;
            q = p;
            while(*q != ' ' && *q != '\t' && *q != ':' && *q != '\0') q++;

            int i;
            for(i = 0; i < RTSP_HEADER_NUM; i++){
                if (0 == strncmp(g_rtsp_headers[i], p, q - p)){
                    break;
                }
            }
            if (i != RTSP_HEADER_NUM){
                p = q;
                while (*p == ' ' || *p == '\t' || *p == ':') p++;
                presp->headers[i] = p;
            }
        }
        pos = strtok_r(NULL, "\r\n", &saveptr);
    }

    if (NULL == presp->headers[CSEQ]){
        printf("wrong cseq\n");
        return -1;
    }

    presp->type = RTSP_MSG_RESPONSE;

    return 0;
}

int rtsp_print_msg(rtsp_msg_t* pmsg)
{
    int ret = 0;

    if (pmsg->type == RTSP_MSG_REQUEST) {

        printf("--> %s %s RTSP/%s\n", rtsp_get_method_str(pmsg->method), pmsg->uri, pmsg->version);
        int i;
        for(i = 0; i < RTSP_HEADER_NUM; i++) {
            if (pmsg->headers[i] != NULL) {
                printf("--> %s: %s\n", g_rtsp_headers[i], pmsg->headers[i]);
            }
        }
        printf("\n");
        if (pmsg->body) {
            printf("\n%s\n", pmsg->body);
        }

    }
    else if (pmsg->type == RTSP_MSG_RESPONSE) {

        printf("<-- RTSP/%s %d %s\n", pmsg->version, pmsg->status, pmsg->status_msg);
        int i;
        for(i = 0; i < RTSP_HEADER_NUM; i++) {
            if (pmsg->headers[i] != NULL) {
                printf("<-- %s: %s\n", g_rtsp_headers[i], pmsg->headers[i]);
            }
        }
        printf("\n");
        if (pmsg->body) {
            printf("\n%s\n", pmsg->body);
        }

    }

    return ret;
}


int rtsp_unpack_transport(rtsp_transport_t* ptrans, char* buf)
{
    int ret = 0;

    bzero(ptrans, sizeof(rtsp_transport_t));
    char* pos = strstr(buf, "RTP/AVP/TCP;");
    if (pos) {
        ptrans->trans_mode = RTSP_TRANSPORT_MODE_TCP;
    }
    else {
        pos = strstr(buf, "RTP/AVP;");
        if (pos) {
            ptrans->trans_mode = RTSP_TRANSPORT_MODE_UDP;
        }
    }

    pos = strstr(buf, "multicast;");
    if (pos) {
        ptrans->cast_mode = RTSP_CAST_MODE_MULTICAST;
    }
    else {
        pos = strstr(buf, "unicast;");
        if (pos) {
            ptrans->cast_mode = RTSP_CAST_MODE_UNICAST;
        }
    }

    if (ptrans->trans_mode == RTSP_TRANSPORT_MODE_UDP) {
        int ports[2];
        pos = strstr(buf, "client_port=");
        if (pos){
            int match = sscanf(pos, "client_port=%d-%d", ports, ports + 1);
            if (2 == match) {
                ptrans->client_port[0] = ports[0];
                ptrans->client_port[1] = ports[1];
            }
        }
        pos = strstr(buf, "server_port=");
        if (pos){
            int match = sscanf(pos, "server_port=%d-%d", ports, ports + 1);
            if (2 == match) {
                ptrans->server_port[0] = ports[0];
                ptrans->server_port[1] = ports[1];
            }
        }
    }
    else if (ptrans->trans_mode == RTSP_TRANSPORT_MODE_TCP) {
        pos = strstr(buf, "interleaved=");
        int intl[2];
        if (pos) {
            int match = sscanf(pos, "interleaved=%d-%d", intl, intl + 1);
            if (2 == match) {
                ptrans->interleaved[0] = intl[0];
                ptrans->interleaved[1] = intl[1];
            }
        }
    }
    
    return ret;
}

//返回写入buf的字节数
int rtsp_pack_transport(rtsp_transport_t* ptrans, char* buf, int size)
{
    char* pos = buf;
    int left = size;
    //transport mode
    char* trans_str = ptrans->trans_mode == RTSP_TRANSPORT_MODE_UDP ? "RTP/AVP;" : "RTP/AVP/TCP;";
    int len = strlen(trans_str);
    if (left <= len){
        return 0;
    }
    strcpy(pos, trans_str);
    pos += len;
    left -= len;

    //cast mode
    char* cast_str = ptrans->cast_mode == RTSP_CAST_MODE_UNICAST ? "unicast;" : "multicast;";
    len = strlen(cast_str);
    if (left <= len){
        return 0;
    }
    strcpy(pos, cast_str);
    pos += len;
    left -= len;

    //port/interleave
    char tmp_buf[128];
    if (ptrans->trans_mode == RTSP_TRANSPORT_MODE_UDP) {
        snprintf(tmp_buf, sizeof(tmp_buf), "client_port=%d-%d;server_port=%d-%d",
            ptrans->client_port[0], ptrans->client_port[1], ptrans->server_port[0], ptrans->server_port[1]);
        len = strlen(tmp_buf);
        if (left <= len){
            return 0;
        }
        strcpy(pos, tmp_buf);
        pos += len;
        left -= len;
    }
    else {
        snprintf(tmp_buf, sizeof(tmp_buf), "interleaved=%d-%d",
            ptrans->interleaved[0], ptrans->interleaved[1]);
        len = strlen(tmp_buf);
        if (left <= len){
            return 0;
        }
        strcpy(pos, tmp_buf);
        pos += len;
        left -= len;
    }

    return size - left;
}

char* rtsp_get_method_str(RTSP_METHOD_E method)
{
    if (method >= RTSP_METHOD_NUM){
        return NULL;
    }    

    return g_rtsp_methods[method];
}