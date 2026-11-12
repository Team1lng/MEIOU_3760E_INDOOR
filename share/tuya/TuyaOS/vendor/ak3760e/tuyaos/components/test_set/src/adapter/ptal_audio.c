#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include "ptal_audio.h"
#include "tb_log.h"

int __attribute__((weak)) ptal_test_audio_test(void)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_swept_sine_start(int num)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_swept_over(void)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_read_swept_sine_result(void)
{
    TB_LOGD();
    return -1;
}
