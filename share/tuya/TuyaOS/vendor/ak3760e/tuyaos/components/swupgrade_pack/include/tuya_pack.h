/*
 * (C) Copyright 2020
 * Cheng Lei, Tuya Software Engineering, chenglei@tuya.com.
 */

#ifndef _TUYA_PACK_H
#define _TUYA_PACK_H

#include "tuya_sign.h"

#ifdef BSP_PACK_PART_CNT
#define MAX_OTA_ZONE		BSP_PACK_PART_CNT
#define MAX_CFG_ZONE		BSP_PACK_PART_CNT
#define OTA_HEAD_SIZE		512*BSP_PACK_PART_CNT
#else
#define MAX_OTA_ZONE		10
#define MAX_CFG_ZONE		10
#define OTA_HEAD_SIZE		4096
#endif
#define MAX_OTA_FILE		2
#define MAX_CFG_FILE		2
#define HASH_LENGTH      	32

#define NOT_SIGNED			(0)
#define CLOUD_SIGNED		(1)
#define LOCAL_SIGNED		(2)

typedef struct ota_zone {
	char name[32];
	char version[32];
	int offset;
	int image_size;
	char device[2][32];
	int device_offset;
	char filesystem[32];
	int copy_in_boot;
	char sha256[HASH_LENGTH];
}__attribute__ ((packed)) ota_zone;

typedef struct ota_file {
	char name[32];
	char version[32];
	int image_size;
	char storage_path[128];
	char installer[128];
	char sha256[HASH_LENGTH];
}__attribute__ ((packed)) ota_file;

typedef struct ota_head {
	char magic[32]; /* "TUYA_OTA" */
	char flash_type[32];
	char ota_mode[32]; /* single-copy double-copy recovery */
	int zone_num;
	ota_zone zone[MAX_OTA_ZONE];
	int file_num;
	ota_file file[MAX_OTA_FILE];
}__attribute__ ((packed)) ota_head;

typedef struct cfg_zone {
	char name[32];
	char version[32];
	int offset;
	int size;
	char device[2][32];
	int device_offset;
	char filename[128];
	char filesystem[32];
	int need_ota;
	int copy_in_boot;
}__attribute__ ((packed)) cfg_zone;

typedef struct cfg_file {
	char name[32];
	char version[32];
	char filename[128];
	char storage_path[128];
	char installer[128];
	int need_ota;
}__attribute__ ((packed)) cfg_file;

typedef struct cfg_fw {
	char fw_name[32];
	int flash_size;
	int pack_ota_only;
	char flash_type[32];
	char ota_mode[32];
	int zone_num;
	cfg_zone zone[MAX_CFG_ZONE];
	int file_num;
	cfg_file file[MAX_CFG_FILE];
}__attribute__ ((packed)) cfg_fw;

int tuya_upgrade_pack(char *cfg_fname);

int tuya_upgrade_dump(char *fw_fname);

int tuya_upgrade_extract(char *fw_fname);

int tuya_upgrade_sign_file(char *file_name, char *sign_name);

int tuya_upgrade_load_key(char *file_name);

#endif
