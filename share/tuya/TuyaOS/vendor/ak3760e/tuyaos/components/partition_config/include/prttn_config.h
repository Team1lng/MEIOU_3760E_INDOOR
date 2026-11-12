#ifndef __PRTTN_CONFIG_H__
#define __PRTTN_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PRTTN_SIZE 16
#define MAX_BUF_LEN 32
#define MAX_PARAM_INFO  1024
#define DEFAULT_MAGIC 0xa55a5aa5

typedef struct prttn_info {
    int init;
    char name[MAX_BUF_LEN];
    unsigned int offset;
    unsigned int size;
}__attribute__((packed))PRTTN_INFO_T;

// save  param
typedef struct boot_param {
    char cmd[MAX_PARAM_INFO];
    char args[MAX_PARAM_INFO];
    PRTTN_INFO_T prttn[MAX_PRTTN_SIZE];
}__attribute__((packed))BOOT_PARAM_T;

typedef struct boot_struct
{
    unsigned int magic;
    unsigned int len;
    char sha256[32];
    uint8_t data[0];
}__attribute__((packed))BOOT_T;


#ifdef __cplusplus
extern "C" {
#endif

#endif
