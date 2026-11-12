#include "sat_user_common.h"
#include "onvif_user_token.h"
#include "mxml-private.h"
#include "mxml.h"
#include "onvif.h"
#include "onvif_resource_xml.h"
#include "unistd.h"
/****************************************************************
2022-09-19 author:leo.liu 说明:获取流的xml文件路径
*****************************************************************/
#define ONVIF_XML_PATH "/home/data/onvif/"
/****************************************************************
2022-09-20 author:leo.liu 说明:discover devices
*****************************************************************/
#ifdef XML_FILE
#define GET_DISCONVER_DEVICE ONVIF_XML_PATH "onvif_discover_devices.xml"
/****************************************************************
2022-09-20 author:leo.liu 说明:获取设备信息
*****************************************************************/
#define GET_DEVICE_INFOMATION ONVIF_XML_PATH "device_infomation.xml"
/****************************************************************
2022-09-20 author:leo.liu 说明:获取rtsp url
*****************************************************************/
#define GET_STREAM_URL ONVIF_XML_PATH "rtsp_stream_url.xml"
/****************************************************************
2022-09-21 author:leo.liu 说明:mediaprofile
*****************************************************************/
#define GET_MEIDA_PROFILE ONVIF_XML_PATH "media_profile.xml"
/****************************************************************
2022-09-21 author:leo.liu 说明:mediaprofile
*****************************************************************/
#define DEVICE_EDIT_XML ONVIF_XML_PATH "device_edit.xml"
/****************************************************************
2022-09-21 author:leo.liu 说明:mediaprofile
*****************************************************************/
#define DEVICE_SET_SCOPES_XML ONVIF_XML_PATH "device_service_set_scopes.xml"
#define DEVICE_GET_SCOPES_XML ONVIF_XML_PATH "get_device_service_scopes.xml"
#define DEVICE_SET_PASSWORD_XML ONVIF_XML_PATH "indoor_set_device_password.xml"
#define GET_ONVIF_IMAGEING_GET_OPTIONS_PATH ONVIF_XML_PATH "post_onvif_imaging_get_options.xml"
#define GET_ONVIF_IMAGEING_GET_IMAGING_SETTING_PATH ONVIF_XML_PATH "post_onvif_imaging_get_imaging_setting.xml"
#define SET_ONVIF_IMAGEING_SET_IMAGING_SETTING_PATH ONVIF_XML_PATH "post_onvif_imaging_set_imaging_setting.xml"
// add_xiaoxiao
#define GET_ONVIF_IMAGEING_GET_VIDEOSOURCE_PATH ONVIF_XML_PATH "post_onvif_imaging_get_videosource.xml"
#define GET_ONVIF_NETWORK_INTERFACED_PATH ONVIF_XML_PATH "post_getnetwork_interface.xml"
#define GET_ONVIF_NETWORK_GATEWAY_PATH ONVIF_XML_PATH "post_getnetwork_gateway.xml"
#define GET_ONVIF_NETWORK_DNS_PATH ONVIF_XML_PATH "post_getnetwork_dns.xml"

#define SET_ONVIF_NETWORK_INTERFACED_PATH ONVIF_XML_PATH "post_setnetwork_interface.xml"
#define SET_ONVIF_NETWORK_GATEWAY_PATH ONVIF_XML_PATH "post_setnetwork_gateway.xml"
#define SET_ONVIF_NETWORK_DNS_PATH ONVIF_XML_PATH "post_setnetwork_dns.xml"
#define SET_ONVIF_SYSTEM_TIME_GET_PATH ONVIF_XML_PATH "post_get_system_date_and_time.xml"
#endif

#define COMM_TYPE_UNICAST 1   // 单播
#define COMM_TYPE_MULTICAST 2 // 多播
#define COMM_TYPE_BROADCAST 3 // 广播
#define COMM_TYPE COMM_TYPE_MULTICAST

/* 发送探测消息（Probe）的目标地址、端口号 */
#if COMM_TYPE == COMM_TYPE_UNICAST
#define CAST_ADDR "100.100.100.15" // 单播地址，预先知道的IPC地址
#elif COMM_TYPE == COMM_TYPE_MULTICAST
#define CAST_ADDR "239.255.255.250" // 多播地址，固定的239.255.255.250
#elif COMM_TYPE == COMM_TYPE_BROADCAST
#define CAST_ADDR "100.100.100.255" // 广播地址
#endif
#define CAST_PORT 3702 // 端口号

/****************************************************************
**@日期: 2022-09-17
**@作者: leo.liu
**@功能:解析文本中的xadds
** ipc_type=0x01,SATOZ_DOORBELL
*****************************************************************/
static bool xml_parse_device_ipaddr(const char *xml, char *ip, int len)
{
        char *p = strstr(xml, ":XAddrs>http://");
        if (p == NULL)
        {
                SAT_DEBUG("%s", xml);
                return false;
        }

        char *start = p + 15;
        if (start == NULL)
        {
                return false;
        }
        char *end = strchr(start, '/');
        if (end == NULL)
        {
                return false;
        }
        end[0] = 0;

        strncpy(ip, start, len);
        return true;
}

static bool xml_parse_device_name(const char *xml, char *name, int len)
{
        char *p = strstr(xml, "/name/");
        if (p == NULL)
        {
                return true;
        }

        char *start = p + 6;
        if (start == NULL)
        {
                return false;
        }
        char *end1 = strstr(start, "</");
        char *end2 = strstr(start, "/");
        char *end3 = strstr(start, " ");
        int len1 = 0;
        int len2 = 0;
        int len3 = 0;

        if (end1 != NULL)
                len1 = strlen(end1);
        if (end2 != NULL)
                len2 = strlen(end2);
        if (end3 != NULL)
                len3 = strlen(end3);

        int str_len = len1 > len2 ? len1 > len3 ? len1 : len3 : len2 > len3 ? len2
                                                                            : len3;

        char *end = str_len == len1 ? end1 : str_len == len2 ? end2
                                                             : end3;
        if (end == NULL)
        {

                return false;
        }
        end[0] = 0;

        strncpy(name, start, len);
        return true;
}

static bool xml_parse_device_type(const char *xml, int *type)
{
        *type = 0x00;
        char *p = strstr(xml, "hardware/SATOZ_DOORBELL");
        if (p != NULL)
        {
                *type = 0x01;
                return true;
        }
        p = strstr(xml, "hardware/CIP-70QPT");
        if (p != NULL)
        {
                *type = 0x02;
                return true;
        }
        return true;
}

static bool xml_to_200_ok_parse(const char *xml, char *data)
{
        const char *p = strstr(xml, "HTTP/1.1 200 OK");
        if (p == NULL)
        {
                return false;
        }

        bool reslut = true;
        if (data == NULL)
        {
                return reslut;
        }

        char *pxml = strstr(xml, "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                printf("%s\n", xml);
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                // printf("%s\n", pxml);
                mxml_node_t *url = mxmlFindElementSub(root, root, "Body", NULL, NULL, MXML_DESCEND);
                while (url != NULL)
                {
                        if ((url->child != NULL) && (url->child->value.text.string != NULL))
                        {
                                printf("[%s:%d]%s\n", __func__, __LINE__, url->child->value.text.string);
                                strcpy(data, url->child->value.text.string);
                                break;
                        }
                        url = mxmlFindElementSub(url, url, "Body", NULL, NULL, MXML_DESCEND);
                }
        }
        mxmlDelete(root);
        return reslut;
}
static bool xml_parse_device_infomation(const char *xml, char *ip, char *name, size_t len, int *type)
{
        if (xml_parse_device_ipaddr(xml, ip, len) == false)
        {
                SAT_DEBUG("%s", xml);
                return false;
        }

        if (xml_parse_device_name(xml, name, len) == false)
        {
                strncpy(name, "unknow", len);
                SAT_DEBUG("%s", xml);
        }

        xml_parse_device_type(xml, type);
        return true;
}
/****************************************************************
**@日期: 2022-09-17
**@作者: leo.liu
**@功能:解析文本中的xadds
*****************************************************************/
static bool xml_to_profile_token_parse(const char *file, char profile[8][64], int *profile_num)
{
        /*返回会话没有 200 OK，获取失败*/
        char *p = strstr(file, "200 OK");
        if (p == NULL)
        {
                return false;
        }

        bool reslut = false;
        char *pxml = strstr(file, "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                printf("%s\n", file);
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                // printf("%s\n", pxml);
                mxml_node_t *profile_token = mxmlFindElementSub(root, root, ":Profiles", NULL, NULL, MXML_DESCEND);
                int count = 0;
                while (profile_token != NULL)
                {
                        const char *attr = mxmlElementGetAttr(profile_token, "token");
                        if ((attr != NULL) && (count < 8))
                        {
                                strcpy(profile[count++], attr);
                                *profile_num = count;
                                printf("parse token%d:%s\n", count, attr);
                        }

                        profile_token = mxmlFindElementSub(profile_token, root, ":Profiles", NULL, NULL, MXML_NO_DESCEND);
                        reslut = true;
                }
        }
        mxmlDelete(root);
        return reslut;
}
/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能:解析url
*****************************************************************/
static bool xml_to_stream_url_parse(const char *file, char *rtsp_url, char *sip_uri, int len)
{
        bool reslut = false;
        char *pxml = strstr(file, "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                printf("%s\n", file);
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                // printf("%s\n", pxml);
                mxml_node_t *url = mxmlFindElementSub(root, root, "Uri", NULL, NULL, MXML_DESCEND);
                while (url != NULL)
                {
                        if ((url->child != NULL) && (url->child->value.text.string != NULL) && (strstr(url->child->value.text.string, "rtsp:")))
                        {
                                printf("%s\n", url->child->value.text.string);
                                memset(rtsp_url, 0, len);
                                strncpy(rtsp_url, url->child->value.text.string, len);
                                reslut = true;
                                break;
                        }
                        url = mxmlFindElementSub(url, url, "Uri", NULL, NULL, MXML_DESCEND);
                }

                url = mxmlFindElementSub(root, root, "SIP", NULL, NULL, MXML_DESCEND);
                while (url != NULL)
                {
                        if ((url->child != NULL) && (url->child->value.text.string != NULL) && (strstr(url->child->value.text.string, "sip:")))
                        {
                                printf("%s\n", url->child->value.text.string);
                                memset(sip_uri, 0, len);
                                strncpy(sip_uri, url->child->value.text.string, len);
                                reslut = true;
                                break;
                        }
                        url = mxmlFindElementSub(url, url, "SIP", NULL, NULL, MXML_DESCEND);
                }
                mxmlDelete(root);
        }

        return reslut;
}

/****************************************************************
**@日期: 2022-09-16
**@作者: leo.liu
**@功能: 设备发现
**@参数：
*****************************************************************/
bool ipc_camera_search(char ipc_addr[IPCAMERA_NUM_MAX][32], char device_name[IPCAMERA_NUM_MAX][32], int *num, char device_type, const char *ifname)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        size_t xml_len = 0;
        memset(xml_buffer, 0, XML_FILE_MAX);
        /****************************************************************
        2022-09-20 author:leo.liu 说明:读取discovery xml
        *****************************************************************/
        if ((device_type == 0x00) || (device_type == 0x01))
        {

#ifdef XML_FILE
                xml_len = sat_file_read(GET_DISCONVER_DEVICE, xml_buffer, XML_FILE_MAX);
#else
                strncpy(xml_buffer, GET_DISCONVER_DEVICE, XML_FILE_MAX);
                xml_len = strlen(xml_buffer);
#endif
                if (xml_len <= 0)
                {
                        printf("read %s failed \n", GET_DISCONVER_DEVICE);
                        reslut = false;
                        goto finish;
                }
        }
        /****************************************************************
        2022-09-20 author:leo.liu 说明:打开onvif组播套接字
        *****************************************************************/
        int sokcet_fd = -1;
        if (sat_socket_udp_open(&sokcet_fd, 0, false) == false)
        {
                printf("%s:open socket failed:%d \n", __func__, __LINE__);
                reslut = false;
                goto finish;
        }
        if (sat_socket_multicast_join_iface(sokcet_fd, CAST_ADDR, ifname) == false)
        {
                sat_socket_close(sokcet_fd);
                printf("%s:jion socket failed:%d iface:%s \n", __func__, __LINE__, ifname ? ifname : "(null)");
                reslut = false;
                goto finish;
        }
        /****************************************************************
        2022-09-20 author:leo.liu 说明:发送探测消息
        *****************************************************************/
        for (int i = 0; i < 5; i++)
        {
                if (sat_socket_udp_send(sokcet_fd, xml_buffer, xml_len, CAST_ADDR, CAST_PORT, 100) == false)
                {
                        sat_socket_close(sokcet_fd);
                        printf("sned msg failed:\n%s \n", xml_buffer);
                        reslut = false;
                        goto finish;
                }
                usleep(10 * 1000);
        }
        /****************************************************************
        2022-09-17 author:leo.liu 说明:接受探测的消息
        *****************************************************************/
        memset(xml_buffer, 0, XML_FILE_MAX);
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        while (sat_socket_udp_receive(sokcet_fd, xml_buffer, XML_FILE_MAX, &client_addr, 1500) > 0)
        {
                char xaddres[32] = {0};
                char name[32] = {0};
                int ipc_type = 0x00;
                if (xml_parse_device_infomation(xml_buffer, xaddres, name, sizeof(xaddres), &ipc_type) == true)
                {
                        if (device_type != ipc_type)
                        {
                                continue;
                        }
                        if (*num >= IPCAMERA_NUM_MAX)
                        {
                                printf("serch onvif num > max : %ds\n", IPCAMERA_NUM_MAX);
                                break;
                        }
                        bool same = false;
                        for (int i = 0; i < (*num); i++)
                        {
                                if (strcmp(xaddres, ipc_addr[i]) == 0)
                                {
                                        same = true;
                                }
                        }
                        if (same == false)
                        {
                                printf("serch onvif %s:%s\n", name, xaddres);
                                strncpy(ipc_addr[*num], xaddres, 32);
                                strncpy(device_name[*num], name, 32);
                                (*num)++;
                        }
                }
        }
        sat_socket_close(sokcet_fd);

finish:
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-17
**@作者: leo.liu
**@功能: 获取认证失败后的摘要信息
*****************************************************************/
static bool ipc_onvif_digest_parse(const char *file, char *qop, char *realm, char *nonce, char *opaque)
{
        /*返回会话没有 200 OK，获取失败*/
        char *p = strstr(file, "401 Unauthorized");
        if (p == NULL)
        {
                printf("xml format failed\n(%s)\n", file);
                return false;
        }
        /*获取qop 字符串*/
        {
                char *p_qop = strstr(file, "qop=\"");

                if ((p_qop == NULL) || ((p_qop + 5) == NULL))
                {
                        printf("xml not find qop= \n(%s)\n", file);
                        return false;
                }
                p_qop += 5;
                char *e_qop = strstr(p_qop, "\"");
                if (e_qop == NULL)
                {
                        printf("xml not find qop= \n(%s)\n", file);
                        return false;
                }
                *e_qop = '\0';
                if (strcmp(p_qop, "auth"))
                {
                        printf("qop not support:%s\n", p_qop);
                        return false;
                }
                strcpy(qop, p_qop);
                p = e_qop + 1;
        }

        /*获取realm*/
        {
                if (p == NULL)
                {
                        printf("xml format failed\n(%s)\n", file);
                        return false;
                }
                char *p_realm = strstr(p, "realm=\"");
                if ((p_realm == NULL) || ((p_realm + 7) == NULL))
                {
                        printf("xml not find realm= \n(%s)\n", p);
                        return false;
                }
                p_realm += 7;
                char *e_realm = strstr(p_realm, "\"");
                if (e_realm == NULL)
                {
                        printf("xml not find realm= \n(%s)\n", p);
                        return false;
                }
                *e_realm = '\0';
                strcpy(realm, p_realm);
                p = e_realm + 1;
        }
        /*获取nonce*/
        {
                if (p == NULL)
                {
                        printf("xml format failed\n(%s)\n", file);
                        return false;
                }

                char *p_nonce = strstr(p, "nonce=\"");
                if ((p_nonce == NULL) || ((p_nonce + 7) == NULL))
                {
                        printf("xml not find nonce= \n(%s)\n", p);
                        return false;
                }
                p_nonce += 7;
                char *e_nocne = strstr(p_nonce, "\"");
                if (e_nocne == NULL)
                {
                        printf("xml not find nonce= \n(%s)\n", p);
                        return false;
                }
                *e_nocne = '\0';
                strcpy(nonce, p_nonce);
                p = e_nocne + 1;
        }
        // 此字段非必须，解析失败也不能返回false
        do
        {
                /*获取opaque*/
                {
                        if (p == NULL)
                        {
                                printf("xml format failed\n(%s)\n", file);
                                break;
                        }

                        char *p_opaque = strstr(p, "opaque=\"");
                        if ((p_opaque == NULL) || ((p_opaque + 8) == NULL))
                        {
                                printf("xml not find opaque= \n(%s)\n", p);
                                break;
                        }
                        p_opaque += 8;
                        char *e_opaque = strstr(p_opaque, "\"");
                        if (e_opaque == NULL)
                        {
                                printf("xml not find opaque= \n(%s)\n", p);
                                break;
                        }
                        *e_opaque = '\0';
                        strcpy(opaque, p_opaque);
                }
        } while (0);

        return true;
}
/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能: onvif发送http 口令模式
*****************************************************************/
bool ipc_onvif_tcp_send(char *data, int size, const char *ip, int port, int timeout)
{
        bool reslut = true;

        /****************************************************************
        2022-09-20 author:leo.liu 说明:连接tcp ipc
        *****************************************************************/
        int sokcet_fd;
        char *send_data = (char *)malloc(size + 1);
        if (send_data != NULL)
        {
                strncpy(send_data, data, size + 1);
        }
        sokcet_fd = -1;
        if (sat_socket_tcp_open(&sokcet_fd, 0, -1) == false)
        {
                printf("%s:open socket failed:%d \n", __func__, __LINE__);
                reslut = false;
                goto finish;
        }
        if (sat_socket_tcp_connect(sokcet_fd, ip, port < 0 ? 80 : port, timeout) == false)
        {
                sat_socket_close(sokcet_fd);
                printf("%s:open socket failed:%d \n", __func__, __LINE__);
                reslut = false;
                goto finish;
        }
        /****************************************************************
        *****************************************************************/
        if (sat_socket_tcp_send(sokcet_fd, (unsigned char *)send_data, size, timeout) == false)
        {
                sat_socket_close(sokcet_fd);
                printf("sned msg failed:\n%s \n", data);
                reslut = false;
                goto finish;
        }

        /****************************************************************
         2022-09-17 author:leo.liu 说明:接受探测的消息
        *****************************************************************/
        int read_size = 0;
        int remain_size = XML_FILE_MAX;
        int relut = 0;
        memset(data, 0, XML_FILE_MAX);
        while ((relut = sat_socket_tcp_receive(sokcet_fd, (unsigned char *)&data[read_size], remain_size, timeout)) > 0)
        {
                remain_size -= relut;
                read_size += relut;
                if (remain_size <= 0)
                {
                        break;
                }
        }
        // printf("[%s:%d] recevice msg:%s\n", __func__, __LINE__, data);
        sat_socket_close(sokcet_fd);
        if ((data[0] == 0) && (relut == -2))
        {
                usleep(100 * 1000);
        }
        char *p = strstr(data, "200 OK");
        if (p == NULL)

        {
                printf("[%s:%d] recevice msg:%s\n", __func__, __LINE__, data);
                reslut = false;
        }
finish:
        free(send_data);
        return reslut;
}
/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能: onvif发送http 摘要模式
*****************************************************************/
static bool ipc_onvif_tcp_send_by_digest(char *xml_path, char *data, int size, const char *ip, int port, const char *user, const char *password, const char *math, const char *uri, int argv, char **argc, int timeout)
{
        bool reslut = true;

#ifdef XML_FILE
        size_t xml_len = sat_file_read(xml_path, data, size);
#else
        strncpy(data, xml_path, XML_FILE_MAX);
        size_t xml_len = strlen(data);
#endif
        if (xml_len <= 0)
        {
                printf("read failed  %s \n", xml_path);
                reslut = false;
                goto finish;
        }

        /*先发送无鉴权的http*/
        if ((xml_len = onvif_xml_fomrat_by_digest(data, ip, user, NULL, NULL, NULL, NULL, NULL, NULL, NULL, argv, argc)) <= 0)
        {
                printf("read format failed %s \n", xml_path);
                reslut = false;
                goto finish;
        }
        printf("digest send data is %s\r\n", data);
        /*发送无鉴权数据*/
        reslut = ipc_onvif_tcp_send(data, xml_len, ip, port, timeout);

        if (reslut == false)
        {
                /*鉴权失败后，解析摘要数据*/
                char realm[128] = {0};
                char qop[128] = {0};
                char opaque[128] = {0};
                char nonce[256] = {0};
                reslut = ipc_onvif_digest_parse(data, qop, realm, nonce, opaque);
                if (reslut == false)
                {
                        printf("digest info parse failed\n(%s)\n", data);
                        reslut = false;
                        goto finish;
                }
                /*摘要鉴权后发送*/
                memset(data, 0, XML_FILE_MAX);

#ifdef XML_FILE
                size_t xml_len = sat_file_read(xml_path, data, XML_FILE_MAX);
#else
                strncpy(data, xml_path, XML_FILE_MAX);
                size_t xml_len = strlen(data);
#endif

                if (xml_len <= 0)
                {
                        printf("read failed  %s \n", xml_path);
                        reslut = false;
                        goto finish;
                }

                if ((xml_len = onvif_xml_fomrat_by_digest(data, ip, user, password, math, uri, qop, realm, nonce, opaque, argv, argc)) <= 0)
                {
                        printf("read format failed %s \n", xml_path);
                        reslut = false;
                        goto finish;
                }
                reslut = ipc_onvif_tcp_send(data, xml_len, ip, port, timeout);
                //  printf("\n%s\n", data);
        }
finish:
        return reslut;
}
static bool ipc_onvif_token_send(const char *ip, int port, const char *user, const char *password, const char *xml_path, char **ppxml_buffer, int argc, char **argv, int timeout)
{
        bool reslut = true;
        *ppxml_buffer = malloc(XML_FILE_MAX);
        memset(*ppxml_buffer, 0, XML_FILE_MAX);
#ifdef XML_FILE
        size_t xml_len = sat_file_read(xml_path, *ppxml_buffer, XML_FILE_MAX);
#else
        strncpy(*ppxml_buffer, xml_path, XML_FILE_MAX);
        size_t xml_len = strlen(*ppxml_buffer);
#endif
        if (xml_len <= 0)
        {
                printf("read failed  %s \n", GET_MEIDA_PROFILE);
                reslut = false;
                goto failed;
        }

        /*格式化token字符串*/
        if ((xml_len = onvif_xml_fomrat_by_token(*ppxml_buffer, ip, user, password, argc, argv)) <= 0)
        {
                printf("\n format profile token failed (%s)\n", *ppxml_buffer);
                reslut = false;
                goto failed;
        }
        return ipc_onvif_tcp_send(*ppxml_buffer, xml_len, ip, port, timeout);
failed:
        if (*ppxml_buffer != NULL)
        {
                free(*ppxml_buffer);
                *ppxml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能:获取media profile token
*****************************************************************/
bool ipc_profile_token_get(const char *ip, int port, const char *user, const char *password, char profile[8][64], int *profile_num, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_MEIDA_PROFILE, &xml_buffer, 0, NULL, timeout)) == true)
        {
                reslut = xml_to_profile_token_parse(xml_buffer, profile, profile_num);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能:获取media profile digest
*****************************************************************/
bool ipc_profile_digest_get(const char *ip, int port, const char *user, const char *password, char profile[8][64], int *profile_num, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        reslut = ipc_onvif_tcp_send_by_digest(GET_MEIDA_PROFILE, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/Media", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = xml_to_profile_token_parse(xml_buffer, profile, profile_num);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:获取设备的rtsp
*****************************************************************/
bool ipc_rtsp_token_get(const char *ip, int port, const char *user, const char *password, char *profile_token, char *rtsp, char *sip, int len, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[1];
        argc[0] = profile_token;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_STREAM_URL, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = xml_to_stream_url_parse(xml_buffer, rtsp, sip, len);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:获取设备的rtsp 来自摘要
*****************************************************************/
bool ipc_rtsp_digest_get(const char *ip, int port, const char *user, const char *password, char *profile_token, char *rtsp, char *sip, int len, int tiemout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        char *argc[1];
        argc[0] = profile_token;
        reslut = ipc_onvif_tcp_send_by_digest(GET_STREAM_URL, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/Media", 1, argc, tiemout);
        if (reslut == true)
        {
                reslut = xml_to_stream_url_parse(xml_buffer, rtsp, sip, len);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:编辑注册或者删除设备
*****************************************************************/
static bool ipc_device_token_edit(char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[3];
        argc[0] = edit_type;
        argc[1] = data;
        argc[2] = edit_type;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, DEVICE_EDIT_XML, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
static bool ipc_device_digest_edit(char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        char *argc[3];
        argc[0] = edit_type;
        argc[1] = data;
        argc[2] = edit_type;
        reslut = ipc_onvif_tcp_send_by_digest(DEVICE_EDIT_XML, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 1, argc, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
static bool ipc_camera_device_edit(char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        if (auther_flag == 0x00)
        {

                return ipc_device_token_edit(edit_type, data, ip, port, user, password, timeout);
        }

        return ipc_device_digest_edit(edit_type, data, ip, port, user, password, timeout);
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:set scopes
*****************************************************************/
static bool ipc_camera_device_token_setting_scopes(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[1];
        argc[0] = data;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, DEVICE_SET_SCOPES_XML, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
static bool ipc_camera_device_digest_setting_scopes(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        char *argc[1];
        argc[0] = data;
        reslut = ipc_onvif_tcp_send_by_digest(DEVICE_SET_SCOPES_XML, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 1, argc, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
static bool ipc_camera_device_setting_scopes(const char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        if (auther_flag == 0x00)
        {
                return ipc_camera_device_token_setting_scopes(data, ip, port, user, password, timeout);
        }
        return ipc_camera_device_digest_setting_scopes(data, ip, port, user, password, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:set scopes
*****************************************************************/
static bool ipc_camera_device_token_setting_password(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[1];
        argc[0] = data;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, DEVICE_SET_PASSWORD_XML, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
static bool ipc_camera_device_digest_setting_password(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        char *argc[1];
        argc[0] = data;
        reslut = ipc_onvif_tcp_send_by_digest(DEVICE_SET_PASSWORD_XML, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 1, argc, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

static bool ipc_camera_device_setting_password(const char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        if (auther_flag == 0x00)
        {
                return ipc_camera_device_token_setting_password(data, ip, port, user, password, timeout);
        }
        return ipc_camera_device_digest_setting_password(data, ip, port, user, password, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:get scopes
*****************************************************************/
static bool ipc_camera_device_token_get_scopes(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;

        reslut = ipc_onvif_token_send(ip, port, user, password, DEVICE_GET_SCOPES_XML, &xml_buffer, 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
                if (reslut == true)
                {
                        char *p = strstr(xml_buffer, "/name/");
                        if (p != NULL)
                        {
                                char *end = strstr(p, "</");
                                if (end != NULL)
                                {
                                        *end = '\0';
                                        strncpy(data, p + 6, 63);
                                        printf("[%s:%d] get name:%s\n", __func__, __LINE__, data);
                                }
                        }
                }
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:get scopes
*****************************************************************/
static bool ipc_camera_device_digest_get_scopes(char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        reslut = ipc_onvif_tcp_send_by_digest(DEVICE_GET_SCOPES_XML, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, data);
                if (reslut == true)
                {
                        char *p = strstr(xml_buffer, "/name/");
                        if (p != NULL)
                        {
                                char *end = strstr(p, "</");
                                if (end != NULL)
                                {
                                        *end = '\0';
                                        strncpy(data, p + 6, 63);
                                        printf("[%s:%d] get name:%s\n", __func__, __LINE__, data);
                                }
                        }
                }
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/************************************************************
** 函数说明:解析ideo_token
** 作者: xiaoxiao
** 日期：2024-11-21 17:02:58
** 参数说明:
** 注意事项：
************************************************************/
static bool ipc_camera_device_imaging_videosource_parse(const char *xml, char *video_token)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *profile_token = mxmlFindElementSub(root, root, ":VideoSources", NULL, NULL, MXML_DESCEND);
                if (profile_token != NULL)
                {

                        if (video_token != NULL)
                        {
                                const char *attr = mxmlElementGetAttr(profile_token, "token");
                                if (attr != NULL)
                                {
                                        strncpy(video_token, attr, 32);
                                }
                        }
                }
                return true;
        }
        return false;
}

/************************************************************
** 函数说明:获取设备的video_token 来自令牌
** 作者: xiaoxiao
** 日期：2024-11-21 17:02:58
** 参数说明:
** 注意事项：
************************************************************/
bool ipc_camera_device_token_imaging_get_videosource(const char *ip, int port, const char *user, const char *password, char *videotoken, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_IMAGEING_GET_VIDEOSOURCE_PATH, &xml_buffer, 0, NULL, timeout)) == true)
        {
                reslut = ipc_camera_device_imaging_videosource_parse(xml_buffer, videotoken);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/************************************************************
** 函数说明:获取设备的video_token 来自摘要
** 作者: xiaoxiao
** 日期：2024-11-21 17:02:58
** 参数说明:
** 注意事项：
************************************************************/
bool ipc_camera_device_digest_imaging_get_videosource(const char *ip, int port, const char *user, const char *password, char *videotoken, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_IMAGEING_GET_VIDEOSOURCE_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/media_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, NULL);
                if (reslut == true)
                {
                        reslut = ipc_camera_device_imaging_videosource_parse(xml_buffer, videotoken);
                }
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能:解析options的范围
*****************************************************************/
static bool ipc_camera_device_imaging_options_msg_parse(const char *xml, int brightness[2], int saturation[2], int contrast[2])
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *parent = mxmlFindElementSub(root, root, "Brightness", NULL, NULL, MXML_DESCEND);
                if (parent != NULL)
                {
                        mxml_node_t *node = mxmlFindElementSub(parent, parent, "Min", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                brightness[0] = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Max", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                brightness[1] = (int)atof(mxmlGetText(node, NULL));
                        }

                        //  printf("brightness rang:%d-%d\n", brightness[1], brightness[0]);
                }

                parent = mxmlFindElementSub(root, root, "ColorSaturation", NULL, NULL, MXML_DESCEND);
                if (parent != NULL)
                {
                        mxml_node_t *node = mxmlFindElementSub(parent, parent, "Min", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                saturation[0] = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Max", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                saturation[1] = (int)atof(mxmlGetText(node, NULL));
                        }

                        // printf("[%s:%d] ColorSaturation rang:[%d %d]\n", __func__, __LINE__, saturation[1], saturation[0]);
                }

                parent = mxmlFindElementSub(root, root, "Contrast", NULL, NULL, MXML_DESCEND);
                if (parent != NULL)
                {
                        mxml_node_t *node = mxmlFindElementSub(parent, parent, "Min", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                contrast[0] = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Max", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                contrast[1] = (int)atof(mxmlGetText(node, NULL));
                        }
                        //  printf("Contrast rang:%d-%d\n", contrast[1], contrast[0]);
                }
                return true;
        }
        return false;
}

/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能:token 获取图像数值范围
*****************************************************************/
static bool ipc_camera_device_token_imaging_get_options(const char *ip, int port, const char *user, const char *password, const char *video_token, int brightness[2], int saturation[2], int contrast[2], int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[1];
        argc[0] = (char *)video_token;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_IMAGEING_GET_OPTIONS_PATH, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = ipc_camera_device_imaging_options_msg_parse(xml_buffer, brightness, saturation, contrast);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: token 获取当前数值
*****************************************************************/
static bool ipc_camera_device_digest_imaging_get_options(const char *ip, int port, const char *user, const char *password, const char *video_token, int *brightness, int *saturation, int *contrast, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argc[1];
        argc[0] = (char *)video_token;
        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_IMAGEING_GET_OPTIONS_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/imaging", sizeof(argc) / sizeof(char *), argc, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, NULL);
                if (reslut == true)
                {
                        reslut = ipc_camera_device_imaging_options_msg_parse(xml_buffer, brightness, saturation, contrast);
                }
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2023-09-20
**@作者: leo.liu
**@功能: 当前值解析
*****************************************************************/
static bool ipc_camera_device_imaging_get_imaging_setting_msg_parse(const char *xml, int *brightness, int *saturation, int *contrast)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *node = mxmlFindElementSub(root, root, "Brightness", NULL, NULL, MXML_DESCEND);
                if (node != NULL)
                {
                        *brightness = (int)atof(mxmlGetText(node, NULL));
                        // printf("brightness :%d\n", *brightness);
                }

                node = mxmlFindElementSub(root, root, "ColorSaturation", NULL, NULL, MXML_DESCEND);
                if (node != NULL)
                {
                        *saturation = (int)atof(mxmlGetText(node, NULL));
                        // printf("brightness :%d\n", *saturation);
                }

                node = mxmlFindElementSub(root, root, "Contrast", NULL, NULL, MXML_DESCEND);
                if (node != NULL)
                {
                        *contrast = (int)atof(mxmlGetText(node, NULL));
                        // printf("contrast :%d\n", *contrast);
                }
                return true;
        }
        return false;
}
/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: 摘要获取图像
*****************************************************************/
static bool ipc_camera_device_token_imaging_get_imaging_settings(const char *ip, int port, const char *user, const char *password, const char *video_token, int *brightness, int *saturation, int *contrast, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argc[1];
        argc[0] = (char *)video_token;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_IMAGEING_GET_IMAGING_SETTING_PATH, &xml_buffer, sizeof(argc) / sizeof(char *), argc, timeout)) == true)
        {
                reslut = ipc_camera_device_imaging_get_imaging_setting_msg_parse(xml_buffer, brightness, saturation, contrast);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: digest 获取当前数值
*****************************************************************/
static bool ipc_camera_device_digest_imaging_get_imaging_settings(const char *ip, int port, const char *user, const char *password, const char *video_token, int *brightness, int *saturation, int *contrast, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argv[3] = {0};
        char bright[32] = {0};
        char color[32] = {0};
        char cont[32] = {0};
        sprintf(bright, "%d", brightness);
        sprintf(color, "%d", saturation);
        sprintf(cont, "%d", contrast);
        argv[0] = bright;
        argv[1] = color;
        argv[2] = cont;
        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_IMAGEING_GET_IMAGING_SETTING_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/imaging", 3, argv, timeout);
        if (reslut == true)
        {
                reslut = xml_to_200_ok_parse(xml_buffer, NULL);
                if (reslut == true)
                {
                        reslut = ipc_camera_device_imaging_get_imaging_setting_msg_parse(xml_buffer, brightness, saturation, contrast);
                }
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2023-09-21
**@作者: leo.liu
**@功能: token 调整图像效果
*****************************************************************/
static bool ipc_camera_device_token_setting_imaging(const char *ip, int port, const char *user, const char *password, const char *video_token, int brightness, int saturation, int contrast, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;

        char *argv[4] = {0};
        char bright[32] = {0};
        char color[32] = {0};
        char cont[32] = {0};
        sprintf(bright, "%d", brightness);
        sprintf(color, "%d", saturation);
        sprintf(cont, "%d", contrast);
        argv[0] = (char *)video_token;
        argv[1] = bright;
        argv[2] = color;
        argv[3] = cont;

        reslut = ipc_onvif_token_send(ip, port, user, password, SET_ONVIF_IMAGEING_SET_IMAGING_SETTING_PATH, &xml_buffer, sizeof(argv) / sizeof(char *), argv, timeout);
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2023-09-21
**@作者: leo.liu
**@功能: digest 调整图像效果
*****************************************************************/
static bool ipc_camera_device_digest_setting_imaging(const char *ip, int port, const char *user, const char *password, const char *video_token, int brightness, int saturation, int contrast, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argv[1] = {0};
        argv[0] = (char *)video_token;

        reslut = ipc_onvif_tcp_send_by_digest(SET_ONVIF_IMAGEING_SET_IMAGING_SETTING_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/imaging", 1, argv, timeout);

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网络信息 token
*****************************************************************/
static bool ipc_camera_device_token_get_network_interface_parse(const char *xml, bool *udhcp, char *ipaddr, char *mask, char *net_token)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {

                root = mxmlFindElementSub(root, root, ":NetworkInterfaces", NULL, NULL, MXML_DESCEND);
                if (root != NULL)
                {
                        if (net_token != NULL)
                        {
                                const char *str = mxmlElementGetAttr(root, "token");

                                if (str != NULL)
                                {
                                        strncpy(net_token, str, 24);
                                        printf("ipaddr:%s\n", ipaddr);
                                }
                        }
                }
                mxml_node_t *node = NULL;
                root = mxmlFindElementSub(root, root, "IPv4", NULL, NULL, MXML_DESCEND);
                if (root != NULL)
                {
                        if (ipaddr != NULL)
                        {
                                node = mxmlFindElementSub(root, root, "Address", NULL, NULL, MXML_DESCEND);
                                if (node != NULL)
                                {
                                        const char *str = mxmlGetText(node, NULL);
                                        if (str != NULL)
                                        {
                                                strncpy(ipaddr, str, 16);
                                                printf("ipaddr:%s\n", ipaddr);
                                        }
                                }
                        }

                        if (udhcp != NULL)
                        {

                                node = mxmlFindElementSub(root, root, ":DHCP", NULL, NULL, MXML_DESCEND);
                                if (node != NULL)
                                {
                                        const char *str = mxmlGetText(node, NULL);
                                        if (str != NULL)
                                        {
                                                printf("DHCP:%s\n", str);
                                                if (strncmp(str, "false", 5) == 0)
                                                {
                                                        *udhcp = false;
                                                }
                                                else
                                                {
                                                        *udhcp = true;
                                                }
                                        }
                                }
                        }
                        if (mask != NULL)
                        {
                                node = mxmlFindElementSub(root, root, "PrefixLength", NULL, NULL, MXML_DESCEND);
                                if (node != NULL)
                                {
                                        const char *str = mxmlGetText(node, NULL);
                                        if (str != NULL)
                                        {
                                                int length = atoi(str);
                                                unsigned int hex = 0xffffffff << (32 - length);
                                                sprintf(mask, "%d.%d.%d.%d", (hex >> 24) & 0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
                                                printf("MASK:%s:%d\n", mask, length);
                                        }
                                }
                        }
                }
                return true;
        }
        return false;
}

static bool ipc_camera_device_token_get_gateway_parse(const char *xml, char *gateway)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *node = NULL;
                node = mxmlFindElementSub(root, root, "IPv4Address", NULL, NULL, MXML_DESCEND);
                if (node != NULL)
                {
                        if (gateway != NULL)
                        {
                                const char *str = mxmlGetText(node, NULL);
                                if (str != NULL)
                                {
                                        strncpy(gateway, mxmlGetText(node, NULL), 16);
                                        printf("gateway:%s\n", gateway);
                                }
                        }
                }
                return true;
        }
        return false;
}

static bool ipc_camera_device_token_get_dns_parse(const char *xml, char *dns)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *node = NULL;
                node = mxmlFindElementSub(root, root, "IPv4Address", NULL, NULL, MXML_DESCEND);
                if (node != NULL)
                {
                        if (dns != NULL)
                        {
                                const char *str = mxmlGetText(node, NULL);
                                if (str != NULL)
                                {
                                        strncpy(dns, str, 16);
                                        printf("default dns:%s\n", dns);
                                }
                        }
                }
                return true;
        }
        return false;
}
static bool ipc_camera_device_token_get_network_interface(const char *ip, int port, const char *user, const char *password, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_NETWORK_INTERFACED_PATH, &xml_buffer, 0, NULL, timeout)) == true)
        {
                reslut = ipc_camera_device_token_get_network_interface_parse(xml_buffer, &info->udhcp, info->ipaddr, info->mask, info->net_token);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网关 token
*****************************************************************/
static bool ipc_camera_device_token_get_gateway(const char *ip, int port, const char *user, const char *password, char gatway[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_NETWORK_GATEWAY_PATH, &xml_buffer, 0, NULL, timeout)) == true)
        {
                reslut = ipc_camera_device_token_get_gateway_parse(xml_buffer, gatway);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取dns token
*****************************************************************/
static bool ipc_camera_device_token_get_dns(const char *ip, int port, const char *user, const char *password, char dns[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, GET_ONVIF_NETWORK_DNS_PATH, &xml_buffer, 0, NULL, timeout)) == true)
        {
                reslut = ipc_camera_device_token_get_dns_parse(xml_buffer, dns);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网络信息 digest
*****************************************************************/
static bool ipc_camera_device_digest_get_network_interface(const char *ip, int port, const char *user, const char *password, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_NETWORK_INTERFACED_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = ipc_camera_device_token_get_network_interface_parse(xml_buffer, &info->udhcp, info->ipaddr, info->mask, info->net_token);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网关 digest
*****************************************************************/
static bool ipc_camera_device_digest_get_gateway(const char *ip, int port, const char *user, const char *password, char gatway[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_NETWORK_GATEWAY_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = ipc_camera_device_token_get_gateway_parse(xml_buffer, gatway);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取dns digest
*****************************************************************/
static bool ipc_camera_device_digest_get_dns(const char *ip, int port, const char *user, const char *password, char dns[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);

        reslut = ipc_onvif_tcp_send_by_digest(GET_ONVIF_NETWORK_DNS_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = ipc_camera_device_token_get_dns_parse(xml_buffer, dns);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/***********************************************
** 作者: leo.liu
** 日期: 2023-1-7 14:46:50
** 说明: 将子网掩码转换成长度
***********************************************/
int convert_subnet_mask(char *mask)
{
        if (mask == NULL)
        {
                return 0;
        }
        int length = 0;
        char *saveptr = NULL;
        char *token = strtok_r(mask, ".", &saveptr);
        while (token != NULL)
        {
                int number = atoi(token);
                length += number == 255 ? 8 : (number == 0 ? 0 : -1);
                token = strtok_r(NULL, ".", &saveptr);
        }
        return length;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 设置网络接口
*****************************************************************/
static bool ipc_camera_device_token_set_network_interface(const char *ip, int port, const char *user, const char *password, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argv[4] = {0};
        argv[0] = info->net_token;
        argv[1] = info->ipaddr;
        char mask[32] = {0};
        sprintf(mask, "%d", convert_subnet_mask(info->mask));
        argv[2] = mask;
        char dhcp[32] = {0};
        argv[3] = dhcp;
        sprintf(dhcp, "%s", info->udhcp ? "true" : "false");
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, SET_ONVIF_NETWORK_INTERFACED_PATH, &xml_buffer, sizeof(argv) / sizeof(char *), argv, timeout)) == false)
        {
                printf("[%s:%d] setting network interface failed\n", __func__, __LINE__);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网关 token
*****************************************************************/
static bool ipc_camera_device_token_set_gateway(const char *ip, int port, const char *user, const char *password, char gatway[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argv[1] = {0};

        argv[0] = gatway;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, SET_ONVIF_NETWORK_GATEWAY_PATH, &xml_buffer, sizeof(argv) / sizeof(char *), argv, timeout)) == false)
        {
                printf("[%s:%d] setting network gateway failed\n", __func__, __LINE__);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取dns token
*****************************************************************/
static bool ipc_camera_device_token_set_dns(const char *ip, int port, const char *user, const char *password, char dns[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        char *argv[1] = {0};

        argv[0] = dns;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, SET_ONVIF_NETWORK_DNS_PATH, &xml_buffer, sizeof(argv) / sizeof(char *), argv, timeout)) == false)
        {
                printf("[%s:%d] setting network interface failed\n", __func__, __LINE__);
        }
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网络信息 digest
*****************************************************************/
static bool ipc_camera_device_digest_set_network_interface(const char *ip, int port, const char *user, const char *password, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argv[4] = {0};

        argv[0] = info->net_token;
        argv[1] = info->ipaddr;
        char mask[32] = {0};
        sprintf(mask, "%d", convert_subnet_mask(info->mask));
        argv[2] = mask;
        char dhcp[32] = {0};
        argv[3] = dhcp;
        sprintf(dhcp, "%s", info->udhcp ? "true" : "false");

        reslut = ipc_onvif_tcp_send_by_digest(SET_ONVIF_NETWORK_INTERFACED_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", sizeof(argv) / sizeof(char *), argv, timeout);
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}

/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取网关 digest
*****************************************************************/
static bool ipc_camera_device_digest_set_gateway(const char *ip, int port, const char *user, const char *password, char gatway[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argv[1] = {0};

        argv[0] = gatway;

        reslut = ipc_onvif_tcp_send_by_digest(SET_ONVIF_NETWORK_GATEWAY_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", sizeof(argv) / sizeof(char *), argv, timeout);
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取dns digest
*****************************************************************/
static bool ipc_camera_device_digest_set_dns(const char *ip, int port, const char *user, const char *password, char dns[24], int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        char *argv[1] = {0};

        argv[0] = dns;

        reslut = ipc_onvif_tcp_send_by_digest(SET_ONVIF_NETWORK_DNS_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", sizeof(argv) / sizeof(char *), argv, timeout);
        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取时间 token
*****************************************************************/
static bool ipc_camera_device_get_system_time_parse(char *xml, struct tm *tm)
{
        const char *pxml = strstr((const char *)xml, "<"); // "<?xml version=\"1.0\"");
        if (pxml == NULL)
        {
                return false;
        }
        mxml_node_t *root = mxmlLoadString(NULL, pxml, MXML_NO_CALLBACK);
        if (root != NULL)
        {
                mxml_node_t *parent = mxmlFindElementSub(root, root, "LocalDateTime", NULL, NULL, MXML_DESCEND);
                if (parent != NULL)
                {
                        mxml_node_t *node = mxmlFindElementSub(parent, parent, "Hour", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_hour = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Minute", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_min = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Second", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_sec = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Year", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_year = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Month", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_mon = (int)atof(mxmlGetText(node, NULL));
                        }
                        node = mxmlFindElementSub(parent, parent, "Day", NULL, NULL, MXML_DESCEND);
                        if (node != NULL)
                        {
                                tm->tm_mday = (int)atof(mxmlGetText(node, NULL));
                        }
                        return true;
                }
        }
        return false;
}
static bool ipc_camera_device_token_get_system_time(const char *ip, int port, const char *user, const char *password, struct tm *tm, int timeout)
{
        bool reslut = true;
        char *xml_buffer = NULL;
        if ((reslut = ipc_onvif_token_send(ip, port, user, password, SET_ONVIF_SYSTEM_TIME_GET_PATH, &xml_buffer, 0, NULL, timeout)) == false)
        {
                printf("[%s:%d] setting network interface failed\n", __func__, __LINE__);
        }
        if (reslut == true)
        {
                reslut = ipc_camera_device_get_system_time_parse(xml_buffer, tm);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
                xml_buffer = NULL;
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 获取时间 digest
*****************************************************************/
static bool ipc_camera_device_digest_get_system_time(const char *ip, int port, const char *user, const char *password, struct tm *tm, int timeout)
{
        bool reslut = true;
        char *xml_buffer = malloc(XML_FILE_MAX);
        memset(xml_buffer, 0, XML_FILE_MAX);
        reslut = ipc_onvif_tcp_send_by_digest(SET_ONVIF_SYSTEM_TIME_GET_PATH, xml_buffer, XML_FILE_MAX, ip, port, user, password, "POST", "/onvif/device_service", 0, NULL, timeout);
        if (reslut == true)
        {
                reslut = ipc_camera_device_get_system_time_parse(xml_buffer, tm);
        }

        if (xml_buffer != NULL)
        {
                free(xml_buffer);
        }
        return reslut;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:get scopes
*****************************************************************/
static bool ipc_camera_device_get_scopes(const char *edit_type, char *data, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        if (auther_flag == 0x00)
        {
                return ipc_camera_device_token_get_scopes(data, ip, port, user, password, timeout);
        }

        return ipc_camera_device_digest_get_scopes(data, ip, port, user, password, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 查询是否在线
*****************************************************************/
bool ipc_camera_device_name_get(char *name, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        return ipc_camera_device_get_scopes("GetName", name, ip, port, user, password, auther_flag, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 设置用户名
*****************************************************************/
bool ipc_camera_device_name_set(char *name, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        return ipc_camera_device_setting_scopes("SetName", name, ip, port, user, password, auther_flag, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 密码修改
*****************************************************************/
bool ipc_camera_device_password_change(char *pwd, const char *ip, int port, const char *user, const char *password, char auther_flag, int timeout)
{
        return ipc_camera_device_setting_password("ChangePassword", pwd, ip, port, user, password, auther_flag, timeout);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能: 数据同步
*****************************************************************/
bool ipc_camera_device_send_data(char *data_type, char *data, const char *ip, int port, const char *user, const char *password, int timeout)
{
        return ipc_camera_device_edit(data_type, data, ip, port, user, password, 0x00, timeout);
}
/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: 获取亮度色度对比度数据,百分比
*****************************************************************/
bool ipc_camera_device_get_imaging(const char *ip, int port, const char *user, const char *password, char auther_flag, const char *video_token, int brightness[3], int saturation[3], int contrast[3], int timeout)
{
        if (auther_flag == 0x00)
        {

                if (ipc_camera_device_token_imaging_get_options(ip, port, user, password, video_token, brightness, saturation, contrast, timeout) == false)
                {
                        return false;
                }

                if (ipc_camera_device_token_imaging_get_imaging_settings(ip, port, user, password, video_token, &brightness[2], &saturation[2], &contrast[2], timeout) == false)
                {
                        return false;
                }
        }
        else
        {

                if (ipc_camera_device_digest_imaging_get_options(ip, port, user, password, video_token, brightness, saturation, contrast, timeout) == false)
                {
                        return false;
                }
                if (ipc_camera_device_digest_imaging_get_imaging_settings(ip, port, user, password, video_token, &brightness[2], &saturation[2], &contrast[2], timeout) == false)
                {
                        return false;
                }
        }
        return true;
}
/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: 设置亮度色度对比度数据,百分比
*****************************************************************/
bool ipc_camera_device_setting_imaging(const char *ip, int port, const char *user, const char *password, char auther_flag, const char *video_token, int brightness, int saturation, int contrast, int timeout)
{
        if (auther_flag == 0x00)
        {

                return ipc_camera_device_token_setting_imaging(ip, port, user, password, video_token, brightness, saturation, contrast, timeout);
        }

        return ipc_camera_device_digest_setting_imaging(ip, port, user, password, video_token, brightness, saturation, contrast, timeout);
}
/****************************************************************
**@日期: 2023-09-19
**@作者: leo.liu
**@功能: 获取设备的网络信息
*****************************************************************/
bool ipc_camera_device_get_network(const char *ip, int port, const char *user, const char *password, char auther_flag, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        char gateway[24] = {"0.0.0.0"};
        char dns[24] = {"0.0.0.0"};
        if (auther_flag == 0x00)
        {
                if ((reslut = ipc_camera_device_token_get_network_interface(ip, port, user, password, info, timeout)) == false)
                {
                        printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                        reslut = false;
                }

                if (ipc_camera_device_token_get_gateway(ip, port, user, password, gateway, timeout) == true)
                {
                        strncpy(info->gateway, gateway, sizeof(info->gateway));
                }

                if (ipc_camera_device_token_get_dns(ip, port, user, password, dns, timeout) == true)
                {
                        strncpy(info->dns, dns, sizeof(info->dns));
                }
                return reslut;
        }

        if (ipc_camera_device_digest_get_network_interface(ip, port, user, password, info, timeout) == false)
        {
                printf("[%s:%d] diget network interface failed\n", __func__, __LINE__);
                reslut = false;
        }

        if (ipc_camera_device_digest_get_gateway(ip, port, user, password, gateway, timeout) == true)
        {
                strncpy(info->gateway, gateway, sizeof(info->gateway));
        }

        if (ipc_camera_device_digest_get_dns(ip, port, user, password, dns, timeout) == true)
        {
                strncpy(info->dns, dns, sizeof(info->dns));
        }

        return reslut;
}
/****************************************************************
**@日期: 2023-09-21
**@作者: leo.liu
**@功能: 设置网络信息
*****************************************************************/
bool ipc_camera_device_setting_network(const char *ip, int port, const char *user, const char *password, char auther_flag, struct ipcamera_network *info, int timeout)
{
        bool reslut = true;
        if (auther_flag == 0x00)
        {
                if (ipc_camera_device_token_set_gateway(ip, port, user, password, info->gateway, timeout) == false)
                {
                        printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                        // reslut = false;
                }

                if (ipc_camera_device_token_set_dns(ip, port, user, password, info->dns, timeout) == false)
                {
                        printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                        //  reslut = false;
                }

                /*放在最后，用于重启*/
                if (ipc_camera_device_token_set_network_interface(ip, port, user, password, info, timeout) == false)
                {
                        printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                        reslut = false;
                }

                return reslut;
        }

        if (ipc_camera_device_digest_set_dns(ip, port, user, password, info->dns, timeout) == false)
        {
                printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                // reslut = false;
        }

        if (ipc_camera_device_digest_set_gateway(ip, port, user, password, info->gateway, timeout) == false)
        {
                printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                // reslut = false;
        }
        /*放在最后，用于重启*/
        if (ipc_camera_device_digest_set_network_interface(ip, port, user, password, info, timeout) == false)
        {
                printf("[%s:%d] diget network interface failed\n", __func__, __LINE__);
                reslut = false;
        }
        return reslut;
}

/****************************************************************
**@日期: 2023-09-21
**@作者: leo.liu
**@功能: 获取对方时间
*****************************************************************/
bool ipc_camera_device_get_system_time(const char *ip, int port, const char *user, const char *password, char auther_flag, struct tm *tm, int timeout)
{
        bool reslut = true;
        if (auther_flag == 0x00)
        {
                /*放在最后，用于重启*/
                if ((reslut = ipc_camera_device_token_get_system_time(ip, port, user, password, tm, timeout)) == false)
                {
                        printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                        reslut = false;
                }
                return reslut;
        }

        if ((reslut = ipc_camera_device_digest_get_system_time(ip, port, user, password, tm, timeout)) == false)
        {
                printf("[%s:%d] token network interface failed\n", __func__, __LINE__);
                reslut = false;
        }

        return reslut;
}