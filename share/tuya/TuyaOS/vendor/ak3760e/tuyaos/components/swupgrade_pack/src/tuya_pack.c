/*
 * (C) Copyright 2020
 * Cheng Lei, TUYA Software Engineering, chenglei@tuya.com.
 * Tuya pack code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "cJSON.h"
#include "tuya_pack.h"
#include "tuya_verify.h"

int add_sign = 0;

static int json_node_to_string(cJSON *j_root, char *node, char *buf, int buf_len)
{
	cJSON *j_node = NULL;

	if (j_root == NULL || node == NULL || buf == NULL)
		return -1;

	j_node = cJSON_GetObjectItemCaseSensitive(j_root, node);
	if (j_node && cJSON_IsString(j_node) && (j_node->valuestring != NULL))
		strncpy(buf, j_node->valuestring, buf_len);
	else
		return -1;

	return 0;
}

static int json_node_to_int(cJSON *j_root, char *node, int *value)
{
	char buf[32] = {0};

	if (value == NULL)
		return -1;

	if (json_node_to_string(j_root, node, buf, 32) < 0)
		return -1;

	if ((0 == strncmp(buf, "0x", 2)) || (0 == strncmp(buf, "0X", 2)))
		sscanf(buf+2, "%x", value);
	else
		sscanf(buf, "%d", value);

	return 0;
}

static int get_file_size(char *fname)
{
        FILE *fp = NULL;
        int file_size = 0;

	if (fname == NULL)
		return -EINVAL;

	if (strlen(fname) == 0)
		return 0;

        if ((fp = fopen(fname, "rb+")) == NULL) {
                printf("\n[%s]: open file %s failed.\n", __func__, fname);
                return -EINVAL;
        }

        fseek(fp, 0L, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        fclose(fp);

        return file_size;
}

static int parse_cfg_file(char *buf, int buf_len, cfg_fw *cfg)
{
	int i = 0;
	int j = 0;
	cJSON *j_root = NULL;
	cJSON *j_parts = NULL;
	cJSON *j_zone = NULL;
	cJSON *j_files = NULL;
	cJSON *j_node = NULL;

	if (buf == NULL || cfg == NULL)
		return -1;

	memset(cfg, 0, sizeof(cfg_fw));

	j_root = cJSON_Parse(buf);
	if (j_root == NULL)
		return -1;

 	do {
		if (json_node_to_string(j_root, "product_name", cfg->fw_name, 32) < 0)
			return -1;
		if (json_node_to_string(j_root, "flash_type", cfg->flash_type, 32) < 0)
			return -1;
		if (json_node_to_int(j_root, "flash_size", &cfg->flash_size) < 0)
			return -1;
		if (json_node_to_int(j_root, "pack_ota_only", &cfg->pack_ota_only) < 0)
			return -1;
		if (json_node_to_string(j_root, "ota_mode", cfg->ota_mode, 32) < 0)
			return -1;
		/* parse partitions */
		j_parts = cJSON_GetObjectItemCaseSensitive(j_root, "partitions");
		if (j_parts == NULL)
			return -1;
		cJSON_ArrayForEach(j_zone, j_parts) {
			if (json_node_to_string(j_zone, "name", cfg->zone[i].name, 32) < 0)
				return -1;
			if (json_node_to_string(j_zone, "version", cfg->zone[i].version, 32) < 0)
				return -1;
			if (json_node_to_int(j_zone, "offset", &cfg->zone[i].offset) < 0)
				return -1;
			if (json_node_to_int(j_zone, "size", &cfg->zone[i].size) < 0)
				return -1;
			if (cfg->zone[i].size < get_file_size(cfg->zone[i].filename)) {
				printf("%s: zone %s size smaller than file %s size\n", __func__,
						cfg->zone[i].name, cfg->zone[i].filename);
				return -1;
			}

			cJSON *dev_ary = cJSON_GetObjectItem(j_zone, "device");
			if (dev_ary != NULL) {
				int ary_size = 2;//cJSON_GetArraySize(dev_ary);
				for (j = 0; j < ary_size; j++) {
					cJSON *psub = cJSON_GetArrayItem(dev_ary, j);
					if (NULL == psub) {
						continue;
					}
					memcpy(cfg->zone[i].device[j], psub->valuestring, 32);
				}
			} else {
				return -1;
			}

			if (json_node_to_int(j_zone, "device_offset", &cfg->zone[i].device_offset) < 0)
				return -1;
			if (json_node_to_string(j_zone, "filename", cfg->zone[i].filename, 32) < 0)
				return -1;
			if (json_node_to_string(j_zone, "filesystem", cfg->zone[i].filesystem, 32) < 0)
				return -1;
			if (json_node_to_int(j_zone, "need_ota", &cfg->zone[i].need_ota) < 0)
				return -1;
			/* optional */
			json_node_to_int(j_zone, "copy_in_boot", &cfg->zone[i].copy_in_boot);
			i++;
		}
		cfg->zone_num = i;

		/* parse files */
		i = 0;
		j_files = cJSON_GetObjectItemCaseSensitive(j_root, "files");
		if (j_files == NULL)
			break;
		cJSON_ArrayForEach(j_node, j_files) {
			if (json_node_to_string(j_node, "name", cfg->file[i].name, 32) < 0)
				return -1;
			if (json_node_to_string(j_node, "version", cfg->file[i].version, 32) < 0)
				return -1;
			if (json_node_to_string(j_node, "storage_path", cfg->file[i].storage_path, 128) < 0)
				return -1;
			if (json_node_to_string(j_node, "installer", cfg->file[i].installer, 128) < 0)
				return -1;
			if (json_node_to_string(j_node, "filename", cfg->file[i].filename, 128) < 0)
				return -1;
			if (json_node_to_int(j_node, "need_ota", &cfg->file[i].need_ota) < 0)
				return -1;
			i++;
		}
		cfg->file_num = i;
	} while (0);

	return 0;
}

static int load_cfg_file(const char *fname, cfg_fw *cfg)
{
	int ret = -1;
	FILE *fp = NULL;
	int file_len = 0;
	char *buf = NULL;
	int buf_len = 0;

	do {
		fp = fopen(fname, "rb");
		if (fp == NULL)
			break;
		if (fseek(fp, 0, SEEK_END) != 0)
			break;
		file_len = ftell(fp);
		if (file_len <= 0)
			break;
		if (fseek(fp, 0, SEEK_SET) != 0)
			break;

		buf = (char*)malloc(file_len + 1);
		if (buf == NULL)
			break;
		memset(buf, 0, file_len + 1);

		buf_len = fread(buf, 1, file_len, fp);
		if (buf_len != file_len)
			break;

		ret = parse_cfg_file(buf, buf_len, cfg);
	} while (0);

	if (fp)
		fclose(fp);
	if (buf)
		free(buf);

	return ret;
}

static int construct_ota_head(cfg_fw *cfg, ota_head *head)
{
	int i = 0, j = 0;

	memset(head, 0, sizeof(ota_head));
	strncpy(head->magic, "TUYA_OTA", 32);
	strncpy(head->flash_type, cfg->flash_type, 32);
	strncpy(head->ota_mode, cfg->ota_mode, 32);

	for (i = 0; i < MAX_CFG_ZONE; i++) {
		if (cfg->zone[i].need_ota == 0 && strcmp(cfg->zone[i].name, "backup"))
			continue;
		strncpy(head->zone[j].name, cfg->zone[i].name, 32);
		strncpy(head->zone[j].version, cfg->zone[i].version, 32);
		head->zone[j].offset = cfg->zone[i].offset;
		strncpy(head->zone[j].filesystem, cfg->zone[i].filesystem, 32);
		strncpy(head->zone[j].device[0], cfg->zone[i].device[0], 32);
		strncpy(head->zone[j].device[1], cfg->zone[i].device[1], 32);
		head->zone[j].device_offset = cfg->zone[i].device_offset;
		head->zone[j].image_size = get_file_size(cfg->zone[i].filename);
		if (head->zone[j].image_size == 0)
			;
		else if (tuya_sha256sum_file(cfg->zone[i].filename, (unsigned char *)head->zone[j].sha256) < 0)
			return -1;
		head->zone[j].copy_in_boot = cfg->zone[i].copy_in_boot;
		j++;
	}

	head->zone_num = j;

	j = 0;
	for (i = 0; i < MAX_CFG_FILE; i++) {
		if (cfg->file[i].need_ota == 0)
			continue;
		strncpy(head->file[j].name, cfg->file[i].name, 32);
		strncpy(head->file[j].version, cfg->file[i].version, 32);
		strncpy(head->file[j].storage_path, cfg->file[i].storage_path, 128);
		strncpy(head->file[j].installer, cfg->file[i].installer, 128);
		head->file[j].image_size = get_file_size(cfg->file[i].filename);
		if (head->file[j].image_size == 0)
			;
		else if (tuya_sha256sum_file(cfg->file[i].filename, (unsigned char *)head->file[j].sha256) < 0)
			return -1;
		j++;
	}

	head->file_num = j;

	return 0;
}


static int construct_sd_ota_head(cfg_fw *cfg, ota_head *head)
{
	int i = 0, j = 0;

	memset(head, 0, sizeof(ota_head));
	strncpy(head->magic, "TUYA_OTA", 32);
	strncpy(head->flash_type, cfg->flash_type, 32);
	strncpy(head->ota_mode, cfg->ota_mode, 32);

	for (i = 0; i < MAX_CFG_ZONE; i++) {
		if (cfg->zone[i].filename == 0 || strlen(cfg->zone[i].filename) == 0 ||
			strcmp(cfg->zone[i].name, "factory") == 0) {
			continue;
		}
		strncpy(head->zone[j].name, cfg->zone[i].name, 32);
		strncpy(head->zone[j].version, cfg->zone[i].version, 32);
		head->zone[j].offset = cfg->zone[i].offset;
		strncpy(head->zone[j].filesystem, cfg->zone[i].filesystem, 32);
		strncpy(head->zone[j].device[0], cfg->zone[i].device[0], 32);
		strncpy(head->zone[j].device[1], cfg->zone[i].device[1], 32);
		head->zone[j].device_offset = cfg->zone[i].device_offset;
		head->zone[j].image_size = get_file_size(cfg->zone[i].filename);

		if (head->zone[j].image_size == 0)
			;
		else if (tuya_sha256sum_file(cfg->zone[i].filename, (unsigned char *)head->zone[j].sha256) < 0)
			return -1;
		head->zone[j].copy_in_boot = cfg->zone[i].copy_in_boot;
		j++;
	}

	head->zone_num = j;

	return 0;
}


static int load_file(char *fname, char *buf, int buf_len)
{
	FILE *fp = NULL;
	int ret = -1;

	if (fname == NULL || buf == NULL || buf_len <= 0)
		return -1;

	if ((fp = fopen(fname, "rb+")) == NULL) {
		printf("%s: open file %s failed.\n", __func__, fname);
		return -EINVAL;
	}

	ret = fread(buf, 1, buf_len, fp);
	if (ret < 0)
		return -1;

	fclose(fp);

	return ret;
}

static int tuya_pack_zone_OTA(FILE *fw_fp, char *img_name, int zone_size)
{
	int ret = -1;
	int img_size = 0;
	char *buf = NULL;

	if ((img_size = get_file_size(img_name)) < 0)
		return -1;

	if (img_size > zone_size)
		return -1;

	if (img_size == 0)
		return 0;

	buf = (char*)malloc(img_size);
	if (buf == NULL)
		return -ENOMEM;

	ret = load_file(img_name, buf, img_size);
	if (ret != img_size) {
		free(buf);
		return -1;
	}

	ret = fwrite(buf, 1, img_size, fw_fp);
	if (ret != img_size) {
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}

static int tuya_pack_file_OTA(FILE *fw_fp, char *img_name, int zone_size)
{
	return tuya_pack_zone_OTA(fw_fp, img_name, zone_size);
}

static void sort_zone_if_needed(cfg_fw *cfg)
{
	int i;
	cfg_zone tmp_zone;
	int idx = 0;

	if (strncmp(cfg->ota_mode, "single-copy", 32))
		return ;

	for (i = 0; i < cfg->zone_num; i++) {
		if (cfg->zone[i].copy_in_boot) {
			idx = i;
			break;
		}
	}

	i = cfg->zone_num - 1;
	if (idx == i)
		return ;

	memcpy(&tmp_zone, &cfg->zone[i], sizeof(cfg_zone));
	memcpy(&cfg->zone[i], &cfg->zone[idx], sizeof(cfg_zone));
	memcpy(&cfg->zone[idx], &tmp_zone, sizeof(cfg_zone));
}

static int pack_ota_firmware(cfg_fw *fwcfg)
{
	int ret = -1;
	int i = 0;
	int j = 0;
	char fw_fname[64] = {0};
	FILE *fw_fp = NULL;
	ota_head *phead = NULL;
	ota_sign sign;
	cfg_fw *cfg = NULL;

	memset(&sign,0,sizeof(ota_sign));

	do {
		cfg = (cfg_fw *)calloc(1, sizeof(cfg_fw));
		if (cfg == NULL) {
			ret = -ENOMEM;
			break;
		}

		memcpy(cfg, fwcfg, sizeof(cfg_fw));
		sort_zone_if_needed(cfg);

		phead = (ota_head *)calloc(1, OTA_HEAD_SIZE);
		if (phead == NULL) {
			ret = -ENOMEM;
			break;
		}

		if (construct_ota_head(cfg, phead) < 0)
			break;

		if (add_sign == CLOUD_SIGNED) {
			if (tuya_sign_data((unsigned char*)phead, OTA_HEAD_SIZE, &sign) < 0)
				break;
		} else if (add_sign == LOCAL_SIGNED) {
			if (tuya_sign_data_local((unsigned char*)phead, OTA_HEAD_SIZE, &sign) < 0)
				break;
		}

		snprintf(fw_fname, 64, "%s%s", cfg->fw_name, "_ota.img");
		fw_fp = fopen(fw_fname, "wb");
		if (fw_fp == NULL)
			break;

		if (OTA_HEAD_SIZE != fwrite(phead, 1, OTA_HEAD_SIZE, fw_fp)) {
			printf("%s fwrite failed.\n", __func__);
			break;
		}

		i = sizeof(ota_sign);
		if (i != fwrite(&sign, 1, i, fw_fp)) {
			printf("%s fwrite failed.\n", __func__);
			break;
		}

		for (i = 0; i < MAX_CFG_ZONE; i++) {
			if (cfg->zone[i].need_ota == 0 && strcmp(cfg->zone[i].name, "backup"))
				continue;
			if (tuya_pack_zone_OTA(fw_fp, cfg->zone[i].filename, cfg->zone[i].size) < 0)
				break;
		}

		for (j = 0; j < MAX_CFG_FILE; j++) {
			if (cfg->file[j].need_ota == 0)
				continue;
			if (tuya_pack_file_OTA(fw_fp, cfg->file[j].filename, cfg->zone[j].size) < 0)
				break;
		}

		if (i == MAX_CFG_ZONE && j == MAX_CFG_FILE) {
			ret = 0;
			printf("\n[OTA] firmware pack OK.\n");
		}
        else {
			ret = -1;
            printf("\n[OTA] firmware pack fail, i=%d[%s] j=%d MAX_CFG_ZONE=%d MAX_CFG_FILE=%d\n", i, cfg->zone[i].filename, j, MAX_CFG_ZONE, MAX_CFG_FILE);
        }
	} while (0);

	if (fw_fp)
		fclose(fw_fp);
	if (phead)
		free(phead);
	if (cfg)
		free(cfg);
	return ret;
}


static int pack_sd_ota_firmware(cfg_fw *fwcfg)
{
	int ret = -1;
	int i = 0;
	int j = 0;
	char fw_fname[64] = {0};
	FILE *fw_fp = NULL;
	ota_head *phead = NULL;
	ota_sign sign;
	cfg_fw *cfg = NULL;

	memset(&sign,0,sizeof(ota_sign));

	do {
		cfg = (cfg_fw *)calloc(1, sizeof(cfg_fw));
		if (cfg == NULL) {
			ret = -ENOMEM;
			break;
		}

		memcpy(cfg, fwcfg, sizeof(cfg_fw));
		sort_zone_if_needed(cfg);

		phead = (ota_head *)calloc(1, OTA_HEAD_SIZE);
		if (phead == NULL) {
			ret = -ENOMEM;
			break;
		}

		if (construct_sd_ota_head(cfg, phead) < 0)
			break;

		if (add_sign == CLOUD_SIGNED) {
			if (tuya_sign_data((unsigned char*)phead, OTA_HEAD_SIZE, &sign) < 0)
				break;
		} else if (add_sign == LOCAL_SIGNED) {
			if (tuya_sign_data_local((unsigned char*)phead, OTA_HEAD_SIZE, &sign) < 0)
				break;
		}

		snprintf(fw_fname, 64, "%s%s", cfg->fw_name, "_sd_ota.img");
		fw_fp = fopen(fw_fname, "wb");
		if (fw_fp == NULL)
			break;

		if (OTA_HEAD_SIZE != fwrite(phead, 1, OTA_HEAD_SIZE, fw_fp)) {
			printf("%s fwrite failed.\n", __func__);
			break;
		}

		i = sizeof(ota_sign);
		if (i != fwrite(&sign, 1, i, fw_fp)) {
			printf("%s fwrite failed.\n", __func__);
			break;
		}

		for (i = 0; i < MAX_CFG_ZONE; i++) {
			if (strcmp(cfg->zone[i].name, "factory") == 0 ||
				cfg->zone[i].name == NULL || strlen(cfg->zone[i].name) == 0) {
				continue;
			 }

			if (tuya_pack_zone_OTA(fw_fp, cfg->zone[i].filename, cfg->zone[i].size) < 0)
				break;

		}

		if (i == MAX_CFG_ZONE) {
		    ret = 0;
		    printf("\n[OTA] firmware pack OK.\n");
		}else {
		    ret = -1;
                    printf("\n[OTA] firmware pack fail, i=%d[%s] j=%d MAX_CFG_ZONE=%d\n",
				            i, cfg->zone[i].filename, j, MAX_CFG_ZONE);
               }
	} while (0);

	if (fw_fp)
		fclose(fw_fp);
	if (phead)
		free(phead);
	if (cfg)
		free(cfg);
	return ret;
}

static int tuya_pack_zone_ALL(FILE *fw_fp, char *img_name, int zone_size)
{
	int ret = -1;
	char *buf = NULL;
	int img_size = 0;

	if (fw_fp == NULL || img_name == NULL || zone_size <= 0)
		return -1;

	img_size = get_file_size(img_name);
	if (img_size < 0 || img_size > zone_size)
		return -1;

	buf = (char*)calloc(1, zone_size);
	if (buf == NULL)
		return -ENOMEM;

	if (img_size > 0) {
		ret = load_file(img_name, buf, img_size);
		if (ret != img_size) {
			free(buf);
			return -1;
		}
	} else {
	    memset(buf, 0xff, zone_size);
	}

	if (zone_size != fwrite(buf, 1, zone_size, fw_fp)) {
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}

static int pack_all_firmware(cfg_fw *cfg)
{
	int i = 0;
	char fw_fname[64] = {0};
	FILE *fw_fp = NULL;
	int fw_size = 0;

	snprintf(fw_fname, 64, "%s%s", cfg->fw_name, "_all.img");
	fw_fp = fopen(fw_fname, "wb");
	if (fw_fp == NULL)
		return -1;

	for (i = 0; i < MAX_CFG_ZONE; i++) {
		if (tuya_pack_zone_ALL(fw_fp, cfg->zone[i].filename, cfg->zone[i].size) < 0)
			break;
	}
	fseek(fw_fp, 0L, SEEK_END);
	fw_size = ftell(fw_fp);
	if (fw_size != cfg->flash_size) {
		printf("\nfw_size(%d) != flash_size(%d)\n", fw_size, cfg->flash_size);
		fclose(fw_fp);
		return -1;
	}

    printf("\n[ALL] firmware pack OK.\n");
	fclose(fw_fp);
	return 0;
}

int tuya_upgrade_pack(char *cfg_fname)
{
	int ret = 0;
	cfg_fw cfg;

	memset(&cfg, 0, sizeof(cfg_fw));

	ret = load_cfg_file(cfg_fname, &cfg);
	if (ret < 0) {
		printf("parse cfg file failed.\n");
		return -EINVAL;
	}

	ret = pack_ota_firmware(&cfg);
	if (ret < 0) {
		printf("pack ota firmware failed.\n");
		return -EINVAL;
	}

	if(cfg.pack_ota_only == 2) {

            ret =  pack_sd_ota_firmware(&cfg);
	    if(ret < 0 ) {
	        printf("pack sd ota firmware fail.\n");
		return -EINVAL;
	    }
	}

	if (cfg.pack_ota_only == 1)
		return 0;

	ret = pack_all_firmware(&cfg);
	if (ret < 0) {
		printf("pack ota firmware failed.\n");
		return -EINVAL;
	}

	return 0;
}

int tuya_upgrade_dump(char *fw_fname)
{
	int i = 0;
	int j = 0;
	FILE *fp = NULL;
	char buf[OTA_HEAD_SIZE] = {0};
	int buf_len = 0;
	ota_head *phead = NULL;
	ota_sign zone_sign;
	int sign_len = sizeof(ota_sign);

	if ((fp = fopen(fw_fname, "rb+")) == NULL) {
		printf("%s: open file %s failed.\n", __func__, fw_fname);
		return -EINVAL;
	}

	buf_len = fread(buf, 1, OTA_HEAD_SIZE, fp);
	if (buf_len != OTA_HEAD_SIZE) {
		fclose(fp);
		return -1;
	}

	if (fread(&zone_sign, 1, sign_len, fp) != sign_len) {
		fclose(fp);
		return -1;
	}

	phead = (ota_head *)buf;

	printf("magic: %s\n", phead->magic);
	printf("flash_type: %s\n", phead->flash_type);
	printf("ota mode: %s\n", phead->ota_mode);
	printf("zone_num: %d\n", phead->zone_num);
	for (i = 0; i < phead->zone_num; i++) {
		printf("\tname: %s\n", phead->zone[i].name);
		printf("\tversion: %s\n", phead->zone[i].version);
		printf("\toffset: 0x%x\n", phead->zone[i].offset);
		printf("\timage_size: %d\n", phead->zone[i].image_size);
		printf("\tdevice[0]: %s\n", phead->zone[i].device[0]);
		printf("\tdevice[1]: %s\n", phead->zone[i].device[1]);
		printf("\tdevice_offset: 0x%x\n", phead->zone[i].device_offset);
		printf("\tfilesystem: %s\n", phead->zone[i].filesystem);
		printf("\tcopy_in_boot: %d\n", phead->zone[i].copy_in_boot);
		printf("\thash: ");
		for (j = 0; j < 32; j++)
			printf("%02hhx", phead->zone[i].sha256[j]);
		printf("\n");
		printf("\t-----------------------------\n");
	}

	printf("file_num: %d\n", phead->file_num);
	for (i = 0; i < phead->file_num; i++) {
		printf("\tname: %s\n", phead->file[i].name);
		printf("\tversion: %s\n", phead->file[i].version);
		printf("\timage_size: %d\n", phead->file[i].image_size);
		printf("\tstorage_path: %s\n", phead->file[i].storage_path);
		printf("\tinstaller: %s\n", phead->file[i].installer);
		printf("\thash: ");
		for (j = 0; j < 32; j++)
			printf("%02hhx", phead->file[i].sha256[j]);
		printf("\n");
		printf("\t-----------------------------\n");
	}

	fclose(fp);
	return 0;
}

static int save_data_to_file(char *fname, char *buf, int buf_len)
{
	FILE *fp = NULL;

	if (fname == NULL || buf == NULL || buf_len == 0)
		return -1;

	fp = fopen(fname, "wb");
	if (fp == NULL) {
		printf("%s: open file failed.\n", __func__);
		return -1;
	}

	if (buf_len != fwrite(buf, 1, buf_len, fp)) {
		printf("%s: write file failed.\n", __func__);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

static void format_sign(unsigned char *data, int len, unsigned char* sign_data)
{
	int i;
	unsigned char sign1, sign2;
	unsigned char sign[160] = {0};
	int s_index;

	for (i = 0; i < len; i = i + 2) {
		if ((*(data + i) >= '0') && (*(data + i) <= '9'))
			sign1 = *(data + i) - '0';
		if ((*(data + i) >= 'a') && (*(data + i) <= 'f'))
			sign1 = *(data + i) - 'a' + 10;
		if ((*(data + i + 1) >= '0') && (*(data + i + 1) <= '9'))
			sign2 = *(data + i + 1) - '0';
		if ((*(data + i + 1) >= 'a') && (*(data + i + 1) <= 'f'))
			sign2 = *(data + i + 1) - 'a' + 10;
		sign[i/2] = sign1 * 16 + sign2;
	}
	//sign_len = sign_len/2;

	/* r len */
	if (sign[3] == 0x21) {
		memcpy(sign_data, &(sign[5]), 32);
		s_index = 38;
	} else {
		memcpy(sign_data, &(sign[4]), 32);
		s_index = 37;
	}

	/* s len */
	if (sign[s_index] == 0x21)
		memcpy(&(sign_data[32]), &(sign[s_index+2]), 32);
	else
		memcpy(&(sign_data[32]), &(sign[s_index+1]), 32);
}

static int tuya_verify_head(char *buf, int buf_len, char *sign, int sign_len)
{
	if (add_sign == NOT_SIGNED)
		return 0;

	ota_sign *psign = NULL;
	unsigned char sign_hex[64] = {0};

	psign = (ota_sign *)sign;

	if (psign->len < 0 || psign->len > TUYA_SIGN_SIZE) {
		printf("%s: invalid sign data.\n", __func__);
		return -EINVAL;
	}

	format_sign(psign->data, psign->len, sign_hex);
	if (tuya_verify_data((unsigned char *)buf, buf_len, sign_hex) < 0) {
		printf("%s: verify failed.\n", __func__);
		return -1;
	}

	return 0;
}

static int tuya_verify_zone(char *buf, int buf_len, unsigned char sum[32])
{
	if (add_sign == NOT_SIGNED)
		return 0;

	unsigned char tmp[32] = {0};

	sha256sum((unsigned char *)buf, buf_len, tmp);

	if (memcmp(sum, tmp, 32) == 0)
		return 0;
	else
		return -1;
}

static int tuya_check_file(char *buf, int buf_len, unsigned char sum[32])
{
	return tuya_verify_zone(buf, buf_len, sum);
}

int tuya_upgrade_extract(char *fw_fname)
{
	int i = 0;
	FILE *fp = NULL;
	ota_head *phead;
	char head_buf[OTA_HEAD_SIZE] = {0};
	int head_buf_len = 0;
	ota_sign zone_sign;
	int sign_len = sizeof(ota_sign);
	char *zone_buf = NULL;
	int zone_buf_len = 0;
	char *file_buf = NULL;
	int file_buf_len = 0;
	char img_name[64] = {0};

	if ((fp = fopen(fw_fname, "rb+")) == NULL) {
		printf("%s: open file %s failed.\n", __func__, fw_fname);
		return -EINVAL;
	}

	head_buf_len = fread(head_buf, 1, OTA_HEAD_SIZE, fp);
	if (head_buf_len != OTA_HEAD_SIZE) {
		fclose(fp);
		return -1;
	}

	if (fread(&zone_sign, 1, sign_len, fp) != sign_len) {
		fclose(fp);
		return -1;
	}

	phead = (ota_head *)head_buf;
	if (tuya_verify_head(head_buf, OTA_HEAD_SIZE, (char*)&zone_sign, sign_len) < 0) {
		fclose(fp);
		return -1;
	}

	for (i = 0; i < phead->zone_num; i++) {
		if (phead->zone[i].image_size == 0)
			continue;
		zone_buf = (char*)malloc(phead->zone[i].image_size);
		if (zone_buf == NULL) {
			fclose(fp);
			return -ENOMEM;
		}
		zone_buf_len = fread(zone_buf, 1, phead->zone[i].image_size, fp);
		if (zone_buf_len != phead->zone[i].image_size) {
			printf("%s: read file failed, ret %d\n", __func__, zone_buf_len);
			fclose(fp);
			free(zone_buf);
			return -1;
		}

		if (tuya_verify_zone(zone_buf, zone_buf_len, (unsigned char *)phead->zone[i].sha256) < 0) {
			printf("verify zone failed.\n");
			fclose(fp);
			free(zone_buf);
			return -1;
		}

		snprintf(img_name, 64, "%s-0x%x.bin", phead->zone[i].name, phead->zone[i].device_offset);
		if (save_data_to_file(img_name, zone_buf, zone_buf_len) < 0) {
			fclose(fp);
			free(zone_buf);
			return -1;
		}

		free(zone_buf);
		zone_buf = NULL;
	}

	for (i = 0; i < phead->file_num; i++) {
		if (phead->file[i].image_size == 0)
			continue;
		file_buf = (char*)malloc(phead->file[i].image_size);
		if (file_buf == NULL) {
			fclose(fp);
			return -ENOMEM;
		}
		file_buf_len = fread(file_buf, 1, phead->file[i].image_size, fp);
		if (file_buf_len != phead->file[i].image_size) {
			printf("%s: read file failed, ret %d\n", __func__, file_buf_len);
			fclose(fp);
			free(file_buf);
			return -1;
		}

		if (tuya_check_file(file_buf, file_buf_len, (unsigned char *)phead->file[i].sha256) < 0) {
			printf("verify file failed.\n");
			fclose(fp);
			free(file_buf);
			return -1;
		}

		if (save_data_to_file(phead->file[i].name, file_buf, file_buf_len) < 0) {
			fclose(fp);
			free(file_buf);
			return -1;
		}

		free(file_buf);
		file_buf = NULL;
	}

	free(fp);
	return 0;
}

int tuya_upgrade_sign_file(char *file_name, char *sign_name)
{
	ota_sign sign;

	if (file_name == NULL)
		return -1;

	memset(&sign, 0, sizeof(sign));
	if (tuya_sign_file(file_name, &sign) < 0)
		return -1;

	printf("sign length:%d\n", sign.len);
	printf("sign data:%s\n", sign.data);

	return sign_name?save_data_to_file(sign_name, (char *)sign.data, sign.len):0;
}

int tuya_upgrade_load_key(char *file_name)
{
	// TODO:
	return 0;
}
