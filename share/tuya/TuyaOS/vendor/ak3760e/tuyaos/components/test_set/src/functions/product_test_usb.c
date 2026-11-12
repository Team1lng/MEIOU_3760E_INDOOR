/* 
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "product_test_utils.h"
#include "product_test_usb.h"
#include "ptal_usb.h"
#include "tb_log.h"

int product_usb_get_usb_info(USB_INFO_T *info)
{
    return ptal_usb_get_usb_info(info); 
}
