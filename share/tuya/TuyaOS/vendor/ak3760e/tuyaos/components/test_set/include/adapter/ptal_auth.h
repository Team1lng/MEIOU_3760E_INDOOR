#ifndef __PTAL_AUTH_H__
#define __PTAL_AUTH_H__

#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_get_auth_info(char *pid,char *uuid,char *auzkey);

extern int ptal_set_auth_info(char *pid,char *uuid,char *auzkey);

#ifdef __cplusplus__
}
#endif

#endif

