#include "stdio.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define TARGET_DIR "../"
#define FIRMWARE "SAT_ANYKA.IMG"
// 烧录目录
#define BURNTOOL_DIR TARGET_DIR "../AnyCloud37E_SDK_V1.10/tools/burntool/"
// 分区目录
#define PARRTION_DIR BURNTOOL_DIR "37E/"
// 环境分区目录
#define ENVTOOL_DIR TARGET_DIR "../AnyCloud37E_SDK_V1.10/tools/envtool/"

// rootfs dir
#define ROOTFS_DIR TARGET_DIR "../AnyCloud37E_SDK_V1.10/rootfs/parttion/"
// uboot idr
#define UBOOT_DIR TARGET_DIR "../AnyCloud37E_SDK_V1.10/os/uboot/"

// kernel dir
#define KERNEL_DIR TARGET_DIR "../AnyCloud37E_SDK_V1.10/os/kernel/"

// boot引导文件
#define BOOT_512BIT_BIN PARRTION_DIR "xmc128c_boot.bin"
#define UBOOT_NAME "u-boot.bin"
#define UBOOT_BIN PARRTION_DIR UBOOT_NAME
#define UBOOT_PARRTION_NAME "UBOOT"
#define ENV_NAME "env.img"
#define ENV_BIN PARRTION_DIR ENV_NAME
#define ENV_PARRTION_NAME "ENV"
#define ENVBK_NAME "envbk.img"
#define ENVBK_PARRTION_NAME "ENVBK"
#define DTB_NAME "EVB_CBDM_AK3760E_V1.0.1.dtb"
#define DTB_BIN PARRTION_DIR DTB_NAME
#define DTB_PARRTION_NAME "DTB"
#define LOGO_NAME "logo.jpg"
#define LOGO_BIN PARRTION_DIR LOGO_NAME
#define LOGO_PARRTION_NAME "LOGO"
#define BG_NAME "background.jpg"
#define BG_BIN PARRTION_DIR BG_NAME
#define BG_PARRTION_NAME "BG"
#define KERNEL_NAME "uImage"
#define KERNEL_BIN PARRTION_DIR KERNEL_NAME
#define KERNEL_PARRTION_NAME "KERNEL"
#define ROOTFS_NAME "rootfs.img"
#define ROOTFS_BIN PARRTION_DIR ROOTFS_NAME
#define ROOTFS_PARRTION_NAME "ROOTFS"
#define DATA_NAME "data.img"
#define DATA_BIN PARRTION_DIR DATA_NAME
#define DATA_PARRTION_NAME "DATA"
#define TUYA_NAME "tuya.img"
#define TUYA_BIN PARRTION_DIR TUYA_NAME
#define TUYA_PARRTION_NAME "TUYA"
#define APP_NAME "app.img"
#define APP_BIN PARRTION_DIR APP_NAME
#define APP_PARRTION_NAME "APP"
#define RES_NAME "res.img"
#define RES_BIN PARRTION_DIR RES_NAME
#define RES_PARRTION_NAME "RES"
#define ENV_CFG_NAME "env.cfg"
#define ENV_CFG PARRTION_DIR ENV_CFG_NAME

// 分区大小定义
#define UBOOT_PARRTION_SIZE (300 * 1024)
#define ENV_PARRTION_SIZE (4 * 1024)
#define ENVBK_PARRTION_SIZE (4 * 1024)
#define DTB_PARRTION_SIZE (64 * 1024)
#define LOGO_PARRTION_SIZE (200 * 1024)
#define BG_PARRTION_SIZE (200 * 1024)
#define KERNEL_PARRTION_SIZE (1536 * 1024)
#define ROOTFS_PARRTION_SIZE (4096 * 1024)
#define DATA_PARRTION_SIZE (2048 * 1024)
#define TUYA_PARRTION_SIZE (128 * 1024)
#define APP_PARRTION_SIZE (4096 * 1024)
#define RES_PARRTION_SIZE (3836 * 1024 - TUYA_PARRTION_SIZE)

static char is_upgrade_uboot = 0;
static char is_upgrade_env = 0;
static char is_upgrade_dtb = 0;
static char is_upgrade_logo = 0;
static char is_upgrade_bg = 0;
static char is_upgrade_kernel = 0;
static char is_ugprade_rootfs = 0;
static char is_upgrade_data = 0;
static char is_upgrade_tuya = 0;
static char is_upgrade_app = 0;
static char is_upgrade_res = 0;

static char *uboot_data_addres = NULL;
static long uboot_parrtion_addres = 0;
static long uboot_data_offset = 0;
static long uboot_data_size = 0;

static char *env_data_addres = NULL;
static long env_parrtion_addres = 0;
static long env_data_offset = 0;
static long env_data_size = 0;

static char *envbk_data_addres = NULL;
static long envbk_parrtion_addres = 0;
static long envbk_data_offset = 0;
static long envbk_data_size = 0;

static char *dtb_data_addres = NULL;
static long dtb_parrtion_addres = 0;
static long dtb_data_offset = 0;
static long dtb_data_size = 0;

static char *logo_data_addres = NULL;
static long logo_parrtion_addres = 0;
static long logo_data_size = 0;
static long logo_data_offset = 0;

static char *bg_data_addres = NULL;
static long bg_parrtion_addres = 0;
static long bg_data_size = 0;
static long bg_data_offset = 0;

static char *kernel_data_addres = NULL;
static long kernel_parrtion_addres = 0;
static long kernel_data_size = 0;
static long kernel_data_offset = 0;

static char *rootfs_data_addres = NULL;
static long rootfs_parrtion_addres = 0;
static long rootfs_data_size = 0;
static long rootfs_data_offset = 0;

static char *data_data_addres = NULL;
static long data_parrtion_addres = 0;
static long data_data_size = 0;
static long data_data_offset = 0;

static char *tuya_data_addres = NULL;
static long tuya_parrtion_addres = 0;
static long tuya_data_size = 0;
static long tuya_data_offset = 0;

static char *app_data_addres = NULL;
static long app_parrtion_addres = 0;
static long app_data_size = 0;
static long app_data_offset = 0;

static char *res_data_addres = NULL;
static long res_parrtion_addres = 0;
static long res_data_size = 0;
static long res_data_offset = 0;

static long firmware_data_offset = 0;

#define PARTION_END_STR "# <- this is end of image parttion\n"

static char format_ugprade_parttion(const char *parttion_name)
{
    printf("upgrade %s?[y/n]", parttion_name);
    fflush(stdout);

    /* stdin 处于 EOF（/dev/null、空管道、CI）时不再提问，默认升级全部分区 */
    int in = fgetc(stdin);
    if (in == EOF)
        return 'y';

    int c;
    while ((c = fgetc(stdin)) != '\n' && c != EOF)
        ;

    return (char)in;
}

static void input_upgrade_parttion(void)
{
    is_upgrade_app = format_ugprade_parttion("app");
    is_upgrade_res = format_ugprade_parttion("res");
    is_upgrade_tuya = format_ugprade_parttion("tuya");
    is_upgrade_data = format_ugprade_parttion("data");
    is_ugprade_rootfs = format_ugprade_parttion("rootfs");
    is_upgrade_kernel = format_ugprade_parttion("kernel");
    is_upgrade_bg = format_ugprade_parttion("bg");
    is_upgrade_logo = format_ugprade_parttion("logo");
    is_upgrade_dtb = format_ugprade_parttion("dtb");
    is_upgrade_env = format_ugprade_parttion("env");
    is_upgrade_uboot = format_ugprade_parttion("uboot");
}

static void update_env_img(void)
{
    FILE *rcfg_fp = fopen(ENV_CFG, "r");
    if (!rcfg_fp)
    {
        printf("error :open %s failed\n", ENV_CFG);
        exit(0);
    }
    FILE *Wcfg_fp = fopen(ENVTOOL_DIR ENV_CFG_NAME, "w");
    if (!Wcfg_fp)
    {
        fclose(rcfg_fp);
        printf("error :open %s failed\n", ENVTOOL_DIR ENV_CFG_NAME);
        exit(0);
    }

    // 分区地址自动计算（已插入 TUYA）
    uboot_parrtion_addres = 0;
    env_parrtion_addres = UBOOT_PARRTION_SIZE;
    envbk_parrtion_addres = env_parrtion_addres + ENV_PARRTION_SIZE;
    dtb_parrtion_addres = envbk_parrtion_addres + ENVBK_PARRTION_SIZE;
    logo_parrtion_addres = dtb_parrtion_addres + DTB_PARRTION_SIZE;
    bg_parrtion_addres = logo_parrtion_addres + LOGO_PARRTION_SIZE;
    kernel_parrtion_addres = bg_parrtion_addres + BG_PARRTION_SIZE;
    rootfs_parrtion_addres = kernel_parrtion_addres + KERNEL_PARRTION_SIZE;
    data_parrtion_addres = rootfs_parrtion_addres + ROOTFS_PARRTION_SIZE;
    tuya_parrtion_addres = data_parrtion_addres + DATA_PARRTION_SIZE;
    app_parrtion_addres = tuya_parrtion_addres + TUYA_PARRTION_SIZE;
    res_parrtion_addres = app_parrtion_addres + APP_PARRTION_SIZE;

    // 自动生成 mtdparts，包含 TUYA
    char new_mtdparts[1024] = {0};
    snprintf(new_mtdparts, sizeof(new_mtdparts) - 1,
             " mtdparts mtdparts=spi0.0:"
             "%dK@%#lx(UBOOT),"
             "%dK@%#lx(ENV),"
             "%dK@%#lx(ENVBK),"
             "%dK@%#lx(DTB),"
             "%dK@%#lx(LOGO),"
             "%dK@%#lx(BG),"
             "%dK@%#lx(KERNEL),"
             "%dK@%#lx(ROOTFS),"
             "%dK@%#lx(DATA),"
             "%dK@%#lx(TUYA),"
             "%dK@%#lx(APP),"
             "%dK@%#lx(RES)\n",

             UBOOT_PARRTION_SIZE / 1024, uboot_parrtion_addres,
             ENV_PARRTION_SIZE / 1024, env_parrtion_addres,
             ENVBK_PARRTION_SIZE / 1024, envbk_parrtion_addres,
             DTB_PARRTION_SIZE / 1024, dtb_parrtion_addres,
             LOGO_PARRTION_SIZE / 1024, logo_parrtion_addres,
             BG_PARRTION_SIZE / 1024, bg_parrtion_addres,
             KERNEL_PARRTION_SIZE / 1024, kernel_parrtion_addres,
             ROOTFS_PARRTION_SIZE / 1024, rootfs_parrtion_addres,
             DATA_PARRTION_SIZE / 1024, data_parrtion_addres,
             TUYA_PARRTION_SIZE / 1024, tuya_parrtion_addres,
             APP_PARRTION_SIZE / 1024, app_parrtion_addres,
             RES_PARRTION_SIZE / 1024, res_parrtion_addres);

    char line[1024] = {0};
    while (fgets(line, sizeof(line), rcfg_fp) != NULL)
    {
        if (strstr(line, "mtdparts mtdparts=spi0.0:") != NULL)
        {
            fputs(new_mtdparts, Wcfg_fp);
            continue;
        }
        fputs(line, Wcfg_fp);
    }

    fclose(rcfg_fp);
    fclose(Wcfg_fp);
    printf("[%s:%d]-------------------\n", __func__, __LINE__);
}

static void upgrade_parttion_show(void)
{
    printf("parttion upgrade status:\n");
    printf(" ---------------------------------------\n");
    printf("| No.\tName\tstatus\tsize(kb)\t|\n");
    printf("| 1\tuboot\t%s\t%d\t\t|\n", is_upgrade_uboot == 'y' ? "yes" : "no", UBOOT_PARRTION_SIZE / 1024);
    printf("| 2\tenv\t%s\t%d\t\t|\n", is_upgrade_env == 'y' ? "yes" : "no", ENV_PARRTION_SIZE / 1024);
    printf("| 3\tenvbk\t%s\t%d\t\t|\n", is_upgrade_env == 'y' ? "yes" : "no", ENVBK_PARRTION_SIZE / 1024);
    printf("| 4\tdtb\t%s\t%d\t\t|\n", is_upgrade_dtb == 'y' ? "yes" : "no", DTB_PARRTION_SIZE / 1024);
    printf("| 5\tlogo\t%s\t%d\t\t|\n", is_upgrade_logo == 'y' ? "yes" : "no", LOGO_PARRTION_SIZE / 1024);
    printf("| 6\tbg\t%s\t%d\t\t|\n", is_upgrade_bg == 'y' ? "yes" : "no", BG_PARRTION_SIZE / 1024);
    printf("| 7\tkernel\t%s\t%d\t\t|\n", is_upgrade_kernel == 'y' ? "yes" : "no", KERNEL_PARRTION_SIZE / 1024);
    printf("| 8\trootfs\t%s\t%d\t\t|\n", is_ugprade_rootfs == 'y' ? "yes" : "no", ROOTFS_PARRTION_SIZE / 1024);
    printf("| 9\tdata\t%s\t%d\t\t|\n", is_upgrade_data == 'y' ? "yes" : "no", DATA_PARRTION_SIZE / 1024);
    printf("| 10\ttuya\t%s\t%d\t\t|\n", is_upgrade_tuya == 'y' ? "yes" : "no", TUYA_PARRTION_SIZE / 1024);
    printf("| 11\tapp\t%s\t%d\t\t|\n", is_upgrade_app == 'y' ? "yes" : "no", APP_PARRTION_SIZE / 1024);
    printf("| 12\tres\t%s\t%d\t|\n", is_upgrade_res == 'y' ? "yes" : "no", RES_PARRTION_SIZE / 1024);
    printf(" ---------------------------------------\n");
}

static int file_size(const char *file)
{
    struct stat st;
    if (stat(file, &st) == -1)
    {
        printf("error:%s stat failed\n", file);
        exit(1);
    }
    return st.st_size;
}

static int firmware_read(const char *file, char **data_addres, long *data_size, long part_size, long *part_offset, long *firmware_offset)
{
    int size = file_size(file);
    char *data = (char *)malloc(size);
    memset(data, 0, size);
    int fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        printf("error:[%s:%d]open %s fialed\n", __func__, __LINE__, file);
        free(data);
        exit(0);
    }
    int ret = read(fd, data, size);
    if ((ret < 0) || (ret > part_size))
    {
        printf("error:read %s fialed,size failed(%d)\n", file, ret);
        free(data);
        close(fd);
        exit(0);
    }
    close(fd);

    *data_addres = data;
    *data_size = size;
    *part_offset = *firmware_offset;
    *firmware_offset += size;
    printf("read %s success:%dkb\n", file, size / 1024);
    return 0;
}

static void uboot_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./build.sh;cd -", UBOOT_DIR);
    system(cmd);

    firmware_read(UBOOT_BIN, &uboot_data_addres, &uboot_data_size, UBOOT_PARRTION_SIZE, &uboot_data_offset, &firmware_data_offset);
    int fd = open(BOOT_512BIT_BIN, O_RDONLY);
    if (fd < 0)
    {
        printf("error:open %s fialed\n", BOOT_512BIT_BIN);
        free(uboot_data_addres);
        exit(0);
    }
    int ret = read(fd, uboot_data_addres, 512);
    if (ret != 512)
    {
        printf("error :read uboot after 512bytes failed(%d)\n", ret);
        free(uboot_data_addres);
        exit(0);
    }
    close(fd);

    remove(UBOOT_BIN);
    fd = open(UBOOT_BIN, O_CREAT | O_WRONLY, 0777);
    if (fd < 0)
    {
        printf("error:[%s:%d]open %s fialed\n", __func__, __LINE__, UBOOT_BIN);
        free(uboot_data_addres);
        exit(0);
    }
    ret = write(fd, uboot_data_addres, uboot_data_size);
    if (ret != uboot_data_size)
    {
        printf("error :write uboot after 512bytes failed\n");
        free(uboot_data_addres);
        exit(0);
    }
    close(fd);
}

static void env_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mkenv.sh %s;cd -", ENVTOOL_DIR, ENV_CFG_NAME);
    system(cmd);

    remove(ENV_BIN);
    rename(ENVTOOL_DIR "env.img", ENV_BIN);
    firmware_read(ENV_BIN, &env_data_addres, &env_data_size, ENV_PARRTION_SIZE, &env_data_offset, &firmware_data_offset);
    firmware_read(ENV_BIN, &envbk_data_addres, &envbk_data_size, ENVBK_PARRTION_SIZE, &envbk_data_offset, &firmware_data_offset);
}

static void dtb_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./build.sh;cd -", KERNEL_DIR);
    system(cmd);
    firmware_read(DTB_BIN, &dtb_data_addres, &dtb_data_size, DTB_PARRTION_SIZE, &dtb_data_offset, &firmware_data_offset);
}

static void logo_upgrade_genrate(void)
{
    firmware_read(LOGO_BIN, &logo_data_addres, &logo_data_size, LOGO_PARRTION_SIZE, &logo_data_offset, &firmware_data_offset);
}

static void bg_upgrade_genrate(void)
{
    firmware_read(BG_BIN, &bg_data_addres, &bg_data_size, BG_PARRTION_SIZE, &bg_data_offset, &firmware_data_offset);
}

static void kernel_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./build.sh;cd -", KERNEL_DIR);
    system(cmd);
    firmware_read(KERNEL_BIN, &kernel_data_addres, &kernel_data_size, KERNEL_PARRTION_SIZE, &kernel_data_offset, &firmware_data_offset);
}

static void rootfs_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mkrootfs.sh;cd -", ROOTFS_DIR);
    system(cmd);
    firmware_read(ROOTFS_BIN, &rootfs_data_addres, &rootfs_data_size, ROOTFS_PARRTION_SIZE, &rootfs_data_offset, &firmware_data_offset);
}

static void data_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mkdata.sh;cd -", ROOTFS_DIR);
    system(cmd);
    firmware_read(DATA_BIN, &data_data_addres, &data_data_size, DATA_PARRTION_SIZE, &data_data_offset, &firmware_data_offset);
}

static void tuya_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mktuya.sh;cd -", ROOTFS_DIR);
    system(cmd);
    firmware_read(TUYA_BIN, &tuya_data_addres, &tuya_data_size, TUYA_PARRTION_SIZE, &tuya_data_offset, &firmware_data_offset);
}

static void app_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mkapp.sh;cd -", ROOTFS_DIR);
    system(cmd);
    firmware_read(APP_BIN, &app_data_addres, &app_data_size, APP_PARRTION_SIZE, &app_data_offset, &firmware_data_offset);
}

static void res_upgrade_genrate(void)
{
    char cmd[1024] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cd %s;./mkres.sh;cd -", ROOTFS_DIR);
    system(cmd);
    firmware_read(RES_BIN, &res_data_addres, &res_data_size, RES_PARRTION_SIZE, &res_data_offset, &firmware_data_offset);
}

int main(int argv, char **argc)
{
    printf("Generate firmware Verion:0.0.1\n");
    input_upgrade_parttion();
    update_env_img();
    upgrade_parttion_show();

    firmware_data_offset = 0;
    if (is_upgrade_uboot == 'y')
    {
        uboot_upgrade_genrate();
    }
    if (is_upgrade_env == 'y')
    {
        env_upgrade_genrate();
    }
    if (is_upgrade_dtb == 'y')
    {
        dtb_upgrade_genrate();
    }
    if (is_upgrade_logo == 'y')
    {
        logo_upgrade_genrate();
    }
    if (is_upgrade_bg == 'y')
    {
        bg_upgrade_genrate();
    }
    if (is_upgrade_kernel == 'y')
    {
        kernel_upgrade_genrate();
    }
    if (is_ugprade_rootfs == 'y')
    {
        rootfs_upgrade_genrate();
    }
    if (is_upgrade_data == 'y')
    {
        data_upgrade_genrate();
    }
    if (is_upgrade_tuya == 'y')
    {
        tuya_upgrade_genrate();
    }
    if (is_upgrade_app == 'y')
    {
        app_upgrade_genrate();
    }
    if (is_upgrade_res == 'y')
    {
        res_upgrade_genrate();
    }

    char cmd[1024] = {0};
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "#<upgrade_bin_version=%04d%02d%02d%02d%02d%02d>\n",
            time_info->tm_year + 1900,
            time_info->tm_mon + 1,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec);

    remove(TARGET_DIR FIRMWARE);
    int fd = open(TARGET_DIR FIRMWARE, O_CREAT | O_WRONLY, 0777);
    write(fd, cmd, strlen(cmd));

    if (is_upgrade_uboot == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          UBOOT_PARRTION_NAME,
                          uboot_data_offset, uboot_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_env == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          ENV_PARRTION_NAME,
                          env_data_offset, env_data_size);
        write(fd, cmd, ret);

        memset(cmd, 0, sizeof(cmd));
        ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                      ENVBK_PARRTION_NAME,
                      envbk_data_offset, envbk_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_dtb == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          DTB_PARRTION_NAME,
                          dtb_data_offset, dtb_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_logo == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          LOGO_PARRTION_NAME,
                          logo_data_offset, logo_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_bg == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          BG_PARRTION_NAME,
                          bg_data_offset, bg_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_kernel == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          KERNEL_PARRTION_NAME,
                          kernel_data_offset, kernel_data_size);
        write(fd, cmd, ret);
    }
    if (is_ugprade_rootfs == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          ROOTFS_PARRTION_NAME,
                          rootfs_data_offset, rootfs_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_data == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          DATA_PARRTION_NAME,
                          data_data_offset, data_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_tuya == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          TUYA_PARRTION_NAME,
                          tuya_data_offset, tuya_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_app == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          APP_PARRTION_NAME,
                          app_data_offset, app_data_size);
        write(fd, cmd, ret);
    }
    if (is_upgrade_res == 'y')
    {
        memset(cmd, 0, sizeof(cmd));
        int ret = sprintf(cmd, "# File Parttion: %s, %ld, %ld \n",
                          RES_PARRTION_NAME,
                          res_data_offset, res_data_size);
        write(fd, cmd, ret);
    }

    write(fd, PARTION_END_STR, strlen(PARTION_END_STR));

    // 写入真实数据
    if (is_upgrade_uboot == 'y')
    {
        write(fd, uboot_data_addres, uboot_data_size);
        free(uboot_data_addres);
    }
    if (is_upgrade_env == 'y')
    {
        write(fd, env_data_addres, env_data_size);
        free(env_data_addres);
        write(fd, envbk_data_addres, envbk_data_size);
        free(envbk_data_addres);
    }
    if (is_upgrade_dtb == 'y')
    {
        write(fd, dtb_data_addres, dtb_data_size);
        free(dtb_data_addres);
    }
    if (is_upgrade_logo == 'y')
    {
        write(fd, logo_data_addres, logo_data_size);
        free(logo_data_addres);
    }
    if (is_upgrade_bg == 'y')
    {
        write(fd, bg_data_addres, bg_data_size);
        free(bg_data_addres);
    }
    if (is_upgrade_kernel == 'y')
    {
        write(fd, kernel_data_addres, kernel_data_size);
        free(kernel_data_addres);
    }
    if (is_ugprade_rootfs == 'y')
    {
        write(fd, rootfs_data_addres, rootfs_data_size);
        free(rootfs_data_addres);
    }
    if (is_upgrade_data == 'y')
    {
        write(fd, data_data_addres, data_data_size);
        free(data_data_addres);
    }
    if (is_upgrade_tuya == 'y')
    {
        write(fd, tuya_data_addres, tuya_data_size);
        free(tuya_data_addres);
    }
    if (is_upgrade_app == 'y')
    {
        write(fd, app_data_addres, app_data_size);
        free(app_data_addres);
    }
    if (is_upgrade_res == 'y')
    {
        write(fd, res_data_addres, res_data_size);
        free(res_data_addres);
    }

    close(fd);
    return 0;
}