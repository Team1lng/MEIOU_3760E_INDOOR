#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "product_test_utils.h"
#include "product_test_usb.h"
#include "tb_log.h"

#define USB_COMMAND  "lsusb"

int __attribute__((weak)) ptal_usb_get_usb_info(void *param) 
{
    int i;
    char buf[4096] = {0};
    char *p;
    uint32_t vendor_id,device_id;
    int n,cnts = 0; 
    USB_INFO_T *info = (USB_INFO_T *) param;

    if(info == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    if(product_test_exec_cmd_with_return(USB_COMMAND,buf,4096)) {
        TB_LOGE("invalid parm");
        return -1;
    }

    TB_LOGD("%s",buf);

    n = strlen(buf) +1;
    p = &buf[0];

    for (i= 0;i < n;i++) 
    {
        char *at;
        
        if(buf[i] != '\n' && buf[i] != '\0') {
            continue;
        }

        buf[i] = 0;

        at = strstr(p,"ID ");

        if(at) {
            TB_LOGD("%s",at);
            sscanf(at,"ID %x:%x",&vendor_id,&device_id);
        }

        TB_LOGD("vendor id:%x,device_id:%x\n",vendor_id,device_id);

        info->usb[cnts].vendor_id = vendor_id;
        info->usb[cnts].device_id = device_id;
        cnts ++;
        info->n = cnts;

        p = &buf[i+1];
    }

    TB_LOGD(" detect %d usb  device",info->n);
    

    return 0;
}

