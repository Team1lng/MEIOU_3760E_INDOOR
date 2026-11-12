#include "ty_sign.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "tuya_verify.h"
#include "ty_sign.h"
#include "cJSON.h"
#include "ty_product_name.h"

#define TYDEBUG printf
#define CONFIG_DEFAUT_FILE "/mnt/sdcard/test_set.json"

static unsigned char file_hash[32]={0};
static unsigned char sign[160] = {0};
static int sign_size=0;
static cJSON *g_croot;

void showusage(void)
{
	TYDEBUG("./ty_sign  <file> <sign>\n");
	TYDEBUG("These are the path \n");
}

static int product_test_file_read(char *path,char *buf,int size)
{
	int fd;
	int iret;
	fd = open(path, O_RDONLY);

	if (fd < 0) {
		printf("open path %s\n",path);
		return -1;
	}

	if ((iret = read(fd,buf,size)) < 0) {
		printf("Failed to read\n");
		return -1;
	}

	close(fd);

	return iret;
}


static int product_test_file_size(char *path, int *size)
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

static int product_test_config_init(char *file_name)
{
	int size;
	char *buf = NULL;
	char *path = file_name;
	char file_path[128] = {0};
	int iret = -1;

	if (g_croot != NULL) {
        	return 0;
	}

    	if (product_test_file_size(path, &size) != 0) {
        	printf("failed to get cfg file size\n");
        	return -1;
    	}
    	TYDEBUG("cfg file:%s size: %d\n",path,size);

	buf = (char *)malloc(size + 1);

	if (NULL == buf) {
		printf("malloc fail\n");
        	return -1;
	}

	memset(buf, 0, size + 1);

	iret = product_test_file_read(path, buf, size);

	if (iret != size) {
		printf("file read fail:%d,size:%d\n",iret,size);
		free(buf);
		buf = NULL;
		return -1;
	}

//	printf("buf:%s",buf);

	g_croot = cJSON_Parse(buf);

	if (NULL == g_croot) {
		printf("cJSON_Parse fail\n");
		iret = -1;
	}

	free(buf);

	return 0;
}

static int product_test_config_uinit(void)
{
	if(g_croot) {
		cJSON_Delete(g_croot);
		g_croot = NULL;	
	}

	return 0;	
}

static int json_node_to_string(cJSON *j_root, char *node, char *buf, int buf_len)
{
	cJSON *j_node = NULL;

	if (j_root == NULL || node == NULL || buf == NULL) {
        	return -1;
	}

	j_node = cJSON_GetObjectItemCaseSensitive(j_root, node);
	if (j_node && cJSON_IsString(j_node) && (j_node->valuestring != NULL)) {
        	strncpy(buf, j_node->valuestring, buf_len);
	} else {
		printf("json_node_to_string fail\n");
        	return -1;
	}

	return 0;
}

static int product_test_config_read_string(char *device, char *key, char *buf, int bufsize)
{
	cJSON *j_device = NULL;

	if ((device == NULL) || (key == NULL) || (buf == NULL)) {
		printf("%s invaid parm\n",__func__);
        	return -1;
	}

	if (product_test_config_init(CONFIG_DEFAUT_FILE) != 0) {
		printf("config init fail\n");
        	return -1;
	}

	if (NULL == g_croot) {
		printf("g_croot init fail\n");
        	return -1;
	}

	j_device = cJSON_GetObjectItemCaseSensitive(g_croot, device);

	if (j_device == NULL) {
		printf("cJSON_GetObjectItemCaseSensitive failed\n");
        	return -1;
	}

	if (json_node_to_string(j_device, key, buf, bufsize) < 0) {
		printf("json_node_to_string failed\n");
        	return -1;
	}

	return 0;
}

static int ty_cmd_excute_shell(const char *pCmd, char cmdOutput[], int cmdOutMaxLen)
{
	FILE *stream;
	int ret;
	int len;

	printf("%s\n",pCmd);
	stream = popen(pCmd, "r");

	if (NULL == stream){	
		perror("popen fail");
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
		perror("pclose fail");
		return -1;
	}

	if(cmdOutput != NULL && strlen(cmdOutput)) {
   		printf("cmdOutput:%s\n",cmdOutput);
	}

	return 0;
}

//Judge state  active/inactive

int tycam_judge_state(int  * status)
{
	char buffer[128] = {0};
     	char cmd[128]={0};
	int ret =0 ;
	int len = sizeof(buffer);

	snprintf(cmd,sizeof(cmd),"nvram get STATUS");
        ret = ty_cmd_excute_shell(cmd , buffer,128);
    	if(ret != 0){
           TYDEBUG("cmd ret = %d , %s\n",ret,strerror(errno));
		return -1;
    	}
	TYDEBUG("read status is %s\n",buffer);
	if((ret = strcmp("active",buffer)) == 0){
		*status = DEV_STATUS_ACTIVE;
	}
	else{
		*status = DEV_STATUS_INACTIVE;
	}
	
	return 0;
}
//run factory
int tycam_run_factory(char *file)
{
	char cmd[128]={0};
	char filename[128]={0};
	int ret =0 ;
	TYDEBUG("unzip\n");

	snprintf(filename, sizeof(filename), "/mnt/sdcard/busybox");
	if(access(filename,0)==0) {
		snprintf(cmd, sizeof(cmd), "cd /mnt/sdcard &&  ./busybox unzip -o %s -d /tmp",file);
	}else{
		//unzip
       		snprintf(cmd, sizeof(cmd), "cd /mnt/sdcard &&  ./unzip -o %s -d /tmp",file);
	}
	// fixed sometimes unzip fail
#if 1
    	system(cmd);
#else
	ret = ty_cmd_excute_shell(cmd, NULL, 0);
	if(ret != 0){
		TYDEBUG("cmd unzip = %d , %s\n",ret,strerror(errno));
		return -1;
	}
	usleep(1000*1000);
#endif
	//chmod 777
	bzero(cmd,sizeof(cmd));

	snprintf(cmd, sizeof(cmd), "chmod 777 /tmp/tuya -R");
	ret = ty_cmd_excute_shell(cmd, NULL, 0);
	if(ret != 0){
        TYDEBUG("cmd chmod = %d , %s\n",ret,strerror(errno));
		return -1;
    }
	usleep(500*1000);
	//run ty_sdcard_factory.sh
	bzero(cmd,sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "/tuya/app/skyeye/script/sdcard/ty_sdcard_check_factory.sh /tmp");
	ret = ty_cmd_excute_shell(cmd, NULL, 0);
	if(ret != 0){
		TYDEBUG("cmd run = %d , %s\n",ret,strerror(errno));
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;      	// 返回值
	int status = 0;		//设备状态值
	char file[128]={0};
	char product_name[65] = {0};
	char *pname = PRODUCT_NAME;
	char sign_file[128]={0};

	#ifdef PRODUCT_SIGN
         printf("tuya verify file ....\n");
        #else
         printf("tuya unverify _file ....\n");
        #endif


	if (argc > 1) {
		showusage();
		return -1;
	}

	if(product_test_config_read_string("product_info","platform",product_name,65)== 0 && strlen(product_name) != 0) {
		printf("read from cfg :%s\n",product_name);
		pname = product_name;	
	}

	snprintf(file, sizeof(file), "/mnt/sdcard/%s.zip",pname);
	snprintf(sign_file, sizeof(sign_file), "/mnt/sdcard/%s.zip.sign",pname);

	TYDEBUG("%s\n",file);

	if(access(file,0)  || access(sign_file,0))  {
	    return -1;
	}

	if (tycam_judge_state(&status) !=0)
		return -1;

	if(status == DEV_STATUS_INACTIVE) {		//非激活不惜要验证
		tycam_run_factory(file);
		return 0;
	}

         #ifdef PRODUCT_SIGN
         printf("tuya verify file ....\n");
  	 ret = tuya_verify_file(file,sign_file);
         #endif

	 
	 if (ret  == 0 ){
		 TYDEBUG( " verify signature ok\n" );
		 //run factory
		 tycam_run_factory(file);
	 }else{
		 TYDEBUG( "tuya_verify failed:%d\n", ret );
	 }
	 
	return 0;
}

