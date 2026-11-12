/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include "product_test_motor.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "ptal_motor.h"
#include "tb_log.h"

/* platform start */
/******motor config*******/
#define MAX_PAN_STEP    6090 
#define MAX_TILT_STEP   1360 
#define PAN_CIRCLE_T    8
#define GOAL_PAN_SPEED  1
#define GOAL_TILT_SPEED 1

#define MOTOR_LIMIT_SWITCH_KEY "motor_limit_switch" 
#define MOTOR_DEVNAME_KEY "motor_dev"

/*current value cmd*/


// self check
typedef enum {
    E_FAC_CHECK_INIT,     //init 
    E_PRE_FAC_PAN_CHECK,  //preview check 
    E_FAC_PAN_CHECK,      //pan check
    E_FAC_TILT_CHECK,     //tilt check
    E_CHECK_FINISH,       //check finish
    E_FAC_CHECK_BACK_HOME,//check back to home
    E_FAC_CHECK_NULL,     //check exit
}CHECK_STATE_E;

//motor move status 
typedef enum {
    E_STATE_NONE,  //not move
    E_ACTING,      //move active
    E_FINISH       //move finish
}MOVE_STATE_E;

typedef struct motor_move_param_s { 
    int finish_flag;    //move over flag
    int direct;         //move direction
    int speed;          //move run speed
    int pan_pos;        //move pan position
    int tilt_pos;       //move tilt position
}MOTOR_MOVE_PARAM_T;

typedef struct motor_move_cmd_s {
    int type;   
    MOTOR_MOVE_PARAM_T param;
}MOTOR_MOVE_CMD_T;

/*motor parm*/
typedef struct motor_param_s {
    int max_pan_step;       // pan max step
    int max_tilt_step;      // tilt max step
    int pan_speed;          // pan speed
    int tilt_speed;         // tilt speed
}MOTOR_PARAM_T;

typedef struct motor_move_handle_s {
    MOTOR_MOVE_CMD_T cur_cmd;
    MOTOR_PARAM_T motor_param;
    int init_check;
    int fd;
} MOTOR_MOVE_HANDLE_T;


static int sg_motor_init_flag = 0;
static MOTOR_PARAM_T sg_motor_param = {0};

static int sg_self_check_run = 1;
static int sg_motor_left_opto_flag = 0;
static int sg_motor_right_opto_flag = 0;
static pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;

static int __product_test_read_motor_parm(MOTOR_PARAM_T *parm)
{
    char *dev,*key;
    int enable;
    int value;

    dev = CONFIG_DEVICE_MOTOR;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = CONFIG_DEVICE_MOTOR;
    key = CONFIG_KEY_PAN_SPEED;
    
    if(product_test_config_read_int(dev,key,&value)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    parm->pan_speed = value;

    key = CONFIG_KEY_TILT_SPEED;
    
    if(product_test_config_read_int(dev,key,&value)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    parm->tilt_speed = value;

    key = CONFIG_KEY_PAN_MAX_STEP;
    
    if(product_test_config_read_int(dev,key,&value)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    parm->max_pan_step = value;

    key = CONFIG_KEY_TILT_MAX_STEP;

    if(product_test_config_read_int(dev,key,&value)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    parm->max_tilt_step = value;

    return 0;
}

static int __product_test_config_motor_get_devname (char*name)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_MOTOR;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DEV_NAME;
    
    if(product_test_config_read_string(dev,key,name,32)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

static int __product_test_config_motor_get_limitswitch_support(int *flag)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_MOTOR;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_LIMIT_SWITCH;
    
    if(product_test_config_read_int(dev,key,flag)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

int product_test_motor_set_limitswitch_support(int flag)
{
    return product_test_utils_write_int(MOTOR_LIMIT_SWITCH_KEY,flag,0);
}

int product_test_motor_get_limitswitch_support(int* flag) 
{
    int iret;

    iret = product_test_utils_read_int(MOTOR_LIMIT_SWITCH_KEY,flag);
    if(iret) {
    
        iret = __product_test_config_motor_get_limitswitch_support(flag); 
    }
    return iret;    
}

static MOTOR_MOVE_HANDLE_T * __motor_move_get_handle(void)
{
    static MOTOR_MOVE_HANDLE_T sg_move_handle;
    return &sg_move_handle;
}

static void __motor_set_static_param(MOTOR_PARAM_T *dev_param)
{

    MOTOR_PARAM_T motor_parm = {0};
    dev_param->max_pan_step = MAX_PAN_STEP;
    dev_param->max_tilt_step = MAX_TILT_STEP;
    dev_param->pan_speed = GOAL_PAN_SPEED;
    dev_param->tilt_speed = GOAL_TILT_SPEED;

    if(__product_test_read_motor_parm(&motor_parm) == 0) {
        dev_param->max_pan_step = motor_parm.max_pan_step;
        dev_param->max_tilt_step = motor_parm.max_tilt_step;
        dev_param->pan_speed = motor_parm.pan_speed;
        dev_param->tilt_speed = motor_parm.tilt_speed;
    }

    if(sg_motor_param.max_pan_step) {
        dev_param->max_pan_step = sg_motor_param.max_pan_step;
    }

    if(sg_motor_param.max_tilt_step) {
        dev_param->max_tilt_step = sg_motor_param.max_tilt_step;
    }

    if(sg_motor_param.pan_speed) {
        dev_param->pan_speed = sg_motor_param.pan_speed;
    }

    if(sg_motor_param.tilt_speed) {
        dev_param->tilt_speed = sg_motor_param.tilt_speed;
    }

    return ;
}

int product_test_motor_set_devname(char *name)
{
    return product_test_utils_write_string(MOTOR_DEVNAME_KEY,name,0);
}

int product_test_motor_get_devname(char *name)
{
    int iret ;

    iret =  product_test_utils_read_string(MOTOR_DEVNAME_KEY,name);

    if(iret || strlen(name) == 0 ) {
        iret = __product_test_config_motor_get_devname(name); 
    }
    return iret;
}


static int __product_test_motor_init(char *dev_name)
{
    int iret = 0;
    int fd;
    int flag;
    struct ty_motor_param_init_s parm= {0};
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();

    TB_LOGD();

    if(sg_motor_init_flag) {
        TB_LOGI("motor has inited");
        return 0;
    }

    __motor_set_static_param(&phdl->motor_param);

    parm._pan_step_max = phdl->motor_param.max_pan_step;
    parm._tilt_step_max = phdl->motor_param.max_tilt_step;
    if(product_test_motor_get_limitswitch_support(&flag)==0 && flag == 1) {
        parm._pan_roll_step =  phdl->motor_param.max_pan_step;
        parm._infi_rot_en  = 1;

    }

    TB_LOGD("init max pan:%d max ilt:%d",parm._pan_step_max,parm._tilt_step_max);

    if(ptal_motor_init(dev_name,&parm,&fd)) {
        TB_LOGE("motor init fail"); 
        iret = -1;  
    }else {
        sg_motor_init_flag  = 1;
	phdl->fd = fd;
    }

    return iret;
}

static int __product_test_motor_uinit(void)
{
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();
    sg_motor_init_flag  = 0;

    return ptal_motor_deinit(phdl->fd);
}

static int __product_test_motor_clear_opto_flag(void)
{
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();

    return ptal_motor_ops_cmd(phdl->fd,SET_CLEAR_OPTO_FLAG,NULL);
}

static int __product_test_motor_get_left_opto_flag(int * flag) 
{
    int iret = 0;
    struct ty_motor_state state = {0};
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();

    if(flag == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

    iret = ptal_motor_ops_state(phdl->fd,GET_LEFT_OPTO_FLAG,&state);
    *flag = state.left_opto_flag;

    return iret;
}

static int __product_test_motor_get_right_opto_flag(int * flag) 
{
    int iret = 0;
    struct ty_motor_state state = {0};
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();

    if(flag == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

    iret = ptal_motor_ops_state(phdl->fd,GET_RIGHT_OPTO_FLAG,&state);

    *flag = state.right_opto_flag;

    return iret;
}

static int __product_test_motor_get_cur_pos(int* pan_cur, int *tilt_cur)
{
    int iret = 0;
    struct ty_motor_state state = {0};
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();

    if(pan_cur == NULL || tilt_cur == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

    iret |=  ptal_motor_ops_state(phdl->fd,GET_PAN_POS,&state);
    iret |=  ptal_motor_ops_state(phdl->fd,GET_TILT_POS,&state);

    *pan_cur = state.pan_pos;
    *tilt_cur = state.tilt_pos;

    return iret;
}

int product_test_motor_get_cur_pos(int* pan_cur, int *tilt_cur)
{
    char name[32] = {0};
//  MOTOR_MOVE_HANDLE_T *phdl = NULL;
    int pan_cur_pos     = 0;
    int tilt_cur_pos    = 0;

    if(pan_cur == NULL || tilt_cur == NULL) {
        TB_LOGE("invalid parm"); 
        return -1;	
    }

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }
//  phdl = __motor_move_get_handle();

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }
    
    if(__product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos)) {
        TB_LOGE("get cur pos fail");
        return -1; 
    }

    __product_test_motor_uinit();
    *pan_cur = pan_cur_pos;
    *tilt_cur = tilt_cur_pos;

    TB_LOGI("pan_cur:%d,tilt_cur:%d",*pan_cur,*tilt_cur);

    return 0;
}


static int __product_test_motor_self_check(int *check_flag)
{
    MOTOR_MOVE_HANDLE_T *phdl = __motor_move_get_handle();
    struct ty_motor_param_s param;
    struct ty_motor_state state = {0};
    static int temp_pan_action = 0; 
    static int temp_tilt_action = 0;
    static int self_check_state = 0;
    static int back_pan = 0;
    static int back_tilt = 0;
    int pan_cur_pos = 0;
    static int old_pan_pos=0; 
    int tilt_cur_pos = 0;
    int iret;
    int flag = 0;
    if (*check_flag == E_STATE_NONE) {
        self_check_state = E_FAC_CHECK_INIT;
    }

    switch (self_check_state) {
        case E_FAC_CHECK_INIT:
            temp_pan_action = 0;
            temp_tilt_action = 0;
            back_pan = phdl->motor_param.max_pan_step;
            back_tilt =phdl->motor_param.max_tilt_step;
            //TB_LOGD("back_pan:%d,back_tilt:%d",back_pan,back_tilt);
            iret = ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_STOP,NULL);
            if(iret) {
                TB_LOGE("ptal_motor_ops_cmd %d fail",MOTOR_MCMD_STOP);
            }
            self_check_state = E_FAC_TILT_CHECK;
            *check_flag = E_ACTING;
            TB_LOGI("pan_init\n");
            break;
        case E_FAC_PAN_CHECK:
            /*send_pan_check_cmd()*/
        //  TB_LOGD();
            if (temp_pan_action == 0) {
                param.motor_func = PAN_SELF_CHECK;
                param.goal_pan_speed = phdl->motor_param.pan_speed;
                param.goal_tilt_speed = phdl->motor_param.tilt_speed;
                //TB_LOGD("pan speed:%d tilt speed:%d",param.goal_pan_speed,param.goal_tilt_speed);

                iret = ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);
                if(iret) {
                    TB_LOGE("ptal_motor_ops_cmd %d fail",MOTOR_MCMD_SET);
                }

                temp_pan_action = 1;
                TB_LOGI("pan_check\n");
            }

            iret = ptal_motor_ops_state(phdl->fd,GET_PAN_POS,&state);

            if(iret) {
                TB_LOGE("ptal_motor_ops_state %d fail",GET_PAN_POS);
            }

//          sleep(1);
            /*if pan check_finish,judge the pan cur pos*/
            /*get_the_cur_pos compare with the max pan*/
        //  TB_LOGD("state_value:%d,max_pan_step:%d",state.pan_pos,phdl->motor_param.max_pan_step);
            //if (g_param.state_value == phdl->motor_param.max_pan_step) {
            if (state.pan_pos == 0) {
                TB_LOGI("pan_check over\n");
                self_check_state = E_CHECK_FINISH;
            }
            *check_flag = E_ACTING;
            break;
        case E_FAC_TILT_CHECK:
            //TB_LOGD();
            if (temp_tilt_action == 0) {
                param.motor_func = TILT_SELF_CHECK;
                param.goal_pan_speed = phdl->motor_param.pan_speed;
                param.goal_tilt_speed = phdl->motor_param.tilt_speed;
                iret = ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);

                if(iret) {
                    TB_LOGE("ptal_motor_ops_cmd %d fail",MOTOR_MCMD_SET);
                }

                temp_tilt_action = 1;
                TB_LOGI("tilt_check\n");
            }


            iret = ptal_motor_ops_state(phdl->fd,GET_TILT_POS,&state);

            if(iret) {
                TB_LOGE("ptal_motor_ops_state %d fail",GET_PAN_POS);
            }
//          sleep(1);
            /*if pan check_finish,judge the pan cur pos*/
            /*get_the_cur_pos compare with the max pan*/
        //  TB_LOGD("tile value:%d",state.tilt_pos);
            if (state.tilt_pos == 0) {
                TB_LOGI("pan_check start\n");
                self_check_state = E_FAC_PAN_CHECK;
            }
            *check_flag = E_ACTING;
            break;
        case E_CHECK_FINISH:
            /*自检完成后，需要恢复之前设置的巡航动作*/   
            TB_LOGI("check over");
//          param.motor_func = GO_POS;
//

            if(product_test_motor_get_limitswitch_support(&flag)==0 && flag == 1) {
                TB_LOGD("RIGHT UP");
                param.motor_func = RIGHT_UP;
            }else {
                TB_LOGD("GO POS");
                param.motor_func = GO_POS;
            }
/*
            TB_LOGD("GO POS");
            param.motor_func = GO_POS;
*/

            param.goal_pan_pos = back_pan;
            param.goal_tilt_pos = back_tilt;
            param.goal_pan_speed = phdl->motor_param.pan_speed;
            param.goal_tilt_speed = phdl->motor_param.tilt_speed;
            iret = ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);

            if(iret) {
                TB_LOGE("ptal_motor_ops_cmd %d fail",MOTOR_MCMD_SET);
            }

            *check_flag = E_ACTING;
            self_check_state = E_FAC_CHECK_BACK_HOME;

            TB_LOGI("check back home");
            break;
        case E_FAC_CHECK_BACK_HOME:
            //TB_LOGD();
            __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);
//          sleep(1);
        //  TB_LOGD("pan_cur_pos:%d,tilt_cur_pos:%d,back_pan:%d,back_tilt:%d,old_pan_pos:%d",pan_cur_pos ,tilt_cur_pos,back_pan,back_tilt,old_pan_pos);
            if ((pan_cur_pos >= back_pan || pan_cur_pos < old_pan_pos) && (tilt_cur_pos >= back_tilt)) {
                    param.motor_func = GO_POS;
                param.goal_pan_pos = phdl->motor_param.max_pan_step/2;
                param.goal_tilt_pos = phdl->motor_param.max_tilt_step/2;
                param.goal_pan_speed = phdl->motor_param.pan_speed;
                param.goal_tilt_speed = phdl->motor_param.tilt_speed;
                iret = ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);
                if(iret) {
                    TB_LOGE("ptal_motor_ops_cmd %d fail",GO_POS);
                }

                self_check_state = E_FAC_CHECK_NULL;
                TB_LOGI("check to half");
            }
            old_pan_pos = pan_cur_pos;
    
            break;
        case E_FAC_CHECK_NULL:

           TB_LOGI("wait ...");
           do{
               usleep(10000); 
               __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);

	//       TB_LOGI("max_pan:%d,max_tilt:%d pan_cur_pos:%d,tilt_cur_pos\n",phdl->motor_param.max_pan_step/2,phdl->motor_param.max_tilt_step/2,pan_cur_pos,tilt_cur_pos);

           }while(pan_cur_pos != phdl->motor_param.max_pan_step/2 || tilt_cur_pos != phdl->motor_param.max_tilt_step/2);

           TB_LOGI("motor test over");


        //  iret = ptal_motor_ops_cmd(MOTOR_MCMD_STOP,NULL);  
        //  if(iret) {
        //      TB_LOGE("ptal_motor_ops_cmd %d fail",MOTOR_MCMD_STOP);
        //  }
            sg_self_check_run = 0;
            *check_flag = E_STATE_NONE;
            break;
        default:
            break;
    }
    return 0;
}

int product_test_motor_move_up(int speed,int steps, int sync)
{
    char name[32] = {0};
    MOTOR_MOVE_HANDLE_T *phdl = NULL;
    struct ty_motor_param_s param;
    int pan_cur_pos     = 0;
    int tilt_cur_pos    = 0;
    int cur_pan,cur_tilt;

    TB_LOGD();

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }
    phdl = __motor_move_get_handle();

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }
    
    __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);

    tilt_cur_pos = tilt_cur_pos + steps;
    if (tilt_cur_pos > phdl->motor_param.max_tilt_step)
        tilt_cur_pos = phdl->motor_param.max_tilt_step;
    
    param.motor_func = GO_POS;
    param.goal_pan_pos = pan_cur_pos;
    param.goal_tilt_pos = tilt_cur_pos;
    param.goal_pan_speed = phdl->motor_param.pan_speed;
    param.goal_tilt_speed = phdl->motor_param.tilt_speed;

    ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);


    if(!sync) {
        goto UINIT;	
    }

    do{
        usleep(10000);
        __product_test_motor_get_cur_pos(&cur_pan, &cur_tilt);

    }while(cur_pan != pan_cur_pos || cur_tilt != tilt_cur_pos);

UINIT:
    __product_test_motor_uinit();
    
    return 0;
}

int product_test_motor_move_down(int speed,int steps,int sync) 
{
    char name[32] = {0};
    MOTOR_MOVE_HANDLE_T *phdl = NULL;
    struct ty_motor_param_s param;
    int pan_cur_pos     = 0;
    int tilt_cur_pos    = 0;
    int cur_pan,cur_tilt;

    TB_LOGD();

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }
    phdl = __motor_move_get_handle();

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }

    __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);

    tilt_cur_pos = tilt_cur_pos - steps;
    if (tilt_cur_pos < 0)
        tilt_cur_pos = 0;
    
    param.motor_func = GO_POS;
    param.goal_pan_pos = pan_cur_pos;
    param.goal_tilt_pos = tilt_cur_pos;
    param.goal_pan_speed = phdl->motor_param.pan_speed;
    param.goal_tilt_speed = phdl->motor_param.tilt_speed;

    ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);

    if(!sync) {
        goto UINIT;	
    }

    do{
        usleep(10000); 
        __product_test_motor_get_cur_pos(&cur_pan, &cur_tilt);

    }while(cur_pan != pan_cur_pos || cur_tilt != tilt_cur_pos);

UINIT:
    __product_test_motor_uinit();
    
    return 0;
}

int product_test_motor_move_left(int speed,int steps,int sync) 
{
    char name[32] = {0};
    MOTOR_MOVE_HANDLE_T *phdl = NULL;
    struct ty_motor_param_s param;
    int pan_cur_pos     = 0;
    int tilt_cur_pos    = 0;
    int cur_pan,cur_tilt;

    TB_LOGD();

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }

    __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);

    pan_cur_pos = pan_cur_pos - steps;
    if (pan_cur_pos < 0)
        pan_cur_pos = 0;
    
    param.motor_func = GO_POS;
    param.goal_pan_pos = pan_cur_pos;
    param.goal_tilt_pos = tilt_cur_pos;

    phdl = __motor_move_get_handle();

    param.goal_pan_speed = phdl->motor_param.pan_speed;
    param.goal_tilt_speed = phdl->motor_param.tilt_speed;

    ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);

    if(!sync) {
        goto UINIT;	
    }

    do{
        usleep(10000); 
        __product_test_motor_get_cur_pos(&cur_pan, &cur_tilt);

    }while(cur_pan != pan_cur_pos || cur_tilt != tilt_cur_pos);

UINIT:
    __product_test_motor_uinit();
    
    return 0;

}

int product_test_motor_move_right(int speed,int steps,int sync) 
{
    char name[32] = {0};
    MOTOR_MOVE_HANDLE_T *phdl = NULL;
    struct ty_motor_param_s param;
    int pan_cur_pos     = 0;
    int tilt_cur_pos    = 0;
    int cur_pan,cur_tilt;

    TB_LOGD();

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }

    __product_test_motor_get_cur_pos(&pan_cur_pos, &tilt_cur_pos);

    phdl = __motor_move_get_handle();

    pan_cur_pos = pan_cur_pos + steps;
    if (pan_cur_pos > phdl->motor_param.max_pan_step)
        pan_cur_pos = phdl->motor_param.max_pan_step;
    
    param.motor_func = GO_POS;
    param.goal_pan_pos = pan_cur_pos;
    param.goal_tilt_pos = tilt_cur_pos;

    param.goal_pan_speed = phdl->motor_param.pan_speed;
    param.goal_tilt_speed = phdl->motor_param.tilt_speed;

    ptal_motor_ops_cmd(phdl->fd,MOTOR_MCMD_SET,&param);

    if(!sync) {
        goto UINIT;	
    }

    do{
        usleep(10000); 
        __product_test_motor_get_cur_pos(&cur_pan, &cur_tilt);

    }while(cur_pan != pan_cur_pos || cur_tilt != tilt_cur_pos);

UINIT:
    __product_test_motor_uinit();
    
    return 0;
}


int product_test_motor_self_check(void) 
{
    int flag;
    char name[32] = {0};
    int check_flag;
    int left_flag=0,right_flag=0;
//  MOTOR_MOVE_HANDLE_T *phdl;

    if(product_test_motor_get_devname(name)) {
        TB_LOGE("invalid parm");
        return -1;
    }

    TB_LOGD("dev_name:%s\n",name);
//  phdl = __motor_move_get_handle();

    if(__product_test_motor_init(name)) {
        TB_LOGE("motor init fail");
        return -1;	
    }

    if(product_test_motor_get_limitswitch_support(&flag)==0 && flag == 1) {
        TB_LOGD("clear limt switch");
        __product_test_motor_clear_opto_flag();

        pthread_mutex_lock(&sg_mutex);
        sg_motor_left_opto_flag = 0;
        sg_motor_right_opto_flag = 0;
        pthread_mutex_unlock(&sg_mutex);
    }

    sg_self_check_run = 1;

    check_flag = E_STATE_NONE;

    while(sg_self_check_run) {
        __product_test_motor_self_check(&check_flag);
    }

    if(product_test_motor_get_limitswitch_support(&flag)==0 && flag == 1) {

        TB_LOGD("get limt switch");
        __product_test_motor_get_left_opto_flag(&left_flag);
        __product_test_motor_get_right_opto_flag(&right_flag); 
    }

    pthread_mutex_lock(&sg_mutex);
    sg_motor_left_opto_flag = left_flag;
    sg_motor_right_opto_flag = right_flag;
    pthread_mutex_unlock(&sg_mutex);

    __product_test_motor_uinit();

    TB_LOGI("left_flag:%d,right_flag:%d",left_flag,right_flag);


    return 0;
}


int product_test_get_left_limitswitch_flag(int *flag) 
{
    pthread_mutex_lock(&sg_mutex);
    *flag = sg_motor_left_opto_flag;
    pthread_mutex_unlock(&sg_mutex);
    return 0;
}

int product_test_get_right_limitswitch_flag(int *flag) 
{
    pthread_mutex_lock(&sg_mutex);
    *flag  = sg_motor_right_opto_flag;
    pthread_mutex_unlock(&sg_mutex);
    return 0;
}

int product_test_set_pan_max_step(unsigned int max_step) 
{
    MOTOR_PARAM_T *parm  = &sg_motor_param;
    parm->max_pan_step = max_step;
    return 0;
}

int product_test_set_tilt_max_step(int max_step) 
{
    MOTOR_PARAM_T *parm  = &sg_motor_param;
    parm->max_tilt_step = max_step;
    return 0;
}

int product_test_set_pan_speed(int speed) 
{
    MOTOR_PARAM_T *parm  = &sg_motor_param;
    parm->pan_speed = speed;
    return 0;
}

int product_test_set_tilt_speed(int speed) 
{
    MOTOR_PARAM_T *parm  = &sg_motor_param;
    parm->tilt_speed = speed;
    return 0;
}
