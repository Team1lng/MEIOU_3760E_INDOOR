/*
 * (C) Copyright 2020
 * Cheng Lei, TUYA Software Engineering, chenglei@tuya.com.
 * Tuya pack code
 */

#ifndef tuya_sign__h
#define tuya_sign__h
 
#ifdef __cplusplus
extern "C" 
{
#endif

#define TUYA_SIGN_SIZE	256

typedef struct ota_sign {
	int len;
	unsigned char data[TUYA_SIGN_SIZE];
}__attribute__ ((packed)) ota_sign;

int tuya_sign_data(unsigned char* buf, unsigned int buf_len, ota_sign* sign);

int tuya_sign_data_local(unsigned char* buf, unsigned int buf_len, ota_sign* sign);

int tuya_sign_file(const char* name, ota_sign* sign);

int sha256sum(unsigned char* buf, unsigned int buf_len, unsigned char sum[32]);

int tuya_sha256sum_file(const char* name, unsigned char sum[32]);

#ifdef __cplusplus
}
#endif

#endif
