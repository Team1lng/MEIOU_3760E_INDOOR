/*!
 * @file
 * @brief hal log interface
 */
#ifndef HAL_LOG_H
#define HAL_LOG_H

/* log level define */
#define HAL_LOG_EMERG       0       /* system is unusable */
#define HAL_LOG_ALERT       1       /* action must be taken immediately */
#define HAL_LOG_CRIT        2       /* critical conditions */
#define HAL_LOG_ERR         3       /* error conditions */
#define HAL_LOG_WARNING     4       /* warning conditions */
#define HAL_LOG_NOTICE      5       /* normal but significant condition */
#define HAL_LOG_INFO        6       /* informational */
#define HAL_LOG_DEBUG       7       /* debug-level messages */

/**
 * @brief log record interface
 * @param level log level
 * @param format string
 * @param ... var_list
 * @return none
 */
extern void hal_log(int level, const char* format, ...);

/**
 * @brief open log record interface
 * @param ident is prepended to every message, if ident is NULL, the program name is used
 * @param option a bit mask constructed by ORing, detail see syslog man-page
 * @param facility is used to specify what type of program is logging the message
 * @return none
 * @note The use of hal_log_open() is optional; it will automatically be called by
 *  hal_log() if necessary, in which case ident will default to NULL.
 */
extern void hal_log_open(const char *ident, int option, int facility);

/**
 * @brief close log fd
 * @return none
 * @note hal_log_close() closes the file descriptor being used to write to the
 *  system logger. The use of hal_log_close() is optional.
 */
extern void hal_log_close(void);

#endif
