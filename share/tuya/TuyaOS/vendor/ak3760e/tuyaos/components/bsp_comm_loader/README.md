# bsp_uboot_made
#for tuyaos uboot costum

step1
将组件下的文件拷贝到uboot源码中
cp bsp_uboot_made/include/* uboot/include/ -rf
cp bsp_uboot_made/common/* uboot/common/ -rf


step2
修改common目录下的Makefile和common/main.c函数
1.在common/Makefile最后加上如下：
obj-y +=ecdsa/asn1parse.o
obj-y +=ecdsa/bignum.o
obj-y +=ecdsa/ecdsa_verify.o
obj-y +=ecdsa/ecp.o
obj-y +=ecdsa/ecp_curves.o
obj-y +=ecdsa/platform_util.o

obj-y += bsp_nvram/aes.o
obj-y += bsp_nvram/uboot_flash.o
obj-y += bsp_nvram/uboot_nvram.o
obj-y += bsp_nvram/uni_base64.o

obj-y += swupgrade_bootloader/tuya_upgrade_adaptation.o
obj-y += swupgrade_bootloader/tuya_upgrade_platform.o
obj-y += swupgrade_bootloader/bsp_verify/ecdsa.o
obj-y += swupgrade_bootloader/bsp_verify/sha256.o
obj-y += swupgrade_bootloader/bsp_verify/tuya_verify.o

obj-y += uboot_made.o
obj-y += flash_adapter.o
obj-y += auto_update_adaptation.o

2.在common/main.c函数添加入口函数
由于UBOOT源码版本的迭代，main.c函数存在较大的差异，我们根据版本号添加入口函数。

#if ubootversion >= uboot-2015.01
--- a/u-boot-2015.01/common/main.c
+++ b/u-boot-2015.01/common/main.c
@@ -13,6 +13,8 @@
 #include <version.h>
 #include <SsRecovery.h>

+#include "uboot_made.h"
+
 DECLARE_GLOBAL_DATA_PTR;

 /*
@@ -101,7 +103,13 @@ extern int update_dfu(void);
        if (cli_process_fdt(&s))
                cli_secure_boot_cmd(s);

+        // add by uboot_made
+        uboot_made_process_delay();
+
        autoboot_command(s);

+        // add by uboot_made
+        uboot_made_cli();
+
        cli_loop();
 }

#else if ubootversion >= uboot-2010.06
+#include "uboot_made.h"

void main_loop(void)
{
#ifdef CONFIG_BOOTDELAY  // #define CONFIG_BOOTDELAY 	3
	process_boot_delay();
	
+	# add by uboot_made
+	uboot_made_process_delay();
	
#endif

	/*
	 * Main Loop for Monitor Command Processing
	 */
#ifdef CONFIG_SYS_HUSH_PARSER // CONFIG_SYS_HUSH_PARSER=n
	parse_file_outer();
	/* This point is never reached */
	for (;;);
#else
	for (;;) {

		len = readline (CONFIG_SYS_PROMPT);
		
+		# add by uboot_made
+		uboot_made_cli();

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;
}
#endif

step3
需要根据各项目需求，填写不同的选项。

1.登入密码(目前需要手动替换代码中数组typwd[])，默认:tygw@SSD20x

2.以下宏定义在顶层config文件中已经按需选择，在uboot/local.mk中需要-D导入uboot源码。
#define CONFIG_USER_LOAD_ADDR   ((unsigned char *)0x21000000)
#define NVRAM_HEADER_LOAD_ADDR ((unsigned char *)0x21C00000)
#define NVRAM_VALUE_LOAD_ADDR  ((unsigned char *)0x21800000)
#define NVRAM_COMMIT_ADDR      ((unsigned char *)0x21400000)

#define UBOOT_MADE_HD_INIT 0
#define UBOOT_MADE_OTA_BACKUP 0
#define UBOOT_MADE_BOOT_SD 0
#define UBOOT_MADE_OTA_SD 0
#define FLASH_DOUBLE_BACK 0
#define UBOOT_VERSION 0
#define IMAGE_MEDIUM 0


每个平台的uboot代码存在差异，举例SSD212
在u-boot-2015.01/include/configs/pioneer3.h中增加默认值
#define CONFIG_USER_LOAD_ADDR 0x21000000
#define NVRAM_HEADER_LOAD_ADDR 0x21C00000
#define NVRAM_VALUE_LOAD_ADDR 0x21800000
#define NVRAM_COMMIT_ADDR 0x21400000
#define UBOOT_MADE_HD_INIT 0
#define UBOOT_MADE_OTA_BACKUP 0
#define UBOOT_MADE_BOOT_SD 0
#define UBOOT_MADE_OTA_SD 0
#define FLASH_DOUBLE_BACK 0
#define UBOOT_VERSION 0
#define IMAGE_MEDIUM 0

在uboot/local.mk中增加如下：
字符串string
		sed -i '/#define CONFIG_USER_LOAD_ADDR/c #define CONFIG_USER_LOAD_ADDR $(CI_CONFIG_UBOOT_USER_LOAD_ADDR)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define NVRAM_HEADER_LOAD_ADDR/c #define NVRAM_HEADER_LOAD_ADDR $(CI_CONFIG_NVRAM_HEADER_LOAD_ADDR)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define NVRAM_VALUE_LOAD_ADDR/c #define NVRAM_VALUE_LOAD_ADDR $(CI_CONFIG_NVRAM_VALUE_LOAD_ADDR)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define NVRAM_COMMIT_ADDR/c #define NVRAM_COMMIT_ADDR $(CI_CONFIG_NVRAM_COMMIT_ADDR)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define UBOOT_MADE_HD_INIT/c #define UBOOT_MADE_HD_INIT $(CI_CONFIG_UBOOT_MADE_HD_INIT)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define UBOOT_MADE_OTA_BACKUP/c #define UBOOT_MADE_OTA_BACKUP $(CI_CONFIG_UBOOT_MADE_OTA_BACKUP)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define UBOOT_MADE_BOOT_SD/c #define UBOOT_MADE_BOOT_SD $(CI_CONFIG_UBOOT_MADE_BOOT_SD)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define UBOOT_MADE_OTA_SD/c #define UBOOT_MADE_OTA_SD $(CI_CONFIG_UBOOT_MADE_OTA_SD)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define FLASH_DOUBLE_BACK/c #define FLASH_DOUBLE_BACK $(CI_CONFIG_FLASH_DOUBLE_BACK)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define UBOOT_VERSION/c #define UBOOT_VERSION $(CI_CONFIG_UBOOT_VERSION)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h
        sed -i '/#define IMAGE_MEDIUM/c #define IMAGE_MEDIUM $(CI_CONFIG_IMAGE_MEDIUM)' $(bootloader_src)/u-boot-2015.01/include/configs/pioneer3.h



##################################################################
tips:
由于组件用到了md5 sha256等，故uboot代码中需要支持，其他的按需添加。
+#define CONFIG_MD5
+#define CONFIG_SUPPORT_EMMC_RPMB

后续会把nvram加密的密钥KEY传入的方式做修改。。。




