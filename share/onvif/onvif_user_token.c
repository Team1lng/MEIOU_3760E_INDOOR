
#include "onvif_user_token.h"
#include "mxml-private.h"
#include "mxml.h"
#include "libbase64.h"
#include "sha1/sha1.h"
#include <time.h>
#include <sys/time.h>
#include "onvif.h"
#include "md5.h"
/****************************************************************
**@日期: 2022-09-17
**@作者: leo.liu
**@功能:产生一个随机数字
*****************************************************************/
static unsigned long nonece_random_number(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        /****************************************************************
        2022-09-17 author:leo.liu 说明:获取随机数值
        *****************************************************************/
        srand((unsigned long)time(NULL));
        ms += rand();
        return ms;
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:获取随机数及base64decode 的随机数
*****************************************************************/
static size_t onvif_random_create(char *random, char *random_base64_encode, int string_len)
{
        memset(random, 0, string_len);
        memset(random_base64_encode, 0, string_len);
        sprintf(random, "%lu", nonece_random_number());
        base64_encode(random, strlen(random), random_base64_encode, (size_t *)&string_len, 0);
        return string_len;
}
/****************************************************************
**@日期: 2022-09-17
**@作者: leo.liu
**@功能:获取格林标准时间
*****************************************************************/
static size_t onvif_time_create(char *created, int len)
{
        time_t n;
        time_t t = time(&n);
        struct tm *p = gmtime(&t);
        snprintf(created, len, "%04d-%02d-%02dT%02d:%02d:%03dZ", 1900 + (p->tm_year), 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        return strlen(created);
}
/****************************************************************
**@日期: 2022-09-20
**@作者: leo.liu
**@功能:获取密码令牌
*****************************************************************/
static bool onvif_usertoken_create(const unsigned char *base64dec_romdom, size_t base64dec_romdom_len,
                                   const unsigned char *time, size_t time_len,
                                   const unsigned char *password, size_t pssword_len,
                                   char *token, size_t *token_len)
{
        char hash_buf[256] = {0};
        sprintf(hash_buf, "%s%s%s", base64dec_romdom, time, password);
        char hash2[20];
        SHA1(hash2, hash_buf, strlen(hash_buf));

        base64_encode(hash2, sizeof(hash2), token, token_len, 0);
        return true;
}
static bool onvif_usertoken_get(const char *password, char *random, char *random_base64, int random_len, char *created, int created_len, char *usertoken, size_t *token_len)
{
        onvif_random_create(random, random_base64, random_len);
        size_t time_len = onvif_time_create(created, created_len);
        if (onvif_usertoken_create((const unsigned char *)random, strlen(random),
                                   (const unsigned char *)created, time_len,
                                   (const unsigned char *)password, strlen(password),
                                   usertoken, token_len) == false)
        {
                printf("user token create failed \n");
                return false;
        }
        return true;
}
/****************************************************************
 **@日期: 2022-09-20
 **@作者: leo.liu
 **@功能: 获取username token xml
 *****************************************************************/
static bool onvif_token_head_node_get(char *data, int size, const char *user, const char *token, const char *base64dec_random, const char *time)
{
        // snprintf(data, size - 1, "<s:Header>\r\n"
        //                          "<wsse:Security\r\n"
        //                          "xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\"\r\n"
        //                          "xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">\r\n"
        //                          "<wsse:UsernameToken>\r\n"
        //                          "<wsse:Username>%s</wsse:Username>\r\n"
        //                          "<wsse:Password\r\n"
        //                          "Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%s</wsse:Password>\r\n"
        //                          "<wsse:Nonce>%s</wsse:Nonce>\r\n"
        //                          "<wsu:Created>%s</wsu:Created>\r\n"
        //                          "</wsse:UsernameToken>\r\n"
        //                          "</wsse:Security>\r\n"
        //                          "</s:Header>\r\n",
        //          user, token, base64dec_random, time);
        snprintf(data, size - 1, "\r\n<s:Header>\r\n"
                                 "<Security s:mustUnderstand=\"1\"\r\n"
                                 "xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\">\r\n"
                                 "<UsernameToken>\r\n"
                                 "<Username>%s</Username>\r\n"
                                 "<Password\r\n"
                                 "Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%s</Password>\r\n"
                                 "<Nonce\r\n"
                                 "EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">%s</Nonce>\r\n"
                                 "<Created\r\n"
                                 "xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">%s</Created>\r\n"
                                 "</UsernameToken>\r\n"
                                 "</Security>\r\n"
                                 "</s:Header>\r\n",
                 user, token, base64dec_random, time);
        return true;
}

static void MD5HexCover(char Bin[16], char Hex[33])
{
        unsigned short i;
        unsigned char j;

        for (i = 0; i < 32; i++)
        {
                j = (Bin[i] >> 4) & 0xf;
                if (j <= 9)
                        Hex[i * 2] = (j + '0');
                else
                        Hex[i * 2] = (j + 'a' - 10);
                j = Bin[i] & 0xf;
                if (j <= 9)
                        Hex[i * 2 + 1] = (j + '0');
                else
                        Hex[i * 2 + 1] = (j + 'a' - 10);
        };
        Hex[32] = '\0';
}

/****************************************************************
 **@日期: 2022-09-20
 **@作者: leo.liu
 **@功能: 获取digest 文本
 *****************************************************************/
static bool onvif_digest_get(const char *user, const char *password, const char *math, const char *uri, const char *qop, const char *realm, const char *nonce, const char *opaque, char *digest_string, size_t digest_size)
{
        char cnonce[256] = {0};
        char response[66] = {0};

        char digest_nc_string[32] = {0};
        /*static*/ unsigned long digest_nc = 1;
        sprintf(digest_nc_string, "%08lu", digest_nc++);
#if 1
        strcpy(cnonce, nonce);
        cnonce[0] = 'A';
#else
        strcpy(cnonce, "023C097935E1282AE32669E6E58F225E");
#endif
        /*获取response 公式：*/
        /*HA1 = MD5（<username>:<reaml>:<psd>）*/
        /*HA2 = MD5(<method>:<disgestUriPath>) */
        /*Response = MD5(HA1:<nonce>:<nc>:<conce>:<qop>:HA2)*/
        MD5_CTX ctx;
        char HA1[16] = {0};
        char CvtHA1Hex1[66] = {0};
        char CvtHA1Hex2[66] = {0};

        MD5Init(&ctx);
        MD5Update(&ctx, (unsigned char *)user, strlen(user));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)realm, strlen(realm));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)password, strlen(password));
        MD5Final((unsigned char *)HA1, &ctx);
        MD5HexCover(HA1, CvtHA1Hex1);

        MD5Init(&ctx);
        MD5Update(&ctx, (unsigned char *)math, strlen(math));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)uri, strlen(uri));
        MD5Final((unsigned char *)HA1, &ctx);
        MD5HexCover(HA1, CvtHA1Hex2);

        MD5Init(&ctx);
        MD5Update(&ctx, (unsigned char *)CvtHA1Hex1, strlen(CvtHA1Hex1));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)nonce, strlen(nonce));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)digest_nc_string, strlen(digest_nc_string));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)cnonce, strlen(cnonce));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)qop, strlen(qop));
        MD5Update(&ctx, (unsigned char *)":", 1);
        MD5Update(&ctx, (unsigned char *)CvtHA1Hex2, strlen(CvtHA1Hex2));
        MD5Final((unsigned char *)HA1, &ctx);
        MD5HexCover(HA1, response);

        if (opaque[0] != 0)
        {
                snprintf(digest_string, digest_size - 1, "Authorization: Digest username=\"%s\",realm=\"%s\",qop=\"%s\",algorithm=MD5,uri=\"%s\",nonce=\"%s\",nc=%s,cnonce=\"%s\",opaque=\"%s\",response=\"%s\"\r\n",
                         user,
                         realm,
                         qop,
                         uri,
                         nonce,
                         digest_nc_string,
                         cnonce,
                         opaque,
                         response);
        }
        else
        {
                snprintf(digest_string, digest_size - 1, "Authorization: Digest username=\"%s\",realm=\"%s\",qop=\"%s\",algorithm=MD5,uri=\"%s\",nonce=\"%s\",nc=%s,cnonce=\"%s\",response=\"%s\"\r\n",
                         user,
                         realm,
                         qop,
                         uri,
                         nonce,
                         digest_nc_string,
                         cnonce,
                         response);
        }

        return true;
}
/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能: 口令格式获取
*****************************************************************/
size_t onvif_xml_fomrat_by_token(char *data, const char *ip, const char *user, const char *password, int argc, char **argv)
{
        char random[128] = {0};
        char base64dec_random[128] = {0};
        char time[128] = {0};
        char token[256] = {0};
        size_t toekn_len = 0;
        if (onvif_usertoken_get(password, random, base64dec_random, sizeof(random), time, sizeof(time), token, &toekn_len) == false)
        {
                printf("usertoken get failed \n");
                return -1;
        }

        char *pxml = strstr(data, "<"); //?xml version=\"1.0\" encoding=\"utf-8\"?>
        if (pxml == NULL)
        {
                printf("not find <?xml version=\"1.0\" encoding=\"utf-8\"?> \n");
                return -1;
        }
        /*分配10k数据用于存储head节点信息*/
        const int p_head_node_size = (10 * 1024);
        char *p_head_node_data = (char *)malloc(p_head_node_size);
        memset(p_head_node_data, 0, p_head_node_size);
        onvif_token_head_node_get(p_head_node_data, p_head_node_size, user, token, base64dec_random, time);

        char *temp_file = malloc(XML_FILE_MAX);
        memset(temp_file, 0, XML_FILE_MAX);
        if (argc == 0)
        {
                sprintf(temp_file, pxml, p_head_node_data);
        }
        else if (argc == 1)
        {
                sprintf(temp_file, pxml, p_head_node_data, argv[0]);
        }
        else if (argc == 2)
        {
                sprintf(temp_file, pxml, p_head_node_data, argv[0], argv[1]);
        }
        else if (argc == 3)
        {
                sprintf(temp_file, pxml, p_head_node_data, argv[0], argv[1], argv[2]);
        }
        else if (argc == 4)
        {
                sprintf(temp_file, pxml, p_head_node_data, argv[0], argv[1], argv[2], argv[3]);
        }
        free(p_head_node_data);
        /*头文件与xml分离*/
        pxml[0] = 0;
        int tmp_file_length = strlen(temp_file);

        char *head = malloc(XML_FILE_MAX);
        sprintf(head, data, ip, "", tmp_file_length);
        strcpy(data, head);
        strcat(data, temp_file);

        free(temp_file);
        free(head);
        return strlen(data);
}
/****************************************************************
**@日期: 2022-09-21
**@作者: leo.liu
**@功能: 摘要格式获取
*****************************************************************/
size_t onvif_xml_fomrat_by_digest(char *file, const char *ip, const char *user, const char *password, const char *math, const char *uri, const char *qop, const char *realm, const char *nonce, const char *opaque, int argc, char **argv)
{
        char digest[512] = {0};
        if (password != NULL)
        {
                if (onvif_digest_get(user, password, math, uri, qop, realm, nonce, opaque, digest, sizeof(digest)) == false)
                {
                        printf("digest get failed \n");
                        return -1;
                }
        }
        char *pxml = strstr(file, "<"); //?xml version=\"1.0\" encoding=\"utf-8\"?>
        if (pxml == NULL)
        {
                printf("not find <?xml version=\"1.0\" encoding=\"utf-8\"?> \n");
                return false;
        }
        char *temp_file = malloc(XML_FILE_MAX);
        memset(temp_file, 0, XML_FILE_MAX);
        if (argc == 0)
        {
                sprintf(temp_file, pxml, "");
        }
        else if (argc == 1)
        {
                sprintf(temp_file, pxml, "", argv[0]);
        }
        else if (argc == 2)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1]);
        }
        else if (argc == 3)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1], argv[2]);
        }
        else if (argc == 4)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1], argv[2], argv[3]);
        }
        else if (argc == 5)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1], argv[2], argv[3], argv[4]);
        }
        else if (argc == 6)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
        }
        else if (argc == 7)
        {
                sprintf(temp_file, pxml, "", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
        }
        /*头文件与xml分离*/
        pxml[0] = 0;

        int tmp_file_length = strlen(temp_file);

        char *head = malloc(XML_FILE_MAX);
        memset(head, 0, XML_FILE_MAX);

        sprintf(head, file, ip ? ip : "", password != NULL ? digest : "", tmp_file_length);

        strcpy(file, head);
        strcat(file, temp_file);

        free(temp_file);
        free(head);
        return strlen(file);
}