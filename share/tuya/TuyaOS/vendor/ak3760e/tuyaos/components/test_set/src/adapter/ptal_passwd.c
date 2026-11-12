#define  _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "product_test_passwd.h"
#include "product_test_utils.h"
#include "tb_log.h"

#define TY_SALT_MAX_LEN     8

#define TY_CHECK_VALID(c) \
        (((c) >= '0' && (c) <= '9') \
            || ((c) >= 'A' && (c) <= 'Z') \
            || ((c) >= 'a' && (c) <= 'z'))


static int ty_passwd_get_salt(char* puuid, char* psalt)
{
    int i = 0, j = 0;
    int len = strlen(puuid);
    char temp[TY_SALT_MAX_LEN + 1] = {0};

    for(i = len - 1; i >= 0; i--) {
        if(TY_CHECK_VALID(puuid[i])) {
            temp[j++] = puuid[i];
        }

        if(j >= TY_SALT_MAX_LEN) {
            break;
        }
    }

    if(j <= 0) {
        return -1;
    }

    while(j < TY_SALT_MAX_LEN) {
        temp[j] = 'm';
        j++;
    }

    sprintf(psalt, "$1$%s$", temp);

    return 0;
}

static int ty_passwd_get_key(char* pauthKey, char* pkey)
{
    int i = 0, j = 0;
    int len = strlen(pauthKey);

    for(i = 0; i < len; i++) {
        if(TY_CHECK_VALID(pauthKey[i])) {
            pkey[j++] = pauthKey[i];
        }
    }

    if(j <= 0) {
        return -1;
    }

    return 0;
}

static int ty_passwd_update(char* path, char* puuid, char* pauthKey)
{
    char* passwd;
    char salt[TY_SALT_MAX_LEN + 4 + 1] = {0};
    char key[128] = {0};

    if(puuid == NULL || strlen(puuid) <= 0
        || pauthKey == NULL || strlen(pauthKey) <= 0) {
        return -1;
    }

    if(ty_passwd_get_salt(puuid, salt) < 0) {
        return -1;
    }

    if(ty_passwd_get_key(pauthKey, key) < 0) {
        return -1;
    }

    passwd = crypt((const char*)key, (const char*)salt);

    if(passwd == NULL) {
        return -1;
    }

    if(strlen(path) > 0) {
        char cmd[128] = {0};

        sprintf(cmd, "echo root:\'%s\':15874:0:99999:7::: > %s",
                    passwd, path);
        product_test_exec_cmd_no_return(cmd);
    } else {
        printf("%s\n", passwd);
    }

    return 0;
}
#if 0
int main(int argc, char*argv[])
{
    int res = -1;
    char uuid[128] = {0};
    char authKey[128] = {0};
    char filePath[64] = {0};

    while((res = getopt(argc, argv, "?u:a:f:h")) != -1) {
        switch(res) {
        case 'u':
            strcpy(uuid, optarg);
            break;

        case 'a':
            strcpy(authKey, optarg);
            break;

        case 'f':
            strcpy(filePath, optarg);
            break;

        case 'h':
        default:
            return -1;
        }
    }

    if(ty_passwd_update(filePath, uuid, authKey) < 0) {
        return -1;
    }

    return 0;
}
#endif

int __attribute__((weak)) ptal_passwd_set(char *uuid,char *authkey) 
{
    char path[128] ={0};

    if(product_test_get_passwd_path(path) || strlen(path) == 0) {
        TB_LOGE("passwd path get fail");
        return -1;  
    }

    if(ty_passwd_update(path,uuid,authkey) < 0) {
        TB_LOGE("passwd update fail");
         return -1;
    }

    return 0;
}

int __attribute__((weak)) ptal_passwd_reset(void)
{
    char cmd[128] = {0};
    TB_LOGD();
    sprintf(cmd,"rm -rf %s/%s","/etc","shadow");
    return product_test_exec_cmd_no_return(cmd);
}



