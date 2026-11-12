#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include "rtsp.h"
#include "rtsp_session.h"
#include "byte_array_buf.h"
#include "sock_util.h"
#include "byte_array_buf.h"
#include "rtsp_server.h"
#include "rtp_server.h"
#include "utils.h"
#include "uni_md5.h"
#include "uni_base64.h"
#include "rtsp_internal.h"
#include "rtsp_common.h"
#include "sdp.h"

#define USER_NAME_LEN 64
#define RTSP_SERVER_REALM "Tuya RTSP Server"
#define RTSP_SERVER_KEY "rtsp_key123"

#define MAX_RTSP_SESSION 5
#define MAX_STREAM_SRC 5

typedef enum {
    RTSP_SERVER_CMD_NULL,
    RTSP_SERVER_CMD_REMOVE_ALL_CLIENTS,
} RTSP_SERVER_CMD_E;

typedef struct rtsp_client_t {
    struct rtsp_client_t* next;
    rtsp_session_t session;
    RTP_SERVER_HANDLE svr_handle;
    byte_array_buf_t rx_buf;
    byte_array_buf_t tx_buf;
    rtsp_msg_t in_msg;
    int cseq;
    int sockfd;
    uint64_t hb_ts; //heart beat time stamp
    uint8_t teardown;
} rtsp_client_t;

typedef struct {
    rtsp_client_t* client_head;
    int active_session_num;
    int pfd[2];
    pthread_t pid;
    int thread_exit;
    int sockfd;
    rtsp_stream_src_t stream_src[MAX_STREAM_SRC];
    uint64_t now_ms;
    char username[USER_NAME_LEN];
    char password[USER_NAME_LEN];
    int remove_all_clients;

} rtsp_server_handle_t;

static rtsp_server_handle_t* rtsp_server_get_handle()
{
    static rtsp_server_handle_t hdl = {0};

    return &hdl;
}

static void __send_cmd_to_server(RTSP_SERVER_CMD_E cmd)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    char data = cmd&0xff;
    write(phdl->pfd[1], &data, 1);
}

static int _rtsp_server_reply(rtsp_msg_t* presp, rtsp_client_t* pclient)
{
    char* buf = NULL;

    int size = byte_array_buf_write_ptr_lock(&pclient->tx_buf, &buf);
    int bytes_written = rtsp_pack_response(presp, buf, size);
    byte_array_buf_write_ptr_unlock(&pclient->tx_buf, bytes_written);

    return 0;
}

static int _rtsp_server_reply_msg(rtsp_client_t* pclient, RTSP_STATUS_CODE_E status, char* msg)
{
    rtsp_msg_t resp = {0};
    char buf[8];
    sprintf(buf, "%d", pclient->cseq);

    strncpy(resp.status_msg, msg, sizeof(resp.status_msg));
    strncpy(resp.version, "1.0", sizeof(resp.version));
    resp.status = status;
    resp.headers[CSEQ] = buf;

    return _rtsp_server_reply(&resp, pclient);
}

static int _rtsp_server_check_auth(rtsp_msg_t* preq, rtsp_msg_t* presp, char* buf, int buf_size)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    do {
        if (preq->headers[AUTHORIZATION] == NULL){
            printf("no Authorization header\n");
            break;
        }
        char *username, *realm, *nonce, *response, *uri, *tmp;
        int username_len, realm_len, nonce_len, response_len, uri_len;
        //parse username
        tmp = strstr(preq->headers[AUTHORIZATION], "username=\"");
        if (tmp == NULL){
            printf("no username\n");
            break;
        }
        username = tmp + strlen("username=\"");

        tmp = strstr(username, "\"");
        if (tmp == NULL){
            printf("no username end\n");
            break;
        }
        username_len = tmp - username;
        //parse realm 
        tmp = strstr(preq->headers[AUTHORIZATION], "realm=\"");
        if (tmp == NULL){
            printf("no realm\n");
            break;
        }
        realm = tmp + strlen("realm=\"");

        tmp = strstr(realm, "\"");
        if (tmp == NULL){
            printf("no realm end\n");
            break;
        }
        realm_len= tmp - realm;
        //parse nonce
        tmp = strstr(preq->headers[AUTHORIZATION], "nonce=\"");
        if (tmp == NULL){
            printf("no nonce\n");
            break;
        }
        nonce = tmp + strlen("nonce=\"");

        tmp = strstr(nonce, "\"");
        if (tmp == NULL){
            printf("no nonce end\n");
            break;
        }
        nonce_len= tmp - nonce;
        //parse response
        tmp = strstr(preq->headers[AUTHORIZATION], "response=\"");
        if (tmp == NULL){
            printf("no response\n");
            break;
        }
        response = tmp + strlen("response=\"");

        tmp = strstr(response, "\"");
        if (tmp == NULL){
            printf("no response end\n");
            break;
        }
        response_len= tmp - response; 
        //parse uri 
        tmp = strstr(preq->headers[AUTHORIZATION], "uri=\"");
        if (tmp == NULL){
            printf("no uri\n");
            break;
        }
        uri = tmp + strlen("uri=\"");

        tmp = strstr(uri , "\"");
        if (tmp == NULL){
            printf("no uri end\n");
            break;
        }
        uri_len= tmp - uri; 

        if (strlen(phdl->username) != username_len || strncmp(phdl->username, username, username_len)){
            printf("username error\n");
            break;
        }
        if (strlen(RTSP_SERVER_REALM) != realm_len || strncmp(RTSP_SERVER_REALM, realm, realm_len)){
            printf("realm error\n");
            break;
        }
        //check timestamp
        char ts_md5_a1[128] = {0};
        char bak = nonce[nonce_len];
        nonce[nonce_len] = 0;
        tuya_base64_decode(nonce, (unsigned char*)ts_md5_a1);

        uint64_t ts;
        sscanf(ts_md5_a1, "%" PRIu64, &ts);
        if (phdl->now_ms - ts > 5 * 60 * 1000){
            //large than 5 mins
            printf("timestamp error\n");
            break;
        }
        //check response
        char a1[128];
        char a1_md5[40] = {0};
        sprintf(a1, "%s:%s:%s", phdl->username, RTSP_SERVER_REALM, phdl->password);
        md5_encode(a1, strlen(a1), a1_md5);

        char a2[128];
        char a2_md5[40] = {0};
        char* method_str = rtsp_get_method_str(preq->method);
        int method_len = strlen(method_str);
        memcpy(a2, method_str, method_len);
        memcpy(a2 + method_len, ":", 1);
        memcpy(a2 + method_len + 1, uri, uri_len);
        md5_encode(a2, method_len + 1 + uri_len, a2_md5);

        char a3[256];
        char a3_md5[40] = {0};
        sprintf(a3, "%s:%s:%s", a1_md5, nonce, a2_md5);
        md5_encode(a3, strlen(a3), a3_md5);

        if (strncmp(a3_md5, response, response_len)){
            printf("response error\n");
            break;
        }

        nonce[nonce_len] = bak;

        return 0;

    } while(0);

    strcpy(presp->version, "1.0");
    presp->status = UNAUTHORIZED;
    strcpy(presp->status_msg, "Unauthorized");
    presp->headers[CSEQ] = preq->headers[CSEQ];

    //a1=timestamp+random+uri+key
    //nonce=base64(timestamp:md5(a1))
    char nonce[128];
    char a1[128];
    char a1_md5[64] = {0};

    sprintf(a1, "%" PRIu64 "%04d%s%s", phdl->now_ms, rand() % 10000, preq->uri, RTSP_SERVER_KEY);
    sprintf(a1_md5, "%" PRIu64 ":", phdl->now_ms);

    md5_encode(a1, strlen(a1), a1_md5 + strlen(a1_md5));
    tuya_base64_encode((unsigned char*)a1_md5, nonce, strlen(a1_md5));

    buf[buf_size - 1] = '\0';
    snprintf(buf, buf_size - 1, "Digest realm=\"%s\", nonce=\"%s\"", RTSP_SERVER_REALM, nonce);
    presp->headers[WWW_AUTHENTICATE] = buf;

    return -1;
}

static int _rtsp_server_pre_hdl_req(rtsp_msg_t* preq, rtsp_client_t* pclient)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    int ret = 0;

    int cseq = atoi(preq->headers[CSEQ]);
    if (cseq <= 0){
        _rtsp_server_reply_msg(pclient, BAD_REQUEST, "Bad Request");
        return -1;
    }

    pclient->cseq = cseq;


    //authorization
    if (preq->method != OPTIONS && phdl->username[0] != '\0'){
        rtsp_msg_t resp = {0};
        char auth_buf[256] = {0};
        ret = _rtsp_server_check_auth(preq, &resp, auth_buf, 256);

        if (ret < 0){
            _rtsp_server_reply(&resp, pclient);
            return -1;
        }
    }

    return 0;
}

int rtsp_server_create_client(int client_fd)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    rtsp_client_t* pclient = malloc(sizeof(rtsp_client_t));
    if (!pclient){
        printf("malloc rtsp client failed\n");
        return -1;
    }

    memset(pclient, 0, sizeof(rtsp_client_t));

    byte_array_buf_init(&pclient->tx_buf, NULL, 64 * 1024, 1);
    byte_array_buf_init(&pclient->rx_buf, NULL, 64 * 1024, 1);

    pclient->sockfd = client_fd;
    pclient->hb_ts = get_monotonic_time_ms();

    //add to list
    pclient->next = phdl->client_head;
    phdl->client_head = pclient;

    return 0;
}

int rtsp_server_remove_client(rtsp_client_t* pclient)
{
    int ret = 0;

    printf("remove client\n");

    if (pclient->sockfd > 0){
        close(pclient->sockfd);
        pclient->sockfd = -1;
    }

    if (pclient->svr_handle != RTP_SERVER_INVALID_HANDLE){
        rtp_server_destroy(pclient->svr_handle);
    }

    rtsp_session_t* psession = &pclient->session;
    if (psession->mode == RTSP_TRANSPORT_MODE_UDP){
        int i;
        for (i = 0; i < RTSP_SESSION_CHN_NUM; i++){
            if (psession->transport[i].fd > 0){
                close(psession->transport[i].fd);
            }

            byte_array_buf_destroy(&psession->transport[i].rx_buf);
            byte_array_buf_destroy(&psession->transport[i].tx_buf);
        }
    }

    //remove from list
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    rtsp_client_t* p = phdl->client_head;
    if (p == pclient){
        phdl->client_head = p->next;
    }
    else{

        while(p->next){
            if (p->next == pclient){
                p->next = pclient->next;
                break;
            }
            else{
                p = p->next;
            }
        }
    }

    byte_array_buf_destroy(&pclient->rx_buf);
    byte_array_buf_destroy(&pclient->tx_buf);

    free(pclient);

    return ret;
}

int rtsp_server_handle_options(rtsp_msg_t* preq, char* buf, int size)
{
    printf("receive OPTIONS\n");

    rtsp_msg_t resp = {0};
    resp.status = 200;
    strcpy(resp.status_msg, "OK");
    strcpy(resp.version, "1.0");
    resp.headers[CSEQ] = preq->headers[CSEQ];
    resp.headers[PUBLIC] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY";

    return rtsp_pack_response(&resp, buf, size);
}

int rtsp_server_get_sdp(int stream_id, char* sdp_buf, int buf_len)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    rtsp_stream_src_t* psrc = phdl->stream_src + stream_id;
    int ret = 0;

    int userid = psrc->start();
    if (userid < 0){
        printf("stream start failed\n");
        return -1;
    }

    char* buf = NULL;
    int len = 0;
    uint64_t pts = 0;
    ret = psrc->get_frame(userid, RTSP_MEDIA_TYPE_VIDEO, &buf, &len, &pts);
    if (0 != ret){
        printf("get frame for sdp failed\n");
	ret = -1;
	goto error_out;
    }

    int v_sample_rate = psrc->get_sample_rate(RTSP_MEDIA_TYPE_VIDEO);
    int a_sample_rate = psrc->get_sample_rate(RTSP_MEDIA_TYPE_AUDIO);
    RTP_CODEC_E v_codec = psrc->get_codec(RTSP_MEDIA_TYPE_VIDEO);
    RTP_CODEC_E a_codec = psrc->get_codec(RTSP_MEDIA_TYPE_AUDIO);
    
    sdp_info_t sdp = {0};
    int sps_pos = -1, pps_pos = -1, i_pos = -1, vps_pos = -1, sei_pos = -1;
    int pos = 0;

    if (v_codec == RTP_CODEC_H264){
        for(pos = 0; pos < len - 5; pos++){
            if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1){
                unsigned char nal_type = buf[pos + 3] & 0x1f;
                if (nal_type == 0x7){
                    sps_pos = pos;
                }else if (nal_type == 0x8){
                    pps_pos = pos;
                }else{
                    if (sps_pos > 0 && pps_pos > 0){
                        i_pos = pos;
                        break;
                    }
                }    
            }
        }
        printf("sps(%d) pps(%d) i(%d) \n", sps_pos, pps_pos, i_pos);
        if (sps_pos < 0 || pps_pos < 0 || i_pos < 0){
            ret = -1;
            goto error_out;
        }
    }
    else if (v_codec == RTP_CODEC_H265){

        for(pos = 0; pos < len - 5; pos++){
            if (buf[pos] == 0 && buf[pos + 1] == 0
                && buf[pos + 2] == 1){
                unsigned char nal_type = (buf[pos + 3] >> 1) & 0x3f;
                if (nal_type == 32){
                    vps_pos = pos;
                }else if (nal_type == 33){
                    sps_pos = pos;
                }else if (nal_type == 34){
                    pps_pos = pos;
                }else if (nal_type == 39){
                    sei_pos = pos;
                }else if (nal_type == 19){
                    i_pos = pos;
                    break;
                }    
            }
        }
        printf("vps(%d) sps(%d) pps(%d) sei(%d) i(%d) \n", vps_pos, sps_pos, pps_pos, sei_pos, i_pos);
        if (vps_pos < 0 || sps_pos < 0 || pps_pos < 0 || (sei_pos < 0 && i_pos < 0)){
            ret = -1;
            goto error_out;
        }

        if (sei_pos != -1 && sei_pos < i_pos){
            i_pos = sei_pos;
        }
    }
    else if (v_codec == RTP_CODEC_JPEG){
        //do nothing
    }
    else {
        printf("unknown codec(%d)\n", v_codec);
        ret = -1;
        goto error_out;
    }

    if (a_codec == RTP_CODEC_PCM) {
        strcpy(sdp.audio_format, "L16");
    }
    else if (a_codec == RTP_CODEC_G711A){
        strcpy(sdp.audio_format, "PCMA");
    }
    else if (a_codec == RTP_CODEC_G711U){
        strcpy(sdp.audio_format, "PCMU");
    }
    else {
        printf("unknown audio codec(%d)\n", a_codec);
    }

    sdp.audio_sample_rate = a_sample_rate;
    sdp.audio_channels = 1;
    strcpy(sdp.server_ip, "127.0.0.1");

    if (v_codec == RTP_CODEC_H264){
        strcpy(sdp.video_format, "H264");
        sdp.sps_len = pps_pos - sps_pos - 3;
        memcpy(sdp.sps, buf + sps_pos + 3, sdp.sps_len);
        sdp.pps_len = i_pos - pps_pos - 3;
        memcpy(sdp.pps, buf + pps_pos + 3, sdp.pps_len);
    }
    else if (v_codec == RTP_CODEC_H265){
        strcpy(sdp.video_format, "H265");
        sdp.sps_len = pps_pos - sps_pos - 3;
        memcpy(sdp.sps, buf + sps_pos + 3, sdp.sps_len);
        sdp.pps_len = i_pos - pps_pos - 3;
        memcpy(sdp.pps, buf + pps_pos + 3, sdp.pps_len);
        sdp.vps_len = sps_pos - vps_pos - 3;
        memcpy(sdp.vps, buf + vps_pos + 3, sdp.vps_len);
    }
    else if (v_codec == RTP_CODEC_JPEG){
        strcpy(sdp.video_format, "JPEG");
    }
    else {
        printf("unknown video codec(%d)\n", v_codec);
    }
    sdp.video_sample_rate = v_sample_rate;

    psrc->stop(userid);
    return sdp_encode(&sdp, sdp_buf, buf_len);

error_out:
    psrc->stop(userid);
    return ret;

}

static int __find_stream_id(char* uri)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    int i;
    for(i = 0; i < MAX_STREAM_SRC; i++){
        if (phdl->stream_src[i].get_name){
            int len = 64;
            char name[len];
            memset(name, 0, len);
            phdl->stream_src[i].get_name(name, &len);

            char* pos = strstr(uri, name);
            if (NULL != pos){
                printf("find stream with name %s at %d\n", name, i);
                break;
            }
        }
    }

    return i;
}

int rtsp_server_handle_describe(rtsp_msg_t* preq, char* buf, int size, rtsp_client_t* pclient)
{
    printf("receive DESCRIBE\n");

    rtsp_msg_t resp = {0};

    //check uri

    int sid = __find_stream_id(preq->uri);
    if (sid == MAX_STREAM_SRC){
        printf("stream not found: %s\n", preq->uri);
        goto error;
    }

    strcpy(resp.version, "1.0");
    resp.headers[CSEQ] = preq->headers[CSEQ];
    resp.status = OK;
    strcpy(resp.status_msg, "OK");
    //build sdp
    char sdp_buf[4096];
    int len = 0;

    len = rtsp_server_get_sdp(sid, sdp_buf, sizeof(sdp_buf));
    if (len < 0){
        printf("get sdp failed\n");
        goto error;
    }
    sdp_buf[len] = '\0';

    char len_buf[8];
    sprintf(len_buf, "%d", len);
    resp.headers[CONTENT_LENGTH] = len_buf;
    resp.body = sdp_buf;

    return rtsp_pack_response(&resp, buf, size);

error:
    _rtsp_server_reply_msg(pclient, BAD_REQUEST, "Bad Request");
    return 0;
}

void __activate_net()
{
    __send_cmd_to_server(RTSP_SERVER_CMD_NULL);
}

static void __process_pipe_cmd(RTSP_SERVER_CMD_E cmd)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    switch(cmd)
    {
        case RTSP_SERVER_CMD_NULL:
            //printf("net activated\n");
            break;
        case RTSP_SERVER_CMD_REMOVE_ALL_CLIENTS:
            phdl->remove_all_clients = 1;
            break;
        default:
            break;
    }
}

int rtsp_server_handle_setup(rtsp_msg_t* preq, char* buf, int len, rtsp_client_t* pclient)
{
    printf("receive SETUP\n");

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    int sid = __find_stream_id(preq->uri);
    if (sid == MAX_STREAM_SRC){
        printf("stream not found: %s\n", preq->uri);
        goto error;
    }
    //check uri
    int trackid;
    char* pos = strstr(preq->uri, "/track");
    if (NULL == pos){
        printf("no track found: %s\n", preq->uri);
        goto error;
    }
    int match = sscanf(pos, "/track%u", &trackid);
    if (1 != match || !phdl->stream_src[sid].start
        || (trackid != 0 && trackid != 1)){
        printf("parse trackid failed: %s\n", preq->uri);
        goto error;
    }
    if (NULL == preq->headers[TRANSPORT]){
        printf("no transport found in header\n");
        goto error;
    }

    //unpack
    rtsp_transport_t trans = {0};
    rtsp_unpack_transport(&trans, preq->headers[TRANSPORT]);
    //check
    if (trans.cast_mode != RTSP_CAST_MODE_UNICAST && trans.cast_mode != RTSP_CAST_MODE_MULTICAST){
        goto error;
    }
    if (trans.trans_mode == RTSP_TRANSPORT_MODE_TCP) {
        if (trans.interleaved[1] != trans.interleaved[0] + 1) {
            goto error;
        }
    }
    else if (trans.trans_mode == RTSP_TRANSPORT_MODE_UDP) {
        //数据端口为偶数，且控制端口=数据端口+1
        if ((trans.client_port[0] & 1) != 0 || trans.client_port[1] != trans.client_port[0] + 1){
            goto error;
        }
    }
    else{
        goto error;
    }

    rtsp_session_t *psession = &pclient->session;
    if (NULL == preq->headers[SESSION]){
        if (psession->active){
            goto error;
        }
        //create session
        rtsp_session_create(psession, trans.trans_mode, &pclient->rx_buf, &pclient->tx_buf);
        psession->stream_id = sid;

        sock_getpeername(pclient->sockfd, psession->client_ip, sizeof(psession->client_ip));
        
        rtp_server_param_t param = {
            .psession = psession,
            .pstream_src = &phdl->stream_src[sid],
            .activate_net = __activate_net,
        };
        pclient->svr_handle = rtp_server_create(&param);
    }
    else{

        if (0 != strcmp(psession->session_id, preq->headers[SESSION])){
            goto error;
        }
        if (trans.trans_mode != psession->mode){
            goto error;
        }
    }

    RTSP_SESSION_CHN_E data_chn, ctrl_chn;
    if (RTSP_MEDIA_TYPE_VIDEO== trackid){
        data_chn = RTSP_VID_DATA_CHN;
        ctrl_chn = RTSP_VID_CTRL_CHN;
    }
    else{
        data_chn = RTSP_AUD_DATA_CHN;
        ctrl_chn = RTSP_AUD_CTRL_CHN;
    }

    if (trans.trans_mode == RTSP_TRANSPORT_MODE_UDP) {
        //bind udp port
        int server_ports[2], fds[2];
        udp_continuous_bind(server_ports, fds);
        fd_set_nonblocking(fds[0]);
        fd_set_nonblocking(fds[1]);

        //set fd and port
        rtsp_session_set_fd(psession, data_chn, fds[0]);
        rtsp_session_set_fd(psession, ctrl_chn, fds[1]);
        rtsp_session_set_cli_port(psession, data_chn, trans.client_port[0]);
        rtsp_session_set_cli_port(psession, ctrl_chn, trans.client_port[1]);
        rtsp_session_set_svr_port(psession, data_chn, server_ports[0]);
        rtsp_session_set_svr_port(psession, ctrl_chn, server_ports[1]);

        rtsp_session_init_buffer(psession, data_chn);
        rtsp_session_init_buffer(psession, ctrl_chn);

        trans.server_port[0] = server_ports[0];
        trans.server_port[1] = server_ports[1];

    }
    else{
        rtsp_session_set_interleaved(psession, data_chn, trans.interleaved[0]);
        rtsp_session_set_interleaved(psession, ctrl_chn, trans.interleaved[1]);
    }

    psession->track_flag |= (1 << trackid);

    char trans_buf[128];
    rtsp_pack_transport(&trans, trans_buf, sizeof(trans_buf));

    rtsp_msg_t resp = {0};
    strcpy(resp.version, "1.0");
    resp.headers[CSEQ] = preq->headers[CSEQ];
    resp.status = OK;
    strcpy(resp.status_msg, "OK");
    resp.headers[TRANSPORT] = trans_buf;
    resp.headers[SESSION] = psession->session_id;

    return _rtsp_server_reply(&resp, pclient);

error:
    _rtsp_server_reply_msg(pclient, BAD_REQUEST, "Bad Request");
    return 0;
}

int rtsp_server_handle_play(rtsp_msg_t* preq, char* buf, int size, rtsp_client_t* pclient)
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    printf("receive PLAY\n");

    if (NULL == preq->headers[SESSION] || strcmp(preq->headers[SESSION], pclient->session.session_id)){
        printf("session check failed\n");
        goto error;
    }

    int sid = __find_stream_id(preq->uri);
    if (sid == MAX_STREAM_SRC){
        printf("stream not found: %s\n", preq->uri);
        goto error;
    }

    uint16_t seq = rand() & 0xffff;
    uint32_t ts = rand();
    rtp_server_start(pclient->svr_handle, seq, ts);

    rtsp_msg_t resp = {0};
    strcpy(resp.version, "1.0");
    resp.headers[CSEQ] = preq->headers[CSEQ];
    resp.status = OK;
    strcpy(resp.status_msg, "OK");

    char tmp_buf[128] = {0};
    char name[64] = {0};
    int name_len = 64;
    phdl->stream_src[sid].get_name(name, &name_len);

    snprintf(tmp_buf, sizeof(tmp_buf), "url=%s;seq=%u;rtptime=%u", name, seq, ts);
    resp.headers[RANGE] = preq->headers[RANGE];
    resp.headers[RTP_INFO] = tmp_buf;

    return _rtsp_server_reply(&resp, pclient);

error:
    _rtsp_server_reply_msg(pclient, BAD_REQUEST, "Bad Request");
    return 0;
}

int rtsp_server_handle_teardown(rtsp_msg_t* preq, char* buf, int size, rtsp_client_t* pclient)
{
    printf("receive TEARDOWN\n");

    if (NULL == preq->headers[SESSION]
        || pclient->session.session_id[0] == '\0'
        || strcmp(preq->headers[SESSION], pclient->session.session_id)){
        goto error;
    }

    //不要在这里关闭rtp server，有可能导致死锁
    //rtp_server_stop(pclient->svr_handle);
    pclient->teardown = 1;

    rtsp_msg_t resp = {0};
    strcpy(resp.version, "1.0");
    resp.headers[CSEQ] = preq->headers[CSEQ];
    resp.status = OK;
    strcpy(resp.status_msg, "OK");

    return _rtsp_server_reply(&resp, pclient);

error:
    _rtsp_server_reply_msg(pclient, BAD_REQUEST, "Bad Request");
    return 0;
}

int rtsp_server_accept_client()
{
    int ret = 0;
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    int client_fd = tcp_server_accept(phdl->sockfd);

    if (client_fd < 0){
        return -1;
    }

    fd_set_nonblocking(client_fd);

    ret = rtsp_server_create_client(client_fd);

    return ret;
}

int rtsp_server_handle_msg(void* priv, rtsp_msg_t* pmsg)
{
    int ret = 0;
    char* buf = NULL;
    rtsp_client_t* pclient = (rtsp_client_t*)priv;

    int len = byte_array_buf_write_ptr_lock(&pclient->tx_buf, &buf);
    if (len <= 0) {
        byte_array_buf_write_ptr_unlock(&pclient->tx_buf, 0);
        printf("tx_buf full\n");
        return -1;
    }

    int bytes_written = 0;

    if (pmsg->type == RTSP_MSG_REQUEST) {
        //先进行预处理
        rtsp_print_msg(pmsg);
        ret = _rtsp_server_pre_hdl_req(pmsg, pclient);
        if (ret < 0) {
            goto unlock;
        }

        switch (pmsg->method)
        {
        case OPTIONS:
            bytes_written = rtsp_server_handle_options(pmsg, buf, len);
            break;
        case DESCRIBE:
            bytes_written = rtsp_server_handle_describe(pmsg, buf, len, pclient);
            break;
        case SETUP:
            bytes_written = rtsp_server_handle_setup(pmsg, buf, len, pclient);
            break;
        case PLAY:
            bytes_written = rtsp_server_handle_play(pmsg, buf, len, pclient);
            break;
        case TEARDOWN:
            bytes_written = rtsp_server_handle_teardown(pmsg, buf, len, pclient);
            break;
        default:
            printf("unhandled request:\n");
            rtsp_print_msg(pmsg);
            break;
        }
    }
    else {
        //response

    }

unlock:
    byte_array_buf_write_ptr_unlock(&pclient->tx_buf, bytes_written);

    return ret;
}

static int __rtsp_server_handle_rtp(void* pri_data, int chn, char* rtp_buf, int buf_len)
{
    printf("unhandled rtp pkt\n");
    return 0;
}

int rtsp_server_recv_data(rtsp_client_t* pclient)
{
    int ret = 0;

    printf("recv data\n");
    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(&pclient->rx_buf, &buf);
    if (len > 0){
        ret = sock_read(pclient->sockfd, buf, len);
        if (ret > 0){
            byte_array_buf_widx_advance(&pclient->rx_buf, ret);
            
            rtsp_common_parse_data(pclient, rtsp_server_handle_msg, __rtsp_server_handle_rtp, &pclient->rx_buf, &pclient->in_msg);
        }
        else {
            rtsp_server_remove_client(pclient);
        }
    }
    byte_array_buf_write_ptr_unlock(&pclient->rx_buf, 0);

    return ret;
}

int rtsp_server_send_data(rtsp_client_t* pclient)
{
    int ret = 0;
    char* buf = NULL;
    int len = byte_array_buf_read_ptr_lock(&pclient->tx_buf, &buf);
    if (len > 0) {
        ret = sock_write(pclient->sockfd, buf, len);
        if (ret > 0) {
            byte_array_buf_ridx_advance(&pclient->tx_buf, ret);
            byte_array_buf_trim(&pclient->tx_buf);
        }
    }
    byte_array_buf_read_ptr_unlock(&pclient->tx_buf, 0);

    return ret;
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

void* rtsp_server_proc(void* arg)
{
    prctl(PR_SET_NAME, "rtsp_server");

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    phdl->sockfd = tcp_server_create(8554);
    if (phdl->sockfd < 0) {
        return NULL;
    }

    fd_set_nonblocking(phdl->sockfd);

    fd_set rfds, wfds;
    int max_fd;

    while (!phdl->thread_exit) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        FD_SET(phdl->sockfd, &rfds);
        max_fd = phdl->sockfd;

        phdl->now_ms = get_monotonic_time_ms();

        _add_fd_to_set(&rfds, phdl->pfd[0], &max_fd);
        rtsp_client_t* pclient = phdl->client_head;
        while(pclient){
            //提前保存next，防止pclient被删除时链表断掉
            rtsp_client_t *next = pclient->next;

            if (phdl->now_ms - pclient->hb_ts > 60000 || phdl->remove_all_clients
                || pclient->teardown){
                //60秒超时
                rtsp_server_remove_client(pclient);
                pclient = next;
                continue;
            }

            _add_fd_to_set(&rfds, pclient->sockfd, &max_fd);
            if (byte_array_buf_data_len(&pclient->tx_buf) > 0) {
                _add_fd_to_set(&wfds, pclient->sockfd, &max_fd);
            }
            //在udp模式下需要添加对应的文件描述符
            rtsp_session_t* psession = &pclient->session;
            if (psession->active && psession->mode == RTSP_TRANSPORT_MODE_UDP){

                int i;
                for(i = 0; i < RTSP_SESSION_CHN_NUM; i++){
                    _add_fd_to_set(&rfds, psession->transport[i].fd, &max_fd);
                    if (byte_array_buf_data_len(&psession->transport[i].tx_buf) > 0){
                        _add_fd_to_set(&wfds, psession->transport[i].fd, &max_fd);
                    }
                }
            }

            pclient = next;
        }
        phdl->remove_all_clients = 0;

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

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
            if (FD_ISSET(phdl->sockfd, &rfds)) {
                //accept new client
                rtsp_server_accept_client();
            }

            if (FD_ISSET(phdl->pfd[0], &rfds)) {
                unsigned char buf[16];
                int i;
                int bytes_read = read(phdl->pfd[0], buf, sizeof(buf));
                for(i = 0; i < bytes_read; i++){
                    __process_pipe_cmd((RTSP_SERVER_CMD_E)buf[i]);
                }
            }
            
            pclient = phdl->client_head;
            while(pclient) {
                //提前保存next，防止pclient被删除时链表断掉
                rtsp_client_t* next = pclient->next;

                if (pclient->sockfd > 0 && FD_ISSET(pclient->sockfd, &rfds)) {
                    //read client data
                    ret = rtsp_server_recv_data(pclient);
                    if (ret > 0){
                        //update heart beat
                        pclient->hb_ts = phdl->now_ms;
                    }
                }
                if (pclient->sockfd > 0 && FD_ISSET(pclient->sockfd, &wfds)) {
                    //write client data
                    rtsp_server_send_data(pclient);
                }

                rtsp_session_t* psession = &pclient->session;
                //处理UDP连接的数据
                if (psession->active && psession->mode == RTSP_TRANSPORT_MODE_UDP){
                    int i;
                    for(i = 0; i < RTSP_SESSION_CHN_NUM; i++){
                        if (FD_ISSET(psession->transport[i].fd, &rfds)) {
                            rtp_server_recv_data(pclient->svr_handle, i);
                        }
                        if (FD_ISSET(psession->transport[i].fd, &wfds)) {
                            rtp_server_send_data(pclient->svr_handle, i);
                        }
                    }
                }

                pclient = next;
            }
        }

    }
    
    close(phdl->sockfd);
    printf("rtsp server exit\n");
    
    return NULL;
}

int rtsp_server_start(char* username, char* password)
{
    srand(time(NULL));

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    if (phdl->pid > 0){
        printf("rtsp server thread already started\n");
        return 0;
    }

    if (username && strlen(username) > 0 && password && strlen(password) > 0){
        memset(phdl->username, 0, sizeof(phdl->username));
        memset(phdl->password, 0, sizeof(phdl->password));
        strncpy(phdl->username, username, sizeof(phdl->username) - 1);
        strncpy(phdl->password, password, sizeof(phdl->password) - 1);
    }

    int ret = pipe(phdl->pfd);
    if (ret != 0){
        printf("create pipe failed(%d)\n", ret);
        return -1;
    }

    fd_set_nonblocking(phdl->pfd[0]);
    fd_set_nonblocking(phdl->pfd[1]);

    if (0 != pthread_create(&phdl->pid, NULL, rtsp_server_proc, NULL)) {
        phdl->pid = -1;
        printf("failed creating rtsp server thread\n");
        return -1;
    }

    return 0;
}

int rtsp_server_stop()
{
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();

    if (phdl->pid == 0) {
        printf("rtsp server thread not started yet\n");
        return 0;
    }

    phdl->thread_exit = 1;
    pthread_join(phdl->pid, NULL);
    phdl->pid = 0;
    close(phdl->pfd[0]);
    close(phdl->pfd[1]);

    rtsp_client_t* pclient = phdl->client_head;
    while(pclient){
        //提前保存next，防止pclient被删除时链表断掉
        rtsp_client_t *next = pclient->next;

        rtsp_server_remove_client(pclient);
        pclient = next;
    }
    memset(phdl, 0, sizeof(rtsp_server_handle_t));

    return 0;
}

static int __get_default_stream_name(char* buf, int* buf_len)
{
    strncpy(buf, "stream", *buf_len);
    *buf_len = 7;
    return 0;
}

//return stream id
STREAM_SRC_ID rtsp_server_register_stream_src(rtsp_stream_src_t* psrc)
{
    int ret = 0;
    if (psrc->get_frame == NULL || psrc->get_sample_rate == NULL
    || psrc->start == NULL || psrc->stop == NULL || psrc->get_codec == NULL){
        printf("stream src invalid\n");
        return -1;
    }

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    int i;
    for(i = 0; i < MAX_STREAM_SRC; i++) {
        if (NULL == phdl->stream_src[i].start) {
            phdl->stream_src[i] = *psrc;
            if (phdl->stream_src[i].get_name == NULL){
                phdl->stream_src[i].get_name = __get_default_stream_name;
            }
            return i;
        }
    }
    
    return ret;
}

int rtsp_server_set_password(char* password)
{
    if (NULL == password){
        printf("password is NULL\n");
        return -1;
    }
    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    if (0 == phdl->pid){
        printf("set password after start server!!!\n");
        return -1;
    }

    memset(phdl->password, 0, sizeof(phdl->password));
    strncpy(phdl->password, password, sizeof(phdl->password) - 1);

    __send_cmd_to_server(RTSP_SERVER_CMD_REMOVE_ALL_CLIENTS);
    return 0;
}

int rtsp_server_get_url(STREAM_SRC_ID id, char* url, int* plen)
{
    if (id < 0 || id >= MAX_STREAM_SRC){
        printf("id(%d) exceed range(0-%d)\n", id, MAX_STREAM_SRC);
        return -1;
    }
    if (NULL == url || NULL == plen){
        printf("parameter error: url(%p), plen(%p)\n", url, plen);
        return -1;
    }

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    if (phdl->stream_src[id].get_name == NULL){
        printf("stream not registered: %d\n", id);
        return -1;
    }
    char name[64] = {0};
    int name_len = 64;
    phdl->stream_src[id].get_name(name, &name_len);

    char ip[32] = {0};
    int ret = get_my_ip(ip);
    if (ret != 0){
        printf("get ip failed\n");
    }

    int url_len = snprintf(url, *plen, "rtsp://%s:8554/%s", ip, name);
    if (url_len >= *plen){
        printf("url trancated\n");
        return -1;
    }

    *plen = url_len;
    
    return 0;
}

int rtsp_server_set_url(STREAM_SRC_ID id, int (*get_name)(char* buf, int* buf_len))
{
    if (id < 0 || id >= MAX_STREAM_SRC){
        printf("id(%d) exceed range(0-%d)\n", id, MAX_STREAM_SRC);
        return -1;
    }
    if (get_name){
        printf("parameter error: get_name(%p)\n", get_name);
        return -1;
    }

    rtsp_server_handle_t* phdl = rtsp_server_get_handle();
    if (phdl->stream_src[id].get_name == NULL){
        printf("stream not registered: %d\n", id);
        return -1;
    }

    phdl->stream_src[id].get_name = get_name;

    return 0;
}
