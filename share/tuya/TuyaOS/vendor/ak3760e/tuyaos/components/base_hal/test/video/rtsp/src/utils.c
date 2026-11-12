#include <time.h>
#include <string.h>
#include <stdio.h>

#include "uni_md5.h"
#include "utils.h"

uint64_t get_monotonic_time_ms()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int md5_encode(char *input, int len, char* output)
{
    UNI_MD5_CTX_S ctx;
    unsigned char tmp[16];
    
    uni_md5_init(&ctx);
    uni_md5_update(&ctx, (unsigned char*)input, len);
    uni_md5_final(&ctx, tmp);

    int i;
    for(i = 0; i < 16; i++){
        sprintf(output + i * 2, "%02x", tmp[i]);
    }
    output[32] = '\0';

    return 0;
}