#include "gpio_ctrl.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "fcntl.h"
#include "Amy_log.h"
#include "Amy_sdk.h"

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_BASE "/sys/class/gpio/gpio"

static const char* direction_to_str(gpio_direction_t dir) {
    switch (dir) {
    case GPIO_DIR_IN: return "in";
    case GPIO_DIR_OUT: return "out";
    case GPIO_DIR_OUT_HIGH: return "high";
    case GPIO_DIR_OUT_LOW: return "low";
    default: return "in";
    }
}
#if 0
static const char* edge_to_str(gpio_edge_t edge) {
    switch (edge) {
    case GPIO_EDGE_NONE: return "none";
    case GPIO_EDGE_RISING: return "rising";
    case GPIO_EDGE_FALLING: return "falling";
    case GPIO_EDGE_BOTH: return "both";
    default: return "none";
    }
}
    #endif


int Amy_gpio_set_direction(int pin, int dir) {
    char path[64] = { 0 };
    snprintf(path, sizeof(path), "%s%d/direction", GPIO_BASE, pin);

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        AMY_LOG_ERROR("open %s failed\n", path);
        return -1;
    }

    const char* dir_str = direction_to_str((gpio_direction_t)dir);
    int ret = write(fd, dir_str, strlen(dir_str));
    if (ret != strlen(dir_str)) {
        AMY_LOG_ERROR("write %s failed\n", dir_str);
        return -1;
    }
    close(fd);
    return 0;
}

int Amy_gpio_init(int pin, int dir) {
    int fd = open(GPIO_EXPORT, O_WRONLY);
    if (fd >= 0) {
        char buf[10];
        int len = snprintf(buf, sizeof(buf), "%d", pin);
        write(fd, buf, len);
        close(fd);
        Amy_msleep(100);
    }

    if (Amy_gpio_set_direction(pin, dir) != 0) {
        return -1;
    }
    return 0;
}


int Amy_gpio_set(int pin, int val) {
    char path[64] = { 0 };
    snprintf(path, sizeof(path), "%s%d/value", GPIO_BASE, pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        AMY_LOG_ERROR("open %s failed\n", path);
        return -1;
    }

    if (write(fd, val ? "1" : "0", 1) != 1) {
        AMY_LOG_ERROR("write gpio%d write %d failed\n", pin, val);
        return -1;
    }
    close(fd);
    return 0;
}

int Amy_gpio_get_value_open(int pin) {
    char path[64] = { 0 };
    snprintf(path, sizeof(path), "%s%d/value", GPIO_BASE, pin);
    return open(path, O_RDONLY);
}

int Amy_gpio_get(int fd) {
    char value;

    lseek(fd, 0, SEEK_SET);
    if (read(fd, &value, 1) != 1) {
        AMY_LOG_ERROR("read failed\n");
        return -1;
    }
    return value == '1' ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
}

int Amy_gpio_get_value_close(int fd) {
    return close(fd);
}
