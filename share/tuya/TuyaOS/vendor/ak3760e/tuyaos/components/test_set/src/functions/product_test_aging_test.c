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
#include "product_test_led.h"
#include "product_test_ircut.h"
#include "product_test_media.h"
#include "product_test_camera.h"
#include "product_test_motor.h"
#include "product_test_utils.h"
#include "product_test_speaker.h"
#include "product_test_aging_test.h"
#include "product_test_cfg_info.h"
#include "product_test_config.h"
#include "tb_log.h"

#define PLAY_FILE "test.pcm"

static AGING_TEST_PARM_T g_aging_test_param = {0};

static int __product_test_config_aging_test_parm_configure(AGING_TEST_PARM_T *param) 
{
    char *dev,*key;
    int value;

    if(param == NULL) {
        TB_LOGE("invalid param");
        return -1;  
    }

    memset(param,0,sizeof(AGING_TEST_PARM_T));

    dev = CONFIG_DEVICE_AGING_TEST_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&value) || value != 1) {
        TB_LOGE("device:%s param:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CAMERA_TEST;
    if(product_test_config_read_int(dev,key,&value) == 0) {
        param->camera_test = value;
    }

    key = CONFIG_KEY_IRCUT_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->ircut_test = value;
    }

    key = CONFIG_KEY_IRLED_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->irled_test = value;
    }

    key = CONFIG_KEY_IRLED_TEST; 
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->irled_test = value;
    }

    key = CONFIG_KEY_LED_GROUP_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->led_group_test = value;
    }

    key = CONFIG_KEY_AUDIO_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->audio_test = value;
    }

    key = CONFIG_KEY_WHITE_LED_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->white_led_test = value;
    }

    key = CONFIG_KEY_MOTOR_TEST;
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->motor_test = value;
    }

    key = CONFIG_KEY_SLEEP;         
    if(product_test_config_read_int(dev,key,&value)==0) {
        param->sleep = value;
    }

    return 0;
}

int product_test_aging_test_parm_configure(AGING_TEST_PARM_T *param) 
{
    AGING_TEST_PARM_T _param  = {0};
    AGING_TEST_PARM_T *pparm = &g_aging_test_param;

    if(param) {
        memcpy(pparm,param,sizeof(AGING_TEST_PARM_T));    
        return 0;
    }

    if(__product_test_config_aging_test_parm_configure(&_param) == 0) {
        memcpy(pparm,&_param,sizeof(AGING_TEST_PARM_T));  
    }   

    return 0;
}

int product_test_aging_test(int time)
{
    int i;
    int seconds = time*60*60;
    int run_seconds=0;
    int total_times= 0;
    int aging_exit = 0;

    struct timeval st,end;
    
    AGING_TEST_PARM_T *param = &g_aging_test_param;


    gettimeofday(&st,NULL);

    if(param->camera_test) {
        product_test_camera_test_start(); 
    }

    while(!aging_exit) {

        gettimeofday(&end,NULL);
        run_seconds = end.tv_sec -st.tv_sec;
        TB_LOGW("===need run %d hours (%d)seconds,current run %d seconds===",time,seconds,run_seconds);

        if(run_seconds >=  seconds) {
            aging_exit  = 1;
            TB_LOGW("===aging_exit===");
            break;
        }
	
	if(param->white_led_test) {
            product_test_one_led_on(E_LED_WHITE);
        }

        sleep(1);

        if(param->white_led_test) {
            product_test_one_led_off(E_LED_WHITE);
        }

        if(param->irled_test) {
            product_test_irled_on();
        }

        if(param->ircut_test) {
            product_test_ircut_day();
			usleep(300*1000);
			product_test_ircut_off();
        }

        if(param->led_group_test) {
            for(i=0;i<E_LED_MAX;i++) {
                if(product_test_led_is_in_group(i)) {
                    if(product_test_one_led_on(i)) {
                        TB_LOGE("led on fail");
                        break;
                    }
                }
            }
        }

        if(param->sleep) {
            sleep(param->sleep);
        } else {
            sleep(10);
        }

        gettimeofday(&end,NULL);
        run_seconds = end.tv_sec -st.tv_sec;
        TB_LOGW("===need run %d hours (%d)seconds,current run %d seconds===",time,seconds,run_seconds);

        if(run_seconds >=  seconds) {
            aging_exit  = 1;
            TB_LOGW("===aging_exit===");
            break;
        }

        if(param->irled_test) {  
            product_test_irled_off();
        }

        if(param->ircut_test) {
            product_test_ircut_night();
			usleep(300*1000);
			product_test_ircut_off();
        }

        if(param->led_group_test) {
            for(i=0;i<E_LED_MAX;i++) {
                if(product_test_led_is_in_group(i)) {
                    if(product_test_one_led_off(i)) {
                        TB_LOGE("led on fail");
                        break;
                    }
                }
            }
        }

        if(param->sleep) {
            sleep(param->sleep);
        } else {
            sleep(10);
        }

        gettimeofday(&end,NULL);
        run_seconds = end.tv_sec -st.tv_sec;
        TB_LOGW("===need run %d hours (%d)seconds,current run %d seconds===",time,seconds,run_seconds);

        if(run_seconds >=  seconds) {
            aging_exit  = 1;
            TB_LOGW("===aging_exit===");
            break;
        }

        if(param->audio_test) {
            char path[128]= {0};
            char name[65] = {0};

            product_test_get_config_dir(path);
            product_test_speaker_get_play_filename(name);
            strcat(path,name);
            TB_LOGD("path:%s",path);

            product_test_speaker_pcm_play_start(path);
        }

        if(param->motor_test) {
            product_test_motor_self_check();
        }

        TB_LOGW("===run %d times===",++total_times);
    }

    if(param->led_group_test) {
        for(i=0;i<E_LED_MAX;i++) {
            if(product_test_led_is_in_group(i)) {
                if(product_test_one_led_off(i)) {
                    TB_LOGE("led on fail");
                    break;
                    }
                }
            }
    }


    if(param->led_group_test) {
        product_test_one_led_on(E_LED_GREEN);
    }

    if(param->irled_test) {
        product_test_irled_off();
    }
    
    if(param->ircut_test) {
        product_test_ircut_off();
    }

    if(param->audio_test) {
        product_test_speaker_play_stop();
    }

    return 0;
}
