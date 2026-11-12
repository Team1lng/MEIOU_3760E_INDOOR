#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include "tb_log.h"
#include "ptal_general_download.h"

int __attribute__((weak)) ptal_general_download_file_analysis(char *file,char *type)
{
    TB_LOGD();
    return 0;
}


int  ptal_read_acoustics_setting(char *acoustics_setting, char *acoustics_setting2)
{

    TB_LOGD();
    return -1;
}

int  ptal_write_acoustics_setting(char *acoustics_setting)
{

    TB_LOGD();
    return -1;
}
