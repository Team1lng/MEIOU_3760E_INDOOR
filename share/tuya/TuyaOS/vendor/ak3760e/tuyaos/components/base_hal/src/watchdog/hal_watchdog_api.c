#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/watchdog.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_watchdog.h"
#include "hal_watchdog_core.h"

int hal_watchdog_open()
{
	if (NULL == hal_if_watchdog.open)
		return hal_watchdog_open_common();

	return hal_if_watchdog.open();
}

int hal_watchdog_close(int dev_fd)
{
	if (NULL == hal_if_watchdog.close)
		return hal_watchdog_close_common(dev_fd);

	return hal_if_watchdog.close(dev_fd);
}

int hal_watchdog_keepalive(int dev_fd)
{
	if (NULL == hal_if_watchdog.keepalive)
		return hal_watchdog_keepalive_common(dev_fd);

	return hal_if_watchdog.keepalive(dev_fd);
}

int hal_watchdog_set_timeout(int dev_fd, int* timeout)
{
	if (NULL == hal_if_watchdog.set_timeout)
		return hal_watchdog_set_timeout_common(dev_fd, timeout);

	return hal_if_watchdog.set_timeout(dev_fd, timeout);
}

int hal_watchdog_get_timeout(int dev_fd, int* timeout)
{
	if (NULL == hal_if_watchdog.get_timeout)
		return hal_watchdog_get_timeout_common(dev_fd, timeout);

	return hal_if_watchdog.get_timeout(dev_fd, timeout);
}

int hal_watchdog_get_timeleft(int dev_fd, int* timeleft)
{
	if (NULL == hal_if_watchdog.get_timeleft)
		return hal_watchdog_get_timeleft_common(dev_fd, timeleft);

	return hal_if_watchdog.get_timeleft(dev_fd, timeleft);
}
