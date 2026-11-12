#include "Amy_sdk.h"
#include "ak_common.h"
#include "Amy_log.h"
#include "gpio_ctrl.h"
#include "Amy_tfcard.h"
#include "i2c_ctrl.h"
#include "res.h"
#include <ctype.h>
#include "pwm_ctrl.h"
#include "boot_time.h"
#include "../project/pwm.h"
static char *skip_space(const char *str)
{
    while ((str) && ((*str) == ' '))
        str++;
    return (char *)str;
}
static char *find_end(const char *str)
{
    while ((str) && ((*str) != ' ') && ((*str) != '\0'))
        str++;
    return (char *)str;
}
int find_string_between_spaces(const char *str, char **next, char *val, int len)
{
    char *s = skip_space(str);
    if (!s)
    {
        return -1;
    }
    char *e = find_end(s);
    if (!e)
    {
        return -1;
    }
    if ((e - s) > len)
    {
        printf("parttion val over fill (src:%d,len:%d)\n", e - s, len);
        return -1;
    }
    memset(val, 0, len);
    strncpy(val, s, e - s);
    *next = e + 1;
    return 0;
}

char *string_spaces_after_char(const char *string, int n)
{
    int space_count = 0;
    const char *ptr = string;
    while ((*ptr) != '\0')
    {
        if ((*ptr) == ' ')
        {
            if ((*(ptr + 1) != ' ') && (*(ptr + 1) != '\0'))
            {
                space_count++;
                if (space_count == n)
                {
                    return (char *)(ptr + 1);
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

// 获取字符串末尾开始的第一个连续数字的首地址
char *find_tail_string_number(char *string)
{
    char *ptr = string + strlen(string) - 1;
    if (!isdigit(*ptr))
    {
        return NULL;
    }

    while ((ptr >= string) && (isdigit(*ptr)))
    {
        ptr--;
    }
    return ptr + 1;
}

int system_cmd_execution(cmd_execution_param_t *param)
{
    int reslut = 0;
    char buf[128] = {0};
    int line = 0;
    //  AMY_LOG_INFO("%s\n", param->cmd);
    FILE *fp = popen(param->cmd, param->isread ? "r" : "w");
    if (!fp)
    {
        AMY_LOG_ERROR("popen %s failed\n", param->cmd);
        return -1;
    }

    if (param->cb)
    {
        if (param->isread)
        {
            reslut = -1;
            while (fgets(buf, sizeof(buf) - 1, fp) != NULL)
            {
                char *s = strstr(buf, "\n");
                if (s)
                {
                    *s = '\0';
                }

                if (param->cb(param->userdata, buf, line) == 0)
                {
                    if (param->reslut_string)
                    {
                        strncpy(param->reslut_string, buf, (size_t)param->reslut_strean_len);
                    }
                    reslut = 0;
                }
                line++;
            }
        }
        else
        {
            reslut = param->cb(param->userdata, (char *)fp, 0);
        }
    }
    pclose(fp);
    return reslut;
}

static int modlues_exist_cb(const void *src, char *dst, int line)
{
    return strstr(dst, src) ? 0 : 1;
}

static int module_install(void)
{
    static const char *modlues[][2] = {
        // {"ak_ion", "insmod /usr/modules/ak_ion.ko"},
        // {"ak_uio", "insmod /usr/modules/ak_uio.ko"},
        // // mci
        // {"mmc_core", "insmod /usr/modules/mmc_core.ko"},
        // {"mmc_block", "insmod /usr/modules/mmc_block.ko"},
        // {"ak_mci", "insmod /usr/modules/ak_mci.ko"},

        // // audio
        // {"ak_pcm", "insmod /usr/modules/ak_pcm.ko"},

        // // touch
        // {"ak_i2c", "insmod /usr/modules/ak_i2c.ko"},
        // {"ts_gsl", "insmod /usr/modules/ts_gsl.ko"},
        // // {"ts_ft5446u", "insmod /usr/modules/ts_ft5446u.ko"},

        // // pwm
        // {"ak_pwm_char", "insmod /usr/modules/ak_pwm_char.ko"},

        // display
        {"ak_fb", "insmod /usr/modules/ak_fb.ko"},
        {"ak_gui", "insmod /usr/modules/ak_gui.ko"},
    };

    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));

    int size = sizeof(modlues) / sizeof(modlues[0]);
    for (int i = 0; i < size; i++)
    {
        param.cmd = "cat /proc/modules";
        param.isread = 1;
        param.cb = modlues_exist_cb;
        param.userdata = (void *)modlues[i][0];
        if (system_cmd_execution(&param))
        {
            param.cmd = modlues[i][1];
            param.isread = 0;
            param.cb = NULL;
            param.userdata = NULL;
            system_cmd_execution(&param);
            AMY_LOG_INFO("%s\n", modlues[i][1]);
        }
        boot_time_tick(modlues[i][0]);
    }
    return 0;
}

#if 1
static int found_pattion_cb(const void *src, char *dst, int line)
{
    return strstr(dst, src) ? 0 : 1;
}

int Amy_mtd_parttion_get(const char *partition_name, char *mtd_name, int name_len, unsigned long *mtd_size)
{
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));
    char reslut_string[128] = {0};

    param.cmd = "cat /proc/mtd";
    param.isread = 1;
    param.cb = found_pattion_cb;
    param.reslut_string = reslut_string;
    param.reslut_strean_len = sizeof(reslut_string);
    param.userdata = partition_name;

    if (system_cmd_execution(&param))
    {
        AMY_LOG_ERROR("not find %s parttion\n", partition_name);
        return -1;
    }

    // AMY_LOG_INFO("find %s\n", reslut_string);
    char *next = NULL;
    if (find_string_between_spaces(reslut_string, &next, mtd_name, name_len))
    {
        AMY_LOG_ERROR("reslut_string:%s not find\n", reslut_string);
        return -1;
    }
    char *p = strstr(mtd_name, ":");
    if (p)
    {
        *p = '\0';
    }
    //    AMY_LOG_INFO("%s->%s\n", partition_name, mtd_name);

    if (mtd_size)
    {
        char mtd_size_arry[32] = {0};
        if (find_string_between_spaces(next, &next, mtd_size_arry, sizeof(mtd_size_arry)))
        {
            AMY_LOG_ERROR("reslut_string:%s not find\n", next);
            return -1;
        }
        *mtd_size = strtoul(mtd_size_arry, NULL, 16);
        // AMY_LOG_INFO("%s->%s :%#x\n", partition_name, mtd_name, *mtd_size);
    }
    return 0;
}
#endif
void Amy_sdk_init(const audio_ring_resource *rings_resource, int rings_cnt, const firamware_resource *ttf)
{

    sdk_run_config cfg;
    memset(&cfg, 0, sizeof(sdk_run_config));

    boot_time_tick("Amy_sdk_init enter");

    Amy_pwm_ctrl_init();
    backlight_enable(false);
    boot_time_tick("Amy_pwm_ctrl_init");

    module_install();
    boot_time_tick("module_install");

    ak_sdk_init(&cfg);
    boot_time_tick("ak_sdk_init");

    Amy_lvgl_init(RES_BIN, ttf->offset, ttf->size);
    boot_time_tick("Amy_lvgl_init");

    Amy_gpio_ctr_init();
    boot_time_tick("Amy_gpio_ctr_init");

    Amy_audio_play_init(RES_BIN, rings_resource, rings_cnt);
    boot_time_tick("Amy_audio_play_init");

    Amy_tfcard_init();
    boot_time_tick("Amy_tfcard_init");

    i2c_device_init();
    boot_time_tick("i2c_device_init");

    // tp9950_device_init(ad_ic_power_cb);
}
