#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/watchdog.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_board.h"
#include "hal_watchdog.h"
#include "hal_watchdog_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_watchdog_t hal_if_watchdog = { 0 };


#define HAL_WATCHDOG_DEFAULT_DEVICE "/dev/watchdog"

int hal_watchdog_open_common(void)
{
    char dev_name[MAX_PATH] = {0};
    int fd = -1;
    int ret;

    ret = hal_cfg_get_item_string(ITEM_WATCHDOG, KEY_WATCHDOG_DEV_NAME,
                                  dev_name, sizeof(dev_name));
    if (ret != 0) {
        strncpy(dev_name, HAL_WATCHDOG_DEFAULT_DEVICE,
                strlen(HAL_WATCHDOG_DEFAULT_DEVICE) + 1);
    }

    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "error: Failed to open %s\n", dev_name);
        return -errno;
    }

    return fd;
}

static int watchdog_ioctl(int fd, int ioctl_cmd, int* arg)
{
    int ret = 0;

    if (fd <= 0) {
        hal_log(HAL_LOG_ERR, "error: watchdog not initialized\n");
        return -EPERM;
    }

    ret = ioctl(fd, ioctl_cmd, arg);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "error: ioctl 0x%X failed\n", ioctl_cmd);
        return -errno;
    }

    return ret;
}

int hal_watchdog_close_common(int fd)
{
    char magic = 'V';
    int ret = 0;
    int option = WDIOS_DISABLECARD;

    if (fd <= 0) {
        hal_log(HAL_LOG_ERR, "error: watchdog not initialized\n");
        return -EPERM;
    }

    /*
     * A magic character 'V' must be written before closing the watchdog device.
     * Otherwise, the watchdog will reset the system after closing fd.
     */
    ret = write(fd, &magic, sizeof(magic));
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "error: write magic word failed\n");
    }

    ret = ioctl(fd, WDIOC_SETOPTIONS, &option);
    if(ret < 0) {
        hal_log(HAL_LOG_ERR, "error: WDIOC_SETOPTIONS failed\n");
    }

    close(fd);
    return ret;
}

int hal_watchdog_keepalive_common(int fd)
{
    return watchdog_ioctl(fd, WDIOC_KEEPALIVE, NULL);
}

int hal_watchdog_set_timeout_common(int fd, int* timeout)
{
    if (!timeout) {
        hal_log(HAL_LOG_ERR, "%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_SETTIMEOUT, timeout);
}

int hal_watchdog_get_timeout_common(int fd, int* timeout)
{
    if (!timeout) {
        hal_log(HAL_LOG_ERR, "%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_GETTIMEOUT, timeout);
}

int hal_watchdog_get_timeleft_common(int fd, int* timeleft)
{
    if (!timeleft) {
        hal_log(HAL_LOG_ERR, "%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_GETTIMELEFT, timeleft);
}
