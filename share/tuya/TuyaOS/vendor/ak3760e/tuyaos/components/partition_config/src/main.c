#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include "aes.h"
#include "sha256.h"
#include "cJSON.h"
#include "prttn_config.h"
#include "config.h"

#define ITEM_PRODUCT_NAME        "product_name"
#define ITEM_FLASH_TYPE          "flash_type"
#define ITEM_FLASH_SIZE          "flash_size"
#define ITEM_PACK_OTA_ONLY       "pack_ota_only"
#define ITEM_OTA_MODE            "ota_mode"
#define ITEM_FILES               "files"
#define ITEM_PARTITION           "partitions"
#define ITEM_PART                "part%d"
#define ITEM_PART_NAME           "name"
#define ITEM_PART_VERSION        "version"
#define ITEM_PART_OFFSET         "offset"
#define ITEM_PART_SIZE           "size"
#define ITEM_PART_DEVICE         "device"
#define ITEM_PART_DEVICE_OFFSET  "device_offset"
#define ITEM_PART_FILENAME       "filename"
#define ITEM_PART_NEED_OTA       "need_ota"
#define ITEM_PART_FILESYSTEM     "filesystem"
#define ITEM_PART_COPY_IN_BOOT   "copy_in_boot"

#define _1M_MASK (0xfffff)
#define _1K_MASK (0x3ff)

#define MAX_PRTTN_STRING 512
#define MAX_PATH_LEN 128

#define DEFAULT_PRTTN_INFO_FILE "prttn_info.bin"
#define DEFAULT_PACK_INFO_FILE  "partition.json"


typedef struct prttn_cfg {
    int init;
    char name[MAX_BUF_LEN];
    char version[MAX_BUF_LEN];
    uint32_t offset;
    uint32_t size;
    char dev[2][MAX_BUF_LEN];
    uint32_t device_offset;
    char filename[MAX_BUF_LEN];
    int need_ota;
    char fs_type[MAX_BUF_LEN];
    int copy_in_boot;
}PRTTN_CFG_T;

typedef struct param_info {
    int init;
    char buf[MAX_PARAM_INFO];
} __attribute__((packed)) PARAM_INFO_T;

typedef struct boot_info {
    char  product_name[MAX_BUF_LEN];
    char flash_type[MAX_BUF_LEN];
    uint32_t flash_size;
    int ota_only_flag;
    char ota_mode[MAX_BUF_LEN];
    int bp_prttn_index; //boot param partition index
    PARAM_INFO_T phead;
    PARAM_INFO_T cmd;
    PARAM_INFO_T args;
    PRTTN_CFG_T prttn[MAX_PRTTN_SIZE];
}BOOT_INFO_T;


#define  PRTTN_INFO_CRT(i)  \
 do { \
    PRTTN_CFG_T *prttn = &sg_boot_info.prttn[i]; \
    memset (prttn, 0, sizeof(PRTTN_CFG_T)); \
    if(i >= MAX_PRTTN_SIZE) { \
       fprintf(stderr, "prttn index(%d) > prttn max(%d)\n", i, MAX_PRTTN_SIZE);  \
       return -1; \
    } \
    memset(prttn, 0, sizeof(PRTTN_CFG_T)); \
    prttn->init = 1;  \
    strncpy(prttn->name, TUYA_PARTITION_##i##_NAME, MAX_BUF_LEN-1); \
    strncpy(prttn->version, TUYA_PARTITION_##i##_VERSION, MAX_BUF_LEN-1); \
    prttn->offset = TUYA_PARTITION_##i##_OFFSET; \
    prttn->size = TUYA_PARTITION_##i##_SIZE; \
    strncpy(prttn->dev[0], TUYA_PARTITION_##i##_DEVICE_0, MAX_BUF_LEN-1); \
    strncpy(prttn->dev[1], TUYA_PARTITION_##i##_DEVICE_1, MAX_BUF_LEN-1); \
    prttn->device_offset = TUYA_PARTITION_##i##_DEVICE_OFFSET; \
    strncpy(prttn->filename, TUYA_PARTITION_##i##_FILE_NAME, MAX_BUF_LEN-1); \
    prttn->need_ota = TUYA_PARTITION_##i##_NEED_OTA? 1 : 0; \
    strncpy(prttn->fs_type, TUYA_PARTITION_##i##_FIELSYSTEM_TYPE, MAX_BUF_LEN-1); \
    prttn->copy_in_boot = TUYA_PARTITION_##i##_COPY_IN_BOOT? 1 :0; \
}while(0)

#define PARAM_INFO_CRT(t,c) \
do {  \
    PARAM_INFO_T *param = &sg_boot_info.c; \
    memset(param, 0, sizeof(PARAM_INFO_T)); \
    param->init = 1; \
    strncpy(param->buf, TUYA_TARGET_##t, MAX_PARAM_INFO-1); \
}while(0)

static BOOT_INFO_T sg_boot_info = {0};

static char sg_buf[MAX_BUF_LEN] = {0};


static char *hex2string(uint32_t val)
{
    memset(sg_buf,0,MAX_BUF_LEN);
    snprintf(sg_buf, MAX_BUF_LEN, "0x%x", val);
    return sg_buf;
}

static char *int2string(int val)
{
    memset(sg_buf,0,MAX_BUF_LEN);
    snprintf(sg_buf, MAX_BUF_LEN, "%d", val);
    return sg_buf;
}

static char *idx2name(int val)
{
    memset(sg_buf,0,MAX_BUF_LEN);
    snprintf(sg_buf, MAX_BUF_LEN, ITEM_PART, val+1);
    return sg_buf;
}

static void __usage(char *program_name)
{
    fprintf(stdout, "Usage %s [OPTION]\n", program_name);
    fprintf(stdout,
        " -h, --help              : print this help and exit\n"
        " -b, --boot              : boot info partition info\n"
        " -o, --output            : output pack  partition info file\n"
        " -d, --dump              : dump boot info \n"
           );
}

static const char *short_options = "hb:o:d";

static struct option long_options[] = {
    {"help",  no_argument, NULL, 'h'},
    {"boot",  required_argument,NULL,'b'},
    {"output",required_argument,NULL,'o'},
    {"dump",required_argument,NULL,'d'},
    {NULL, 0, NULL, 0}
};

static int __dump_boot_parm(BOOT_PARAM_T *param) 
{
    int i;

    if(param == NULL) {
        fprintf(stderr, "%s invalid param\n", __func__);
        return -1;
    }

    for(i = 0; i < MAX_PRTTN_SIZE; i++) {
       PRTTN_INFO_T *prttn = &param->prttn[i];
       if(prttn->init) {
           printf("prttn num:%d\n", i);
           printf("prttn name:%s\n", prttn->name);
           printf("prttn offset:0x%x\n", prttn->offset);
           printf("prttn size:0x%x\n", prttn->size);
       }
   }

   printf("bootcmd:%s\n", param->cmd);
   printf("bootargs:%s\n", param->args);

   return 0;
}

static int __dump_boot_bin(char *file) 
{
    int len = 0;
    int iret = 0;
    FILE *fp;
    uint8_t sha256_val[32] = {0};
    BOOT_T *p = NULL;
    BOOT_PARAM_T *b_data = NULL;

#ifdef TUYA_BOOT_PARAMS_CRYPT_ENABLE
    unsigned int b_len = 0;
#endif

    if(file == NULL) {
        fprintf(stderr, "%s invalid param\n", __func__);
        return -1;	
    }

    fp = fopen(file, "r");

    if(fp == NULL) {
        fprintf(stderr, "open %s fail\n", file); 
	return -1;
    }

    fseek(fp, 0, SEEK_END);

    len = ftell(fp);

    printf("%s size:%d\n",file,len);

    p = (BOOT_T *)malloc(len);

    if(p == NULL) {
        fprintf(stderr, "malloc fail\n");
	iret = -1;
        goto EXIT_FD;
    }

    fseek(fp, 0, SEEK_SET);

    if(len != fread(p, 1, len, fp)) {
        fprintf(stderr, "fread fail,len:%d\n",len);
	iret = -1;
	goto EXIT_BUF;
    }

    if(p->magic != DEFAULT_MAGIC) {
        fprintf(stderr, "magic(0x%x) error\n", p->magic);
	iret = -1;
	goto EXIT_BUF;
    }

    if(p->len != len - sizeof(BOOT_T)) {
        fprintf(stderr, "data len:%d error\n", p->len);
	iret = -1;
	goto EXIT_BUF;
    }

    if(sha256(p->data, p->len, sha256_val)) {
        fprintf(stderr, "sha256 fail\n");
	iret = -1;
	goto EXIT_BUF;
    }

    if(memcmp(p->sha256, sha256_val, 32)) {
        fprintf(stderr, "sha256 is not same\n"); 
	iret = -1;
	goto EXIT_BUF;
    }

#ifdef TUYA_BOOT_PARAMS_CRYPT_ENABLE
    printf(" ---- crypt support ----\n");
    if(aes128_data_decode((const unsigned char *)p->data, (const unsigned int)p->len,
                              (unsigned char **)&b_data, &b_len,
                              (const unsigned char *)TUYA_BOOT_PARAMS_CRYPT_KEY)) {
 
         fprintf(stderr, "data decode fail\n");

         if(b_data) {
             free(b_data); 
         }

         iret = -1;
         goto EXIT_BUF;
    }

     __dump_boot_parm(b_data);
     free(b_data);
#else 
    printf(" ---- crypt unsupport ----\n");
    b_data = (BOOT_PARAM_T *)p->data;
    __dump_boot_parm(b_data);
#endif

EXIT_BUF:
    free(p);
EXIT_FD:
    fclose(fp);

    return iret;
}

static int __dump_boot_cfg(BOOT_INFO_T *info) 
{
    int i;

    if(info == NULL) {
        fprintf(stderr, "%s invalid param\n", __func__);
        return -1;
    }

    printf("product_name       :%s\n",info->product_name);
    printf("flash type         :%s\n",info->flash_type);
    printf("flash size         :0x%x\n",info->flash_size);
    printf("ota only           :%d\n",info->ota_only_flag);
    printf("ota mode           :%s\n",info->ota_mode);
    printf("bp prttn index     :%d\n",info->bp_prttn_index);

    for(i = 0; i < MAX_PRTTN_SIZE; i++) {

        PRTTN_CFG_T *prttn = &info->prttn[i];

        if(!prttn->init) {
            continue;
        }
        printf("prttn num          :%d\n", i);
        printf("prttn name         :%s\n", prttn->name);
        printf("prttn version      :%s\n", prttn->version);
        printf("prttn offset       :0x%x\n", prttn->offset);
        printf("prttn size         :0x%x\n", prttn->size);
        if(strlen(prttn->dev[0])) {
            printf("partition dev[%d]  :%s\n", 0, prttn->dev[0]);
        }
        if(strlen(prttn->dev[1])) {
	    printf("partition dev[%d]  :%s\n", 1, prttn->dev[1]);
	}

	printf("prttn device offset:0x%x\n",prttn->device_offset);
	printf("prttn filename     :%s\n",prttn->filename);
	printf("prttn need_ota     :%d\n",prttn->need_ota);
	printf("prttn fs_type      :%s\n",prttn->fs_type);
	printf("prttn copy_in_boot :%d\n",prttn->copy_in_boot);
   }

   if(info->phead.init) { 
       printf("prttn head:%s\n", info->phead.buf);
   }

   if(info->args.init) {
       printf("boot args:%s\n", info->args.buf);
   }

   if(info->cmd.init) {
       printf("boot cmd:%s\n", info->cmd.buf);
   }

   return 0;
}

static int __boot_info_init(void)
{
    BOOT_INFO_T *info = &sg_boot_info; 

    memset(info, 0, sizeof(BOOT_INFO_T));

    strncpy(info->product_name, TUYA_TARGET_PRODUCT_NAME, MAX_BUF_LEN -1);
    strncpy(info->flash_type, TUYA_TARGET_FLASH_TYPE, MAX_BUF_LEN -1);
    info->flash_size = TUYA_TARGET_FLASH_TOTAL_SIZE;
    info->ota_only_flag = TUYA_TARGET_PACK_OTA_ONLY? 1 : 0;
    strncpy(info->ota_mode, TUYA_TARGET_OTA_MODE, MAX_BUF_LEN -1);
    info->bp_prttn_index = TUYA_BOOT_PARAM_PARTITION_INDEX;

    #ifdef TUYA_PARTITION_0
    PRTTN_INFO_CRT(0);
    #endif
    #ifdef TUYA_PARTITION_1
    PRTTN_INFO_CRT(1);
    #endif
    #ifdef TUYA_PARTITION_2
    PRTTN_INFO_CRT(2);
    #endif
    #ifdef TUYA_PARTITION_3
    PRTTN_INFO_CRT(3);
    #endif
    #ifdef TUYA_PARTITION_4
    PRTTN_INFO_CRT(4);
    #endif
    #ifdef TUYA_PARTITION_5
    PRTTN_INFO_CRT(5);
    #endif
    #ifdef TUYA_PARTITION_6
    PRTTN_INFO_CRT(6);
    #endif
    #ifdef TUYA_PARTITION_7
    PRTTN_INFO_CRT(7);
    #endif
    #ifdef TUYA_PARTITION_8
    PRTTN_INFO_CRT(8);
    #endif
    #ifdef TUYA_PARTITION_9
    PRTTN_INFO_CRT(9);
    #endif
    #ifdef TUYA_PARTITION_10
    PRTTN_INFO_CRT(10);
    #endif
    #ifdef TUYA_PARTITION_11
    PRTTN_INFO_CRT(11);
    #endif
    #ifdef TUYA_PARTITION_12
    PRTTN_INFO_CRT(12);
    #endif
    #ifdef TUYA_PARTITION_13
    PRTTN_INFO_CRT(13);
    #endif
    #ifdef TUYA_PARTITION_14
    PRTTN_INFO_CRT(14);
    #endif
    #ifdef TUYA_PARTITION_15
    PRTTN_INFO_CRT(15);
    #endif

    #ifdef TUYA_TARGET_BOOT_COMMAND_ENABLE
    PARAM_INFO_CRT(BOOT_COMMAND, cmd);
    #endif

    #ifdef TUYA_TARGET_MTDPARTS_ENABLE
    PARAM_INFO_CRT(MTDPARTS_HEAD, phead);
    #endif

    #ifdef TUYA_TARGET_BOOTARGS_ENABLE
    PARAM_INFO_CRT(BOOTARGS, args);
    #endif

    __dump_boot_cfg(info); 

    return 0;
}

static int __u32tostring(unsigned int data, char *buf, int size)
{
    char c;
    int n;

    if(!(data&_1M_MASK)) {
        c='m';
        data >>= 20;
    } else if (!(data&_1K_MASK)) {
        c='k';
        data >>= 10;
    }else {
        printf("0x%x need 1024 align", data);
	return -1;
    }

    n = snprintf(buf, size, "%d%c", data, c);

    return n > 0 ? 0 : -1;
}

static int __save_data(char *file, char *buf, int len, char *mode)
{
    FILE *fp;
    int iret = 0;

    if(file == NULL || buf == NULL || len <= 0 || mode == NULL) {
         fprintf(stderr, "%s invalid param\n", __func__);
         return -1;	 
    }

    fp = fopen(file, mode);

    if(fp == NULL) {
        fprintf(stderr, "open %s fail\n", file);
	return -1;
    }

    if (len != fwrite(buf, 1, len, fp)) {
        fprintf(stderr,"fwrite fail\n");
	iret = -1;
    }

    fclose(fp);

    return iret;
}

static int __create_partition_file(BOOT_INFO_T *info, char *out_info_file)
{
    int i;
    int iret = -1;
    char *json_data;
    cJSON *c_prttn;
    cJSON* c_root = cJSON_CreateObject();

    if(info == NULL || out_info_file == NULL) {
        printf("invalid param\n");
        return -1;	
    }

    cJSON_AddStringToObject(c_root, ITEM_PRODUCT_NAME, info->product_name);
    cJSON_AddStringToObject(c_root, ITEM_FLASH_TYPE, info->flash_type);
    cJSON_AddStringToObject(c_root, ITEM_FLASH_SIZE, hex2string(info->flash_size));
    cJSON_AddStringToObject(c_root, ITEM_PACK_OTA_ONLY, int2string(info->ota_only_flag));
    cJSON_AddStringToObject(c_root, ITEM_OTA_MODE, info->ota_mode);
    cJSON_AddObjectToObject(c_root, ITEM_FILES);

    c_prttn = cJSON_AddObjectToObject(c_root, ITEM_PARTITION);

    for(i = 0; i< MAX_PRTTN_SIZE; i++) {

        cJSON *c_part;
        cJSON *c_dev;
        int dev_num = 0;
        PRTTN_CFG_T *prttn = &info->prttn[i];
        char *dev_array[2] = {0};

        if(!prttn->init) {
            continue; 
        }

        c_part = cJSON_AddObjectToObject(c_prttn,idx2name(i));

        cJSON_AddStringToObject(c_part, ITEM_PART_NAME, info->prttn[i].name);
        cJSON_AddStringToObject(c_part, ITEM_PART_VERSION, info->prttn[i].version);
        cJSON_AddStringToObject(c_part, ITEM_PART_OFFSET, hex2string(info->prttn[i].offset));
        cJSON_AddStringToObject(c_part, ITEM_PART_SIZE, hex2string(info->prttn[i].size));

        if(strlen(info->prttn[i].dev[0])) {
            dev_num ++; 
            dev_array[0] = info->prttn[i].dev[0];
        }

        if(strlen(info->prttn[i].dev[1])) {
            dev_num ++; 
            dev_array[1] = info->prttn[i].dev[1];
        }

        c_dev = cJSON_CreateStringArray((const char **)dev_array, dev_num);

        cJSON_AddItemToObject(c_part, ITEM_PART_DEVICE, c_dev);

        cJSON_AddStringToObject(c_part, ITEM_PART_DEVICE_OFFSET, hex2string(info->prttn[i].device_offset));
        cJSON_AddStringToObject(c_part, ITEM_PART_FILENAME, info->prttn[i].filename);
        cJSON_AddStringToObject(c_part, ITEM_PART_NEED_OTA, int2string(info->prttn[i].need_ota));
        cJSON_AddStringToObject(c_part, ITEM_PART_FILESYSTEM, info->prttn[i].fs_type);
        cJSON_AddStringToObject(c_part, ITEM_PART_COPY_IN_BOOT, int2string(info->prttn[i].copy_in_boot));
    }

    json_data = cJSON_Print(c_root);

    iret = __save_data(out_info_file, json_data, strlen(json_data), "w+");

    printf("---------------------------------------------------------------------------------\n");
    printf("%s\n",json_data);
    printf("---------------------------------------------------------------------------------\n");
 
    cJSON_Delete(c_root);

    return iret;
}

static int __get_compose_partition(BOOT_INFO_T *info)
{
    int i,j;

    if(info == NULL) {
        fprintf(stderr, "invalid param\n"); 
        return -1; 
    }

    for(i = 0; i < MAX_PRTTN_SIZE; i++) {

       if(!info->prttn[i].init) {
           continue;
       }

       for(j = i + 1; j < MAX_PRTTN_SIZE; j++) {

          if(!info->prttn[j].init) {
	      continue;
	  }

	  if(!strcmp(info->prttn[i].dev[0],info->prttn[j].dev[0])) {
	      if(info->prttn[i].size == info->prttn[j].device_offset) { 

	          info->prttn[j].init = 0;
	          info->prttn[i].size += info->prttn[j].size;

		  if(info->prttn[i].offset  > info->prttn[j].offset)
	              info->prttn[i].offset = info->prttn[j].offset;

	      } else {
	 	  fprintf(stderr, "prttn[%d] && prttn[%d] parameter conflict\n", i+1, j+1); 
		  return -1;
                 }
            }
        }
    }

    return 0;	
}


static int __create_full_bootargs(BOOT_INFO_T *info, char *bootargs, int max_len)
{
    int i;
    int n = 0;
    char t[32] = {0};
    char *buf = bootargs;

    if(info == NULL || buf == NULL) {
        fprintf(stderr, "%s invalid param\n", __func__);
	return -1;
    }

    if(info->args.init) {
        n += sprintf(&buf[n],"%s ", info->args.buf);
    }
    
    if((strlen(info->args.buf) + MAX_PRTTN_STRING + 1) >= max_len) {
        fprintf(stderr,"bootargs buf is too small\n");
	return -1;
    }

    if(!info->phead.init) {
         buf[n-1] = '\0';
         return 0;
    }

    n += sprintf(&buf[n],"%s", info->phead.buf);

    for (i = 0; i < MAX_PRTTN_SIZE; i++) {

        PRTTN_CFG_T *prttn = &info->prttn[i];

        if(!prttn->init) {
	    goto END;
	}

        memset(t,0,32);

	if(__u32tostring(prttn->size, t, 32)) {
            fprintf(stderr,"prttn size 0x%x is not valid\n", prttn->size);
	    return -1;
	}

        n +=sprintf(&buf[n], "%s@", t);

        memset(t, 0, 32);

	if(__u32tostring(prttn->offset, t, 32)) {
            fprintf(stderr,"prttn offset 0x%x is not valid\n", prttn->offset);
	    return -1;
	}

        n += sprintf(&buf[n],"%s(",t);

        n += sprintf(&buf[n],"%s),", prttn->name);

    }

END:
    buf[n-1] = '\0';

    printf("<bootargs>= %s\n",buf);

    return 0;
}



static int __save_boot_param(char *file, char *bootargs, char *bootcmd, BOOT_INFO_T *info)
{
    int i;
    int iret = 0;
    uint8_t sha256_val[32];
    unsigned char *edata = NULL;
    unsigned int elen = 0;
    BOOT_PARAM_T param  = {0};
    BOOT_T *pboot = NULL;

    if(file == NULL || info == NULL) {
        fprintf(stderr, "%s invalid param\n", __func__);
	return -1;
    }

    if(bootargs != NULL && strlen(bootargs)) {
        strncpy(param.args, bootargs, MAX_PARAM_INFO-1);
	printf("args:%s\n", param.args);
    }
    
    if(bootcmd != NULL && strlen(bootcmd)) {
       strncpy(param.cmd, bootcmd, MAX_PARAM_INFO-1); 
       printf("cmd:%s\n", param.cmd);
    }

   for(i = 0; i < MAX_PRTTN_SIZE; i++) {
       PRTTN_CFG_T *prttn = &info->prttn[i];
       if(prttn->init) {
           printf("prttn num:%d\n", i);
           printf("prttn name:%s\n", prttn->name);
           printf("prttn offset:0x%x\n", prttn->offset);
           printf("prttn size:0x%x\n", prttn->size);
	   param.prttn[i].init = 1;
	   param.prttn[i].size = prttn->size;
	   param.prttn[i].offset = prttn->offset;
	   strncpy(param.prttn[i].name, prttn->name, MAX_BUF_LEN-1);
       }
   }

#ifdef TUYA_BOOT_PARAMS_CRYPT_ENABLE

    printf(" ---- crypt support ----\n");
    if(aes128_data_encode((const unsigned char *)&param,
			  (const unsigned int)sizeof(BOOT_PARAM_T),
                           &edata, &elen, (const unsigned char *)TUYA_BOOT_PARAMS_CRYPT_KEY)) {
        fprintf(stderr, "encode fail\n");

        if(edata) {
            free(edata);
	}

	return -1;
    }
#else 
    printf(" ---- crypt unsupport ----\n");
    edata = (unsigned char *)&param;
    elen = (unsigned int) sizeof(BOOT_PARAM_T);
#endif


    if(sha256(edata, elen, sha256_val)) {
        fprintf(stderr, "sha256 fail\n");
	free(edata);
	return -1;
    }

    pboot = (BOOT_T*) malloc(sizeof(BOOT_T)+elen);

    if(pboot == NULL) {
        fprintf(stderr, "malloc fail\n");
#ifdef TUYA_BOOT_PARAMS_CRYPT_ENABLE
        free(edata);
#endif
	return -1;	
    }

    pboot->magic = DEFAULT_MAGIC;
    pboot->len = elen;
    memcpy(pboot->sha256, sha256_val, 32);

    iret |= __save_data(file, (char*)pboot, sizeof(BOOT_T), "w+");
    iret |= __save_data(file, (char*)edata, elen, "a+");

    if(iret) {
        fprintf(stderr, "save %s fail\n", file);
	iret =  -1;
    }

#ifdef TUYA_BOOT_PARAMS_CRYPT_ENABLE
    free(edata);
#endif
    free(pboot);

    return iret;
}

int main(int argc,char *argv[]) 
{
    int c = 0;

    int dump_flag = 0;
    char boot_param_file[MAX_PATH_LEN] = {0};
    char out_info_file[MAX_PATH_LEN] = {0};

    char bootargs[MAX_PARAM_INFO] = {0};

    BOOT_INFO_T *pinfo = &sg_boot_info;

    fprintf(stdout, "%s (compiled %s %s)\n", argv[0], __DATE__, __TIME__);

    memset(pinfo, 0, sizeof(BOOT_INFO_T));

    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF) {
        switch (c) {
        case 'b':
	    memset(boot_param_file, 0, MAX_PATH_LEN);
	    strncpy(boot_param_file, optarg, sizeof(boot_param_file)-1);
	    break;
	case 'o':
	    memset(out_info_file, 0, MAX_PATH_LEN);
	    strncpy(out_info_file, optarg, sizeof(out_info_file)-1);
	    break;
	case 'd':
	    dump_flag = 1;
	    break;
	case 'h':
	default:
	    __usage(argv[0]);
	    exit(0);
	    break;
	}
    }

    if(strlen(boot_param_file) == 0) {
        strcpy(boot_param_file, DEFAULT_PRTTN_INFO_FILE);
    }

    if(strlen(out_info_file) == 0) {
        strcpy(out_info_file, DEFAULT_PACK_INFO_FILE);
    }

    printf("boot_param_file:%s\n", boot_param_file);
    printf("out_info_file:%s\n", out_info_file);
   
    if(dump_flag) {
        __dump_boot_bin(boot_param_file);	    
        return 0;
    }    

    if(__boot_info_init()) {
        fprintf(stderr, "boot_info_init fail\n");
        return -1;	
    }

    if(__create_partition_file(pinfo, out_info_file)) {
        fprintf(stderr, "create partition file fail\n"); 
	return -1;
    }

    if(__get_compose_partition(pinfo)) {
        fprintf(stderr, "get compose partition fail\n");
        return -1;
    }

    if(__create_full_bootargs(pinfo, bootargs, MAX_PARAM_INFO)) {
        fprintf(stderr, "create bootargs fail\n");
	return -1;
    }

    if(__save_boot_param(boot_param_file, bootargs, pinfo->cmd.buf, pinfo)) {
        fprintf(stderr, "save boot param fail\n");
	return -1;
    }

    printf("%s execute succeed\n",argv[0]);

    return 0; 
}
