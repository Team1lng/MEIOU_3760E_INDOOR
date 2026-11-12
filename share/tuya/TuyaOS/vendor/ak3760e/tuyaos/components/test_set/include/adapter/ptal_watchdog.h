#ifndef __PTAL_WATCHDOG_H__
#define __PTAL_WATCHDOG_H__

#ifdef __cplusplus
extern "C" { 
#endif

extern int ptal_watchdog_init(int ms);
extern int ptal_watchdog_uinit(void);
extern int ptal_watchdog_feed(void);

#ifdef __cplusplus
}
#endif

#endif

