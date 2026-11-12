/*
UBOOT_VERSION==1
fh8636:uboot-2010.06
T31: uboot-2013.07
ak3918: uboot-2013.10
xm:u-boot-2014.04

UBOOT_VERSION==2
ssd2x2:uboot-2015.01
rk3308:uboot-2017.09
nt9856x:uboot-2019.04
*/

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <bootretry.h>


#define PROP_MAX_LEN 256

#if  FLASH_DOUBLE_BACK == 1
#define	CONFIG_BOOTARGS_0	 "console=ttyS0,115200 root=/dev/mtdblock4 rootfstype=squashfs ro init=/linuxrc LX_MEM=0x7f00000 mma_heap=mma_heap_name0,miu=0,sz=0x3800000 mma_memblock_remove=1 "MTDPARTS_DEFAULT
#define	CONFIG_BOOTARGS_1	 "console=ttyS0,115200 root=/dev/mtdblock5 rootfstype=squashfs ro init=/linuxrc LX_MEM=0x7f00000 mma_heap=mma_heap_name0,miu=0,sz=0x3800000 mma_memblock_remove=1 "MTDPARTS_DEFAULT
#define	CONFIG_BOOTCOMMAND_0	  \
	"mw 1f001cc0 11; "	  \
	"gpio out 8 0; "	  \
	"sf probe 0; "	          \
	"nand read.e 0x22000000 KERNEL0 0x300000;" \
	"gpio out 8 1; "			          \
	"bootm 0x22000000;"

#define	CONFIG_BOOTCOMMAND_1	  \
	"mw 1f001cc0 11; "	  \
	"gpio out 8 0; "	  \
	"sf probe 0; "	          \
	"nand read.e 0x22000000 KERNEL1 0x300000;" \
	"gpio out 8 1; "			          \
	"bootm 0x22000000;"
#endif

#if UBOOT_VERSION == 2
#include <cli.h>
#include <u-boot/md5.h>
#include <autoboot.h>
#endif

#include <uboot_made/uboot_made.h>

extern char console_buffer[];
extern int dev_nvram_init(void);
extern int device_upgrade(char *buf,int len);
extern int restore_img_from_backup(void);
extern int load_fw_from_sd(void);

extern int nvram_get(const char *name, char *value);
extern int cli_readline_into_buffer(const char *const prompt, char *buffer,int timeout);
extern void md5 (unsigned char *input, int len, unsigned char output[16]);

static UBOOT_MADE_GET_UART_CHAR_T uboot_made_get_uart_char = getc;

int register_uboot_made_get_uart_char(UBOOT_MADE_GET_UART_CHAR_T func)
{
    if (func) {
         uboot_made_get_uart_char = func;
    }

    return 0;
}

#if UBOOT_MADE_OTA_BACKUP == 1
static void restore_backup(void)
{
	dev_nvram_init();
	restore_img_from_backup();
}
#endif

#if UBOOT_MADE_BOOT_SD == 1
static void boot_from_sd(void)
{
	/*read ty_uImage to boot from SD */
	extern int boot_from_typt(void);
	boot_from_typt();
}
#endif

#if UBOOT_MADE_OTA_SD == 1
static void upgrade_from_sd(void)
{
	extern int check_update_status(void);
	if(!check_update_status()) {
		printf("reset is pressed\n");
		long sz = 0;
		sz = load_fw_from_sd();
		if (sz > 0)
			device_upgrade((char *)UBOOT_USER_LOAD_ADDR,sz);
	}
}
#endif


#if FLASH_DOUBLE_BACK == 1
extern int prop_get(const char *key, char *value);

static int tuya_atoi(const char* str)
{
	int sign = 1, base = 0, i = 0;

	if (!str)
		return -1;

	/* if whitespaces then ignore. */
	while (str[i] == ' ') {
		i++;
	}

	/* sign of number */
	if (str[i] == '-' || str[i] == '+') {
		sign = 1 - 2 * (str[i++] == '-');
	}

	while (str[i] >= '0' && str[i] <= '9') {
		base = 10 * base + (str[i++] - '0');
		if (base > INT_MAX) {
			if (sign == 1)
				return INT_MAX;
			else
				return (0 - INT_MAX);
		}
	}

	return base * sign;
}

static int get_active_slot(void)
{
	int ret;
	char value[PROP_MAX_LEN] = {0};

	ret = prop_get("persist.ota.slot", value);
	if (ret < 0) {
		printf("use default slot 0.");
		return 0;
	}
	return tuya_atoi(value);
}

static void save_boot_param(void)
{
	dev_nvram_init();

	int slot = 0;
	slot = get_active_slot();
	printf("get persist.ota.slot = %d\n",slot);
	if(slot == 0){
	    setenv("bootargs", CONFIG_BOOTARGS_0);
	    setenv("bootcmd",CONFIG_BOOTCOMMAND_0);
	}else if(slot == 1){
	    setenv("bootargs", CONFIG_BOOTARGS_1);
	    setenv("bootcmd",CONFIG_BOOTCOMMAND_1);
	}else{
	    printf("invalid slot value,use default part 0\n");
	    setenv("bootargs", CONFIG_BOOTARGS_0);
	    setenv("bootcmd",CONFIG_BOOTCOMMAND_0);
	}
}
#endif

static int passwd_check(const char *const prompt)
{
    u8 typwd[16];
    char pwd[64];
    unsigned char pwd_out[64];
    char c;
    int index;
    unsigned long ts;
    static int bPwd = 0;
    static int flag = 0;
    char nvram[32]={0};

    typwd[0]=0x67;
    typwd[1]=0x89;
    typwd[2]=0xa5;
    typwd[3]=0x20;
    typwd[4]=0xea;
    typwd[5]=0xa7;
    typwd[6]=0xe8;
    typwd[7]=0xd;
    typwd[8]=0x4f;
    typwd[9]=0xed;
    typwd[10]=0xc7;
    typwd[11]=0x6;
    typwd[12]=0x2;
    typwd[13]=0x75;
    typwd[14]=0x21;
    typwd[15]=0x53;

    if(flag == 0) {
	    dev_nvram_init();
	    nvram_get("persist.uboot.enter",nvram);
	    printf("nvram:%s,flag:%d\n",nvram,flag);
	    if(strcmp(nvram,"on")==0) {
		    bPwd = 1;
		    flag = 1;
	    }else{
		    puts("### Warning:current uboot shell is unavailable ###\n");
		    do_reset(NULL, 0, 0, NULL);
		    return -2; 
	    }
    }
    while (bPwd) {
        char PID[20];
        memset(PID, 0, sizeof(PID));

        puts("### Please input uboot password: ###\n");
        index = 0;

 	ts = get_timer(0);

        for (;;) {
	  if(!tstc()){
               if (get_timer(ts) > 20000) {
                   puts("### waitting input timeout, reset ###\n");
                   return (-2);    /* timed out */
               }
	  }

          if ((c = uboot_made_get_uart_char()) != '\r' && c != 0xff) {
             if (c == 8) /* Backspace */
             {
                if (index > 0) {
                        printf ("\b \b");
                        index--;
                    }
                    continue;
              }

              putc('*');
              pwd[index] = c;
              index++;

              if (index == 64)
                  index = 0;
            } else if (c  == '\r'){
                break;
            }
	}

        pwd[index] = '\0';
        putc ('\n');

        //extern void md5 (unsigned char *input, int len, unsigned char output[16]);
        md5((unsigned char *) pwd, index, pwd_out);

#if 0
        do{
                int i;
                printf("passwd:%s\n",pwd);
                for(i=0;i<16;i++) {
                        printf("typwd[%d]=0x%x;\n",i,pwd_out[i]);
                }
                printf("\n");

        }while(0);
#endif


        if (!memcmp (pwd_out, typwd, 16)){
            printf("login success\n");
	    bPwd = 0;
        }
    }

	/*
	 * If console_buffer isn't 0-length the user will be prompted to modify
	 * it instead of entering it from scratch as desired.
	 */
	console_buffer[0] = '\0';

	return cli_readline_into_buffer(prompt, console_buffer, 0);
}

static UBOOT_MADE_INIT_T s_hw_init = NULL;

int register_uboot_made_hw_init(UBOOT_MADE_INIT_T func)
{
    if (func) {
        s_hw_init = func;
    }

    return 0;
}

/*
 *hardware init
 *backup
 *tysign
 *uboot ota
 *single/double slot 
*/
void uboot_made_process(void)
{
#if UBOOT_MADE_HW_INIT == 1
    /* hardware init front */
    if (s_hw_init) {
        printf("== hw init ==\n");
        s_hw_init(); 
    } else {
        printf("Warning: uboot made hw init is not defined\n"); 
    }
   // hw_init_front();
#endif

#if UBOOT_MADE_OTA_BACKUP == 1
    /* backup from persist.ota.start */
    printf("== restore_backup ==\n");
    restore_backup();
#endif

#if UBOOT_MADE_BOOT_SD == 1
    /* kernel image from ty_uImage and ty_uImage.sign */
    printf("== boot_from_sd ==\n");
    boot_from_sd();
#endif

#if UBOOT_MADE_OTA_SD == 1
    /* uboot mmc flush ota */
    printf("== upgrade_from_sd ==\n");
    upgrade_from_sd();
#endif

#if FLASH_DOUBLE_BACK == 1
    /* save env for persist.slot */
    printf("== save_boot_param ==\n");
    save_boot_param();
#endif
}

/*
 * passwd_check
*/
void uboot_made_cli(void)
{
	static char lastcommand[CONFIG_SYS_CBSIZE] = { 0, };

	int len;
	int flag;
	int rc = 1;

	for (;;) {
		if (rc >= 0) {
			/* Saw enough of a valid command to
			 * restart the timeout.
			 */
			bootretry_reset_cmd_timeout();
		}
		len = passwd_check(CONFIG_SYS_PROMPT);

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy(lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (len == -2) {
			/* -2 means timed out, retry autoboot
			 */
			puts("\nTimed out waiting for command\n");
# ifdef CONFIG_RESET_TO_RETRY
			/* Reinit board to run initialization code again */
			do_reset(NULL, 0, 0, NULL);
# else
			return;		/* retry autoboot */
# endif
		}
#endif

		if (len == -1)
			puts("<INTERRUPT>\n");
		else
			rc = run_command_repeatable(lastcommand, flag);

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
}

