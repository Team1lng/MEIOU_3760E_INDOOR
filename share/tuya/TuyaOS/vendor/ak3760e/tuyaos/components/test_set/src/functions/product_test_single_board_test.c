/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "product_test_led.h"
#include "product_test_ircut.h"
#include "product_test_media.h"
#include "product_test_camera.h"
#include "product_test_motor.h"
#include "product_test_utils.h"
#include "product_test_speaker.h"
#include "product_test_single_board_test.h"
#include "product_test_sub_mcu.h"
#include "product_test_config.h"
#include "product_test_cfg_info.h"
#include "tb_log.h"

static SINGLE_BOARD_TEST_PARM_T sg_single_board_test_parm = {0};

static int __product_test_config_single_board_test_parm_configure(SINGLE_BOARD_TEST_PARM_T *parm) 
{
    char *dev,*key;
    int value;

    if(parm == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    memset(parm,0,sizeof(SINGLE_BOARD_TEST_PARM_T));

    dev = CONFIG_DEVICE_SINGLE_BOARD_TEST_CFG;  
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&value) || value != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CAMERA_TEST;
    if(product_test_config_read_int(dev,key,&value) == 0) {
        parm->camera_test = value;
    }

    key = CONFIG_KEY_IRCUT_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->ircut_test = value;
    }

    key = CONFIG_KEY_IRLED_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->irled_test = value;
    }

    key = CONFIG_KEY_IRLED_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->irled_test = value;
    }

    key = CONFIG_KEY_LED_GROUP_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->led_group_test = value;
    }

    key = CONFIG_KEY_AUDIO_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->audio_test = value;
    }

    key = CONFIG_KEY_WHITE_LED_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->white_led_test = value;
    }

    key = CONFIG_KEY_MOTOR_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->motor_test = value;
    }

    key = CONFIG_KEY_SLEEP;         
    if(product_test_config_read_int(dev,key,&value)==0) {
        parm->sleep = value;
    }

    return 0;
}

int product_test_single_board_test_parm_configure(SINGLE_BOARD_TEST_PARM_T *parm)
{
    SINGLE_BOARD_TEST_PARM_T _parm  = {0};
    SINGLE_BOARD_TEST_PARM_T *pparm = &sg_single_board_test_parm;

    if(parm) {
        memcpy(pparm,parm,sizeof(SINGLE_BOARD_TEST_PARM_T)); 
        return 0;
    }

    if(__product_test_config_single_board_test_parm_configure(&_parm) == 0) {
        memcpy(pparm,&_parm,sizeof(SINGLE_BOARD_TEST_PARM_T));   
    }   

    return 0;
}

static void* __single_board_proc(void *arg)
{
    int i;
    SINGLE_BOARD_TEST_PARM_T  *parm = &sg_single_board_test_parm;

    pthread_detach(pthread_self());


    if(parm->camera_test)
        product_test_camera_test_start();


    while(1) {

        if(parm->ircut_test)
            product_test_ircut_day();

        if(parm->led_group_test) {
            for(i=0;i<E_LED_MAX;i++) {
                if(product_test_led_is_in_group(i)) {
                    if(product_test_one_led_on(i)) {
                        TB_LOGE("led on %d fail",i);
                        break;
                    }
                }
            }
        }

        if(parm->white_led_test) {
            product_test_one_led_on(E_LED_WHITE);
        }

        if(parm->sleep)
            sleep(parm->sleep);
        else
            sleep(10);

        if(parm->ircut_test) {
            product_test_ircut_night();
        }

        if(parm->white_led_test) {
            product_test_one_led_off(E_LED_WHITE);
        }

        if(parm->led_group_test) {
            for(i=0;i<E_LED_MAX;i++) {
                if(product_test_led_is_in_group(i)) {
                    if(product_test_one_led_off(i)) {
                        TB_LOGE("led on %d fail",i);
                        break;
                    }
                }
            }
        }

        if(parm->sleep)
            sleep(parm->sleep);
        else
            sleep(10);

        if(parm->audio_test) {
            char path[128]= {0};
            char name[65]= {0};
            product_test_get_config_dir(path);
	    product_test_speaker_get_play_filename(name);
	    strcat(path,name);
            TB_LOGD("path:%s",path);
            product_test_speaker_pcm_play_start(path);
        }

        if(parm->motor_test) {
            product_test_motor_self_check();
        }

    }

    return NULL;
}

static void* __single_board_irled_proc(void *arg)
{
    pthread_detach(pthread_self());

    while(1) {

        char cmd[128] = {0};

        product_test_irled_on();

        sleep(1);

        product_test_irled_off();
    
        sleep(1);

        snprintf(cmd,sizeof(cmd),"%s","cat /proc/meminfo");
        product_test_exec_cmd_no_return(cmd);
    }
    return NULL;    
}


int product_test_single_board_test(void)
{
    pthread_t tid1,tid2;

    if(pthread_create(&tid1,NULL,__single_board_proc,NULL)<0) {
            perror("pthread create tycam_fac_video_test error\n");
            return -1;
    }

    if(pthread_create(&tid2,NULL,__single_board_irled_proc,NULL)<0) {
            perror("pthread create tycam_fac_video_test error\n");
            return -1;
    }
    while(1) {
        int flag;

        if(product_test_sub_mcu_get_sleep_flag(&flag)==0 && flag == 1) {
            TB_LOGD("==== enter to deep sleep ====");
            product_test_sub_mcu_set_deep_sleep();
        }

        usleep(1000000);
    }

    return 0;
}
