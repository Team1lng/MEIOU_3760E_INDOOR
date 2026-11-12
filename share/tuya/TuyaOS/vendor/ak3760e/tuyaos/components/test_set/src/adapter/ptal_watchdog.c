#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/watchdog.h>
#include "product_test_utils.h"
#include "product_test_config.h"
#include "product_test_watchdog.h"
#include "ptal_watchdog.h"
#include "tb_log.h"

static int s_wdt_fd = -1;

#define WATCHDOG_DEFAULT_DEVICE "/dev/watchdog"

static int __watchdog_ioctl(int fd, int ioctl_cmd, int* arg)
{
    int iret = 0;

    if (fd <= 0) {
        TB_LOGE("watchdog not initialized");
        return -1;
    }

    iret = ioctl(fd, ioctl_cmd, arg);

    if (iret < 0) {
        TB_LOGE("ioctl 0x%X failed", ioctl_cmd);
        return -1;
    }

    return iret;
}

int __attribute__((weak)) ptal_watchdog_init(int ms) 
{

    int fd;
    int timeout; // in seconds
    char *dev_name = WATCHDOG_DEFAULT_DEVICE;
    char name[65] = {0};

    TB_LOGD();

    if (ms <= 0) {
        TB_LOGE("invalid param");
	return 0;
    }

    if(product_test_watchdog_get_devname(name) == 0 && strlen(name)){
       dev_name = name;
    }

    fd = open(dev_name, O_RDWR);

    if (fd < 0) {
        TB_LOGE("error: Failed to open %s\n", dev_name);
        return 0;
    }

    timeout = ms / 1000;

    if(__watchdog_ioctl(fd,WDIOC_SETTIMEOUT,&timeout) != 0) {
        TB_LOGE("Failed to set watchdog timeout");
	return 0;
    }

    if(__watchdog_ioctl(fd,WDIOC_GETTIMEOUT,&timeout) != 0) {
        TB_LOGE("Failed to get watchdog timeout");
        return 0; 
    }

    s_wdt_fd = fd;

    return (timeout*1000);
}

int __attribute__((weak)) ptal_watchdog_uinit(void)
{
    char magic = 'V';
    int iret = 0;
    int option = WDIOS_DISABLECARD;

    int fd = s_wdt_fd;

    TB_LOGD();

    if (fd <= 0) {
        TB_LOGE("watchdog not initialized\n");
        return -1;
    }

    /*
     * A magic character 'V' must be written before closing the watchdog device.
     * Otherwise, the watchdog will reset the system after closing fd.
     */
    iret = write(fd, &magic, sizeof(magic));
    if (iret < 0) {
        TB_LOGE("write magic word failed");
	return -1;
    }

    iret =  __watchdog_ioctl(fd,WDIOC_SETOPTIONS,&option);

    if(iret < 0) {
        TB_LOGE("WDIOC_SETOPTIONS");
	return -1;
    }

    close(fd);

    return iret;
}

int __attribute__((weak)) ptal_watchdog_feed(void)
{
    int iret = 0;
    int fd = s_wdt_fd;

    TB_LOGD();

    if (fd <= 0) {
        TB_LOGE("watchdog not initialized");
        return -1; 
    }

    iret =  __watchdog_ioctl(fd, WDIOC_KEEPALIVE, NULL);

    if (iret < 0) {
        TB_LOGE("ioctl 0x%X failed", WDIOC_KEEPALIVE);
        return -1;
    }

    return iret;
}

