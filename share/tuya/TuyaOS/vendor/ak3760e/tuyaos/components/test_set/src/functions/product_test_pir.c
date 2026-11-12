/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <inttypes.h>
#include "product_test_sub_mcu.h"

// async or sync
int product_test_pir_get_awake_times(uint32_t *times)
{
    return 0;
}

int product_test_pir_test(int counts) 
{
    return product_test_sub_mcu_pir_test(counts); 
}
