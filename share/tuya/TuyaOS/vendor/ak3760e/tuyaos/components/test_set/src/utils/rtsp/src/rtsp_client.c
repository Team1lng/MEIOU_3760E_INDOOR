#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>

#include "rtp.h"
#include "rtsp_client.h"
#include "byte_array_buf.h"
#include "utils.h"
#include "sock_util.h"
#include "rtsp_session.h"
#include "rtsp_common.h"
#include "rtsp_internal.h"
#include "rtp_client.h"
#include "wheel_timer.h"

#define HEART_BEAT_PERIOD_MS 20000
#define SLEEP_AFTER_CONN_FAIL_MS 5000


typedef struct rtsp_client_info_s {
    struct rtsp_client_info_s        *next;
    RTSP_CLIENT_STATUS_E             status;
    char                             url[RTSP_URL_LEN];
    char                             username[RTSP_USERNAME_LEN];
    char                             password[RTSP_PASSWORD_LEN];
    char                             realm[RTSP_REALM_LEN];
    char                             nonce[RTSP_NONCE_LEN];
    char                             ip[64];
    int                              port;
    byte_array_buf_t                 rx_buf;
    byte_array_buf_t                 tx_buf;
    int                              fd;
    int                              cseq;
    rtsp_media_info_t                media_info[RTSP_MEDIA_TYPE_NUM];
    uint64_t                         hb_ms;            //心跳时间戳
    uint64_t                         reactivate_ms;    //重新进入事件循环的时间戳，用于连接失败时暂停一段时间
    uint64_t                         recv_ms;          //收到数据的时间戳，用于检测server端是否断开
    rtsp_session_t                   session;
    rtsp_msg_t                       msg;
    RTP_CLIENT_HANDLE                rtp_handle;
    int                              stop;

    rtsp_client_put_data_cb          put_data_cb;
    rtsp_client_status_change_cb     status_change_cb;
    void*                            cb_priv;
} rtsp_client_info_s;

typedef enum {
    RTSP_CLIENT_MSG_ADD,
    RTSP_CLIENT_MSG_REMOVE,
} RTSP_CLIENT_MSG_E;

typedef struct {
    RTSP_CLIENT_MSG_E          type;
    rtsp_client_info_s*        pinfo;

} rtsp_client_msg_s;   //用于添加删除client

typedef struct {
    int                   inited;
    pthread_t             tid;
    int                   thread_exit;
    rtsp_client_info_s*   head;
    int                   pfd[2];         //管道
    uint64_t              now_ms;

} rtsp_client_handle_s;

static RTSP_CLIENT_HANDLE __client_info_to_handle(rtsp_client_info_s* pinfo)
{
    return pinfo;
}

static rtsp_client_info_s* __client_info_from_handle(RTSP_CLIENT_HANDLE handle)
{
    return (rtsp_client_info_s*)handle;
}

static int __parse_ip_port_from_url(char* url, char* ip_buf, int buf_len, int* port)
{
    char url_bak[strlen(url) + 1];
    strcpy(url_bak, url);

    char* sep = strstr(url_bak, "://");
    if (!sep){
        printf(":// not found in url\n");
        return -1;
    }

    *port = 554;

    char* domain_start = sep + 3;
    char* colon = strstr(domain_start, ":");
    char* slash = strstr(domain_start, "/");

    if (colon != NULL && slash != NULL) {
        if (colon < slash){
            *colon = '\0';
        }
        else {
            colon = NULL;
            *slash = '\0';
        }
    }
    else {
        if (NULL != colon){
            *colon = '\0';
        }
        else if (NULL != slash) {
            *slash = '\0';
        }
    }

    if (NULL != colon){
        if (1 != sscanf(colon + 1, "%d", port)){
            printf("parse port failed\n");
            return -1;
        }
    }

    return domain_to_ip(domain_start, ip_buf, buf_len);
}

//异步，写入管道，在io线程进行实际操作
static void __send_to_pipe(rtsp_client_handle_s* phdl, rtsp_client_msg_s* pmsg)
{
    void* pos = pmsg;
    int left = sizeof(rtsp_client_msg_s);

    while (left > 0){
        int ret = write(phdl->pfd[1], pos, left);
        if (ret < 0){
            printf("write failed\n");
            break;
        }

        pos += ret;
        left -= ret;
        usleep(20 * 1000);
    }
}

static void __read_from_pipe(rtsp_client_handle_s* phdl, rtsp_client_msg_s* pmsg)
{
    void* pos = pmsg;
    int left = sizeof(rtsp_client_msg_s);

    while(left > 0){
        int ret = read(phdl->pfd[0], pos, left);
        if (ret <= 0){
            printf("read failed\n");
            break;
        }

        pos += ret;
        left -= ret;
        usleep(20 * 1000);
    }
}

static void __add_client_info_to_list(rtsp_client_handle_s* phdl, rtsp_client_info_s* pinfo)
{
    pinfo->next = phdl->head;
    phdl->head = pinfo;
}


static void __remove_client_info_from_list(rtsp_client_handle_s* phdl, rtsp_client_info_s* pinfo)
{
    rtsp_client_info_s* p = phdl->head;
    if (p == pinfo){
        phdl->head = p->next;
    }
    else{

        while(p->next){
            if (p->next == pinfo){
                p->next = pinfo->next;
                break;
            }
            else{
                p = p->next;
            }
        }
    }
}

static void _add_fd_to_set(fd_set* pset, int fd, int* max_fd)
{
    if (fd > 0) {
        FD_SET(fd, pset);
        if (fd > *max_fd) {
            *max_fd = fd;
        }
    }
}

static void __close_fd(int* pfd)
{
    if (*pfd > 0){
        close(*pfd);
        *pfd = 0;
    }
}

static void __reset_client(rtsp_client_info_s* pcinfo)
{
    if (pcinfo->fd > 0){
        close(pcinfo->fd);
        pcinfo->fd = -1;
    }

    if (pcinfo->rtp_handle){
        rtp_client_destroy(pcinfo->rtp_handle);
        pcinfo->rtp_handle = NULL;
    }
    
    if (pcinfo->session.mode == RTSP_TRANSPORT_MODE_UDP){
        __close_fd(&pcinfo->session.transport[RTSP_VID_DATA_CHN].fd);
        __close_fd(&pcinfo->session.transport[RTSP_VID_CTRL_CHN].fd);
        __close_fd(&pcinfo->session.transport[RTSP_AUD_DATA_CHN].fd);
        __close_fd(&pcinfo->session.transport[RTSP_AUD_CTRL_CHN].fd);
    }

    pcinfo->status = RTSP_CLIENT_STATUS_INIT;
    pcinfo->cseq = 1;
    pcinfo->recv_ms = 0;
    pcinfo->session.session_id[0] = '\0';
    byte_array_buf_clear(&pcinfo->rx_buf);
    byte_array_buf_clear(&pcinfo->tx_buf);
}

static void __remove_client(rtsp_client_handle_s* phdl, rtsp_client_info_s* pcinfo)
{
    __reset_client(pcinfo);
    __remove_client_info_from_list(phdl, pcinfo);

    byte_array_buf_destroy(&pcinfo->tx_buf);
    byte_array_buf_destroy(&pcinfo->rx_buf);

    free(pcinfo);
}

static rtsp_client_handle_s* rtsp_client_get_handle()
{
    static rtsp_client_handle_s hdl = {0};
    return &hdl;
}

/**
 * describe example:
 * 
 * v=0
 * o=- 14665860 345678943 IN IP4 127.0.0.2
 * s=Session streamed by "Tuya RTSP Server"
 * t=0 0
 * m=video 0 RTP/AVP 96
 * c=IN IP4 0.0.0.0
 * b=AS:4096
 * a=range:npt=0-
 * a=control:track0
 * a=rtpmap:96 H264/90000
 * a=fmtp:96 packetization-mode=1; profile-level-id=64001E; sprop-parameter-sets=Z2QAHqyyAUBf8uAiAAADAAIAAAMAeB4sXJA=,aOvDyyLA
 * m=audio 0 RTP/AVP 8
 * a=rtpmap:8 PCMA/8000/1
 * a=control:track1
**/
static int __parse_describe(rtsp_client_info_s* pcinfo, rtsp_msg_t* pmsg)
{
    RTSP_MEDIA_TYPE_E type = RTSP_MEDIA_TYPE_INVALID;
    memset(pcinfo->media_info, 0, sizeof(pcinfo->media_info));

    char* line = strtok(pmsg->body, "\r\n");
    while(line && line < pmsg->body + pmsg->body_len){
        //按行处理describe，先找到m=开头的行确定音视频，再找对应的
        //codec,sample rate,channel,uri
        //m=<media> <port> <proto> <fmt>
        if(0 == strncmp(line, "m=", 2)) {
            char* media = strchr(line, '=') + 1;
            char* port = strchr(media, ' ') + 1;
            char* proto = strchr(port, ' ') + 1;
            char* fmt = strchr(proto, ' ') + 1;

            if (0 == strncmp(media, "video", 5)){
                type = RTSP_MEDIA_TYPE_VIDEO;
                pcinfo->media_info[type].track_avail = 1;
            }
            else if (0 == strncmp(media, "audio", 5)){
                type = RTSP_MEDIA_TYPE_AUDIO;
                pcinfo->media_info[type].track_avail = 1;
                if (fmt > line && fmt < pmsg->body + pmsg->body_len){
                    int payload_fmt = -1;
                    sscanf(fmt, "%d", &payload_fmt);
                    if (payload_fmt == 0){
                        pcinfo->media_info[type].sample_rate = 8000;
                        pcinfo->media_info[type].codec = RTP_CODEC_G711U;
                        pcinfo->media_info[type].channels = 1;
                    }
                    else if (payload_fmt == 8){
                        pcinfo->media_info[type].sample_rate = 8000;
                        pcinfo->media_info[type].codec = RTP_CODEC_G711A;
                        pcinfo->media_info[type].channels = 1;
                    }
                }
            }
            else
            {
                type = RTSP_MEDIA_TYPE_INVALID;
            }
            
        }
        else if (type != RTSP_MEDIA_TYPE_INVALID && 0 == strncmp(line, "a=", 2)){
            char* ctrl = strstr(line, "control:");
            if (ctrl){
                char* value = ctrl + strlen("control:");
                if (0 == strncmp(value, "rtsp://", 7)){
                    strncpy(pcinfo->media_info[type].track_uri, value, sizeof(pcinfo->media_info[type].track_uri));
                }
                else{
                    snprintf(pcinfo->media_info[type].track_uri, sizeof(pcinfo->media_info[type].track_uri),
                            "%s%s%s",
                            pcinfo->url,
                            pcinfo->url[strlen(pcinfo->url) - 1] == '/' ? "" : "/",
                            value);
                }
            }
            char* rtpmap = strstr(line, "rtpmap:");
            if (rtpmap){
                //a=rtpmap:<payload fmt> <encoding name>/<clock rate>[/<encoding parameters>]
                char* fmt = strchr(line, ':') + 1;
                char* codec = strchr(fmt, ' ') + 1;
                char* sample_rate = strchr(codec, '/') + 1;
                char* param = strchr(sample_rate, '/') + 1;

                if (codec > line){
                    if (0 == strncmp(codec, "H264", 4)){
                        pcinfo->media_info[type].codec = RTP_CODEC_H264;
                    }
                    else if (0 == strncmp(codec, "H265", 4)){
                        pcinfo->media_info[type].codec = RTP_CODEC_H265;
                    }
                    else if (0 == strncmp(codec, "PCMA", 4)){
                        pcinfo->media_info[type].codec = RTP_CODEC_G711A;
                    }
                    else if (0 == strncmp(codec, "PCMU", 4)){
                        pcinfo->media_info[type].codec = RTP_CODEC_G711U;
                    }
                    else if (0 == strncmp(codec, "L16", 3) || 0 == strncmp(codec, "PCM", 3)){
                        pcinfo->media_info[type].codec = RTP_CODEC_PCM;
                    }
                    else {
                        printf("unknown codec: %s\n", line);
                    }
                }
                if (sample_rate > line){
                    sscanf(sample_rate, "%d", &pcinfo->media_info[type].sample_rate);
                }
                if (param > line){
                    sscanf(param, "%d", &pcinfo->media_info[type].channels);
                }
            }
        }

        line = strtok(NULL, "\r\n");

    }

    int i;
    for(i = RTSP_MEDIA_TYPE_VIDEO; i < RTSP_MEDIA_TYPE_NUM; i++){
        rtsp_media_info_t* pinfo = &pcinfo->media_info[i];
        if (pinfo->track_avail == 1
            && (pinfo->codec == RTP_CODEC_INVALID
            || pinfo->sample_rate == 0
            || pinfo->track_uri[0] == 0)){
            printf("sdp error\n");
            printf("%s\n", pmsg->body);
            return -1;
        }
    }
    return 0;
}

static int __parse_setup(rtsp_client_info_s* pcinfo, rtsp_msg_t* pmsg)
{
    rtsp_session_t* psession = &pcinfo->session;

    strncpy(psession->session_id, pmsg->headers[SESSION], sizeof(psession->session_id));

    if (psession->mode == RTSP_TRANSPORT_MODE_UDP){
        char* server_port = strstr(pmsg->headers[TRANSPORT], "server_port=");
        if (server_port){
            int data_port, ctrl_port;
            int match = sscanf(server_port, "server_port=%d-%d", &data_port, &ctrl_port);
            if (2 != match){
                printf("parse server_port failed: %s\n", pmsg->headers[TRANSPORT]);
                return -1;
            }

            if (pcinfo->status == RTSP_CLIENT_STATUS_SETUP_VID_SENT){
                psession->transport[RTSP_VID_DATA_CHN].svr_port = data_port;
                psession->transport[RTSP_VID_CTRL_CHN].svr_port = ctrl_port;
            }
            else{
                psession->transport[RTSP_AUD_DATA_CHN].svr_port = data_port;
                psession->transport[RTSP_AUD_CTRL_CHN].svr_port = ctrl_port;
            }
        }
        else{
            printf("no server_port in header: %s", pmsg->headers[TRANSPORT]);
            return -1;
        }
    }

    return 0;
}

static int __put_rtp_frm(void* pri_data, rtsp_data_packet_s* pkt)
{
    rtsp_client_info_s* pcinfo = (rtsp_client_info_s*)pri_data;
    RTSP_CLIENT_HANDLE handle = __client_info_to_handle(pcinfo);

    return pcinfo->put_data_cb(handle, pkt, pcinfo->cb_priv);
}

static int __parse_play(rtsp_client_info_s* pcinfo, rtsp_msg_t* pmsg)
{
    uint32_t begin_seq = 0;
    char* rtp_info = pmsg->headers[RTP_INFO];
    if (rtp_info){
        char* seq = strstr(rtp_info, "seq=");
        if (seq){
            sscanf(seq, "seq=%u", &begin_seq);
        }
    }
    pcinfo->session.begin_seq = begin_seq;

    rtp_client_param_s param = {
        .psession = &pcinfo->session,
        .pmedia_info = pcinfo->media_info,
        .put_rtp_frm = __put_rtp_frm,
        .cb_pri_data = pcinfo,
    };

    pcinfo->rtp_handle = rtp_client_create(&param);
    if (!pcinfo->rtp_handle){
        return -1;
    }

    return 0;
}

static int __parse_www_authenticate(rtsp_client_info_s* prci, char* www_auth)
{
    if (!www_auth){
        printf("no WWW_Authenticate header\n");
        return -1;
    }

    char* digest = strstr(www_auth, "Digest");
    if (!digest){
        printf("server don't support digest");
        return -1;
    }

    char* realm = strstr(digest, "realm=\"");
    if (!realm){
        printf("no realm\n");
        return -1;
    }

    realm += strlen("realm=\"");
    char* realm_end = strstr(realm, "\"");
    if (!realm_end){
        printf("no realm end\n");
        return -1;
    }

    char* nonce = strstr(realm_end, "nonce=\"");
    if (!nonce){
        printf("no nonce\n");
        return -1;
    }

    nonce += strlen("nonce=\"");

    char* nonce_end = strstr(nonce, "\"");
    if (!nonce_end){
        printf("no nonce end\n");
        return -1;
    }

    *realm_end = '\0';
    *nonce_end = '\0';
    strncpy(prci->realm, realm, sizeof(prci->realm));
    strncpy(prci->nonce, nonce, sizeof(prci->nonce));
    return 0;
}

int __handle_incoming_msg(void* pri_data, rtsp_msg_t* pmsg)
{
    rtsp_client_info_s* pclient = (rtsp_client_info_s*)pri_data;
    int ret = 0;

    rtsp_print_msg(pmsg);

    if (pmsg->type == RTSP_MSG_RESPONSE) {

        if (pmsg->status != OK){
            if (pmsg->status == UNAUTHORIZED){
                ret = __parse_www_authenticate(pclient, pmsg->headers[WWW_AUTHENTICATE]);
                if (0 != ret){
                    return -1;
                }
                else{
                    pclient->status = RTSP_CLIENT_STATUS_OPTIONS_SENT;
                }

            }
            else {
                printf("msg: %s\n", pmsg->status_msg);
                return -1;
            }
        }

        switch (pclient->status)
        {
        case RTSP_CLIENT_STATUS_OPTIONS_SENT:
            pclient->status = RTSP_CLIENT_STATUS_OPTIONS_RESP;
            break;

        case RTSP_CLIENT_STATUS_DESCRIBE_SENT:
            ret = __parse_describe(pclient, pmsg);
            if (ret != 0){
                return -1;
            }
            else{
                pclient->status = RTSP_CLIENT_STATUS_DESCRIBE_RESP;
            }
            break;

        case RTSP_CLIENT_STATUS_SETUP_VID_SENT:
            ret = __parse_setup(pclient, pmsg);
            if (ret != 0){
                return -1;
            }
            else {
                pclient->status = RTSP_CLIENT_STATUS_SETUP_VID_RESP;
            }
            break;

        case RTSP_CLIENT_STATUS_SETUP_AUD_SENT:
            ret = __parse_setup(pclient, pmsg);
            if (ret != 0){
                return -1;
            }
            else {
                pclient->status = RTSP_CLIENT_STATUS_SETUP_AUD_RESP;
            }
            break;

        case RTSP_CLIENT_STATUS_PLAY_SENT:
            ret = __parse_play(pclient, pmsg);
            if (ret != 0){
                return -1;
            }
            else {
                pclient->status = RTSP_CLIENT_STATUS_PLAY_RESP;
            }
            break;

        case RTSP_CLIENT_STATUS_PLAYING:
            break;

        case RTSP_CLIENT_STATUS_TEARDOWN_SENT:
            pclient->status = RTSP_CLIENT_STATUS_TEARDOWN_RESP;
            break;

        default:
            printf("unhandled response:\n");
            rtsp_print_msg(pmsg);
            break;
        }

        pclient->cseq++;
    }
    else {
        //request
    }

    return ret;
}

int __handle_incoming_rtp(void* pri_data, int rtp_chn, char* rtp_buf, int buf_len)
{
    rtsp_client_info_s* pcinfo = (rtsp_client_info_s*)pri_data;
    rtsp_session_t* psession = &pcinfo->session;
    RTSP_SESSION_CHN_E chn = RTSP_SESSION_CHN_NUM;

    if (psession->mode == RTSP_TRANSPORT_MODE_TCP){
        if (rtp_chn == psession->transport[RTSP_VID_DATA_CHN].interleave){
            chn = RTSP_VID_DATA_CHN;
        }
        else if (rtp_chn == psession->transport[RTSP_VID_CTRL_CHN].interleave){
            chn = RTSP_VID_CTRL_CHN;
        }
        else if (rtp_chn == psession->transport[RTSP_AUD_DATA_CHN].interleave){
            chn = RTSP_AUD_DATA_CHN;
        }
        else if (rtp_chn == psession->transport[RTSP_AUD_CTRL_CHN].interleave){
            chn = RTSP_AUD_CTRL_CHN;
        }
        else {
            printf("unknown interleave(%d)\n", rtp_chn);
        }
    }

    if (chn != RTSP_SESSION_CHN_NUM){
        rtp_client_handle_packet(pcinfo->rtp_handle, chn, rtp_buf, buf_len);
    }

    return 0;
}

static int rtsp_client_read_data(rtsp_client_info_s* pcinfo)
{
    int ret = 0;

    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(&pcinfo->rx_buf, &buf);
    if (len > 0){
        ret = sock_read(pcinfo->fd, buf, len);
        if (ret > 0){
            byte_array_buf_widx_advance(&pcinfo->rx_buf, ret);
            
            ret = rtsp_common_parse_data(pcinfo, __handle_incoming_msg, __handle_incoming_rtp,
                                    &pcinfo->rx_buf, &pcinfo->msg);
        }
        else{
            printf("sock(%d) read failed\n", pcinfo->fd);
            ret = -1;
        }
    }
    byte_array_buf_write_ptr_unlock(&pcinfo->rx_buf, 0);

    return ret;
}

static int rtsp_client_write_data(rtsp_client_info_s* pcinfo)
{
    int ret = 0;
    char* buf = NULL;
    int len = 0;
    rtsp_client_handle_s* phdl = rtsp_client_get_handle();

    switch (pcinfo->status)
    {
    case RTSP_CLIENT_STATUS_CONNECTING:
        ret = tcp_connect_status_get(pcinfo->fd);
        if (0 == ret){ //connect success
            printf("%p connect success\n", pcinfo);
            pcinfo->status = RTSP_CLIENT_STATUS_CONNECTED;
        }
        else{//fail
            printf("%p connect failed\n", pcinfo);
            pcinfo->reactivate_ms = phdl->now_ms + SLEEP_AFTER_CONN_FAIL_MS;
            if (pcinfo->status_change_cb){
                pcinfo->status_change_cb(__client_info_to_handle(pcinfo), RTSP_CLIENT_STATUS_CONNECT_FAIL, pcinfo->cb_priv);
            }
            return -1;
        }
        break;

    default:
        len = byte_array_buf_read_ptr_lock(&pcinfo->tx_buf, &buf);
        if (len > 0) {
            ret = sock_write(pcinfo->fd, buf, len);
            if (ret > 0) {
                byte_array_buf_ridx_advance(&pcinfo->tx_buf, ret);
                byte_array_buf_trim(&pcinfo->tx_buf);
            }
            else{
                ret = -1;
                printf("sock(%d) write failed\n", pcinfo->fd);
            }
        }
        byte_array_buf_read_ptr_unlock(&pcinfo->tx_buf, 0);
        break;
    }

    return 0;
}

static int __build_authorization(char* username, char* passwd, char* method, char* uri, char* realm, char* nonce,
                                char* output, int len)
{
    char a[256];
    char a1_md5[40] = {0};
    snprintf(a, sizeof(a), "%s:%s:%s", username, realm, passwd);
    md5_encode(a, strlen(a), a1_md5);

    char a2_md5[40] = {0};
    snprintf(a, sizeof(a), "%s:%s", method, uri);
    md5_encode(a, strlen(a), a2_md5);

    char a3_md5[40] = {0};
    snprintf(a, sizeof(a), "%s:%s:%s", a1_md5, nonce, a2_md5);
    md5_encode(a, strlen(a), a3_md5);

    int ret = snprintf(output, len, "Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"",
            username, realm, nonce, uri, a3_md5);
    if (ret >= len){
        printf("authorization buffer too short\n");
        return -1;
    }

    return 0;
}

static int __build_request_common(rtsp_client_info_s* pcinfo, rtsp_msg_t* preq)
{
    char cseq_buf[8] = {0};
    char authorization_buf[512] = {0};

    snprintf(cseq_buf, sizeof(cseq_buf), "%d", pcinfo->cseq);
    preq->headers[CSEQ] = cseq_buf;
    preq->type = RTSP_MSG_REQUEST;
    strcpy(preq->version, "1.0");

    if (pcinfo->realm[0] != '\0' && pcinfo->nonce[0] != '\0'){
        __build_authorization(pcinfo->username, pcinfo->password, rtsp_get_method_str(preq->method), pcinfo->url,
                            pcinfo->realm, pcinfo->nonce, authorization_buf, sizeof(authorization_buf));
        preq->headers[AUTHORIZATION] = authorization_buf;
    }

    rtsp_print_msg(preq);

    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(&pcinfo->tx_buf, &buf);

    int ret = rtsp_pack_request(preq, buf, len);
    byte_array_buf_write_ptr_unlock(&pcinfo->tx_buf, ret);

    if (ret == 0){
        return -1;
    }

    return 0;
}

static int __build_options(rtsp_client_info_s* pcinfo)
{
    rtsp_msg_t msg = {0};
    msg.method = OPTIONS;
    strncpy(msg.uri, pcinfo->url, sizeof(msg.uri));

    return __build_request_common(pcinfo, &msg);
}

static int __build_describe(rtsp_client_info_s* pcinfo)
{
    rtsp_msg_t msg = {0};
    msg.method = DESCRIBE;
    msg.headers[ACCEPT] = "application/sdp";
    strncpy(msg.uri, pcinfo->url, sizeof(msg.uri));

    return __build_request_common(pcinfo, &msg);
}

static int __build_setup(rtsp_client_info_s* pcinfo, RTSP_MEDIA_TYPE_E type)
{
    rtsp_msg_t msg = {0};
    rtsp_session_t *psession = &pcinfo->session;
    msg.method = SETUP;
    strncpy(msg.uri, pcinfo->media_info[type].track_uri, sizeof(msg.uri));

    int data_chn = (type == RTSP_MEDIA_TYPE_VIDEO ? RTSP_VID_DATA_CHN : RTSP_AUD_DATA_CHN);
    int ctrl_chn = (type == RTSP_MEDIA_TYPE_VIDEO ? RTSP_VID_CTRL_CHN : RTSP_AUD_CTRL_CHN);

    char transport_buf[64] = {0};
    if (RTSP_TRANSPORT_MODE_TCP == psession->mode) {
        psession->transport[data_chn].interleave = data_chn;
        psession->transport[ctrl_chn].interleave = ctrl_chn;
        snprintf(transport_buf, sizeof(transport_buf), "RTP/AVP/TCP;unicast;interleaved=%d-%d",
                psession->transport[data_chn].interleave,
                psession->transport[ctrl_chn].interleave);
    }
    else {
        int data_fd = psession->transport[data_chn].fd;
        int ctrl_fd = psession->transport[ctrl_chn].fd;
        if (data_fd > 0){
            close(data_fd);
        }
        if (ctrl_fd > 0){
            close(ctrl_fd);
        }

        int ports[2], fds[2];
        udp_continuous_bind(ports, fds);
        fd_set_nonblocking(fds[0]);
        fd_set_nonblocking(fds[1]);

        psession->transport[data_chn].cli_port = ports[0];
        psession->transport[ctrl_chn].cli_port = ports[1];
        psession->transport[data_chn].fd= fds[0];
        psession->transport[ctrl_chn].fd= fds[1];

        snprintf(transport_buf, sizeof(transport_buf), "RTP/AVP;unicast;client_port=%d-%d",
                psession->transport[data_chn].cli_port,
                psession->transport[ctrl_chn].cli_port);
    }

    msg.headers[TRANSPORT] = transport_buf;
    if (psession->session_id[0] != '\0'){
        msg.headers[SESSION] = psession->session_id;
    }

    return __build_request_common(pcinfo, &msg);
}

static int __build_play(rtsp_client_info_s* pcinfo)
{
    rtsp_msg_t msg = {0};
    msg.method = PLAY;
    strncpy(msg.uri, pcinfo->url, sizeof(msg.uri));
    msg.headers[SESSION] = pcinfo->session.session_id;
    msg.headers[RANGE] = "npt=0.000-";

    return __build_request_common(pcinfo, &msg);
}

static int __build_teardown(rtsp_client_info_s* pcinfo)
{
    rtsp_msg_t msg = {0};
    msg.method = TEARDOWN;
    strncpy(msg.uri, pcinfo->url, sizeof(msg.uri));
    msg.headers[SESSION] = pcinfo->session.session_id;

    return __build_request_common(pcinfo, &msg);
}

static int __prepare_data_to_send(rtsp_client_info_s* pcinfo, fd_set* prfds, fd_set* pwfds, int* pmax_fd, uint64_t now_ms)
{
    int ret = 0;

    switch (pcinfo->status){
    //若未建立连接，则先connect
    case RTSP_CLIENT_STATUS_INIT:
        if (pcinfo->status_change_cb){
            pcinfo->status_change_cb(__client_info_to_handle(pcinfo), pcinfo->status, pcinfo->cb_priv);
        }
        if (!pcinfo->stop){

            pcinfo->fd = tcp_connect(pcinfo->ip, pcinfo->port);
            if (pcinfo->fd > 0){
                pcinfo->status = RTSP_CLIENT_STATUS_CONNECTING;
                _add_fd_to_set(pwfds, pcinfo->fd, pmax_fd);
            }
            else{
                pcinfo->reactivate_ms = now_ms + SLEEP_AFTER_CONN_FAIL_MS;
                if (pcinfo->status_change_cb){
                    pcinfo->status_change_cb(__client_info_to_handle(pcinfo), RTSP_CLIENT_STATUS_CONNECT_FAIL, pcinfo->cb_priv);
                }
                printf("connect failed\n");
            }
        }
        break;

    case RTSP_CLIENT_STATUS_CONNECTING:
        _add_fd_to_set(pwfds, pcinfo->fd, pmax_fd);
        break;

    case RTSP_CLIENT_STATUS_CONNECTED:
        if (pcinfo->status_change_cb){
            pcinfo->status_change_cb(__client_info_to_handle(pcinfo), pcinfo->status, pcinfo->cb_priv);
        }
        ret = __build_options(pcinfo);
        if (ret == 0){
            pcinfo->status = RTSP_CLIENT_STATUS_OPTIONS_SENT;
        }
        break;

    case RTSP_CLIENT_STATUS_OPTIONS_RESP:
        ret = __build_describe(pcinfo);
        if (ret == 0){
            pcinfo->status = RTSP_CLIENT_STATUS_DESCRIBE_SENT;
        }
        break;

    //先setup视频，后setup音频
    case RTSP_CLIENT_STATUS_DESCRIBE_RESP:
        if (pcinfo->media_info[RTSP_MEDIA_TYPE_VIDEO].track_avail){
            ret = __build_setup(pcinfo, RTSP_MEDIA_TYPE_VIDEO);
            if (ret == 0){
                pcinfo->status = RTSP_CLIENT_STATUS_SETUP_VID_SENT;
            }
        }
        else if (pcinfo->media_info[RTSP_MEDIA_TYPE_AUDIO].track_avail){
            ret = __build_setup(pcinfo, RTSP_MEDIA_TYPE_AUDIO);
            if (ret == 0){
                pcinfo->status = RTSP_CLIENT_STATUS_SETUP_AUD_SENT;
            }
        }
        else {
            if (pcinfo->session.session_id[0] != '\0'){
                ret = __build_teardown(pcinfo);
                if (ret == 0){
                    pcinfo->status = RTSP_CLIENT_STATUS_TEARDOWN_SENT;
                }
            }
        }
        break;

    case RTSP_CLIENT_STATUS_SETUP_VID_RESP:
        if (pcinfo->media_info[RTSP_MEDIA_TYPE_AUDIO].track_avail){
            ret = __build_setup(pcinfo, RTSP_MEDIA_TYPE_AUDIO);
            if (ret == 0){
                pcinfo->status = RTSP_CLIENT_STATUS_SETUP_AUD_SENT;
            }
        }
        else {
            ret = __build_play(pcinfo);
            if (ret == 0){
                pcinfo->status = RTSP_CLIENT_STATUS_PLAY_SENT;
            }
        }
        break;

    case RTSP_CLIENT_STATUS_SETUP_AUD_RESP:
        ret = __build_play(pcinfo);
        if (ret == 0){
            pcinfo->status = RTSP_CLIENT_STATUS_PLAY_SENT;
        }
        break;

    case RTSP_CLIENT_STATUS_PLAY_RESP:
        pcinfo->status = RTSP_CLIENT_STATUS_PLAYING;
        pcinfo->hb_ms = now_ms;
        if (pcinfo->status_change_cb){
            pcinfo->status_change_cb(__client_info_to_handle(pcinfo), pcinfo->status, pcinfo->cb_priv);
        }
        break;

    case RTSP_CLIENT_STATUS_PLAYING:
        if (now_ms - pcinfo->hb_ms > HEART_BEAT_PERIOD_MS){
            ret = __build_options(pcinfo);
            if (ret == 0){
                pcinfo->hb_ms = now_ms;
            }
        }
        //在udp模式下需要添加对应的文件描述符
        rtsp_session_t* psession = &pcinfo->session;
        if (psession->mode == RTSP_TRANSPORT_MODE_UDP){

            int i;
            for(i = 0; i < RTSP_SESSION_CHN_NUM; i++){
                _add_fd_to_set(prfds, psession->transport[i].fd, pmax_fd);
                if (byte_array_buf_data_len(&psession->transport[i].tx_buf) > 0){
                    _add_fd_to_set(pwfds, psession->transport[i].fd, pmax_fd);
                }
            }
        }


        break;

    case RTSP_CLIENT_STATUS_TEARDOWN_RESP:
        __reset_client(pcinfo);
        break;

    default:
        break;
    }

    if (pcinfo->stop){
        //当需要删除一个client时，先发送teardown消息，将状态置为STOP，在下一个循环中进行实际移除
        if (pcinfo->session.session_id[0] != '\0'){
            
            ret = __build_teardown(pcinfo);
            if (ret == 0){
                pcinfo->status = RTSP_CLIENT_STATUS_STOP;
            }
        }
        else {
            pcinfo->status = RTSP_CLIENT_STATUS_STOP;
        }
    }

    if (pcinfo->status >= RTSP_CLIENT_STATUS_CONNECTED
        && pcinfo->status < RTSP_CLIENT_STATUS_TEARDOWN_RESP) {
        _add_fd_to_set(prfds, pcinfo->fd, pmax_fd);
    }

    if (byte_array_buf_data_len(&pcinfo->tx_buf) > 0) {
        _add_fd_to_set(pwfds, pcinfo->fd, pmax_fd);
    }

    return ret;
}

static void* rtsp_client_proc(void* arg)
{
    prctl(PR_SET_NAME, "rtsp_client");

    rtsp_client_handle_s* phdl = (rtsp_client_handle_s*)arg;
    fd_set rfds, wfds;
    int max_fd;

    while(!(phdl->thread_exit && phdl->head == NULL)){
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        FD_SET(phdl->pfd[0], &rfds);
        max_fd = phdl->pfd[0];

        phdl->now_ms = get_monotonic_time_ms();
        //添加fd
        rtsp_client_info_s* pcinfo = phdl->head;
        while(pcinfo) {
            rtsp_client_info_s* next = pcinfo->next;

            //线程退出时确保先把所有client都停止，且释放资源
            if (phdl->thread_exit){
                pcinfo->stop = 1;
            }

            if (phdl->now_ms >= pcinfo->reactivate_ms){
                if (pcinfo->status == RTSP_CLIENT_STATUS_STOP){
                    __remove_client(phdl, pcinfo);
                }
                else{
                    __prepare_data_to_send(pcinfo, &rfds, &wfds, &max_fd, phdl->now_ms);
                }
            }

            if (pcinfo->recv_ms > 0 && phdl->now_ms - pcinfo->recv_ms > 5000){
                printf("recv timeout(5000ms), reconnect\n");
                __reset_client(pcinfo);
            }

            pcinfo = next;
        }

        //select
        struct timeval timeout = {
            .tv_sec = 1,
            .tv_usec = 0,
        };

        int ret = select(max_fd + 1, &rfds, &wfds, NULL, &timeout);
        if (ret == 0) {
            //timeout
        }
        else if (ret < 0){
            if (errno == EINTR){
                continue;
            }
            else{
                printf("select error: %d\n", errno);
		usleep(1000);
                return NULL;
            }
        }
        else{
            if (FD_ISSET(phdl->pfd[0], &rfds)){  //收到管道消息
                rtsp_client_msg_s msg = {0};
                __read_from_pipe(phdl, &msg);
                switch (msg.type){

                case RTSP_CLIENT_MSG_ADD:
                    __add_client_info_to_list(phdl, msg.pinfo);
                    break;

                case RTSP_CLIENT_MSG_REMOVE:
                    msg.pinfo->stop = 1;
                    break;

                default:
                    break;
                }
            }

            phdl->now_ms = get_monotonic_time_ms();
            //处理每一路client读写
            pcinfo = phdl->head;
            while(pcinfo) {
                rtsp_client_info_s* next = pcinfo->next;

                if (pcinfo->fd > 0 && FD_ISSET(pcinfo->fd, &rfds)){
                    pcinfo->recv_ms = phdl->now_ms;
                    ret = rtsp_client_read_data(pcinfo);

                    if (ret < 0 && ret != RTSP_ERR_NEED_MORE_DATA){
                        __reset_client(pcinfo);
                    }
                }

                if (pcinfo->fd > 0 && FD_ISSET(pcinfo->fd, &wfds)){
                    if (0 != rtsp_client_write_data(pcinfo)){
                        __reset_client(pcinfo);
                    }
                }
                //处理UDP连接的数据
                rtsp_session_t* psession = &pcinfo->session;
                if (psession->mode == RTSP_TRANSPORT_MODE_UDP){
                    int i;
                    for(i = 0; i < RTSP_SESSION_CHN_NUM; i++){
                        int fd = psession->transport[i].fd;
                        if (fd > 0 && FD_ISSET(fd, &rfds)) {
                            pcinfo->recv_ms = phdl->now_ms;
                            rtp_client_recv_data(pcinfo->rtp_handle, i);
                        }
                        if (fd > 0 && FD_ISSET(fd, &wfds)) {
                            rtp_client_send_data(pcinfo->rtp_handle, i);
                        }
                    }
                }

                pcinfo = next;
            }
        }

    }

    printf("rtsp client exit\n");

    return NULL;
}

int rtsp_client_init()
{
    rtsp_client_handle_s* phdl = rtsp_client_get_handle();

    if (phdl->inited){
        printf("rtsp_client is inited already\n");
        return 0;
    }

    memset(phdl, 0, sizeof(rtsp_client_handle_s));

    int ret = pipe(phdl->pfd);
    if (ret != 0){
        printf("create pipe failed(%d)\n", ret);
        return -1;
    }

    fd_set_nonblocking(phdl->pfd[0]);
    fd_set_nonblocking(phdl->pfd[1]);

    if (0 != pthread_create(&phdl->tid, NULL, rtsp_client_proc, phdl)){
        printf("create thread failed\n");
        return -1;
    }

    wheel_timer_init();

    phdl->inited = 1;

    return 0;
}

int rtsp_client_deinit()
{
    rtsp_client_handle_s* phdl = rtsp_client_get_handle();

    if (!phdl->inited){
        printf("rtsp client is not inited\n");
        return -1;
    }

    wheel_timer_deinit();

    close(phdl->pfd[0]);
    close(phdl->pfd[1]);
    phdl->thread_exit = 1;
    printf("wait rtsp thread to exit\n");
    pthread_join(phdl->tid, NULL);

    phdl->inited = 0;

    return 0;
}

//返回handle
RTSP_CLIENT_HANDLE rtsp_client_start(rtsp_client_param_s* param)
{
    rtsp_client_handle_s* phdl = rtsp_client_get_handle();

    if (!phdl->inited){
        printf("rtsp client not inited yet\n");
        return NULL;
    }

    int ret = 0;

    rtsp_client_info_s* pinfo = malloc(sizeof(rtsp_client_info_s));
    if (NULL == pinfo){
        printf("rtsp client malloc failed\n");
        return NULL;
    }
    memset(pinfo, 0, sizeof(rtsp_client_info_s));

    //初始化
    pinfo->stop = 0;
    pinfo->put_data_cb = param->put_data_cb;
    pinfo->status_change_cb = param->status_change_cb;
    pinfo->cb_priv = param->priv;
    pinfo->cseq = 1;
    pinfo->status = RTSP_CLIENT_STATUS_INIT;

    pinfo->session.mode = param->trans_mode;
    pinfo->session.ptx_buf = &pinfo->tx_buf;
    pinfo->session.prx_buf = &pinfo->rx_buf;

    strncpy(pinfo->username, param->username, sizeof(pinfo->username));
    strncpy(pinfo->password, param->password, sizeof(pinfo->password));
    strncpy(pinfo->url, param->url, sizeof(pinfo->url));
    
    byte_array_buf_init(&pinfo->rx_buf, NULL, 64 * 1024, 0);
    byte_array_buf_init(&pinfo->tx_buf, NULL, 64 * 1024, 0);

    ret = __parse_ip_port_from_url(pinfo->url, pinfo->ip, sizeof(pinfo->ip), &pinfo->port);
    if (0 != ret){
        return NULL;
    }

    strncpy(pinfo->session.server_ip, pinfo->ip, sizeof(pinfo->session.server_ip));

    //向rtsp client主线程发消息添加一个client
    rtsp_client_msg_s msg = {
        .type = RTSP_CLIENT_MSG_ADD,
        .pinfo = pinfo,
    };
    __send_to_pipe(phdl, &msg);

    return __client_info_to_handle(pinfo);
}

int rtsp_client_stop(RTSP_CLIENT_HANDLE handle)
{
    rtsp_client_handle_s* phdl = rtsp_client_get_handle();
    if (!phdl->inited){
        printf("rtsp client not inited yet\n");
        return -1;
    }

    rtsp_client_info_s* pcinfo = __client_info_from_handle(handle);
    if (NULL == pcinfo){
        printf("invalid handle\n");
        return -1;
    }

    rtsp_client_msg_s msg = {
        .type = RTSP_CLIENT_MSG_REMOVE,
        .pinfo = pcinfo,
    };
    __send_to_pipe(phdl, &msg);

    return 0;
}

int rtsp_client_get_media_info(RTSP_CLIENT_HANDLE handle, rtsp_media_info_t pmedia_info[RTSP_MEDIA_TYPE_NUM])
{
    rtsp_client_info_s* pcinfo = __client_info_from_handle(handle);
    if (NULL == pcinfo){
        printf("invalid handle\n");
        return -1;
    }

    if (pmedia_info == NULL){
        printf("parameter error\n");
        return -1;
    }

    if (pcinfo->status < RTSP_CLIENT_STATUS_DESCRIBE_RESP){
        printf("media info not ready, client status(%d)\n", pcinfo->status);
        return -1;
    }

    memcpy(pmedia_info, pcinfo->media_info, sizeof(rtsp_media_info_t) * RTSP_MEDIA_TYPE_NUM);

    return 0;
}
