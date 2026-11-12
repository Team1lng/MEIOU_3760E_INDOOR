#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include "tkl_adc.h"
#include "tb_log.h"

int __attribute__((weak))ptal_adc_get_value(int n,uint32_t *value)
{
    int iret= -1;
    UINT32_T port;
    UINT_T data;
    OPERATE_RET ret;

    port = (UINT32_T)n;

    tkl_adc_init(port,NULL);

    ret = tkl_adc_read_data(port, (INT_T *)&data, sizeof(data));

    if (ret != OPRT_OK) {
        TB_LOGE("error: tkl_adc_read_data failed\n");
    } else {
        iret = 0;
    }

    tkl_adc_deinit(port);

    *value = data; 

    TB_LOGD("dev %d,chan %d,value:%d",0,n,*value);

    return iret;
}
