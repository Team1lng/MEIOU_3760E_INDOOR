#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/watchdog.h>


#define MAX_PATH    256
#define HAL_WATCHDOG_DEFAULT_DEVICE "/dev/watchdog"

int watchdog_open(void)
{
    char dev_name[MAX_PATH] = {0};
    int fd = -1;
    strncpy(dev_name, HAL_WATCHDOG_DEFAULT_DEVICE,
            strlen(HAL_WATCHDOG_DEFAULT_DEVICE) + 1);

    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        printf("ak_watchog: error Failed to open %s\n", dev_name);
        return -errno;
    }

    return fd;
}

int watchdog_close(int fd) 
{
    int ret = 0;
    char magic = 'V';
    int option = WDIOS_DISABLECARD;

    if (fd <= 0) {
        printf("ak_watchog: error watchdog not initialized\n");
        return -EPERM;
    }

    /*
     * A magic character 'V' must be written before closing the watchdog device.
     * Otherwise, the watchdog will reset the system after closing fd.
     */
    ret = write(fd, &magic, sizeof(magic));
    if (ret < 0) {
        printf("ak_watchog: error write magic word failed\n");
    }

    ret = ioctl(fd, WDIOC_SETOPTIONS, &option);
    if (ret < 0) {
        printf("ak_watchog: error WDIOC_SETOPTIONS failed\n");
    }

    close(fd);
    return ret;
}

static int watchdog_ioctl(int fd, int ioctl_cmd, int *arg)
{
    int ret = 0;

    if (fd <= 0) {
        printf("ak_watchog: error watchdog not initialized\n");
        return -EPERM;
    }

    ret = ioctl(fd, ioctl_cmd, arg);
    if (ret < 0) {
        printf("ak_watchog error: ioctl 0x%X failed\n", ioctl_cmd);
        return -errno;
    }

    return ret;
}

int watchdog_keepalive(int fd)
{
    return watchdog_ioctl(fd, WDIOC_KEEPALIVE, NULL);
}

int watchdog_set_timeout(int fd, int *timeout)
{
    if (!timeout) {
        printf("%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_SETTIMEOUT, timeout);
}

int watchdog_get_timeout(int fd, int *timeout)
{
    if (!timeout) {
        printf("%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_GETTIMEOUT, timeout);
}

int watchdog_get_timeleft(int fd, int *timeleft)
{
    if (!timeleft) {
        printf("%s: Invaild argument\n", __func__);
        return -EINVAL;
    }

    return watchdog_ioctl(fd, WDIOC_GETTIMELEFT, timeleft);
}


int main(int argc, char **argv)
{
    int ret = 0;
    int fd = -1;
    int timeout = atoi(argv[1]);

    printf("AK_WATCHDOG start, keepalive:%d\n", atoi(argv[2]));
    fd = watchdog_open();
    if (fd < 0) {
        ret = -1;
        goto EXIT;
    }

    ret = watchdog_set_timeout(fd, &timeout);
    if (ret < 0)
        goto EXIT;

    ret = watchdog_get_timeout(fd, &timeout);
    if (ret < 0)
        goto EXIT;
    printf("current timeout:%d\n", timeout);

    while(1) {
        ret = watchdog_keepalive(fd);
        if (ret < 0)
            printf("ak_watchog: keepalive failed\n");

        sleep(atoi(argv[2]));
    }
    
    ret = watchdog_close(fd);
    if (ret < 0)
        printf("ak_watchog clsoe failed\n");

EXIT:
    printf("AK_WATCHDOG stop!\n");
    return ret;
}
