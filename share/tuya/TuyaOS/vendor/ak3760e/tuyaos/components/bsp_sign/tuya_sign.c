/*
 * (C) Copyright 2020
 * Cheng Lei, TUYA Software Engineering, chenglei@tuya.com.
 * Tuya pack code
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "tuya_sign.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/pk.h"

#include <openssl/engine.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

//#define TUYA_BSP_SIGN_COULD
//#define USE_CURL_OPENSSL 1

#define PRDBG(...) \
	do { printf(__VA_ARGS__); } while (0);

#define PRERR(...) \
	do { printf(__VA_ARGS__); } while (0);

const char* request_curl = "https://testbook-ng.tuya-inc.top:7799/open/call";
//const char* request_curl = "http://testbook.daily.tuya-inc.cn:7788/open/call";
const char* token_curl = "token:1uya!%2535@";

#ifdef USE_CURL_OPENSSL

#include <openssl/sha.h>
#include <curl/curl.h>
#include <curl/easy.h>

size_t request_curl_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	ota_sign* sign = (ota_sign*)(data);

	cJSON* root = NULL;
	cJSON* result = NULL;
	cJSON* response = NULL;

	root = cJSON_Parse(ptr);	
	if (root == NULL) {
		PRERR("parsed json root failed.\n");
		return 0;
	}

	result = cJSON_GetObjectItemCaseSensitive(root, "data");
	if (result == NULL) {
		PRERR("parsed json data failed.\n");
		return 0;
	}

	result = cJSON_Parse(result->valuestring);
	if (result == NULL) {
		PRERR("parsed json valuestring failed.\n");
		return 0;
	}

	response = cJSON_GetObjectItemCaseSensitive(result, "result");
	if (response == NULL) {
		PRERR("parsed json response failed.\n");
		return 0;
	}

	if (cJSON_IsString(response) && (response->valuestring != NULL)) {
		sign->len = strlen(response->valuestring);
		strncpy(sign->data, response->valuestring, sign->len);
		sign->data[sign->len] = '\0';
		sign->len += 1;
	} else {
		PRERR("invalid json response.\n");
		return 0;
	}

    return (size*nmemb);
}

static int request_sign_from_server(ota_sign* sign, const char* sha256)
{
	CURL* curl_handle = NULL;
	struct curl_slist *pList = NULL;
	long res_code = 0;
	char buf[512] = {0};
	int ret = -1;

	if (sign == NULL) {
		PRERR("get sign pointer fail.\n");
		return -1;
	}

	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (CURLE_OK != res) {
		PRERR("curl init failed.\n");
		return -1;
	}

	curl_handle = curl_easy_init();
	if (NULL == curl_handle) {
		PRERR("init curl failed.\n");
		return -1;
	}

	pList = curl_slist_append(pList, token_curl); 
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, pList); 
	curl_easy_setopt(curl_handle, CURLOPT_URL, request_curl);
	curl_easy_setopt(curl_handle, CURLOPT_HEADER, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, sign);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, request_curl_callback); 

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "region=AY&api=tuya.m.file.sign_1.0&postData={ \"fileHash\" : \"%s\" }&env=prod&verify=False", sha256);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, buf);

	res = curl_easy_perform(curl_handle);
	res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &res_code);

	if ((res == CURLE_OK) && (res_code == 200 || res_code == 201)) {
		PRDBG("get curl request success.\n");
		ret = 0;
	} else {
		PRERR("get curl request failed.\n");
		ret = -1;
	}

	curl_slist_free_all(pList); 
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	return ret;
}

int sha256sum(unsigned char* buf, unsigned int buf_len, unsigned char sum[32])
{
	SHA256_CTX sha;

	SHA256_Init(&sha);
	SHA256_Update(&sha, buf, buf_len);
	SHA256_Final(sum, &sha);

	int i = 0;
	for (i = 0; i < 32; i++) {
		PRDBG("%02x", sum[i]);
	}
	PRDBG("\n");

	return 0;
}

#else

#include "http_inf.h"
#include "mbedtls/sha256.h"

int http_recv(char *buf, int size, void *user_data)
{
	ota_sign* sign = (ota_sign*)(user_data);

	cJSON* root = NULL;
	cJSON* result = NULL;
	cJSON* response = NULL;

	printf("buf: %s\n", buf);

	root = cJSON_Parse(buf);
	if (root == NULL) {
		PRERR("parsed json root failed.\n");
		return 0;
	}

	result = cJSON_GetObjectItemCaseSensitive(root, "data");
	if (result == NULL) {
		PRERR("parsed json data failed.\n");
		return 0;
	}

	result = cJSON_Parse(result->valuestring);
	if (result == NULL) {
		PRERR("parsed json valuestring failed.\n");
		return 0;
	}

	response = cJSON_GetObjectItemCaseSensitive(result, "result");
	if (response == NULL) {
		PRERR("parsed json response failed.\n");
		return 0;
	}

	if (cJSON_IsString(response) && (response->valuestring != NULL)) {
		sign->len = strlen(response->valuestring);
		strncpy((char *)sign->data, response->valuestring, sign->len);
		sign->data[sign->len] = '\0';
		sign->len += 1;
	} else {
		PRERR("invalid json response.\n");
		return 0;
	}

    return 0;
}

int request_sign_from_server(ota_sign* sign, const char* sha256)
{
	int res = 0;	
	char buf[512] = {0};

	if (sign == NULL) {
		printf("get sign pointer fail.\n");
		return -1;
	}

	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "region=AY&api=tuya.m.file.sign_1.0&postData={ \"fileHash\" : \"%s\" }&env=prod&verify=False", sha256);
	//sprintf(buf, "region=daily&api=tuya.m.file.sign_1.0&postData={ \"fileHash\" : \"%s\" }&env=daily", sha256);

	res = tuya_http_inf_post((char*)request_curl, http_recv, (unsigned char *)buf, strlen(buf), (char*)token_curl, sign);
	if (res < 0) {
		printf("request sign from server fail.\n");
		return -1;
	}

    return 0;
}

int sha256sum(unsigned char* buf, unsigned int buf_len, unsigned char sum[32])
{
	if (mbedtls_sha256_ret(buf, buf_len, sum, 0))
		return -1;

	int i = 0;
	for (i = 0; i < 32; i++) {
		PRDBG("%02x", sum[i]);
	}
	PRDBG("\n");

	return 0;
}
#endif

static void int2char(int num, char *ch)
{
	if (num <= 9) {
		*ch = num + '0'; 
	} else {
		*ch = (num -10) + 'a';
	}
}
 
static void hex2str(char *hex, int len, char *str)
{
	int i = 0, j = 0;

	for (i = 0; i < len; ++i) {
		int2char((hex[i] >> 4) & 0x0F, &str[j++]);	
		int2char(hex[i] & 0x0F, &str[j++]);
	}
}

int tuya_sign_data_local(unsigned char* buf, unsigned int buf_len, ota_sign* sign)
{
	int ret = -1;
	unsigned char sum_hex[32] = {0};
	char sum_str[128] = {0};

    FILE* fp;
    EVP_PKEY *privateKey;
    EC_KEY *eckey;
    EC_GROUP *ec_group;
    EC_KEY *ecdh;
    ECDSA_SIG *signature;
    int rnum_len = 32;
    unsigned char *der = NULL;
    ECDSA_SIG *aa;
    int derlen = -1;
    int i;

	memset(sign, 0, sizeof(ota_sign));

	sha256sum(buf, buf_len, sum_hex);

	hex2str((char *)sum_hex, 32, sum_str);
	PRDBG("%s: sum_str %s\n", __func__, sum_str);

    printf("tuya local sign\n");
	/* ecdsa_sign()接口调用无法使用 */
    //ret = ecdsa_sign(3, (unsigned long long *)pri_key, (unsigned long long *)sum_hex, (unsigned long long *)sign->data);

	/* mbedtls库接口无法使用 */
	//tuyaos_mbedtls_sign(sum_hex, 32, privKeyOpenSSLBin, 32);

	/* pk库接口未完成 */
	//tuyaos_pk_sign(privKeyOpenSSL, sum_hex, 32, sign->data);

	/*openssl*/
	fp = fopen("./tuyaos_ecprikey.pem", "r");
	if (fp == NULL) {
		printf( "Could not open private key file\n");
		return -1;;
	}
	/* 从PEC文件获取私钥  */
	privateKey= PEM_read_PrivateKey(fp, NULL, 0, NULL);
	if ((privateKey) == NULL) {
		printf("Could not extract private key from file\n");
		return -1;
	}
	fclose(fp);

	eckey = EC_KEY_new();
	eckey= EVP_PKEY_get1_EC_KEY(privateKey);
	if ( eckey == NULL ) {
		return -1;
	}
	/* 设置椭圆曲线SECP256K1 */
	ec_group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	//const EC_POINT* pub_key;
	//pub_key = EC_KEY_get0_public_key(eckey);

	/* 合成带椭圆曲线参数和来自PEM文件私钥的新公私钥对keypair */
	ecdh = EC_KEY_new();
	EC_KEY_set_group(ecdh, ec_group);
	EC_KEY_set_private_key(ecdh, EC_KEY_get0_private_key(eckey));
	EC_KEY_set_public_key(ecdh, EC_KEY_get0_public_key(eckey));

#if 0
	/* 加签方法1 */
	unsigned char sig[256] = {0};
	unsigned int sigsize;
	ECDSA_sign(0,sum_hex,32,sig,&sigsize,ecdh);
	printf("Signature size:%d \n",sigsize);
	for(int i=0; i <sigsize;i++) {
		printf("%x",sig[i]);
	}
#else
	/* 加签方法2 */
	signature = ECDSA_do_sign(sum_hex, rnum_len, ecdh);
#endif

    aa = signature;
    derlen = i2d_ECDSA_SIG(aa, &der); //DER 2 hex
    sign->len = derlen*2;

    /* ASN.1(DER) ---> hex ---> 0123456789ABCDEF, adapte to ecdsa_veriry() */
    for( i = 0; i < derlen; i++ )
    {
        if((der[i]/16 >= 0) && (der[i]/16 <=9))
        {
            sign->data[2*i] = (der[i]/16)+0x30;
        }
        if((der[i]/16 >= 0xa) && (der[i]/16 <=0xf))
        {
            sign->data[2*i] = (der[i]/16)-0xa+0x61;
        }

        if((der[i]%16 >= 0) && (der[i]%16 <=9))
        {
            sign->data[2*i+1] = (der[i]%16)+0x30;
        }
        if((der[i]%16 >= 0xa) && (der[i]%16 <=0xf))
        {
            sign->data[2*i+1] = (der[i]%16)-0xa+0x61;
        }
    }
	ret =0;
    EC_KEY_free(eckey);
    EC_KEY_free(ecdh);

	if (ret == 0 && sign->len > 0)
		return 0;
	else
		return -1;
}


int tuya_sign_data(unsigned char* buf, unsigned int buf_len, ota_sign* sign)
{
	int ret = -1;
	unsigned char sum_hex[32] = {0};
	char sum_str[128] = {0};


	memset(sign, 0, sizeof(ota_sign));

	sha256sum(buf, buf_len, sum_hex);

	hex2str((char *)sum_hex, 32, sum_str);
	PRDBG("%s: sum_str %s\n", __func__, sum_str);

	ret = request_sign_from_server(sign, sum_str);

	if (ret == 0 && sign->len > 0)
		return 0;
	else
		return -1;
}

static int get_file_len(const char* name)
{
    struct stat st;

    if (stat(name, &st) != 0) {
        PRERR("can't access to %s\n", name);
        return -1;
    }

	PRDBG("%s: %ld\n", __func__, st.st_size);

    return st.st_size;
}

int tuya_sign_file(const char* name, ota_sign* sign)
{
	int file_len = 0;
	unsigned char* buf = NULL;
	FILE* fp = NULL;
	int ret = -1;

	do {
		file_len = get_file_len(name);
		if (file_len <= 0)
			break;

		buf = calloc(file_len, sizeof(unsigned char));
		if (buf == NULL) {
			PRERR("%s: alloc memory failed.\n", __func__);
			break;
		}

		fp = fopen(name, "rb");
		if (fp == NULL) {
			PRERR("%s: open file failed.\n", __func__);
			break;
		}

		if (fread(buf, file_len, 1, fp) != 1) {
			PRERR("%s: read file failed.\n", __func__);
			break;
		}

		ret = tuya_sign_data(buf, file_len, sign);

	} while (0);

	if (buf)
		free(buf);

	if (fp)
		fclose(fp);

	return ret;
}

int tuya_sha256sum_file(const char* name, unsigned char sum[32])
{
	int file_len = 0;
	unsigned char* buf = NULL;
	FILE* fp = NULL;
	int ret = -1;

	do {
		file_len = get_file_len(name);
		if (file_len <= 0)
			break;

		buf = calloc(file_len, sizeof(unsigned char));
		if (buf == NULL) {
			PRERR("%s: alloc memory failed.\n", __func__);
			break;
		}

		fp = fopen(name, "rb");
		if (fp == NULL) {
			PRERR("%s: open file failed.\n", __func__);
			break;
		}

		if (fread(buf, file_len, 1, fp) != 1) {
			PRERR("%s: read file failed.\n", __func__);
			break;
		}

		ret = sha256sum(buf, file_len, sum);

	} while (0);

	if (buf)
		free(buf);

	if (fp)
		fclose(fp);

	return ret;
}
