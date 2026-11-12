#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#ifndef CONFIG_TUYA_TKL_DISPLAY_BACKLIGHT
#define CONFIG_TUYA_TKL_DISPLAY_BACKLIGHT "/sys/devices/platform/backlight/backlight/backlight/brightness"
#endif

static int write_int(char const* path, int value)
{
    FILE *fp;
    fp = fopen(path, "w+");
    if (fp) {
        int ret = fprintf(fp, "%d", value);
        fclose(fp);
        return ret < 0 ? -errno : 0;
    } else {
        printf("write_int failed to open %s\n", path);
        return -errno;
    }
}

static int read_int(char const* path)
{
    FILE *fp;
    int value = 0;
    fp = fopen(path, "r");
    if (fp) {
        int ret = fscanf(fp, "%d", &value);
        fclose(fp);
        return ret == EOF ? 0 : value;
    } else {
        printf("read_int failed to open %s\n", path);
        return 0;
    }
}

int tkl_set_brightness_common(int brightness)
{
    return write_int(CONFIG_TUYA_TKL_DISPLAY_BACKLIGHT, brightness);
}

int tkl_get_brightness_common(void)
{
    return read_int(CONFIG_TUYA_TKL_DISPLAY_BACKLIGHT);
}
