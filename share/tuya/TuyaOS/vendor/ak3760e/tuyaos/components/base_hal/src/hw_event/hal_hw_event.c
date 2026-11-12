#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include "hal_event.h"
#include "hal_log.h"
#include "hal_board.h"

#define USB_STORAGE_DEVICE_NAME        "block/sd"
#define USB_STORAGE_UEVENT_PATH        "/sys/devices/platform"
#define USB_STORAGE_DEVICE_PATH        "ehci"

#define USB_POWER_DEVICE_NAME          "/power_supply/usb"
#define USB_POWER_UEVENT_PATH          "/sys/class/power_supply/usb"

#define BATTERY_DEVICE_NAME            "/power_supply/battery"
#define BATTERY_UEVENT_PATH            "/sys/class/power_supply/battery"

#define SDCARD_DEVICE_NAME             "block/mmcblk"
#define SDCARD_UEVENT_PATH             "/sys/devices/platform/rtk-sdmmc/mmc_host/mmc0"

/* Get uevent PATH of usb storage from configuration file */
static int get_usb_storage_uevent_path(char *path)
{
    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_USB_STORAGE,
                KEY_USB_STORAGE_UEVENT_PATH, buf, sizeof(buf));
    if (ret != 0) {
        return -1;
    }

    strncpy(path, buf, strlen(buf) + 1);
    return 0;
}

/* Get uevent DEVNAME of usb storage from configuration file */
static void get_usb_storage_device_name(char *name)
{
    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_USB_STORAGE,
                KEY_USB_STORAGE_DEVICE_NAME, buf, sizeof(buf));
    if (ret != 0) {
        strncpy(name, USB_STORAGE_DEVICE_NAME,
                strlen(USB_STORAGE_DEVICE_NAME) + 1);
        return;
    }

    strncpy(name, buf, strlen(buf) + 1);
    return;
}


/* Get uevent PATH of battery from configuration file */
static void get_battery_uevent_path(char *path)
{

    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_BATTERY, KEY_BATTERY_UEVENT_PATH, buf, sizeof(buf));

    if (ret != 0){
	strncpy(path,BATTERY_UEVENT_PATH, strlen(BATTERY_UEVENT_PATH) + 1);
	return;
    }

    strncpy(path, buf, strlen(buf) + 1);
    return;
}

static void get_battery_device_name(char *name)
{
    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_BATTERY,KEY_BATTERY_DEVICE_NAME, buf, sizeof(buf));

    if(ret != 0){
	strncpy(name, BATTERY_DEVICE_NAME, strlen(BATTERY_DEVICE_NAME) + 1);
	return;
    }

    strncpy(name, buf, strlen(buf) + 1);
    return;
}


/* Get uevent PATH of sd card from configuration file */
static void get_sdcard_uevent_path(char *path)
{
    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_SDCARD,
                KEY_SDCARD_UEVENT_PATH, buf, sizeof(buf));
    if (ret != 0) {
        strncpy(path, SDCARD_UEVENT_PATH, strlen(SDCARD_UEVENT_PATH) + 1);
        return;
    }

    strncpy(path, buf, strlen(buf) + 1);
    return;
}

/* Get uevent DEVNAME of sd card from configuration file */
static void get_sdcard_device_name(char *name)
{
    char buf[MAX_PATH] = {0};
    int ret;

    ret = hal_cfg_get_item_string(ITEM_SDCARD,
                KEY_SDCARD_DEVICE_NAME, buf, sizeof(buf));
    if (ret != 0) {
        strncpy(name, SDCARD_DEVICE_NAME, strlen(SDCARD_DEVICE_NAME) + 1);
        return;
    }

    strncpy(name, buf, strlen(buf) + 1);
    return;
}

/*
 * decode RTA, save into tb
 */
static void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr, int len)
{
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type <= max) {
            tb[attr->rta_type] = attr;
        }
    }
}

/*
 * parse link information
 * triggered when netinterface link state changed
 * like add/del Ethertnet cable, add/del NIC,
 * enable/disable netinterface etc.
 */
static void parse_ifinfomsg(struct nlmsghdr *nlh, hw_eth_event* event)
{
    int len;
    struct rtattr *tb[IFLA_MAX + 1];
    struct ifinfomsg *ifinfo;

    bzero(tb, sizeof(tb));
    ifinfo = NLMSG_DATA(nlh);
    len = IFLA_PAYLOAD(nlh);

    parse_rtattr(tb, IFLA_MAX, IFLA_RTA (ifinfo), len);

    event->type = nlh->nlmsg_type;
    event->device_state = (ifinfo->ifi_flags & IFF_UP);
    if (tb[IFLA_IFNAME]) {
        strncpy(event->device_name, RTA_DATA(tb[IFLA_IFNAME]),
			strlen(RTA_DATA(tb[IFLA_IFNAME])) + 1);
    }
}

/*
 * parse IP address information
 * triggered when IP address changed
 */
static void parse_ifaddrmsg(struct nlmsghdr *nlh, hw_eth_event* event)
{
    int len;
    struct rtattr *tb[IFA_MAX + 1];
    struct ifaddrmsg *ifaddr;

    bzero(tb, sizeof(tb));
    ifaddr = NLMSG_DATA(nlh);
    len = IFA_PAYLOAD(nlh);

    parse_rtattr(tb, IFA_MAX, IFA_RTA (ifaddr), len);
    event->type = nlh->nlmsg_type;
    event->device_state = (ifaddr->ifa_flags & IFF_UP);
    if (tb[IFA_LABEL]) {
        strncpy(event->device_name, RTA_DATA(tb[IFA_LABEL]),
			strlen(RTA_DATA(tb[IFA_LABEL])) + 1);
    }
}

static int hw_event_filter(int type, char *msg, hw_event* event)
{
    char dev_name[256] = {0};

    if (IS_BATTERY_EVENT(type)) {
	get_battery_device_name(dev_name);
        if (strstr(msg, dev_name)) {
            event->type = BATTERY_EVENT;
            return 1;
        }
    }

    if (IS_USB_POWER_EVENT(type)) {
        if (strstr(msg, USB_POWER_DEVICE_NAME)) {
            event->type = USB_POWER_EVENT;
            return 1;
        }
    }

    if (IS_SDCARD_EVENT(type)) {
        get_sdcard_device_name(dev_name);
        if (strstr(msg, dev_name)) {
            event->type = SDCARD_EVENT;
            return 1;
        }
    }

    if (IS_USB_STORAGE_EVENT(type)) {
        get_usb_storage_device_name(dev_name);
        if (strstr(msg, dev_name)) {
            event->type = USB_STORAGE_EVENT;
            return 1;
        }
    }

    if (IS_ETH_EVENT(type)) {
        event->type = ETH_EVENT;
        struct nlmsghdr* nlh = (struct nlmsghdr*)msg;
        do {
            switch (nlh->nlmsg_type) {
                case RTM_NEWLINK:
                case RTM_DELLINK:
                case RTM_GETLINK:
                    return 1;
                case RTM_NEWADDR:
                case RTM_DELADDR:
                case RTM_GETADDR:
                    return 1;
                default:
                    return 0;
            }
        } while (NLMSG_OK(nlh, event->msg_len));
    }

    return 0;
}

/* we try to walk through all the file in path and write all uevent */
static void _do_uevent_coldboot(DIR *d)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);
    fd = openat(dfd, "uevent", O_WRONLY);
    if (fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while ((de = readdir(d))) {
        DIR *d2;
        if (de->d_type != DT_DIR || de->d_name[0] == '.')
            continue;
        //fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        fd = openat(dfd, de->d_name, O_RDONLY);
        if (fd < 0)
            continue;
        d2 = fdopendir(fd);
        if (d2 == 0) {
            close(fd);
        } else {
            _do_uevent_coldboot(d2);
            closedir(d2);
        }
    }
}

static int do_uevent_coldboot(const char *path)
{
    DIR *d = opendir(path);
    if (d) {
        _do_uevent_coldboot(d);
        closedir(d);

        return 0;
    }

    return -EINVAL;
}

static int do_eth_coldboot(int family, int type, int sock_fd)
{
    int ret;
    struct {
        struct nlmsghdr n;
        struct ifaddrmsg i;
    } req;

    if (sock_fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.n.nlmsg_type = RTM_GETADDR;
    req.i.ifa_family = AF_INET;

    struct nlmsghdr *nh = &req.n;
    struct sockaddr_nl nladdr = { .nl_family = AF_NETLINK };
    struct iovec iov = {
        .iov_base = nh,
        .iov_len = nh->nlmsg_len,
    };

    nh->nlmsg_seq = 1;
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    ret = sendmsg(sock_fd, &msg, 0);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "%s sendmsg failed: %s", __func__, -errno);
        return -errno;
    }

    return 0;
}

static int generate_dynamic_uevent_path(char* dir_path, int dir_len,
		char* device_name, int device_name_len, char* path)
{
    int ret = -ENOENT;
    struct dirent* file;

    DIR* dir = opendir(dir_path);
    if (dir != NULL) {
        /* try to open all file in dir */
        while ((file = readdir(dir))) {
            if (strstr(file->d_name, device_name)) {
                snprintf(path, dir_len+sizeof(file->d_name), "%s/%s", dir_path,
				file->d_name);
                ret = 0;
                break;
            }
        }
    }

    closedir(dir);
    return ret;
}

static int hw_coldboot(hw_event_listener* listener)
{
    int ret = 0;
    int failed = 0;
    char uevent_path[512] = {0};

    if (IS_BATTERY_EVENT(listener->type)) {
        ret = do_uevent_coldboot(BATTERY_UEVENT_PATH);
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: battery do_hw_coldboot failed ret: %d\n",
			    __func__, ret);
            failed = 1;
        }
    }

    if (IS_USB_POWER_EVENT(listener->type)) {
        ret = do_uevent_coldboot(USB_POWER_UEVENT_PATH);
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: usb_power do_hw_coldboot failed ret: %d\n",
			    __func__, ret);
            failed = 1;
        }
    }

    if (IS_USB_STORAGE_EVENT(listener->type)) {
        memset(uevent_path, 0, sizeof(uevent_path));
        ret = get_usb_storage_uevent_path(uevent_path);
        if (ret != 0) {
            ret = generate_dynamic_uevent_path(USB_STORAGE_UEVENT_PATH,
                sizeof(USB_STORAGE_UEVENT_PATH), USB_STORAGE_DEVICE_PATH,
                sizeof(USB_STORAGE_DEVICE_PATH), uevent_path);
            if (ret < 0) {
                hal_log(HAL_LOG_ERR, "%s: usb_storage dynamic path failed ret: %d\n",
                    __func__, ret);
                failed = 1;
            }
        }

        ret = do_uevent_coldboot(uevent_path);
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: usb_storage do_hw_coldboot failed ret: %d\n",
			    __func__, ret);
            failed = 1;
        }
    }

    if (IS_SDCARD_EVENT(listener->type)) {
        memset(uevent_path, 0, sizeof(uevent_path));
        get_sdcard_uevent_path(uevent_path);
        ret = do_uevent_coldboot(uevent_path);
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: sdcard do_hw_coldboot failed ret: %d\n",
                __func__, ret);
            failed = 1;
        }
    }

    if (IS_ETH_EVENT(listener->type)) {
        ret = do_eth_coldboot(AF_INET, RTM_GETADDR, listener->eth_fd);
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: eth do_hw_coldboot failed ret: %d\n",
			    __func__, ret);
            failed = 1;
        }
    }

    if (failed)
        ret = -EPERM;

    return ret;
}

static int init_uevent_socket(hw_event_listener* listener)
{
    struct sockaddr_nl addr;
    int sz = 10 * 1024; /* we need a big buffer for udev event */
    int fd;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    /* set to 0 to let kernel take care the ID */
    addr.nl_pid = 0;
    addr.nl_groups = 0xffffffff;

    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open socket failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        hal_log(HAL_LOG_ERR, "%s: bind socket failed errno:%d\n", __func__, -errno);
        close(fd);
        return -errno;
    }

    listener->uevent_fd = fd;

    return 0;
}

static int init_eth_event_socket(hw_event_listener* listener)
{
    struct sockaddr_nl addr;
    int fd;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    /* set to 0 to let kernel take care the ID */
    addr.nl_pid = 0;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

    fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open socket failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        hal_log(HAL_LOG_ERR, "%s: bind socket failed errno:%d\n", __func__, -errno);
        close(fd);
        return -errno;
    }

    listener->eth_fd = fd;

    return 0;
}

int hal_hw_event_register(hw_event_listener* listener, unsigned int type)
{
    int ret = 0;
    int type_vaild = 0;

    if (!listener || !type) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    /* clean the data of listener */
    listener->uevent_fd = 0;
    listener->eth_fd = 0;
    listener->type = 0;

    if (IS_BATTERY_EVENT(type) | IS_USB_POWER_EVENT(type) | IS_SDCARD_EVENT(type) |
		    IS_USB_STORAGE_EVENT(type)) {
        ret = init_uevent_socket(listener);
        if (ret < 0) {
           hal_log(HAL_LOG_ERR, "%s: init_uevent_socket failed errno:%d\n",
			   __func__, ret);
           return ret;
        }

        type_vaild = 1;
    }

    if (IS_ETH_EVENT(type)) {
        ret = init_eth_event_socket(listener);
        if (ret < 0) {
           hal_log(HAL_LOG_ERR, "%s: init_eth_socket failed errno:%d\n",
			   __func__, ret);
           return ret;
        }

        type_vaild = 1;
    }

    if (!type_vaild) {
        hal_log(HAL_LOG_ERR, "%s: no support event type:%d\n",__func__, type);
        return -EINVAL;
    }

    listener->type = type;
    /* coldboot for generate a event for device info init */
    ret = hw_coldboot(listener);
    if (ret < 0) {
        /* we dont' return error when cold boot failed, because
	 * maybe there's no device point */
        hal_log(HAL_LOG_ERR, "%s: do hw_coldboot failed ret:%d event_type:%d\n",
			__func__, ret, type);
    }

    return 0;
}

int hal_hw_event_recv(hw_event_listener* listener, hw_event* event,
		struct timeval* timeout)
{
    int parse_ret = 0;
    int select_ret = 0;
    ssize_t n;
    char* data;

    fd_set fds;
    int fd_size;

    struct iovec iov;
    struct sockaddr_nl snl;
    iov.iov_base = malloc(UEVENT_MSG_LEN+2);
    iov.iov_len = UEVENT_MSG_LEN+2;
    struct msghdr msg = {&snl, sizeof(snl), &iov, 1, NULL, 0, 0};
    data = (char*)msg.msg_iov->iov_base;

    if (!listener || !event) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        free(iov.iov_base);
        return -EINVAL;
    }

    do {
        /* use select to monitor event fd */
        FD_ZERO(&fds);
        /* add event fd to fd_set */
        if (listener->uevent_fd > 0)
            FD_SET(listener->uevent_fd, &fds);
        if (listener->eth_fd > 0)
            FD_SET(listener->eth_fd, &fds);

        fd_size = (listener->uevent_fd > listener->eth_fd) ?
		(listener->uevent_fd + 1) : (listener->eth_fd + 1);

        select_ret = select(fd_size, &fds, NULL, NULL, timeout);

        if (select_ret == 0) {
            free(iov.iov_base);
            return -ETIMEDOUT;
        }

        if (select_ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: select failed errno:%d\n", __func__, -errno);
            free(iov.iov_base);
            return -errno;
        }

        /* handle eth event */
        if (FD_ISSET(listener->eth_fd, &fds)) {
            n = recvmsg(listener->eth_fd, &msg, 0);
            if (!n) {
                hal_log(HAL_LOG_ERR, "%s End of file\n", __func__);
                continue;
            }
            if (n < 0) {
                hal_log(HAL_LOG_ERR, "%s: recvmsg failed errno:%d\n",
				__func__, -errno);
                free(iov.iov_base);
                return -errno;
            }

            event->msg_len = n;
            if (n > (UEVENT_MSG_LEN + 2)) {
                hal_log(HAL_LOG_ERR, "%s:eth recvmsg overflow len:%d\n", __func__, n);
                free(iov.iov_base);
                return -ENOMEM;
            }

            parse_ret = hw_event_filter(ETH_EVENT, data, event);
        /* handle uevent event */
        } else if (FD_ISSET(listener->uevent_fd, &fds)) {
            n = recvmsg(listener->uevent_fd, &msg, 0);
            if (!n) {
                hal_log(HAL_LOG_ERR, "%s End of file\n", __func__);
                continue;
            }
            if (n < 0) {
                hal_log(HAL_LOG_ERR, "%s: recvmsg failed errno:%d\n",
				__func__, -errno);
                free(iov.iov_base);
                return -errno;
            }

            if ((snl.nl_groups != 1) || (snl.nl_pid != 0)) {
                /* ignoring non-kernel netlink multicast message */
                continue;
            }

            if (n >= UEVENT_MSG_LEN) {
                continue;
            }

            /* these '\0' for msg parse in hw_parse_XXX_event */
            data[n] = '\0';
            data[n + 1] = '\0';
            n = n + 2;
            event->msg_len = n;

            parse_ret = hw_event_filter(listener->type & (~ETH_EVENT_MASK), data,
			    event);
        }
    /* parse_ret=1 match. parse_ret=0 event not match, so continue.*/
    } while (parse_ret == 0);

    memcpy(event->msg, data, n);

    free(iov.iov_base);
    return 0;
}

int hal_hw_parse_eth_event(hw_event* event, hw_eth_event_list* list)
{
    int ret = 0;
    int count = 0;
    int len = 0;
    struct nlmsghdr* nlh;
    hw_eth_event* item;

    if (!event || !list) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    nlh = (struct nlmsghdr*)event->msg;
    len = event->msg_len;
    item = list->head;

    do {
        switch (nlh->nlmsg_type) {
            case RTM_NEWLINK:
            case RTM_DELLINK:
            case RTM_GETLINK:
                parse_ifinfomsg(nlh, item);
                break;
            case RTM_NEWADDR:
            case RTM_DELADDR:
            case RTM_GETADDR:
                parse_ifaddrmsg(nlh, item);
                break;
            default:
                nlh = NLMSG_NEXT(nlh, len);
                continue;
        }
        nlh = NLMSG_NEXT(nlh, len);
        item++;
        count++;

    } while (NLMSG_OK(nlh, len) && count < list->size);

    list->count = count;

    return ret;
}

/* Explanation for hw_parse_xxx_event:
 * The uevent msg which received by NETLINK socket is different,
 * it is a long string but split by a lot of '\0', for example:
 *
 * "add@usb_device\0uevent_action=add\0POWER_SUPPLY_ONLINE=\0"
 *
 * so we add two '\0' at the end of the msg in hw_event_recv().
 * when we get one '\0' in msg, continue search. when we get two '\0', stop.
 */

int hal_hw_parse_battery_event(hw_event* event, hw_battery_event* result)
{
    char* msg = NULL;

    if (!event || !result) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    msg = event->msg;

    while (*msg) {
        if (!strncmp(msg, "POWER_SUPPLY_STATUS=",
				sizeof("POWER_SUPPLY_STATUS=") - 1)) {
            msg += sizeof("POWER_SUPPLY_STATUS=") - 1;
            strncpy(result->battery_status, msg, strlen(msg) + 1);
        } else if (!strncmp(msg, "POWER_SUPPLY_PRESENT=",
				sizeof("POWER_SUPPLY_PRESENT=") - 1)) {
            msg += sizeof("POWER_SUPPLY_PRESENT=") - 1;
            result->battery_present = atoi(msg);
        } else if (!strncmp(msg, "POWER_SUPPLY_CAPACITY=",
				sizeof("POWER_SUPPLY_CAPACITY=") - 1)) {
            msg += sizeof("POWER_SUPPLY_CAPACITY=") - 1;
            result->battery_capacity = atoi(msg);
        } else if (!strncmp(msg, "POWER_SUPPLY_HEALTH=",
				sizeof("POWER_SUPPLY_HEALTH=") - 1)) {
            msg += sizeof("POWER_SUPPLY_HEALTH=") - 1;
            strncpy(result->battery_health, msg, strlen(msg) + 1);
        } else if (!strncmp(msg, "POWER_SUPPLY_TEMP=",
				sizeof("POWER_SUPPLY_TEMP=") - 1)) {
            msg += sizeof("POWER_SUPPLY_TEMP=") - 1;
            result->battery_temperature = atoi(msg);
        }
        /* we have already add two '\0' at the end of msg */
        while (*msg++);
    }

    return 0;
}

int hal_hw_parse_usb_power_event(hw_event* event, hw_usb_power_event* result)
{
    char* msg = NULL;

    if (!event || !result) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    msg = event->msg;

    while (*msg) {
        if (!strncmp(msg, "POWER_SUPPLY_ONLINE=",
				sizeof("POWER_SUPPLY_ONLINE=") - 1)) {
            msg += sizeof("POWER_SUPPLY_ONLINE=") - 1;
            result->power_online = atoi(msg);
        }

        /* we have already add two '\0' at the end of msg */
        while (*msg++);
    }

    return 0;
}

int hal_hw_parse_sdcard_event(hw_event* event, hw_sdcard_event* result)
{
    char* msg = NULL;

    if (!event || !result) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    msg = event->msg;

    while (*msg) {
        if (!strncmp(msg, "ACTION=", sizeof("ACTION=") - 1)) {
            msg += sizeof("ACTION=") - 1;
            if (!strncmp(msg, "add", sizeof("add") - 1)) {
                result->sd_plug = 1;
            } else if (!strncmp(msg, "remove", sizeof("remove") - 1)) {
                result->sd_plug = 0;
            } else {
                hal_log(HAL_LOG_ERR, "%s: invalid sdcard event:%s\n", __func__, msg);
                return -EINVAL;
            }
        } else if (!strncmp(msg, "DEVNAME=", sizeof("DEVNAME=") - 1)) {
            msg += sizeof("DEVNAME=") - 1;
            strncpy(result->dev_name, msg, strlen(msg) + 1);
        } else if (!strncmp(msg, "DEVTYPE=", sizeof("DEVTYPE=") - 1)) {
            msg += sizeof("DEVTYPE=") - 1;
            strncpy(result->dev_type, msg, strlen(msg) + 1);
        }

        /* we have already add two '\0' at the end of msg */
        while (*msg++);
    }

    return 0;
}

int hal_hw_parse_usb_storage_event(hw_event* event, hw_usb_storage_event* result)
{
    char* msg = NULL;

    if (!event || !result) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    msg = event->msg;

    while (*msg) {
        if (!strncmp(msg, "ACTION=", sizeof("ACTION=") - 1)) {
            msg += sizeof("ACTION=") - 1;
            if (!strncmp(msg, "add", sizeof("add") - 1)) {
                result->usb_storage_plug = 1;
            } else if (!strncmp(msg, "remove", sizeof("remove") - 1)) {
                result->usb_storage_plug = 0;
	    } else {
                hal_log(HAL_LOG_ERR, "%s: invalid usb storage event:%s\n",
				__func__, msg);
                return -EINVAL;
            }
        } else if (!strncmp(msg, "DEVNAME=", sizeof("DEVNAME=") - 1)) {
            msg += sizeof("DEVNAME=") - 1;
            strncpy(result->dev_name, msg, strlen(msg) + 1);
        } else if (!strncmp(msg, "DEVTYPE=", sizeof("DEVTYPE=") - 1)) {
            msg += sizeof("DEVTYPE=") - 1;
            strncpy(result->dev_type, msg, strlen(msg) + 1);
        }

        /* we have already add two '\0' at the end of msg */
        while (*msg++);
    }

    return 0;
}

int hal_hw_event_unregister(hw_event_listener* listener)
{
    if (!listener) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    if (listener->uevent_fd > 0)
        close(listener->uevent_fd);

    if (listener->eth_fd > 0)
        close(listener->eth_fd);

    return 0;
}
