/*!
 * @file
 * @brief hal watchdog interface
 */

#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

/**
 * @brief open and enable watchdog device
 * @return success return device fd, failed return -errno
 * @note after hal_watchdog_open(), the watchdog is started.
 *  if you don't call hal_watchdog_close() before you process exit
 *  the watchdog device will reboot the system
 */
extern int hal_watchdog_open();

/**
 * @brief close and disable watchdog device
 * @return success return 0, failed return -errno
 * @param device fd of watchdog
 * @note we should not use this func in most of time.
 */
extern int hal_watchdog_close(int dev_fd);

/**
 * @brief feed the watchdog, keep the system alive
 * @return success return 0, failed return -errno
 * @param device fd of watchdog
 * @note should call this func before watchdog timeout
 */
extern int hal_watchdog_keepalive(int dev_fd);

/**
 * @brief set watchdog timeout
 * @param device fd of watchdog
 * @param timeout time of timeout
 * @return success return 0, failed return -errno
 * @note param timeout is a pointer!, func will rewrite the real take effect
 * time in param, if you set it over the MAX/MIN time
 */
extern int hal_watchdog_set_timeout(int dev_fd, int* timeout);

/**
 * @brief get watchdog timeout
 * @param device fd of watchdog
 * @param timeout current device timeout
 * @return success return 0, failed return -errno
 */
extern int hal_watchdog_get_timeout(int dev_fd, int* timeout);

/**
 * @brief get watchdog time left
 * @param device fd of watchdog
 * @param timeleft current time left
 * @return success return 0, failed return -errno
 */
extern int hal_watchdog_get_timeleft(int dev_fd, int* timeleft);

#endif
