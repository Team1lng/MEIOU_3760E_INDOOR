#ifndef _PTAL_PASSWD_H_
#define _PTAL_PASSWD_H_

#ifdef __cplusplus__
extern "C" {
#endif
int ptal_passwd_set(char *uuid,char *authkey);
int ptal_passwd_reset(void);

#ifdef __cplusplus__
}
#endif
#endif
