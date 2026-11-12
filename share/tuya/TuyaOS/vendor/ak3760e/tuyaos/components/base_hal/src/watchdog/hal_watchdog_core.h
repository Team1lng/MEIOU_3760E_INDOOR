#ifndef __HAL_WATCHDOG_CORE_H__
#define __HAL_WATCHDOG_CORE_H__

/* HAL interface defination */
typedef struct {
	int (*open)(void);
	int (*close)(int fd);
	int (*keepalive)(int fd);
	int (*set_timeout)(int fd, int *timeout);
	int (*get_timeout)(int fd, int *timeout);
	int (*get_timeleft)(int fd, int *timeleft);
} hal_if_watchdog_t;

extern hal_if_watchdog_t hal_if_watchdog;

int hal_watchdog_open_common(void);
int hal_watchdog_close_common(int fd);
int hal_watchdog_keepalive_common(int fd);
int hal_watchdog_set_timeout_common(int fd, int* timeout);
int hal_watchdog_get_timeout_common(int fd, int* timeout);
int hal_watchdog_get_timeleft_common(int fd, int* timeleft);

#endif
