#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "tb_log.h"
#include "product_test_config.h"
#include "product_test_utils.h"

#define CFG_UUID_KEY "UUID"
#define CFG_AUZKEY_KEY "AUTHKEY"
#define CFG_PID_KEY "PID"

int __attribute__((weak)) ptal_set_auth_info(char *pid,char *uuid,char *auzkey)
{
    int iret = 0;

    if(pid != NULL && strlen(pid)) {
         iret |= product_test_utils_write_string(CFG_PID_KEY,pid,1);
    }

    if(uuid != NULL && strlen(uuid)) {
         iret |= product_test_utils_write_string(CFG_UUID_KEY,uuid,1);
    }

    if(auzkey != NULL && strlen(auzkey)) {
         iret |= product_test_utils_write_string(CFG_AUZKEY_KEY,auzkey,1);
    }

    return iret;
}


int __attribute__((weak))  ptal_get_auth_info(char *pid,char *uuid,char *auzkey)
{
    int iret = 0;

    iret |= product_test_utils_read_string(CFG_PID_KEY,pid);
    iret |= product_test_utils_read_string(CFG_UUID_KEY,uuid);
    iret |= product_test_utils_read_string(CFG_AUZKEY_KEY,auzkey);

    return iret;
}
