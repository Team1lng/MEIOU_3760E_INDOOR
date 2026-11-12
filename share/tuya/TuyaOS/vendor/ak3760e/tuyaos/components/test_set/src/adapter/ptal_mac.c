#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "tb_log.h"
#include "product_test_config.h"
#include "product_test_utils.h"

#define CFG_MASTER_MAC_KEY "master_mac"
#define CFG_SLAVE_MAC_KEY "slave_mac%d"


int __attribute__((weak)) ptal_set_mmac(char *mac)
{
    return product_test_utils_write_string(CFG_MASTER_MAC_KEY,mac,1);
}
int __attribute__((weak)) ptal_get_mmac(char *mac)
{
    return product_test_utils_read_string(CFG_MASTER_MAC_KEY,mac);
}

int __attribute__((weak)) ptal_set_smac(int cpu,char *mac)
{
    char key[64] = {0};
    sprintf(key,CFG_SLAVE_MAC_KEY,cpu);

    return product_test_utils_write_string(key,mac,1);
}

int __attribute__((weak)) ptal_get_smac(int cpu,char *mac)
{
    char key[64] = {0};
    sprintf(key,CFG_SLAVE_MAC_KEY,cpu);

    return product_test_utils_read_string(key,mac);
}
