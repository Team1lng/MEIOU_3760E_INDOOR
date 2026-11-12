/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/reboot.h>
#include "tb_log.h"
#include "product_test_utils.h"

int product_test_reboot(int _sync)
{
    if(_sync) 
        sync();

    TB_LOGD("reboot ...");
    reboot(RB_AUTOBOOT);
    return  0;
}
