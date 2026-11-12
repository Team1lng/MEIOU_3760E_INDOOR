#ifndef __BSP_UBOOT_MADE_H__
#define __BSP_UBOOT_MADE_H__

#define KEY1 19
#define KEY2 30
#define KEY3 1
#define KEY4 106
#define KEY5 127
#define KEY6 67
#define KEY7 27
#define KEY8 58
#define KEY9 6
#define KEY10 3
#define KEY11 58
#define KEY12 30
#define KEY13 34
#define KEY14 35
#define KEY15 14
#define KEY16 110
#define NVRAM_FLASH_SIZE 0x10000
#define NVRAM_FLASH_OFFSET 0x00000
#define NVRAM_UBOOT_FLASH_OFFSET 0xfe0000
#define NVRAM_FLASH_PARTITION_SIZE 0x20000

#define UBOOT_USER_LOAD_ADDR 0x80400000
#define UBOOT_OTA_BACKUP_ADDR 0x82000000
#define NVRAM_HEADER_LOAD_ADDR 0x80300000
#define NVRAM_VALUE_LOAD_ADDR 0x80200000
#define NVRAM_COMMIT_ADDR 0x80100000

#define UBOOT_MADE_HW_INIT 0
#define UBOOT_MADE_OTA_BACKUP 1
#define UBOOT_MADE_BOOT_SD 1
#define FLASH_DOUBLE_BACK 0
#define UBOOT_VERSION 2
#define FIRMWARE_STORE_MEDIUM 1
#define UBOOT_MADE_OTA_SD 1
#define UBOOT_MADE_SD_OTA_FW_NAME "sd_ota.bin"
#define NOR_FLASH_PROTECT_SUPPORT 1

typedef int (*UBOOT_MADE_GET_UART_CHAR_T)(void);
extern int register_uboot_made_get_uart_char(UBOOT_MADE_GET_UART_CHAR_T func);

typedef void (*UBOOT_MADE_INIT_T)(void);
extern int register_uboot_made_hw_init(UBOOT_MADE_INIT_T func);

typedef int (*UBOOT_MADE_GET_BUTTON_VALUE_T)(void);

extern int register_uboot_made_get_button_value(UBOOT_MADE_GET_BUTTON_VALUE_T func);

typedef enum {
  E_UPGRADE_STATUS_START,
  E_UPGRADE_STATUS_FINISH,
} UPGRAGE_STATUS_E;

typedef int (*UBOOT_MADE_REFRESH_UPGRADE_STATUS_T)(UPGRAGE_STATUS_E status);

extern int register_uboot_made_refresh_upgrade_status(UBOOT_MADE_REFRESH_UPGRADE_STATUS_T func);

extern void uboot_made_process(void);
extern void uboot_made_cli(void);

#endif //__BSP_UBOOT_MADE_H__


