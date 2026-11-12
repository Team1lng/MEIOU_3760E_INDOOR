#include <stdarg.h>
#ifdef HAL_SYS_LOG
#include <syslog.h>
#else
#include <stdio.h>
#endif
#include "hal_log.h"

void hal_log(int level, const char* format, ...)
{
	va_list args;
	va_start(args, format);

#ifdef HAL_SYS_LOG
	vsyslog(level, format, args);
#else
	vprintf(format, args);
#endif
	va_end(args);
}

void hal_log_open(const char *ident, int option, int facility)
{
#ifdef HAL_SYS_LOG
	openlog(ident, option, facility);
#endif
}

void hal_log_close(void)
{
#ifdef HAL_SYS_LOG
	closelog();
#endif
}
