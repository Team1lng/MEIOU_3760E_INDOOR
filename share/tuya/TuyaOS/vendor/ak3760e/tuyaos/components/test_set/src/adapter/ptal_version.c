#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ptal_sub_mcu.h"
#include "product_test_product_info.h"
#include "tb_log.h"

int __attribute__((weak))  ptal_get_version_info(char *buf,int size)
{
    int i;
    int mode;
    FILE *fp;
    char buffer[256]={0};
    char *str = NULL;
    char version_path[128] = {0};

    TB_LOGD();

    if(product_test_get_version_path(version_path) || strlen(version_path) == 0) {
        TB_LOGE("not find version path");
        return -1;  
    }

    fp = fopen(version_path,"rb");

    if(fp) {

        fread(buffer,1,sizeof(buffer),fp);
        fclose(fp);
        TB_LOGD("info:%s\n",buffer);
        str = strstr(buffer,"=");
        if(str == NULL) {
            TB_LOGE("no find version info\n");
            return -1;
        }

        str++;

        for(i=0;i<strlen(str);i++) {
            if(str[i] == '\n')
                str[i] = '\0';
        }

        TB_LOGD( "%s\n",str);

        if(product_test_get_low_power_mode(&mode) == 0 && mode ==1) {
            int ver_1 = 0;
            int ver_2 = 0;
            int ver_3 = 0;
            unsigned int sub_ver = 0;

            sscanf(str, "%d.%d.%d",&ver_1,&ver_2,&ver_3);
            TB_LOGD("ver [%d-%d-%d]\n",ver_1,ver_2,ver_3);

            if(ptal_sub_mcu_get_version(&sub_ver)) {
                printf("fail get sub version\n");   
                return -1;  
            }

            ver_3 = (sub_ver<<5)|ver_3;
            TB_LOGD("sub mcu ver:%d,ver_3:%d\n",sub_ver,ver_3);
            memset(buffer,0,sizeof(buffer));
            snprintf(buffer,sizeof(buffer),"%d.%d.%d",ver_1,ver_2,ver_3);
            TB_LOGW("VERSION=%s\n",buffer);
            strncpy(buf,buffer,size);
        }else{
            TB_LOGW("VERSION=%s\n",str);
            strncpy(buf,str,size);
        }

    }else {
        TB_LOGE("can't open %s\n",version_path);
        return -1;
    }

    return 0;
}
