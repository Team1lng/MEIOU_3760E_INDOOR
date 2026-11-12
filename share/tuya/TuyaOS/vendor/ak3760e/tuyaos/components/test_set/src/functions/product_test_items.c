/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "product_test_protocol.h"
#include "product_test_adc.h"
#include "product_test_beep.h"
#include "product_test_camera.h"
#include "product_test_ddr.h"
#include "product_test_flash.h"
#include "product_test_general_download.h"
#include "product_test_gpio.h"
#include "product_test_ircut.h"
#include "product_test_ir.h"
#include "product_test_key.h"
#include "product_test_lcd.h"
#include "product_test_led.h"
#include "product_test_mic.h"
#include "product_test_modem.h"
#include "product_test_motor.h"
#include "product_test_ota.h"
#include "product_test_pir.h"
#include "product_test_reboot.h"
#include "product_test_sd.h"
#include "product_test_temp.h"
#include "product_test_ts.h"
#include "product_test_usb.h"
#include "product_test_watchdog.h"
#include "product_test_wifi.h"
#include "product_test_cpu.h"
#include "product_test_speaker.h"
#include "product_test_battery.h"
#include "product_test_etherent.h"
#include "product_test_product_info.h"
#include "product_test_cfg_info.h"
#include "product_test_bt.h"
#include "product_test_subg.h"
#include "product_test_zigbee.h"
#include "product_test_aging_test.h"
#include "product_test.h"
#include "product_test_items.h"
#include "product_test_utils.h"
#include "product_test_alarm.h"
#include "product_test_rf.h"
#include "product_test_sub_mcu.h"
#include "product_test_passwd.h"
#include "product_test_mic.h"
#include "tb_log.h"

#define UPLOAD_DATA_PORT 8095

static int auto_key_test_flag = 0;

static int prev_key_detect_flag = 0;

static int prev_key_value[MAX_KEY_NUM + MAX_SUB_MCU_KEY_NUM] = {0};

static void* __product_test_key_detect_thread (void *arg)
{
    int i;
    int num=0,snum=0;
    int cnts=0,total = 0;
    int key_flag[MAX_KEY_NUM] = {0};
    int sub_mcu_key_flag[MAX_SUB_MCU_KEY_NUM] = {0};

    product_test_key_get_num(&num);
    product_test_sub_mcu_get_key_num(&snum);

    if(num > MAX_KEY_NUM || snum > MAX_SUB_MCU_KEY_NUM) {
        TB_LOGE("num(%d) > max(%d) || snum(%d) > max(%d)",num,MAX_KEY_NUM,snum,MAX_SUB_MCU_KEY_NUM);
        return NULL;           
    }

    total = num + snum;

    TB_LOGD("num:%d,snum:%d,total:%d",num,snum,total);

    auto_key_test_flag = 0;

    pthread_detach(pthread_self());
    
    while(!auto_key_test_flag) {
        int flag;

        for(i=0;i<num;i++) {
            if(product_test_main_cpu_key_get(i,&flag) == 0 && flag == 1) {
                TB_LOGD();
                if(key_flag[i] ==0) {

		    prev_key_value[i] = 1;
                    cnts ++;    
                    key_flag[i] = 1;
                    usleep(100000);
                } 
            }
        }
        for(i=0;i<snum;i++) {
            int mode;
            if(product_test_get_low_power_mode(&mode)==0 && mode == 1) {
                if(product_test_sub_mcu_key_get(i,&flag)==0 && flag ==1) {
                    if(sub_mcu_key_flag[i] == 0) { 
                        cnts ++;
                        sub_mcu_key_flag[i]=1;
		        prev_key_value[i+num] = 1;
                        usleep(100000);
                    }
                } 
    
            }
        }

        if(cnts >= total) {
            auto_key_test_flag = 1;
	    prev_key_detect_flag = 0;
            break;  
        }
        usleep(100*1000);
    }

    return NULL;
}

int  product_test_key_detect(void)
{
    int iret;
    static pthread_t thread;

    iret = pthread_create(&thread,NULL,__product_test_key_detect_thread,NULL);

    if(iret != 0) {
            TB_LOGE("auto key create thread fails %d", iret);
            return -1;
        }

    return 0;
}


static int __product_test_ops_prod_info(int type,void * data,int* len)
{
    int iret = 0;

    int cpu = *((int*)data);
    PROD_INFO_T *inf = (PROD_INFO_T *)data;

    switch (type) {
        case E_INFO_PROD_INFO:
            iret |=  product_test_get_protocol_version(inf->protocol_ver);
            iret |=  product_test_get_product_type(inf->product_type);
            iret |=  product_test_get_mac_write(&inf->mac_write);

            TB_LOGD("ver:%s,type:%s,mac_write:%d\n",inf->protocol_ver,inf->product_type,inf->mac_write);
            break;
        case E_INFO_MFIRM_INFO:
            iret |=  product_test_get_main_firmware_name(inf->mfirmname);
            iret |=  product_test_get_main_firmware_version(inf->mfirmver);
            break;
        case E_INFO_SFIRM_INFO:
            iret |=  product_test_get_sub_firmware_name(cpu,inf->sfirmname);
            iret |=  product_test_get_sub_firmware_version(cpu,inf->sfirmver);
            break;
        default:
            TB_LOGE("invalid parm");
            iret = -1;
    }

    TB_LOGD(" iret:%d\n",iret);
    return iret;
}

static int __product_test_ops_cfg_info(int type,void * data,int* len)
{
    int flag,mode;
    int iret = 0;
//  int cpu = *((int*)data);
    CFG_PARM_T *cfg = (CFG_PARM_T*)data;

    switch(type) {
        case E_CFG_READ_CFG:
            product_test_get_prod_test(&cfg->prod_test);
            product_test_get_ssid(cfg->ap_ssid);
            product_test_get_pwd(cfg->ap_pwd);
            product_test_get_pskkey(cfg->pskKey);
            product_test_get_auth_info(cfg->pid,cfg->uuid,cfg->auzkey);
            break;

        case E_CFG_WRITE_CFG:

            iret  |= product_test_set_auth_info(cfg->pid,cfg->uuid,cfg->auzkey);

            iret |= product_test_set_prod_test(cfg->prod_test);
            iret |= product_test_set_ssid(cfg->ap_ssid);
            iret |= product_test_set_pwd(cfg->ap_pwd);
            iret |= product_test_set_pskkey(cfg->pskKey);

            if(product_test_get_dynamic_passwd_support(&flag) == 0 && flag == 1) {

		if(strlen(cfg->uuid) && strlen(cfg->auzkey)) {
                    #define CFG_ACTIVE_VALUE "active"
                    iret |= product_test_set_active_status(CFG_ACTIVE_VALUE);
                    iret |= product_test_passwd_change(cfg->uuid,cfg->auzkey);
		}else {
	            iret = -1;	
		}
            }

            break;
        case E_CFG_WRITE_MMAC:
        {
            if(product_test_wifi_get_mac_in_efuse_flag(&flag)== 0 && flag == 1) {
                if((iret = product_test_wifi_set_mac(cfg->mmac)) == 0) {
                    //iret |= product_test_set_mmac(cfg->mmac);
                }
            }else if(product_test_get_low_power_mode(&mode) == 0 && mode == 1) { 
                if((product_test_sub_mcu_write_mac(cfg->mmac)) == 0) {
                    iret |= product_test_set_mmac(cfg->mmac);
                }

            }else {
                iret |= product_test_set_mmac(cfg->mmac);
            }
            break;
        }
        case E_CFG_READ_MMAC:
            if(product_test_wifi_get_mac_in_efuse_flag(&flag)== 0 && flag == 1) {
                char mac1[65]={0};
                //char mac2[65]={0};
                if((iret = product_test_wifi_get_mac(mac1)) == 0) {
                    /*
                    iret |= product_test_get_mmac(mac2);

                    if(iret == 0 && strcmp(mac1,mac2) == 0) {
                        strcpy(cfg->mmac,mac1);
                    }
                    */
                    strcpy(cfg->mmac,mac1);
                }
                //TB_LOGD("mac1:%s,mac2:%s\n",mac1,mac2);
                TB_LOGD("mac1:%s\n",mac1);
            }else {
                iret |= product_test_get_mmac(cfg->mmac);
            }

            break;
        case E_CFG_WRITE_SMAC:
            iret |= product_test_set_smac(cfg->snum,cfg->smac);
            break;
        case E_CFG_READ_SMAC:
            iret |= product_test_get_smac(cfg->snum,cfg->smac);
            break;
        case E_CFG_WRITE_BSN:
            iret |= product_test_set_bsn(cfg->bsn);
            break;
        case E_CFG_READ_BSN:
            iret |= product_test_get_bsn(cfg->bsn);
            break;
        case E_CFG_WRITE_SN:
            iret |= product_test_set_sn(cfg->sn);
            break;
        case E_CFG_READ_SN:
            iret |= product_test_get_sn(cfg->sn);
            break;
        case E_CFG_WRITE_COUNTRY:
            iret |= product_test_set_country(cfg->country);
            break;
        case E_CFG_READ_COUNTRY:
            iret |= product_test_get_country(cfg->country);
            break;
        default:
            iret  = -1;
            TB_LOGE("invalid type:%d",type);
            break;
    
    }
    return iret;
}

static void* __product_motor_thread(void *arg) 
{
    TB_LOGD("motor test enter");
    pthread_detach(pthread_self());
    product_test_motor_self_check();
    return NULL;    

}

static int __product_test_ops_motor(int type,void * data,int* len)
{
    pthread_t thread;
    switch (type) {
        case E_MOTOR_SELF_CHECK: 
            pthread_create(&thread,NULL,__product_motor_thread,NULL);
            break;
    }
    return 0;
}

static int __product_test_ops_adc(int type,void * data,int*len)
{
    ADC_PARM_T *parm = (ADC_PARM_T*)data;
    TB_LOGD();

    return product_test_adc_get_value(parm->channel,&parm->value);
}

static int __product_test_ops_pir(int type,void * data,int* len)
{
    int iret = -1; 
    unsigned long start_time;
    int pcba = *(int*)(data);

    TB_LOGD("pcba:%d",pcba);

    if(product_test_pir_test(2) == 0 && pcba == 0) {
	return 0;
    }

    start_time = product_test_get_curtime();

    while(LESS_TARGET(product_test_get_curtime(),start_time,20)) {
	TB_LOGW("wait pir event...");
	if(product_test_pir_test(1) == 0) {
	   iret = 0;
	   break;	
	}
	sleep(1);
    }

    return iret;
}

static int __product_test_ops_camera(int type,void * data,int* len)
{
    int iret = 0;
//  char *ip = (char*)data;

    switch (type) {
        case E_VIDEO:
            iret = product_test_camera_test_start(); 
            break;

        case E_VIDEO_BLACK:
            iret = product_test_black_camera_test_start();
            break;
    }
    return iret;
}

static int __product_test_ops_ir(int type,void * data,int* len)
{
    int iret = -1;
    char *str =  NULL;
    char receive_str[20];
    int receive_len;
    static int  ir_mode = -1;
    int *pmode  = NULL;
    switch(type) {
        case E_IR_SEND:
            if (ir_mode != E_IR_MODE_EMISSION){
                printf("ir no switching mode EMISSION\n");
                return -1;
            }
            str  = (char *)data;
            iret  = product_test_ir_send(str,*len);
            break;

        case E_IR_RECV:
            if (ir_mode != E_IR_MODE_RECEIVE){
                printf("ir no switching mode EMISSION\n");
                return -1;
            }
            str  = (char *)data;
            iret =  product_test_ir_receive(receive_str, &receive_len);
            if (iret < 0){
                return iret;
            }
            if (receive_len != *len ||strcmp(receive_str,str) != 0 ){
                return -1;      
            }
            break;
        case E_IR_MODE:

            pmode  = (int *)data;
            ir_mode =  *pmode;
            iret = 0;
            break;
        default:
            iret  = -1;
            TB_LOGE("invalid type:%d",type);
            break;
    
    }
    return iret;
}

static int __product_test_ops_ir_led(int type,void * data,int* len)
{
    int iret = -1;
    int *state = (int *)data;

    TB_LOGD();

    switch (*state) {
        case 1:
            iret = product_test_irled_on();
            break;
        case 0:
                iret =  product_test_irled_off();
            break;
        default:
            TB_LOGE("error state:%d",*state);
            break;
    }

    return iret;
}

static int __product_test_ops_ircut(int type,void * data,int* len)
{
    int num = *((int *)data);
    
    return product_test_ircut_test(num);

}

static int __product_test_ops_sd(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_battery(int type,void* data,int* len)
{
    int iret = 0;
    uint32_t value;
    BATTERY_PARM_T *parm = (BATTERY_PARM_T*)data;

    TB_LOGD();

    switch(type) {
        case E_BATTERY_GET_VALUE:
            if(parm->type == E_POWER_CAPACITY) {
                iret = product_test_battery_get_current_capacity(&value);
                if(iret == 0) {
                    parm->value = (int)value;   
                }
            }else if(parm->type == E_POWER_VOLTAGE) {
            
            
            }else if(parm->type == E_POWER_CHARGER) {
            
            }else {
                TB_LOGE("invalid type:%d",parm->type);  
                return -1;  
            }
            break;

        case E_BATTERY_SET_INFO:
            iret |= product_test_battery_set_factory(parm->b_factory);
            iret |= product_test_battery_set_type(parm->b_type);
            iret |= product_test_battery_set_capacity(parm->b_capacity);
            iret |= product_test_battery_set_info(parm->b_info); 
            break;
        case E_BATTERY_GET_INFO:
            iret |= product_test_battery_get_factory(parm->b_factory);
            iret |= product_test_battery_get_type(parm->b_type);
            iret |= product_test_battery_get_capacity(&parm->b_capacity);
            iret |= product_test_battery_get_info(parm->b_info);
            break;
        default:
            TB_LOGE("invalid type:%d",type);
            iret = -1;
    
    }

    return iret;
}


#define RECORD_FILEPATH "/tmp/record.wav"

static int __product_test_ops_mic(int type,void * data,int* len)
{
    int iret = 0;
    int *mode = (int*)data;

    TB_LOGD("mode:%d",*mode);

    if(*mode == E_MIC_START) {
        iret  = product_test_mic_record_start(RECORD_FILEPATH,3);
    }

    return iret;
}

static int sg_speaker_run_flag;

static void* __product_speaker_play_loop(void *arg)
{
    char path[128]= {0};
    char name[65]= {0};

    pthread_detach(pthread_self());

    sg_speaker_run_flag = 1;

    product_test_get_config_dir(path);
    product_test_speaker_get_play_filename(name);
    strcat(path,name);

    TB_LOGD("speaker test %s",path);

    while(sg_speaker_run_flag) {
        product_test_speaker_pcm_play_start(path);
        sleep(4);
    }

    return NULL;
}

static int __product_test_ops_speaker(int type,void * data,int* len)
{
    int iret = 0;
    static pthread_t thread;
    int *mode = (int *)data;

    TB_LOGD("mode:%d",*mode);

    if(*mode == E_SPEAKER_START) {
        pthread_create(&thread,NULL,__product_speaker_play_loop,NULL);
    }else {
        sg_speaker_run_flag  = 0;
        iret = product_test_speaker_play_stop();

        if (thread > 0){
            pthread_cancel(thread);
            thread = 0;
        }
    }

    return iret;
}

static int __product_test_ops_led(int type,void * data,int* len)
{
    int iret = -1;
    int *state = (int *)data;

    TB_LOGD("state:%d",*state);
    switch (*state) {
        case 0:
            iret = product_test_led_on();
            break;
        case 1:
            iret = product_test_led_off();
            break;
        case 2:
            iret = product_test_led_flash();
            break;
        case 3:
            iret = product_test_auto_red_on();
            break;
        case 4:
            iret = product_test_auto_green_on();
            break;

        default:
            printf("state:%d is invalid",*state);
    
    }
    return iret;
}

static int __product_test_ops_key(int type,void * data,int* len)
{
//  int tmp_num = 0;
//  int *num = (int*)(data);
//  int ret = product_test_key_report_num(&tmp_num);
//  *num = tmp_num;
//  return ret;
    return 0;
}


static int __product_test_ops_wifi(int type,void * data,int* len)
{
    int iret = 0;

    switch (type) {
        case E_WIFI_R_RSSI:
        {
            int i;
            uint32_t n;
            WIFI_INFO_T *wifi=NULL;  
            WIRELESS_PARM_T *parm =(WIRELESS_PARM_T*)data;
            iret = product_test_wifi_rssi_get(parm->scan_all,(char *)parm->ssid,&wifi,(int *)&n);

            if(iret ||  wifi == NULL || n ==  0) {
                TB_LOGE("rf test fail");
                return -1; 
            }

            if(n > PARM_MAX_SIZE) {
                n = PARM_MAX_SIZE;
            }

            parm->totals = n;   

            for(i= 0;i< n;i++) {
                parm->info[i].rssi = wifi[i].rssi;
                strcpy((char *)parm->info[i].ssid,(char *)wifi[i].ssid);
            }

            break;
        }
        case E_WIFI_CONNECT_AP:
        {
            CFG_PARM_T *cfg = (CFG_PARM_T *)data;
            iret = product_test_wifi_connect_ap(cfg->ap_ssid,cfg->ap_pwd); 
            break;
        }
        default:
            TB_LOGE("type:%d is invalid",type);
            break;
    
    }
    return iret;
}

static int __product_test_ops_scap(int type,void * data,int* len)
{
    int rssi;
    int iret = 0;
    SCAP_PARM_T *parm=(SCAP_PARM_T*)data;

    if(parm->cpu == 1) {
        int num;

	TB_LOGD("channel:%d,num:%d",parm->channel,parm->num);

        iret = product_test_zigbee_test(parm->channel,parm->num,&num); 
	if(!iret) {
            parm->num = num;
	}
    }else if(parm->cpu == 4) {
        iret = product_test_subg_test(&parm->num);  
    }else if(parm->cpu == 2) {
        iret = product_test_bt_get_rssi(parm->ssid,&rssi);
        if(!iret) { 
            parm->rssi = rssi;
        }
    } else {}

    return iret;
}

static int __product_test_ops_subg(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_zigbee(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_beep(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_wdg(int type,void * data,int* len)
{
    return 0;
}

static void* __product_reboot_thread(void *arg) 
{
    pthread_detach(pthread_self());
    sleep(1);
    product_test_reboot(1);
    return NULL;
}

int __product_test_ops_reboot(int type,void * data,int* len)
{
    pthread_t thread;
    
    pthread_create(&thread,NULL,__product_reboot_thread,NULL);

    return 0;
}


static int __product_test_ops_ota(int type,void * data,int* len)
{
    int iret = 0;
    UPGRADE_CFG_T *cfg = (UPGRADE_CFG_T*)data;

    switch(type) {
        case E_OTA_CFG:
            iret |= product_test_ota_init(cfg->baud,cfg->crc32,cfg->size);
            break;
        case E_OTA_START:
            iret |= product_test_ota_start(&cfg->packet_size);
            break;
        case E_OTA_DLING:
            iret |= product_test_ota_recv((char *)data,*len);
            break;
        case E_OTA_END:
            iret |= product_test_ota_end();
            break;
    }

    return iret;
}

static int __product_test_ops_ota_slave(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_usb(int type,void * data,int* len)
{
    USB_INFO_T info = {0};

    TB_LOGD();

    return product_usb_get_usb_info(&info);
}

#define GDL_CFG "/tuya/data/%s.cfg" 

static int __product_test_ops_gdl(int type,void * data,int* len)
{
    int iret = 0;
    GDL_PRAM_T *parm;
    static char file_path[128] = {0};
    char file_name[65] = {0};
    pthread_detach(pthread_self());
    TB_LOGD("action type  %d\n",type);
    switch(type) {  
        case E_GDL_START:
            parm = (GDL_PRAM_T*)data;

	    memset(file_path,0,128);
            if(product_test_get_download_filename(file_name)) {     
                sprintf(file_path,GDL_CFG,parm->type);
	    }else {
                product_test_get_config_dir(file_path);
		strcat(file_path,file_name);
            }

	    TB_LOGD("save download file as %s",file_path);
	    iret = product_test_general_download_start(file_path,parm->md5,parm->size,parm->type);
            break;
        case E_GDL_DOING:
            iret = product_test_general_downloading((char*)data,*len);
            break;
        case E_GDL_END:
            iret |= product_test_general_download_end();
            iret |= product_test_general_download_file_analysis(file_path);
            break;
    }
    
    return iret;
}

static int __product_test_ops_lcd(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_ts(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_ddr(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_temp(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_cpu(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_flash(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_modem(int type,void * data,int* len)
{
    int iret = 0;

    MODEM_DATA_T *modem_data = (MODEM_DATA_T *)data;
    TB_LOGD();
    switch(type) {
        case E_MODEM_ENTER_TEST:
            iret = product_test_modem_enter_test();
            break;
        case E_MODEM_READ_CCID:
            iret |= product_test_modem_get_ccid(modem_data->ccid);
            break;
        case E_MODEM_READ_IMSI:
            iret |= product_test_modem_get_imsi(modem_data->imsi);
            break;
        case E_MODEM_READ_IMEI:
            iret |= product_test_modem_get_imei(modem_data->imei);
        case E_MODEM_READ_RSSI:
            iret |= product_test_modem_get_rssi(&modem_data->rssi);
            break;
    }

    return iret;
}

static int __product_test_ops_etherent(int type,void * data,int* len)
{
    return 0;
}


static int __product_test_ops_self_check(int type,void * data,int* len)
{
    return 0;
}


static int __product_test_ops_flag_detect(int type,void * data,int* len)
{
    int iret = -1;
    int flag;
    int test_type = *((int *)(data));

    TB_LOGD("type:%d",test_type);

    switch (test_type) {
        case E_FLAG_DECT_WIFIRF:
            if(product_test_wifi_get_rf_detect_flag(&flag) == 0 && flag ==  1) {
                iret = 0;
            }
	    TB_LOGD("product_test_wifi_get_rf_detect_flag:iret:%d,flag:%d",iret,flag);
            break;
        case E_FLAG_DECT_BOARDTEST:
            break;
        default:
            TB_LOGD("type:%d invalid",test_type);
            break;
    }

    TB_LOGD("iret:%d",iret);

    return iret;
}

static int __product_test_ops_aging(int type,void * data,int* len)
{
    return 0;
}

static int __product_test_ops_gpio(int type,void * data,int* len)
{
    GPIO_GROUP_T *pgroup = (GPIO_GROUP_T*)data;
    TB_LOGD();

    return  product_test_gpio(pgroup); 
}

static int __product_test_ops_general_switch(int type,void * data,int* len)
{
    int iret = -1;

    GEN_SWITCH_PARM_T *parm = (GEN_SWITCH_PARM_T*)data;

    TB_LOGD("type:%d,state:%d",parm->type,parm->state);

    switch (parm->state) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        default:
            printf("state:%d is invalid",parm->state);
    
    }
    return iret;
}

static int __product_test_ops_passwd(int type,void * data,int* len)
{
    return 0;
}


static void* __product_mic_record(void *arg)
{
    product_test_mic_record_start_no_timeout(RECORD_FILEPATH);
    TB_LOGD("product_mic_record");
    return NULL;
}

static void* __product_speaker_play(void *arg)
{
    char path[128]= {0};
    char name[65]= {0};

    product_test_get_config_dir(path);
    product_test_speaker_get_play_filename(name);
    strcat(path,name);

    TB_LOGD("speaker play %s",path);

  //product_test_speaker_wav_play_start(path);

    product_test_speaker_pcm_play_start(path);

    TB_LOGD("speaker play exit");

    return NULL;
}


static int __product_test_auto_audio_test(int type) 
{
    static pthread_t mic_thread;
    static pthread_t speaker_thread;

    if(type == E_BOOL_AUTO_MIC_ON) {

	TB_LOGD();
    	pthread_create(&mic_thread,NULL,__product_mic_record,NULL);
    	pthread_create(&speaker_thread,NULL,__product_speaker_play,NULL);
	sleep(3);

	TB_LOGD(" wait  speaker_thread exit");	
    	pthread_join(speaker_thread,NULL);

	TB_LOGD("mic record stop");
        product_test_mic_record_stop();
        TB_LOGD("wait mic record stop");
        pthread_join(mic_thread,NULL);

    	product_test_speaker_play_stop();

	sg_speaker_run_flag = 0;
    	pthread_cancel(mic_thread);
    	pthread_cancel(speaker_thread);
    }else if (type == E_BOOL_AUTO_MIC_OFF) {
	sg_speaker_run_flag = 0;
        pthread_join(speaker_thread,NULL);
    	product_test_mic_record_stop();
        product_test_speaker_play_stop();
    	pthread_cancel(mic_thread);
    	pthread_cancel(speaker_thread);
    }

    return 0;
}


static int __product_test_ops_bool_test(int type,void * data,int* len)
{
    TB_LOGD();
    int iret = 0;
    BOOL_PARM_T *parm = (BOOL_PARM_T *)data;
    switch(parm->type) {
        case E_BOOL_LIMIT_SWITCH:
        {
            int lw_left_flag,lw_right_flag;
            TB_LOGD("limit switch");
            product_test_get_left_limitswitch_flag(&lw_left_flag); 
            product_test_get_right_limitswitch_flag(&lw_right_flag);
            parm->num =2;
            parm->value[0][0] = (char)(lw_left_flag&0xff);
            parm->value[0][1] = (char)(lw_right_flag&0xff);
            break;
        }
        case E_BOOL_ALARM_IN:
            iret = product_test_alarm_in_test();
            break;
        case E_BOOL_ALARM_FLASH:
            iret = product_test_alarm_out_flash();
            break;
        case E_BOOL_ALARM_OFF:
            iret = product_test_alarm_out_off();
            break;
        case E_BOOL_DDR:
            if ((parm->num == 1) && (strcmp(parm->key[0],"capcity") == 0)){
                iret = product_test_ddr_capacity(atoi(parm->value[0]));
            } else {
                iret = -1;
            }
            break;
        case E_BOOL_RFFLAG:
            iret = product_test_check_all_rf_status();
            break;
        case E_BOOL_LIGHT_WHITE_FLASH:
            iret = product_test_one_led_flash(E_LED_WHITE);
            break;
        case E_BOOL_LIGHT_WHITE_OFF:
            iret = product_test_one_led_off(E_LED_WHITE);
            break;
		case E_BOOL_AUTO_MIC_ON:
			TB_LOGD();
			iret = __product_test_auto_audio_test(E_BOOL_AUTO_MIC_ON);
			break;
		case E_BOOL_AUTO_MIC_OFF:
			iret = __product_test_auto_audio_test(E_BOOL_AUTO_MIC_OFF);
			break;
		case E_BOOL_AUDIO_START:
			TB_LOGD();
			iret = product_test_audio_test();
			break;
		case E_BOOL_SWEPT_SINE_START:
			TB_LOGD();
			if ((!strcmp(parm->key[0], "num")) && (parm->num == 1)) {
				iret = product_test_swept_sine_start(atoi(parm->value[0]));
			} else {
				iret = -1;
			}
			break;
		case E_BOOL_SWEPT_SINE_OVER:
			TB_LOGD();
			iret = product_test_swept_over();
			break;
		case E_BOOL_READ_SWEPT_SINE_RESULT:
			TB_LOGD();
			iret = product_test_read_swept_sine_result();
			break;
		case E_BOOL_AUTO_LIGHT_COLOR:
			TB_LOGD();
			iret = product_test_auto_light_color();
			break;
    }
    return iret;
}

static ITEMS_TB_T g_item_tbs[] = {
    {E_TYPE_PRODUCT_INFO,__product_test_ops_prod_info},
    {E_TYPE_CFG_INFO,__product_test_ops_cfg_info},
    {E_TYPE_MOTOR,__product_test_ops_motor},
    {E_TYPE_ADC,__product_test_ops_adc},
    {E_TYPE_PIR,__product_test_ops_pir},
    {E_TYPE_VIDEO,__product_test_ops_camera},
    {E_TYPE_IR,__product_test_ops_ir},
    {E_TYPE_IRLED,__product_test_ops_ir_led},
    {E_TYPE_IRCUT,__product_test_ops_ircut},
    {E_TYPE_SD,__product_test_ops_sd},
    {E_TYPE_BATTERY,__product_test_ops_battery},
    {E_TYPE_MIC,__product_test_ops_mic},
    {E_TYPE_SPEAKER,__product_test_ops_speaker},
    {E_TYPE_LED,__product_test_ops_led},
    {E_TYPE_KEY,__product_test_ops_key},
    {E_TYPE_WIFI,__product_test_ops_wifi},
    {E_TYPE_SCAP,__product_test_ops_scap},
    {E_TYPE_SUBG,__product_test_ops_subg},
    {E_TYPE_ZIGBEE,__product_test_ops_zigbee},
    {E_TYPE_BEEP,__product_test_ops_beep},
    {E_TYPE_WDG,__product_test_ops_wdg},
    {E_TYPE_REBOOT,__product_test_ops_reboot},
    {E_TYPE_OTA,__product_test_ops_ota},
    {E_TYPE_SLAVE_OTA,__product_test_ops_ota_slave},
    {E_TYPE_USB,__product_test_ops_usb},
    {E_TYPE_GDL,__product_test_ops_gdl},
    {E_TYPE_LCD,__product_test_ops_lcd},
    {E_TYPE_TS,__product_test_ops_ts},
    {E_TYPE_DDR,__product_test_ops_ddr},
    {E_TYPE_TEMP,__product_test_ops_temp},
    {E_TYPE_CPU,__product_test_ops_cpu},
    {E_TYPE_FLASH,__product_test_ops_flash},
    {E_TYPE_MODEM,__product_test_ops_modem},
    {E_TYPE_ETHERENT,__product_test_ops_etherent},
    {E_TYPE_SELF_CHECK,__product_test_ops_self_check},
    {E_TYPE_FLAG_DECT,__product_test_ops_flag_detect},
    {E_TYPE_AGING,__product_test_ops_aging},
    {E_TYPE_GPIO,__product_test_ops_gpio},
    {E_TYPE_GEN_SWITCH,__product_test_ops_general_switch},
    {E_TYPE_PASSWD,__product_test_ops_passwd},
    {E_TYPE_BOOL_TEST,__product_test_ops_bool_test},
};

static ITEMS_TB_T * __get_tb_by_type(int type)
{
    int i,n;
    ITEMS_TB_T *pitem = NULL;
    ITEMS_TB_T *ptbl = &g_item_tbs[0];

    n= sizeof(g_item_tbs)/sizeof(g_item_tbs[0]);

    for(i= 0;i<n ;i++ ) {
        if(type == ptbl[i].main_type) 
            break;  
    }

    if(i< n) {
        pitem = &ptbl[i];
    }
    
    
    return pitem;
}

static int __key_async_process(ASYNC_PARM_T *parm,PT_HBUF_T* hbuf,PT_FBUF_T*fbuf)
{
    int i;
    int num=0,snum=0;

    int cnts=0,total = 0;;
    unsigned long start_time;
    int key_flag[MAX_KEY_NUM] = {0};
    int sub_mcu_key_flag[MAX_SUB_MCU_KEY_NUM] = {0};

    int *n = (int*)(hbuf->data);

    if(hbuf->result == E_FALSE) {
        TB_LOGE("result :%d",hbuf->result);
        return 0;
    }


    product_test_key_get_num(&num);
    product_test_sub_mcu_get_key_num(&snum);

    if(num > MAX_KEY_NUM || snum > MAX_SUB_MCU_KEY_NUM) {
        TB_LOGE("num(%d) > max(%d) || snum(%d) > max(%d)",num,MAX_KEY_NUM,snum,MAX_SUB_MCU_KEY_NUM);
        return 0;           
    }

    total = num + snum;

    TB_LOGD("num:%d,snum:%d,total:%d",num,snum,total);

    start_time = product_test_get_curtime();

    if(auto_key_test_flag) {
        for(i=0;i<num;i++) {
	    if(prev_key_value[i] == 1) {
                    *n = i;
                    parm->encode(hbuf,fbuf);
                    parm->send(parm->fd,(char *)fbuf->buf,fbuf->len);
                    usleep(100000);
   
	    }
	}
	return 1;
    }

    if(prev_key_detect_flag) {
       auto_key_test_flag = 1; // exit prev thread
       usleep(500000);
    }

    auto_key_test_flag = 0;
    
    while(LESS_TARGET(product_test_get_curtime(),start_time,90)) {
        int flag;

        for(i=0;i<num;i++) {
            if(product_test_main_cpu_key_get(i,&flag) == 0 && flag == 1) {
                TB_LOGD();
                if(key_flag[i] ==0) {
                    cnts ++;    
                    *n = i;
                    key_flag[i] = 1;
                    parm->encode(hbuf,fbuf);
                    parm->send(parm->fd,(char *)fbuf->buf,fbuf->len);
                    usleep(100000);
                } 
            }
        }
        for(i=0;i<snum;i++) {
            int mode;
            if(product_test_get_low_power_mode(&mode)==0 && mode == 1) {
                if(product_test_sub_mcu_key_get(i,&flag)==0 && flag ==1) {
                    if(sub_mcu_key_flag[i] == 0) { 
                        cnts ++;
                   //     *n = i;
                        *n = i + num;
                        sub_mcu_key_flag[i]=1;
                        parm->encode(hbuf,fbuf);
                        parm->send(parm->fd,(char *)fbuf->buf,fbuf->len);
                        usleep(100000);
                    }
                } 
    
            }
        }

        if(cnts >= total) {
            break;  
        }
        usleep(100*1000);
    }

    return 1;
}

static void *__thread_run(void* arg)
{
    
    int *fd = (int*)arg;
    int ret = -1;
    int upload_socket;
    struct sockaddr_in peeraddr;
    char ipAddr[32] = {0}; 
    struct sockaddr_in their_addr;

    int len = sizeof(struct sockaddr_in);

    pthread_detach(pthread_self());
    ret = getpeername(*fd, (struct sockaddr *)&peeraddr, (unsigned int *)&len);
    if (ret != 0) {
        printf("%s getpeername failed.\n", __func__);
        return NULL;
    }

    inet_ntop(PF_INET, &peeraddr.sin_addr, ipAddr, sizeof(ipAddr));  
    upload_socket =socket(AF_INET,SOCK_STREAM,0);
    if (upload_socket <= 0) {
        printf("%s socket create failed.\n", __func__);
        return NULL;
    }

    printf("%s connet addr:%s, port:%d\n", __func__, ipAddr, UPLOAD_DATA_PORT);
    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(UPLOAD_DATA_PORT);
    their_addr.sin_addr.s_addr = inet_addr(ipAddr);
    ret = connect(upload_socket,(struct sockaddr*)&their_addr, sizeof(their_addr));

    if (ret  < 0) {
        close(upload_socket);
        return NULL;
    }

    ret = product_test_mic_tiny_capature_and_upload(upload_socket);
    if ( ret  <  0 ) {
        close(upload_socket);
        return NULL;
    }
    close(upload_socket);
    return NULL;
}

typedef struct mic_async_parm_s {
    PT_FBUF_T* fbuf;
    PT_HBUF_T* hbuf;
    ASYNC_PARM_T parm;
    int mode;
}MIC_ASYNC_PARM_T;

static int __tcp_file_transport(char *file , char *ipaddr)
{
    struct sockaddr_in     serv_addr;  
    char                   buf[1024];
    int                    sock_id;  
    int                    read_len = 0;  
    int                    send_len = -1;  
    FILE                   *fp;  
    int                    i_ret;  

    TB_LOGD("transport %s",file);

    if ((fp = fopen(file,"rb")) == NULL) {  
        TB_LOGE("Open file failed\n");  
        return -1;  
    }

    fseek(fp,0,SEEK_END);
    TB_LOGD("len:%ld",ftell(fp));
    fseek(fp,0,SEEK_SET);

    if ((sock_id = socket(AF_INET,SOCK_STREAM,0)) < 0) {  
        TB_LOGE("Create socket failed\n");  
        goto release;  
    }  

    memset(&serv_addr, 0, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(UPLOAD_DATA_PORT);  
    inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr);  

    TB_LOGD("connect  %s:%d",ipaddr,UPLOAD_DATA_PORT);

    i_ret = connect(sock_id, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));  
    if (-1 == i_ret) {  
        TB_LOGE("Connect server:%s socket failed\n",ipaddr);  
        goto release;  
    }
    TB_LOGD("Connect success #\n");
    bzero(buf,sizeof(buf));

    while (1) {  
        read_len = fread(buf, sizeof(char),sizeof(buf), fp);

        if(read_len <= 0 ){
            break;
        }

        TB_LOGD("socket:%d send %d bytes,%p\n",sock_id,read_len,fp);
        send_len = send(sock_id, buf, read_len, 0);  
        if ( send_len < 0 ) {  
            TB_LOGD("Send file failed\n");  
            break;  
        }

        bzero(buf,sizeof(buf));  
    }  

release:
    if(fp)
        fclose(fp);
    if(sock_id)
        close(sock_id);

    if(send_len < 0) {
        return -1;
    }

    TB_LOGD("Send Finish\n");

    return 0;
}


static int __uart_file_transport(int fd,COM_SEND_T uart_send,char *file)
{
    char                   buf[1024];  
    int                    read_len = 0;  
    int                    send_len = -1;  
    FILE                   *fp;

    if ((fp = fopen(file,"rb")) == NULL) {  
        TB_LOGE("Open file failed\n");  
        return -1;  
    }

    bzero(buf,sizeof(buf));

    while (1) {  
        read_len = fread(buf, sizeof(char),sizeof(buf), fp);
        if(read_len <= 0 ){
            TB_LOGD("read len is %d\n",read_len);
            break;
        }

        send_len = uart_send(fd, buf, read_len);

        if ( send_len < 0 ) {  
            TB_LOGD("Send file failed\n");  
            break;  
        }

        bzero(buf, sizeof(buf));  
    }  

    if(fp)
        fclose(fp);

    if(send_len < 0) {
        return -1;
    }

    TB_LOGD("Send Finish\n");

    return 0;
}

void *mic_process_run(void* arg)
{
    int iret;
    char ipAddr[32] = {0};
    MIC_ASYNC_PARM_T *parm = (MIC_ASYNC_PARM_T*)arg;
    PT_HBUF_T *hbuf = parm->hbuf;
    PT_FBUF_T *fbuf = parm->fbuf;
    ASYNC_PARM_T *async_parm = &parm->parm;

    TB_LOGD();
    
    pthread_detach(pthread_self());

    product_test_mic_record_stop();

    if(async_parm->type == E_COM_TCP) {
        struct sockaddr_in peeraddr;
            int len = sizeof(struct sockaddr_in);
    
            iret = getpeername(async_parm->fd, (struct sockaddr *)&peeraddr, (unsigned int *)&len);

            if (iret != 0) {
                TB_LOGE("getpeername failed.\n");
                return 0;
        }

        inet_ntop(PF_INET, &peeraddr.sin_addr,ipAddr, sizeof(ipAddr));
        __tcp_file_transport(RECORD_FILEPATH,ipAddr);
    
    }else if(async_parm->type == E_COM_UART) {
        __uart_file_transport(async_parm->fd,async_parm->send,RECORD_FILEPATH);
    } 
    
    hbuf->result = E_TRUE;
    TB_LOGD("hbuf:%p,result:%d",hbuf,hbuf->result);
    async_parm->encode(hbuf,fbuf);
    async_parm->send(async_parm->fd,(char *)fbuf->buf,fbuf->len);
    TB_LOGD();
    product_test_free_hbuf(hbuf);
    product_test_free_fbuf(fbuf);
    return NULL;
}

static int __mic_data_upload_process(ASYNC_PARM_T *parm,PT_HBUF_T *hbuf,PT_FBUF_T *fbuf)
{
    MIC_INFACE_TYPE_E mode;

    if(product_test_mic_get_mode(&mode) == 0 && mode == E_MIC_INFACE_TINY_ALSA) {
        static int fd =  0;
        static pthread_t tid  = 0;
        int *mode = (int*)hbuf->data;
        hbuf->result = E_TRUE;

        TB_LOGD();

        if(*mode == E_MIC_START){
            fd = parm->fd ;
                pthread_create(&tid,NULL,__thread_run,&fd);
        }else {
            if (tid > 0){
                pthread_cancel(tid);
                tid = 0;
            }
        }

        return 0;
    } else {
        static MIC_ASYNC_PARM_T async_parm = {0};
        PT_FBUF_T *alloc_fbuf;
        PT_HBUF_T *alloc_hbuf;
        hbuf->result = E_FALSE;
        pthread_t tid  = 0;

        TB_LOGD();

        if(parm == NULL || hbuf == NULL || fbuf == NULL) {
            TB_LOGE("invalid parm");    
            return 0;   
        }

        if(product_test_zalloc_hbuf(hbuf->len,&alloc_hbuf)) {
            TB_LOGE("alloc hbuf fail");
            return 0;
        }

        if(product_test_zalloc_fbuf(128,&alloc_fbuf)) {
            TB_LOGE("alloc fbuf fail");
            product_test_free_fbuf(fbuf);
            return 0;
        }

        async_parm.fbuf = alloc_fbuf;
        async_parm.hbuf = alloc_hbuf;
        alloc_hbuf->main_type = hbuf->main_type;
        alloc_hbuf->sub_type =  hbuf->sub_type;

        alloc_hbuf->len =  hbuf->len;
        alloc_hbuf->result =  hbuf->result;

	TB_LOGD("hbuf->len= %d",hbuf->len);

	memcpy(alloc_hbuf->data,hbuf->data,hbuf->len);

        memcpy(&async_parm.parm,parm,sizeof(ASYNC_PARM_T));

        pthread_create(&tid,NULL,mic_process_run,&async_parm);

        return 1;
    }
}

#define  IPERF_RESULT_FILE "/tmp/iperf_out.txt"

typedef struct wifi_async_parm_s {
    PT_FBUF_T *fbuf;
    PT_HBUF_T *hbuf;
    ASYNC_PARM_T parm;
    WIFI_IPERF_T iperf;  
}WIFI_ASYNC_PARM_T;

static void *__wifi_iperf_run(void* arg)
{
    int iret;
    WIFI_ASYNC_PARM_T *parm = (WIFI_ASYNC_PARM_T*) arg;
    PT_HBUF_T *hbuf = parm->hbuf;
    PT_FBUF_T *fbuf = parm->fbuf;
    ASYNC_PARM_T *async_parm = &parm->parm;
    WIFI_IPERF_T *iperf = &parm->iperf;

    pthread_detach(pthread_self());
    iret =  product_test_wifi_iperf_test(IPERF_RESULT_FILE,iperf);

    TB_LOGD("avr=%.2lf,min=%.2lf,max=%.2lf",iperf->avr_speed,iperf->min_speed,iperf->max_speed);
    memcpy(hbuf->data,iperf,sizeof(WIFI_IPERF_T));
    hbuf->result = iret;
    hbuf->len = sizeof(WIFI_IPERF_T);

    async_parm->encode(hbuf,fbuf);
    async_parm->send(async_parm->fd,(char *)fbuf->buf,fbuf->len);

    product_test_free_hbuf(hbuf);
    product_test_free_fbuf(fbuf);

    return NULL;
}

static int __wifi_iperf_process(ASYNC_PARM_T *parm,PT_HBUF_T *hbuf,PT_FBUF_T *fbuf)
{

    int iret = -1;
    struct sockaddr_in peeraddr;
    int len = sizeof(struct sockaddr_in);
    char ipAddr[32] = {0};
    static WIFI_ASYNC_PARM_T async_parm = {0};
    PT_FBUF_T *alloc_fbuf;
    PT_HBUF_T *alloc_hbuf;
    hbuf->result = E_FALSE;
    pthread_t tid  = 0;

    TB_LOGD();

    if(parm == NULL || hbuf == NULL || fbuf == NULL) {
        TB_LOGE("invalid parm");    
        return 0;   
    }

    if(product_test_zalloc_hbuf(128,&alloc_hbuf)) {
        TB_LOGE("alloc hbuf fail");
        return 0;
    }

    if(product_test_zalloc_fbuf(128,&alloc_fbuf)) {
        TB_LOGE("alloc fbuf fail");
        product_test_free_fbuf(fbuf);
        return 0;
    }

    async_parm.fbuf = alloc_fbuf;
    async_parm.hbuf = alloc_hbuf;
    alloc_hbuf->main_type = hbuf->main_type;
    alloc_hbuf->sub_type =  hbuf->sub_type;
    alloc_hbuf->len =  hbuf->len;
    alloc_hbuf->result =  hbuf->result;

    memcpy(&async_parm.iperf,(WIFI_IPERF_T*)hbuf->data,sizeof(WIFI_IPERF_T));
    memcpy(&async_parm.parm,parm,sizeof(ASYNC_PARM_T));

    if(strlen(async_parm.iperf.server) == 0) {

            iret = getpeername(parm->fd, (struct sockaddr *)&peeraddr, (unsigned int *)&len);

            if (iret != 0) {
                TB_LOGE("getpeername failed.\n");
                return 0;
        }

            inet_ntop(PF_INET, &peeraddr.sin_addr,ipAddr, sizeof(ipAddr));
        strncpy(async_parm.iperf.server,ipAddr,strlen(ipAddr)+1);
    }

    pthread_create(&tid,NULL,__wifi_iperf_run,&async_parm);

    return 1;
}


int product_test_run_ops(ASYNC_PARM_T *parm,PT_HBUF_T *hbuf,PT_FBUF_T *fbuf)
{
    int mtype;
    int stype;
    int len;
    int iret = 0;
    uint8_t *data;
    ITEMS_TB_T *item;

    if(hbuf == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

    mtype = hbuf->main_type;
    stype = hbuf->sub_type;
    len = hbuf->len;
    data = hbuf->data;

    TB_LOGD("mtype:%d",mtype);
    item = __get_tb_by_type(mtype);

    if(item == NULL  || item->ops == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    hbuf->result = item->ops(stype,data,&len);
    hbuf->len = len;

    TB_LOGD("result:%d",hbuf->result);

    switch(mtype) {
        case E_TYPE_KEY:
            iret =  __key_async_process(parm,hbuf,fbuf);
            break;
        case E_TYPE_MIC:
            iret = __mic_data_upload_process(parm,hbuf,fbuf);
            break;
		case E_TYPE_BOOL_TEST:
		{
			BOOL_PARM_T *p_bool = (BOOL_PARM_T *)data;

		     if(p_bool->type == E_BOOL_AUTO_MIC_ON) {
	                 iret = __mic_data_upload_process(parm,hbuf,fbuf);
		     }
	             break;
		}
        case E_TYPE_WIFI:
            if(stype == E_WIFI_IPERF_TEST) {
                TB_LOGD();
                iret = __wifi_iperf_process(parm,hbuf,fbuf);

                if(iret == 0) {
                    hbuf->result =E_FALSE; 
                }
            }
            break;
        case E_TYPE_VIDEO:
            do{
                char ip[128] = {0};
                struct sockaddr_in ser_addr;
                socklen_t ser_len; 
                ser_len = sizeof(ser_addr);
                getsockname(parm->fd,(struct sockaddr *)&ser_addr, &ser_len);
                inet_ntop(AF_INET, &ser_addr.sin_addr,ip, sizeof(ip));
                strcpy((char *)data,ip);
            }while(0);
            break;
        default:
            iret = 0;
            break;
    }

    return iret;
}
