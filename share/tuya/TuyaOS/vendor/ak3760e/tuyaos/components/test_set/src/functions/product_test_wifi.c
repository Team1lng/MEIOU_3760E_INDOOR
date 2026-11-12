/*
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "cJSON.h"
#include "product_test_protocol.h"
#include "product_test_utils.h"
#include "product_test_sub_mcu.h"
#include "product_test_product_info.h"
#include "tkl_wifi.h"
#include "product_test_config.h"
#include "product_test_wifi.h"
#include "tb_log.h"

#define  WIFI_TYPE_KEY "wifi_type"

#define WIFI_SSW101B_VALUE "ssw101b"
#define WIFI_ATBM603X_VALUE "atbm603x"
#define WIFI_HI3881_VALUE "hi3881"
#define WIFI_HI3881_WRITE_MAC  "wifi_write_mac"
#define WIFI_WRITE_MAC_NULL "NULL"
#define WIFI_8720S_VALUE    "8720s_wifi"


#define WIFI_IWPRIV_PATH "/tmp/tuya/fac/bin"
#define WIFI_MAC_IN_EFUSE_FLAG_KEY "mac_in_efuse_flag"

static int atbm603_write_flag = 0;
static char atbm603_mac[32]={0};

static int __product_test_config_get_wifi_type(char *wifi)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_WIFI;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_WIFI_TYPE;
    
    if(product_test_config_read_string(dev,key,wifi,32)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

static int __product_test_config_wifi_get_mac_in_efuse_flag(int *flag)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_WIFI;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_MAC_LOCATION;
    
    if(product_test_config_read_int(dev,key,flag)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
    
}

int product_test_set_wifi_type(char *wifi) 
{
    return product_test_utils_write_string(WIFI_TYPE_KEY,wifi,0);
}

int product_test_get_wifi_type(char *wifi)
{
    int iret;

    iret = product_test_utils_read_string(WIFI_TYPE_KEY,wifi);

    if(iret || strlen(wifi) == 0) {
        iret =  __product_test_config_get_wifi_type(wifi);
    }
    return iret;
}

int product_test_wifi_set_mac_in_efuse_flag(int flag) 
{
    return product_test_utils_write_int(WIFI_MAC_IN_EFUSE_FLAG_KEY,flag,0);
}

int product_test_wifi_get_mac_in_efuse_flag(int *flag)
{
    int iret;
    
    iret = product_test_utils_read_int(WIFI_MAC_IN_EFUSE_FLAG_KEY,flag);

    if(iret) {
        iret =  __product_test_config_wifi_get_mac_in_efuse_flag(flag);
    }

    return iret;
}

int product_test_wifi_set_mac(char *mac)
{
    int i,j;
    char wifi[64] ={0};
    char iwpriv_path[128]={0};
    char mac_tmp[32]={0};
    char cmd[128]= {0};

    
    if(product_test_get_wifi_type(wifi)) {
        TB_LOGE("get wifi type fail");
        return -1;
    }

    TB_LOGD("==wifi type:%s==",wifi);
    if(strcmp(wifi,WIFI_HI3881_VALUE) == 0){
        for(i=0,j=0; i<strlen(mac); i++) {
            mac_tmp[j++] = mac[i];
            if(i%2 != 0) {
                mac_tmp[j++]=':';
            }
        }
        mac_tmp[strlen(mac_tmp)-1]='\0';
        TB_LOGD("mac_tmp is --%s--\n",mac_tmp);
        for (i = 0; i < strlen(mac_tmp); i++){
            mac_tmp[i] = toupper(mac_tmp[i]);
        }
        return product_test_utils_write_string(WIFI_HI3881_WRITE_MAC,mac_tmp,1);
    }else if(strcmp(wifi,WIFI_SSW101B_VALUE) == 0 || strcmp(wifi,WIFI_ATBM603X_VALUE) == 0) {
        for(i=0,j=0; i<strlen(mac); i++) {
            mac_tmp[j++] = mac[i];
            if(i%2 != 0) {
                mac_tmp[j++]=':';
            }     
        }
        mac_tmp[strlen(mac_tmp)-1]='\0';
        TB_LOGD("mac_tmp is --%s--\n",mac_tmp);
        snprintf(iwpriv_path,sizeof(iwpriv_path),"%s/iwpriv",WIFI_IWPRIV_PATH);
        if(access(iwpriv_path,F_OK) == 0 )
            snprintf(cmd,sizeof(cmd),"%s/iwpriv wlan0 common setEfuse_mac,%s",WIFI_IWPRIV_PATH,mac_tmp);
        else
            snprintf(cmd,sizeof(cmd),"iwpriv wlan0 common setEfuse_mac,%s",mac_tmp);    

        if(strcmp(wifi,WIFI_ATBM603X_VALUE) == 0) {
            atbm603_write_flag  = 1;
            memcpy(atbm603_mac,mac_tmp,32);
	}

    }else if (strcmp(wifi,WIFI_8720S_VALUE) == 0) {

	for(i=0,j=0;i<strlen(mac);i++) {
            printf("i = %d, j = %d, %c\n",i,j ,mac[i]);
            if(mac[i] != ':')
                mac_tmp[j++]=mac[i];
        }

	 for (i = 0; i < strlen(mac_tmp); i++){
           mac_tmp[i] = toupper(mac_tmp[i]);
        }

	if(product_test_sub_mcu_set_mac(mac_tmp)) {
	    return -1;
	}
	return 0;
    
    }else {

        for(i=0,j=0;i<strlen(mac);i++) {
            printf("i = %d, j = %d, %c\n",i,j ,mac[i]);
            if(mac[i] != ':')
                mac_tmp[j++]=mac[i];
        }

        TB_LOGD("mac_tmp is --%s--\n",mac_tmp);
        snprintf(iwpriv_path,sizeof(iwpriv_path),"%s/rtwpriv",WIFI_IWPRIV_PATH);
        if(access(iwpriv_path,F_OK) == 0 )
            snprintf(cmd,sizeof(cmd),"%s/rtwpriv wlan0 efuse_set mac,%s",WIFI_IWPRIV_PATH,mac_tmp);
        else
            snprintf(cmd,sizeof(cmd),"rtwpriv wlan0 efuse_set mac,%s",mac_tmp); 
    }

    return product_test_exec_cmd_no_return(cmd);
}

int product_test_wifi_get_mac(char*mac)
{
    int ret;
    int i,j;
    char wifi[64] ={0};
    char iwpriv_path[128]={0};
    char mac_tmp[32]={0};
    char buffer[256] = {0};
    char cmd[128] ={0};
    
    if(product_test_get_wifi_type(wifi)) {
        TB_LOGE("get wifi type fail");
        return -1;
    }

    TB_LOGD("==wifi type:%s==",wifi);

    if(strcmp(wifi,WIFI_SSW101B_VALUE) == 0 ) {

        snprintf(iwpriv_path,sizeof(iwpriv_path),"%s/iwpriv",WIFI_IWPRIV_PATH);

        if(access(iwpriv_path,F_OK) == 0)
            snprintf(cmd,sizeof(cmd),"%s/iwpriv wlan0 getmac",WIFI_IWPRIV_PATH);
        else
            snprintf(cmd,sizeof(cmd),"iwpriv wlan0 getmac");

        ret = product_test_exec_cmd_with_return(cmd,buffer,sizeof(buffer));

        if(ret != 0){
            TB_LOGD("cmd ret = %d , %s\n",ret,strerror(errno));
            return ret;
        }

            for(i=0;i<strlen(buffer);i++)
            if(buffer[i] == '\n')
                buffer[i] = '\0';

        TB_LOGD("buffer:%s\n",buffer);

        for(i=0,j=0; i<strlen(buffer); i++){
            if(buffer[i] == ':') {
                continue;
            }
            if(buffer[i]==' ') {
                mac_tmp[j++]='0';
            } else{
                mac_tmp[j++]=buffer[i];
            }
        }

    }else if (strcmp(wifi,WIFI_ATBM603X_VALUE) == 0){

        if(atbm603_write_flag == 1) {
	    TB_LOGD("get in buffer");
	    ret = 0;
	    memcpy(buffer,atbm603_mac,32);
	}else {

	   TB_LOGD("get in wlan");
           snprintf(cmd,sizeof(cmd),"ifconfig | grep wlan0 | awk '{print $5}' | awk -F: '{print $1 $2 $3 $4 $5 $6}'");
           ret = product_test_exec_cmd_with_return(cmd,buffer,sizeof(buffer));
	}

        if(ret != 0){
            TB_LOGD("cmd ret = %d , %s\n",ret,strerror(errno));
            return ret;
        }

        TB_LOGD("buffer:%s\n",buffer);

        for(i=0;i<strlen(buffer);i++)
            if(buffer[i] == '\n')
                buffer[i] = '\0';

        for(i=0,j=0; i<strlen(buffer); i++){
            if(buffer[i] == ':') {
                continue;
            }
            if(buffer[i]==' ') {
                mac_tmp[j++]='0';
            } else{
                mac_tmp[j++]=buffer[i];
            }
        }
    
    } else if(strcmp(wifi,WIFI_HI3881_VALUE) == 0) {
        char wifi_write_mac[64]= {0};
        ret = product_test_utils_read_string(WIFI_HI3881_WRITE_MAC,wifi_write_mac);

        if(ret || strlen(wifi_write_mac)==0) {
            TB_LOGD("name:%s",wifi_write_mac);
            return 0;
        }
        TB_LOGD("get in wlan");
        if(strcmp(wifi_write_mac,WIFI_WRITE_MAC_NULL) == 0){
                snprintf(cmd,sizeof(cmd),"ifconfig | grep wlan0 | awk '{print $5}' | awk -F: '{print $1 $2 $3 $4 $5 $6}'");
            ret = product_test_exec_cmd_with_return(cmd,buffer,sizeof(buffer));

            if(ret != 0){
                TB_LOGD("cmd ret = %d , %s\n",ret,strerror(errno));
                return ret;
            }
        }else {
            for (i = 0; i < strlen(wifi_write_mac); i++){
                buffer[i] = wifi_write_mac[i];
            }
        }

        TB_LOGD("buffer:%s\n",buffer);

        for(i=0;i<strlen(buffer);i++)
            if(buffer[i] == '\n')
                buffer[i] = '\0';

        for(i=0,j=0; i<strlen(buffer); i++){
            if(buffer[i] == ':') {
                continue;
            }
            if(buffer[i]==' ') {
                mac_tmp[j++]='0';
            } else{
                mac_tmp[j++]=buffer[i];
            }
        }
    }else if (strcmp(wifi,WIFI_8720S_VALUE) == 0) {
        char wifi_write_mac[32]= {0};
        ret = product_test_utils_read_string("master_mac",wifi_write_mac);
        if(ret != 0){
            TB_LOGD("read master mac fail\n");
            return ret;
            }
	    memcpy(mac_tmp,wifi_write_mac,32);
    }else {

        char *ret_strstr = NULL;

        snprintf(iwpriv_path,sizeof(iwpriv_path),"%s/rtwpriv",WIFI_IWPRIV_PATH);
        if(access(iwpriv_path,F_OK) == 0 )
            snprintf(cmd,sizeof(cmd),"%s/rtwpriv wlan0 efuse_get mac",WIFI_IWPRIV_PATH);
        else
            snprintf(cmd,sizeof(cmd),"rtwpriv wlan0 efuse_get mac");

        ret = product_test_exec_cmd_with_return(cmd,buffer,sizeof(buffer));

        if(ret != 0){
            TB_LOGD("cmd ret = %d , %s\n",ret,strerror(errno));
            return ret;
        }

        ret_strstr = strstr(buffer,":");

        if(ret_strstr != NULL)
        {
            TB_LOGD("ret_strstr:%s len:%d\n",ret_strstr, strlen(ret_strstr));
            for(i=0,j=0; i<strlen(ret_strstr); i++){
                if(ret_strstr[i] != ':')
                    mac_tmp[j++]=ret_strstr[i];
            }

            memset(buffer,0,sizeof(buffer));
            for (i = 0; i < strlen(mac_tmp); i++)
            {
                buffer[i] = tolower(mac_tmp[i]);
            }
        }else{
            TB_LOGD("get mac from efuse failed\n");
            return -1;
        }
    }

    for (i = 0; i < strlen(mac_tmp); i++){
        mac[i] = tolower(mac_tmp[i]);
    }

    TB_LOGD("mac%s",mac);

    return 0;
}

//snprintf(cmd,sizeof(cmd),"%s/../tool/iperf3 -c %s -w 64k -t 10 -i 1 -J >  %s",filepath,addr,iperf_tmp_file);


int tuya_get_iperf_result(char *str ,double *avr_v, double *max_v , double *min_v )
{
    double min = 0xffffffff , max = 0, avr = 0, sum = 0;
    int count = 0;
    cJSON *root = cJSON_Parse(str);
    if(!root){
        TB_LOGD("Error before: [%s]\n",cJSON_GetErrorPtr());
        return -1;
    }

//    cJSON *err = cJSON_GetObjectItem(root, "error");
/*    if(err){
        TB_LOGD("Find 'error' in string :\n%s\n",str);
        cJSON_Delete(root);
        return -1;
    }
*/
    cJSON *interv = cJSON_GetObjectItem(root, "intervals");
    if(!interv){
        TB_LOGD("can't Find 'intervals' in string :\n%s\n",str);
        cJSON_Delete(root);
        return -1;
    }
    int i;
    for(i = 0 ; i < cJSON_GetArraySize(interv);i++)
    {
        cJSON * sub = cJSON_GetArrayItem(interv,i);
        if(sub != NULL)
        {
            cJSON * sub1 = cJSON_GetObjectItem(sub, "sum");
            if(sub1){
                cJSON * bits = cJSON_GetObjectItem(sub1, "bits_per_second");

                cJSON * sec = cJSON_GetObjectItem(sub1, "seconds");
                if(sec && bits)
                {
                    double bitrate = (bits->valuedouble / 1000);

                    TB_LOGD("interval:%d : %f kbit/s\n",i,bitrate);
                    count++;
                    if(max < bitrate)
                        max = bitrate;
                    if(min > bitrate)
                        min = bitrate;
                    sum += bitrate;
                }
            }
        }
    }

    if(count)
        avr = sum / count;
    TB_LOGD("aver of [0-%d] : ",count-1);
    if((avr / (1000)) > 5)
        TB_LOGD("%.2lfMbit/s , ",avr/(1000));
    else
        TB_LOGD("%.2lfkbit/s , ",avr);

    if((min / (1000)) > 5)
        TB_LOGD("min is %.2lfMbit/s , ",min/(1000));
    else
        TB_LOGD("min is %.2lfkbit/s , ",min);

    if((max / (1000)) > 5)
        TB_LOGD("max is %.2lfMbit/s \n",max/(1000));
    else
        TB_LOGD("max is %.2lfkbit/s \n",max/(1000));

    *avr_v = avr;
    *max_v = max;
    *min_v = min;

    cJSON_Delete(root);
    return 0;
}

int product_test_wifi_iperf_test(char *path,WIFI_IPERF_T *iperf)
{
    int iret;
    FILE *fp;
    int size;
    char *data;
    char cmd[128] = {0};
    double avr,min,max;

    if(path == NULL || iperf == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

   //   snprintf(cmd,sizeof(cmd),"/tmp/iperf.sh -c %s -w 64k -t 10 -i 1 -J > %s",iperf->server,path);
    snprintf(cmd,sizeof(cmd),"/tmp/tuya/fac/script/ty_iperf.sh %s %s",iperf->server,path);
    sleep(1);
    iret = product_test_exec_cmd_no_return(cmd);

    if(iret != 0) {
        TB_LOGD("cmd %s ret = %d",cmd,iret);
        return -1;  
    }

    fp = fopen(path,"rb");
    if(fp == NULL){
        TB_LOGE("%s open fail",path);
        return -1;
    }
    fseek(fp,0,SEEK_END);
    size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    TB_LOGD("iperf file size %d\n",size);
    data = (char *)malloc(size*sizeof(char));

    if(data == NULL) {
        TB_LOGD("malloc err: %s\n",strerror(errno));
        iret = -1;
        fclose(fp);
        return -1;
    }

    fread(data,1,size,fp);
    fclose(fp);

    TB_LOGD("read from %s:%s\n",path,data);

    iret = tuya_get_iperf_result(data,&avr,&max,&min);

    if(iret != 0){
        TB_LOGD("tuya_get_iperf_result err\n");
        iret = -1;
    }

    free(data);
    iperf->avr_speed = avr;
    iperf->min_speed = min;
    iperf->max_speed = max;

    return iret;
}

int product_test_wifi_read_country_code(uint32_t* code)
{
    return 0;
}

int product_test_wifi_write_country_code(uint32_t code)
{

    return 0;
}

int product_test_wifi_rssi_get(WIFI_SCAN_MODE_E mode, char *ssid,WIFI_INFO_T **wifi,int *total)
{
    int iret = 0;
    uint32_t num;
    AP_IF_S *ap_array = NULL;

    static WIFI_INFO_T wifi_info[PARM_MAX_SIZE] = {0};

    if(mode == E_SCAN_MODE_ALL) {
        int i;
        if(tkl_wifi_scan_ap(NULL,&ap_array,&num) || ap_array == NULL || num == 0) {
            TB_LOGE("scan all ap fail");    
            return -1;
        }

        if(num > PARM_MAX_SIZE) {
            num = PARM_MAX_SIZE;    
        }
        *total  = num;
        for(i=0;i<num;i++){
            wifi_info[i].rssi = ap_array[i].rssi -100;
            if(ap_array[i].ssid)
                strcpy((char *)wifi_info[i].ssid,(char *)ap_array[i].ssid);
            else 
                strcpy((char *)wifi_info[i].ssid,"");

            TB_LOGD("ap:%s,rssi:%d",wifi_info[i].ssid,wifi_info[i].rssi -100);
        }
        *wifi = &wifi_info[0];
        
    }else { 
	int power_mode;
	int tmp_rssi;

        if(product_test_get_low_power_mode(&power_mode) == 0 && power_mode ==1) {
            iret = product_test_sub_mcu_get_wifi_rssi(&tmp_rssi);
	    if(iret ==0 ) {
                wifi_info[0].rssi = tmp_rssi;
	    }
        } else {
            iret = tkl_wifi_scan_ap((signed char *)ssid, &ap_array,&num);

	    if(iret == 0) {
                wifi_info[0].rssi = ap_array[0].rssi -100;
	    }
        }

        if(iret){
            TB_LOGE("get wifi rssi failed\n");
            return -1;
        }

        *total  = 1;
        strcpy((char *)wifi_info[0].ssid,ssid);
        *wifi = &wifi_info[0];

        TB_LOGD("ap:%s,rssi:%d",wifi_info[0].ssid,wifi_info[0].rssi);
    }

    return iret;
}

int product_test_wifi_connect_ap(char *ssid,char *psk) 
{
    int mode;
    int iret = 0;
    TB_LOGD("ssid:%s,psk:%s",ssid,psk);

    if(product_test_get_low_power_mode(&mode) == 0 && mode ==1) {
        TB_LOGD();
        iret = product_test_sub_mcu_connect_ap(ssid,psk);
    }else {
    
    }
            
    return iret;
}
