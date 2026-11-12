#ifndef __UTILS_H__
#define __UTILS_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint64_t get_monotonic_time_ms();

int md5_encode(char *input, int len, char* output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __UTILS_H__

