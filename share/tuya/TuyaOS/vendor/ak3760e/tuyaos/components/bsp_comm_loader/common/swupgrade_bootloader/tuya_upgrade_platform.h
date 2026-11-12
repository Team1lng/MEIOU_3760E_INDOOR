/*
 * (C) Copyright 2020
 * Cheng Lei, Tuya Software Engineering, chenglei@tuya.com.
 */

#ifndef tuya_upgrade_platform__h
#define tuya_upgrade_platform__h
 
#define PROP_MAX_LEN	256
#define TRACE		printf
#define ERROR		printf

int ty_prop_get(const char *key, char *value);

int ty_prop_set(const char *key, const char *value);

int ty_flash_read(int offs, int len, char **buf);

int ty_flash_write(char *buf, int len, int offs);

int ty_upgrade_progress(int status);

#endif
