#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include "tb_log.h"
#include "bcmnvram.h"
#include "product_test_utils.h"

int  product_test_utils_read_string(char *key,char *value)
{

    if(key ==NULL || value == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    if(nvram_get(key,value) != 0) {
        TB_LOGE("nvram get %s fail",key);
        return -1;
    }

    TB_LOGD("key:%s,value:%s\n",key,value);

    return 0;
}

int product_test_utils_write_string(char *key,char *value,int mode)
{
    if(key == NULL || value  == NULL) {
        TB_LOGE("invalid parm");
        return -1;      
    }

    if(!strlen(value)) {
        TB_LOGD("key:%s,value is null",key);
        return 0;   
    }
    

    if(mode == ePRODUCT_WRITE_PERSIST) {
        TB_LOGD("key:%s,value:%s(%d),mode:%d\n",key,value,strlen(value),mode);
        if(nvram_set(key,value) != 0) {
            TB_LOGE("nvram set %s fail",key);
            return -1;
        }
        nvram_commit(); 
    }else {
        TB_LOGD("temp key:%s,value:%s(%d),mode:%d\n",key,value,strlen(value),mode);
        if(nvram_set_with_attr(key,value,ATTR_TEMP) != 0) {
            TB_LOGE("nvram set temp %s fail",key);
            return -1;
        }
    }

    return 0;
}

int product_test_utils_read_int(char *key,int *value)
{
    int iret = -1;
    char buf[32] = {0};


    if(nvram_get(key,buf) != 0) {
        TB_LOGE("nvram get %s fail",key);
        *value = 0;
        return -1;
    }

    TB_LOGD("key:%s,value:%s\n",key,buf);

    if(strlen(buf)) {
        *value = atoi(buf);
        iret = 0;
    }

    return iret;
}

int product_test_utils_write_int(char *key,int value,int mode)
{
    int iret = 0;
    char buf[32] = {0};

    sprintf(buf,"%d",value);

    if(mode == ePRODUCT_WRITE_PERSIST) {
        TB_LOGD("key:%s,value:%s(%d),mode:%d\n",key,buf,strlen(buf),mode);
        if(nvram_set(key,buf) != 0) {
            TB_LOGE("nvram set %s fail",key);
            return -1;
        }
        nvram_commit(); 
    }else {
        TB_LOGD("temp key:%s,value:%s(%d),mode:%d\n",key,buf,strlen(buf),mode);
        if(nvram_set_with_attr(key,buf,ATTR_TEMP) != 0) {
            TB_LOGE("nvram set temp %s fail",key);
            return -1;
        }
    }

    return iret;
}

int product_test_exec_cmd_no_return (const char *format, ...)
{
    va_list vaList;
    char cmdBuff[256]={0};

    va_start (vaList, format);
    vsnprintf ((char *)cmdBuff, sizeof(cmdBuff), format, vaList);
    va_end (vaList);

    TB_LOGD("%s",cmdBuff);

    return system((char *)cmdBuff);
}

int product_test_exec_cmd_with_return(const char *pCmd, char cmdOutput[], int cmdOutMaxLen)
{
    FILE *stream;
    int ret;
    int len;

    TB_LOGD("%s",pCmd);
    stream = popen(pCmd, "r");

    if (NULL == stream){    
        TB_LOGE("popen fail");
        return -1;
    }

    int fd = fileno(stream);
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= (O_NONBLOCK);

    if (fcntl(fd,F_SETFL,flags) < 0) {
        perror("fcntl:");
        goto end_handle;
    }

    fd_set readfds;
    struct timeval timeout;    

    FD_ZERO(&readfds);

    timeout.tv_sec = 5;    
    timeout.tv_usec = 0;

    FD_SET(fd,&readfds);

    ret = select(fd+1,&readfds,NULL,NULL,&timeout);                
    if(ret < 0) {
        printf("%s:select failed\n",pCmd);
        goto end_handle;

    }else if(ret == 0) {
        printf("%s select timeout\n",pCmd);        
        goto end_handle;
    }else {
        if(FD_ISSET(fd,&readfds)) {
            if (NULL != cmdOutput) {        
                memset(cmdOutput, 0, cmdOutMaxLen);
                fread(cmdOutput, cmdOutMaxLen, 1, stream);        
            }

            if (NULL != cmdOutput) {
                len = strlen(cmdOutput);
                if ('\n' == cmdOutput[len - 1])
                {
                    cmdOutput[len - 1] = '\0';
                }
            }

        }


    }

end_handle:
    ret = pclose(stream);
    if (-1 == ret)
    {
        TB_LOGE("pclose fail");
        return -1;
    }

    TB_LOGD("cmdOutput:%s",cmdOutput);

    return 0;
}

int product_test_file_read(char *path,char *buf,int size)
{
    int fd;
    int iret;
    fd = open(path, O_RDONLY);

    if (fd < 0) {
        TB_LOGE("open path %s",path);
        return -1;
    }

    if ((iret = read(fd,buf,size)) < 0) {
        TB_LOGE("Failed to read");
        return -1;
    }

    close(fd);

    return iret;
}

int product_test_file_write(char *path,char *buf,int size)
{
    int i;
    int fd;

    fd = open(path, O_WRONLY);

    if (fd < 0) {
        TB_LOGE("open path %s failed ",path);
        return -1;
    }

    TB_LOGD("write %s %d bytes",path,size); 

    for(i= 0;i<size;i++) {
        TB_LOGD("buf[%d] = %d",i,buf[i]);   
    }


    if (write(fd,buf,size) < 0) {
        TB_LOGE("Failed to write:%s",path);
        return -1;
    }

    close(fd);

    return 0;
}

int product_test_file_is_exists(char *path)
{
    return (access(path, 0) == 0);
}

int product_test_file_size(char *path, int *size)
{
    struct stat file_stat;

    if (NULL == size) {
        return -1; 
    }   

    if (stat(path, &file_stat) != 0) {
        return -1; 
    }   

    *size = file_stat.st_size;
    return 0;
}

unsigned long product_test_get_curtime(void) 
{
    struct timespec time = {0,0};
    clock_gettime(CLOCK_MONOTONIC,&time);
    return time.tv_sec;
};
