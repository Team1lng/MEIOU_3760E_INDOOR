#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "product_test_protocol.h"
#include "product_test_gpio.h"
#include "product_test_led.h"
#include "product_test_utils.h"
#include "product_test_pwm.h"
#include "product_test_config.h"
#include "ptal_led.h"
#include "tb_log.h"

static  pthread_t sg_led_tid;
static pthread_t sg_one_led_tid;
static  pthread_t sg_light_color_tid;
static int sg_one_led_type[E_LED_MAX] = {0};
static int sg_one_led_test_flag[E_LED_MAX] = {0};

#define IRLED_GPIO_NUM_KEY "irled%d_gpio"
#define IRLED_GPIO_LEVEL_KEY "irled%d_level"
#define IRLED_IN_GROUP_KEY "irled%d_in_group"
#define IR_LED_GPIO_LEVEL_KEY "irled_gpio_level"
#define LED_INTERFACE_TYPE_KEY "led%d_interface_type"
#define IRLED_INTERFACE_TYPE_KEY "irled%d_interface_type"
#define LED_GPIO_NUM_KEY   "led%d_gpio"
#define LED_GPIO_LEVEL_KEY "led%d_level"
#define LED_IN_GROUP_KEY   "led%d_in_group"

static int sg_led_test_flag = 0;

typedef struct led_parm_s
{
    int type;
    char *dev_name;
}LED_PARM_T;

static LED_PARM_T sg_led_parm_table[] = {
    {E_LED_RED,CONFIG_DEVICE_LED_RED},
    {E_LED_GREEN,CONFIG_DEVICE_LED_GREEN},
    {E_LED_WHITE,CONFIG_DEVICE_LED_WHITE},
    {E_LED_BLUE,CONFIG_DEVICE_LED_BLUE},
    {E_LED_IDLE,CONFIG_DEVICE_LED_IDLE},
};

static LED_PARM_T sg_irled_parm_table[] = {
    {E_IRLED_RED,CONFIG_DEVICE_IRLED_RED},
    {E_IRLED_GREEN,CONFIG_DEVICE_IRLED_GREEN},
};


static LED_PARM_T *__find_led_parm(LED_TYPE_E type) 
{
    int i;
    int n = sizeof(sg_led_parm_table)/sizeof(sg_led_parm_table[0]); 

    for(i= 0;i<n;i++) {
        LED_PARM_T *parm = &sg_led_parm_table[i];

        if(parm->type == type) {
            return parm;
        }
    }

    return NULL;
}

static LED_PARM_T *__find_irled_parm(int type) 
{
    int i;
    int n = sizeof(sg_irled_parm_table)/sizeof(sg_irled_parm_table[0]); 

    for(i= 0;i<n;i++) {
        LED_PARM_T *parm = &sg_irled_parm_table[i];

        if(parm->type == type) {
            return parm;    
        }
    }

    return NULL;
}


static int __product_test_config_led_get_interfece_type(LED_TYPE_E type,LED_INFACE_TYPE_E *interface_type) 
{
    LED_PARM_T *parm;
    int value;
    char *dev,*key;

    if((parm = __find_led_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev =parm->dev_name; 
    key = CONFIG_KEY_CONTROL_BY_PWM;

    if(product_test_config_read_int(dev,key,&value) == 0 && value == 1) {
        *interface_type = E_LED_INFACE_PWM;
        return 0;
    }

    return -1;
}

static int __product_test_config_irled_get_interface_type(IRLED_TYPE_E type,IRLED_INFACE_TYPE_E *inface_type) 
{
    int value;
    char *dev,*key;
    LED_PARM_T *parm;

    if((parm = __find_irled_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev = parm->dev_name; 
    key = CONFIG_KEY_CONTROL_BY_PWM;

    if(product_test_config_read_int(dev,key,&value) == 0 && value == 1) {
        *inface_type = E_LED_INFACE_PWM;
        return 0;
    }

    return -1;
}

static int __product_test_config_led_get_gpio_num(int type,int*gpio_num) 
{
    char *dev,*key;
    int enable;
    LED_PARM_T *parm;

    if((parm = __find_led_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_GPIO_NUM;

    if(product_test_config_read_int(dev,key,gpio_num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

static int product_test_config_led_get_gpio_level(int type,int*level) 
{
    char *dev,*key;
    int enable;
    LED_PARM_T *parm;

    if((parm = __find_led_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_HI_ACTIVE;;

    if(product_test_config_read_int(dev,key,level)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

static int product_test_config_led_is_in_group(int type) 
{
    char *dev,*key;
    int enable;
    int group;
    int iret = 0;
    LED_PARM_T *parm;

    if((parm = __find_led_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return 0;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return 0;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_IS_IN_GROUP;

    if(product_test_config_read_int(dev,key,&group)== 0 && group == 1) {
        iret  = 1;
    }


    return iret;
}

int product_test_config_irled_get_gpio_num(int type,int *gpio_num)
{
    char *dev,*key;
    int enable;
    LED_PARM_T *parm;

    if((parm = __find_irled_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_GPIO_NUM;

    if(product_test_config_read_int(dev,key,gpio_num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_config_irled_get_gpio_level(int type,int* level)
{
    char *dev,*key;
    int enable;
    LED_PARM_T *parm;

    if((parm = __find_irled_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return -1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_HI_ACTIVE;;

    if(product_test_config_read_int(dev,key,level)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_config_irled_is_in_group(int type) 
{
    char *dev,*key;
    int enable;
    int group;
    int iret = 0;
    LED_PARM_T *parm;

    if((parm = __find_irled_parm(type)) == NULL) {
        TB_LOGE("find parm fail");
        return 0;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return 0;
    }

    dev = parm->dev_name;
    key = CONFIG_KEY_IS_IN_GROUP;

    if(product_test_config_read_int(dev,key,&group) == 0 && group == 1) {
        iret  = 1;
    }

    return iret;
}


int product_test_led_set_interfece_type(LED_TYPE_E type,LED_INFACE_TYPE_E inface_type)
{
    char key[64] = {0};

    sprintf(key,LED_INTERFACE_TYPE_KEY,type);
    return product_test_utils_write_int(key,inface_type,0);
}

int product_test_led_get_interfece_type(LED_TYPE_E type,LED_INFACE_TYPE_E *inface_type) 
{
    int iret;
    char key[64] = {0};
    int interface;

    sprintf(key,LED_INTERFACE_TYPE_KEY,type);
    iret = product_test_utils_read_int(key,&interface);
    *inface_type = (LED_INFACE_TYPE_E)((interface) &0xff);

    if(iret) {
        iret = __product_test_config_led_get_interfece_type(type,inface_type);
    }
    return iret;
}

int product_test_irled_set_interfece_type(IRLED_TYPE_E type,IRLED_INFACE_TYPE_E inface_type)
{
    char key[64] = {0};
    sprintf(key,IRLED_INTERFACE_TYPE_KEY,type);
    return product_test_utils_write_int(key,inface_type,0);
}

int product_test_irled_get_interfece_type(IRLED_TYPE_E type,IRLED_INFACE_TYPE_E *inface_type) 
{
    int iret;
    char key[64] = {0};
    sprintf(key,IRLED_INTERFACE_TYPE_KEY,type);
    int interface;

    iret = product_test_utils_read_int(key,&interface);

    *inface_type = (IRLED_INFACE_TYPE_E)(interface &0xff); 

    if(iret) {
        iret = __product_test_config_irled_get_interface_type(type,inface_type);
    }

    return iret;
}

int product_test_led_set_gpio_num(LED_TYPE_E type,int gpio_num)
{
    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,LED_GPIO_NUM_KEY,type);

    return product_test_utils_write_int(key,gpio_num,0);
}

int product_test_led_get_gpio_num(LED_TYPE_E type,int*gpio_num) 
{
    int iret;
    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,LED_GPIO_NUM_KEY,type);

    iret = product_test_utils_read_int(key,gpio_num);

    if(iret) {
        iret = __product_test_config_led_get_gpio_num(type,gpio_num); 
    
    }

    return iret;
}

int product_test_led_set_gpio_level(LED_TYPE_E type,int level)
{
    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,LED_GPIO_LEVEL_KEY,type);

    return product_test_utils_write_int(key,level,0);
}

int product_test_led_get_gpio_level(LED_TYPE_E type,int*level) 
{
    int iret;
    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,LED_GPIO_LEVEL_KEY,type);

    iret = product_test_utils_read_int(key,level);

    if(iret) {
        iret = product_test_config_led_get_gpio_level(type,level);  
    }

    return iret;
}

int product_test_led_set_in_group(LED_TYPE_E type)
{
    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,LED_IN_GROUP_KEY,type);

    return product_test_utils_write_int(key,1,0);
}

int product_test_led_is_in_group(LED_TYPE_E type) 
{
    int in;
    int iret = 0;

    char key[64] = {0};

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return 0;
    }

    sprintf(key,LED_IN_GROUP_KEY,type);

    if(product_test_utils_read_int(key,&in) == 0 && in == 1) {
        iret = 1;
    }else {
        iret  = product_test_config_led_is_in_group(type);
    }

    return iret;
}


static int __product_test_one_led_on(LED_TYPE_E type) 
{
    return ptal_led_one_led_on(type);
}

int product_test_one_led_on(LED_TYPE_E type) 
{
    int iret;
    LED_INFACE_TYPE_E intface_type;

    if(product_test_led_get_interfece_type(type,&intface_type) == 0  &&
        intface_type == E_LED_INFACE_PWM) {
            iret = product_test_pwm_led_on(type);
    }else {
    
        iret = __product_test_one_led_on(type); 
    }
    return iret;    
}


static  int __product_test_one_led_off(LED_TYPE_E type)
{
    return ptal_led_one_led_off(type);
}

int product_test_one_led_off(LED_TYPE_E type) 
{
    int iret;
    LED_INFACE_TYPE_E intface_type;

    if(sg_one_led_test_flag[type]) {
        sg_one_led_test_flag[type] = 0;
        pthread_join(sg_one_led_tid,NULL);
    }

    if(product_test_led_get_interfece_type(type,&intface_type) == 0  &&
        intface_type == E_LED_INFACE_PWM) {
            iret = product_test_pwm_led_off(type);
    }else {
    
        iret = __product_test_one_led_off(type); 
    }
    return iret;    
}

static void *__one_led_thread_proc(void *arg) 
{
    int type = *(int *)arg;

    TB_LOGD("type=%d",type);

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return NULL;
    }

    while (sg_one_led_test_flag[type]) {
        LED_INFACE_TYPE_E intface_type;

        if(product_test_led_get_interfece_type(type,&intface_type) == 0  &&
            intface_type == E_LED_INFACE_PWM) {
                     product_test_pwm_led_on(type);
        }else {
                __product_test_one_led_on(type); 
        }
        sleep(1);

        if(product_test_led_get_interfece_type(type,&intface_type) == 0  &&
            intface_type == E_LED_INFACE_PWM) {
                     product_test_pwm_led_off(type);
        }else {
                __product_test_one_led_off(type); 
        }

        sleep(1);
    }

    return NULL;
}


int product_test_one_led_flash(LED_TYPE_E type)
{
    TB_LOGD();

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;
    }

    sg_one_led_type[type] = type;
    sg_one_led_test_flag[type] = 1;
    pthread_create(&sg_one_led_tid,NULL,__one_led_thread_proc,&sg_one_led_type[type]);

    return 0;
}

int product_test_led_on(void)
{
    int i;

    TB_LOGD();
    
    for(i=0;i<E_LED_MAX;i++) {
        if(product_test_led_is_in_group(i)) {
            if(product_test_one_led_on(i)) {
                TB_LOGE("led %d on fail",i);
                return -1;
            } 
        } 
    
    }

    return 0;
}

int product_test_led_off(void)
{
    int i;

    TB_LOGD();

    if(sg_led_test_flag) {
        sg_led_test_flag = 0;
        pthread_join(sg_led_tid,NULL);
    }

    for(i=0;i<E_LED_MAX;i++) {
        if(product_test_led_is_in_group(i)) {
            if(product_test_one_led_off(i)) {
                TB_LOGE("led %d off fail",i);
                return -1;
            } 
        } 
    
    }
    return 0;
}

void *__led_thread_proc(void *arg) 
{
    int i;
    while(sg_led_test_flag) {
        for(i=0;i<E_LED_MAX;i++) {
            if(product_test_led_is_in_group(i)) {
                if(product_test_one_led_on(i)) {
                    TB_LOGE("led %d on fail",i);
                    break;
                }
                sleep(1);
            }


            if(product_test_led_is_in_group(i)) {
                if(product_test_one_led_off(i)) {
                    TB_LOGE("led %d on fail",i);
                    break;
                }
                sleep(1);
            }
        }
    }
    return NULL;
}


int product_test_led_flash(void)
{
    TB_LOGD();
    sg_led_test_flag = 1;
    pthread_create(&sg_led_tid,NULL,__led_thread_proc,NULL);

    return 0;
}   


int product_test_irled_set_gpio_num(IRLED_TYPE_E type,int num)
{
    char key[64] = {0};

    sprintf(key,IRLED_GPIO_NUM_KEY,type);

    return product_test_utils_write_int(key,num,0);
}


int product_test_irled_get_gpio_num(IRLED_TYPE_E type,int *num)
{
    int iret;
    char key[64] = {0};
    sprintf(key,IRLED_GPIO_NUM_KEY,type);

    iret = product_test_utils_read_int(key,num);

    if(iret) {
        iret = product_test_config_irled_get_gpio_num(type,num);    
    }
    return iret;
}

int product_test_irled_set_gpio_level(IRLED_TYPE_E type,int level)
{
    char key[64] = {0};
    sprintf(key,IRLED_GPIO_LEVEL_KEY,type);

    return product_test_utils_write_int(key,level,0);
}


int product_test_irled_get_gpio_level(IRLED_TYPE_E type,int* level)
{
    int iret;
    char key[64] = {0};
    sprintf(key,IRLED_GPIO_LEVEL_KEY,type);

    iret = product_test_utils_read_int(key,level);

    if(iret) {
        iret = product_test_config_irled_get_gpio_level(type,level);    
    }

    return iret;
}

int product_test_irled_set_in_group(IRLED_TYPE_E type)
{
    char key[64] = {0};

    if(type >= E_IRLED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;  
    }

    sprintf(key,IRLED_IN_GROUP_KEY,type);

    return product_test_utils_write_int(key,1,0);
}


int product_test_irled_is_in_group(IRLED_TYPE_E type) 
{
    int in;
    int iret = 0;
    char key[64] = {0};

    if(type >= E_IRLED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return 0;
    }

    sprintf(key,IRLED_IN_GROUP_KEY,type);

    if(product_test_utils_read_int(key,&in) == 0 && in == 1) {
        iret = 1;
    }else {
        iret = product_test_config_irled_is_in_group (type);
    }

    return iret;
}


static int __product_test_one_irled_on(IRLED_TYPE_E type) 
{
    return ptal_led_one_irled_on(type); 
}


int product_test_one_irled_on(IRLED_TYPE_E type) 
{
    int iret;
    IRLED_INFACE_TYPE_E intface_type;

    if(product_test_irled_get_interfece_type(type,&intface_type) == 0  &&
        intface_type == E_IRLED_INFACE_PWM) {
            iret = product_test_pwm_irled_on(type);
    }else {
    
        iret = __product_test_one_irled_on(type); 
    }
    return iret;    
}


static int __product_test_one_irled_off(IRLED_TYPE_E type)
{
    TB_LOGD();
    return ptal_led_one_irled_off(type);
}

int product_test_one_irled_off(IRLED_TYPE_E type) 
{
    int iret;
    IRLED_INFACE_TYPE_E intface_type;

    if(product_test_irled_get_interfece_type(type,&intface_type) == 0  &&
        intface_type == E_IRLED_INFACE_PWM) {
            iret = product_test_pwm_irled_off(type);
    }else {
    
        iret = __product_test_one_irled_off(type); 
    }
    return iret;    
}

int product_test_irled_on(void) 
{
    int i;

    for(i=0;i<E_IRLED_MAX;i++) {
        if(product_test_irled_is_in_group(i)) {
            if(product_test_one_irled_on(i)) {
                TB_LOGE("led %d on fail",i);
                return -1;
            } 

        } 
    
    }

    return 0;
}

int product_test_irled_off(void)
{
    int i;

    for(i=0;i<E_IRLED_MAX;i++) {
        if(product_test_irled_is_in_group(i)) {
            if(product_test_one_irled_off(i)) {
                TB_LOGE("led %d off fail",i);
                return -1;
            } 

        } 
    
    }

    return 0;
}

static void *_light_color_auto(void *arg)
{
    TB_LOGD();
    product_test_exec_cmd_no_return ("/tmp/light_prod_test &");
    sleep(8);
    product_test_exec_cmd_no_return ("ps | grep 'light_prod_test' | grep -v 'grep' | awk '{print $1}' | xargs kill");
    pthread_exit(NULL);
    return (void *)0;
}

int product_test_auto_light_color(void)
{
    TB_LOGD();
    pthread_create(&sg_light_color_tid,NULL,_light_color_auto,NULL);
    TB_LOGD(" wait _light_color_auto exit");
    pthread_join(sg_light_color_tid,NULL);
    return 0;
}

int product_test_auto_red_on(void)
{
    if(product_test_led_is_in_group(0)) {
        if(product_test_one_led_on(0)) {
            TB_LOGE("led read on fail");
            return -1;
        };
    }
    return 0;
}

int product_test_auto_green_on(void)
{
    if(product_test_led_is_in_group(1)) {
        if(product_test_one_led_on(1)) {
            TB_LOGE("led green on fail");
            return -1;
        };
    }
    return 0;
}

