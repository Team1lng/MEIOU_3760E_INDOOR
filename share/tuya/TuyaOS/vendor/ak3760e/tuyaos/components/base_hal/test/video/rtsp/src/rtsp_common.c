#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "rtsp_common.h"
#include "rtsp.h"
#include "rtsp_internal.h"
#include "byte_array_buf.h"


static int __parse_rtsp_msg(void* pri_data, rtsp_common_handle_msg_cb msg_cb, rtsp_msg_t* pmsg, char* data, int len)
{
    int ret = 0;

    if (pmsg->body_len > 0){
        pmsg->body = data + len - pmsg->body_len;
    }
    else{
        bzero(pmsg, sizeof(rtsp_msg_t));

        if (0 == strncmp(data, "RTSP", 4)) {
            if (0 != rtsp_unpack_response(pmsg, data, len)) {
                printf("parse response failed\n");
                return -1;
            }
        }
        else{
            if (0 != rtsp_unpack_request(pmsg, data, len)) {
                printf("parse request failed\n");
                return -1;
            }
        }

        pmsg->msg_len = len;

        //若content_length不为0，则设置in_body_len，下次读数据时读入body中
        if (pmsg->headers[CONTENT_LENGTH] != NULL){
            int content_length = atoi(pmsg->headers[CONTENT_LENGTH]);
            if (content_length > 0){
                pmsg->body_len = content_length;
                pmsg->msg_len += content_length;
                return RTSP_ERR_NEED_MORE_DATA;
            }
            else{
                printf("unknown Content-Length: %s\n", pmsg->headers[CONTENT_LENGTH]);
            }
        }
    }

    if (msg_cb){
        ret = msg_cb(pri_data, pmsg);
    }

    pmsg->body_len = 0;
    pmsg->msg_len = 0;
    return ret;
}


int rtsp_common_parse_data(void* pri_data, rtsp_common_handle_msg_cb msg_cb, rtsp_common_handle_rtp_cb rtp_cb,
                            byte_array_buf_t* prx_buf, rtsp_msg_t* pmsg)
{
    int ret = 0;
    char* buf = NULL;

    int check_dollar = 1;
    int advance_len = 0;
    do {
        advance_len = 0;
        int len = byte_array_buf_read_ptr_lock(prx_buf, &buf);
        //若msg带有body
        if (pmsg->body_len > 0) {
            if (len >= pmsg->msg_len) {
                advance_len += pmsg->msg_len;
                ret = __parse_rtsp_msg(pri_data, msg_cb, pmsg, buf, pmsg->msg_len);
            }
        }
        else if (len > 4) {
            if (check_dollar && buf[0] == '$') {
                int chn = buf[1];
                int pkt_len = ntohs(*(uint16_t*)(buf + 2));
                if (len >= pkt_len + 4){
                    //parse packet
                    if (rtp_cb){
                        rtp_cb(pri_data, chn, buf + 4, pkt_len);
                    }
                    advance_len = pkt_len + 4;
                }
            }
            else{
                //parse rtsp msg
                char bak = '\0';
                if (strncmp(buf + len - 4, "\r\n\r\n", 4) != 0){
                    //若buf中的数据不以\r\n\r\n结尾，则将最后一个字符设为\0，防止越界
                    bak = buf[len - 1];
                    buf[len - 1] = '\0';
                }
                char* pos = strstr(buf, "\r\n\r\n");
                if (bak != '\0'){
                    buf[len - 1] = bak;
                }

                if (pos) {
                    *pos = '\0';
                    advance_len = pos - buf + 4;
                    ret = __parse_rtsp_msg(pri_data, msg_cb, pmsg, buf, advance_len);
                    if (ret == RTSP_ERR_NEED_MORE_DATA){ //需要继续读取body
                        if (len - advance_len >= pmsg->body_len){
                            //若body已经被读入buf中
                            ret = rtsp_common_parse_data(pri_data, msg_cb, rtp_cb, prx_buf, pmsg);
                        }
                        advance_len = 0;
                    }
                }
            }
        }
        byte_array_buf_read_ptr_unlock(prx_buf, advance_len);
        byte_array_buf_trim(prx_buf);
    } while (advance_len > 0);
    return ret;
}