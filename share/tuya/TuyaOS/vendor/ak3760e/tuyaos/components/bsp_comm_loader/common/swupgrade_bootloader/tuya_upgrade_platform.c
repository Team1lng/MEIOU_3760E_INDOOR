/*
 * (C) Copyright 2020
 * Cheng Lei, Tuya Software Engineering, chenglei@tuya.com.
 */

#include <common.h>
#include <fat.h>
#include <fs.h>
#include "tuya_upgrade_platform.h"
#include <uboot_made/uboot_made.h>
#include <nvram/bcmnvram.h>
#include <spi_flash.h>


extern int nvram_get(const char *name, char *value);
extern int nvram_set(const char *name, const char *value, int attr);
extern int dev_nvram_init(void);
#if FIRMWARE_STORE_MEDIUM == 1
extern int spinor_flash_write(char *buf, unsigned long offset, unsigned long len);
extern int spinor_flash_read(char *buf, unsigned long offset, unsigned long *len);
#elif FIRMWARE_STORE_MEDIUM == 2
extern int spinand_flash_read(char *buf, unsigned long offset, unsigned long *len);
extern int spinand_flash_write(char *buf, unsigned long offset, unsigned long len);
extern int spinand_flash_isbad(int dev, unsigned long off);
#endif
extern int get_mtd_partition_size(char *partname, int partnamelen);

extern int fs_set_blk_dev(const char *ifname, const char *dev_part_str, int fstype);

#define FS_TYPE_FAT 1

int ty_prop_get(const char *key, char *value)
{
	if (!key || !value)
		return -1;

	return nvram_get(key, value);
}

int ty_prop_set(const char *key, const char *value)
{
	int ret = 0;

	if (!key || !value)
		return -1;

	ret = nvram_set(key, value, ATTR_PERSIST);

	return ret;
}

int ty_flash_read(int offs, int len, char **buf)
{
	*buf = (char*)UBOOT_USER_LOAD_ADDR;
#if FIRMWARE_STORE_MEDIUM == 1
	return spinor_flash_read(*buf, offs, (unsigned long *)&len);
#elif FIRMWARE_STORE_MEDIUM == 2
	return spinand_flash_read(*buf, offs, (unsigned long *)&len);
#endif
}

int ty_flash_write(char *buf, int len, int offs)
{
#if FIRMWARE_STORE_MEDIUM == 1
	return spinor_flash_write(buf, offs, len);
#elif FIRMWARE_STORE_MEDIUM == 2
	return spinand_flash_write(buf, offs, len);
#endif
}

int load_fw_from_sd(void)
{
	long sz = 0;

	char *filename = UBOOT_MADE_SD_OTA_FW_NAME;

	if (fs_set_blk_dev("mmc", "0:1", FS_TYPE_FAT))
		if (fs_set_blk_dev("mmc", "0:0", FS_TYPE_FAT))
			if (fs_set_blk_dev("mmc", "0:4", FS_TYPE_FAT)) {
				printf("failed to read sd card\n");
				return -1;
			}

        sz = file_fat_read(filename, (void *)UBOOT_USER_LOAD_ADDR, 0);


	if (sz <= 0) {
		printf("error to read the file, sz: %ld\n", sz);
		return -1;
	}
	printf("fatload %ld(0x%lx) bytes\n",sz,sz);

	return sz;
}

static UBOOT_MADE_GET_BUTTON_VALUE_T uboot_made_get_button_value = NULL;

int register_uboot_made_get_button_value(UBOOT_MADE_GET_BUTTON_VALUE_T func)
{
    if (func) {
         uboot_made_get_button_value = func;
    }

    return 0;
}

int check_update_status(void)
{
    int iret = 0;

    if (uboot_made_get_button_value) {
         iret = uboot_made_get_button_value();
    }

    return iret;
}

static UBOOT_MADE_REFRESH_UPGRADE_STATUS_T uboot_made_refresh_upgrade_status = NULL;

int register_uboot_made_refresh_upgrade_status(UBOOT_MADE_REFRESH_UPGRADE_STATUS_T func)
{
    if (func) {
        uboot_made_refresh_upgrade_status = func;
    }

    return 0;
}

int ty_upgrade_progress(int status)
{
	printf("upgrade status: %d\n", status);

	if (uboot_made_refresh_upgrade_status) {
              uboot_made_refresh_upgrade_status((UPGRAGE_STATUS_E)status);
	}

	return 0;
}
