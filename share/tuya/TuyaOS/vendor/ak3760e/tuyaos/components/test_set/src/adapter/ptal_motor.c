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
#include "ptal_motor.h"
#include "tb_log.h"


int __attribute__((weak)) ptal_motor_init(char *dev_name,struct ty_motor_param_init_s *parm,int *fd)
{
    int fdev;

    TB_LOGD();
    
    fdev = open(dev_name, O_RDWR);

    if (fdev < 0) {
        TB_LOGD("can't open %s\n",dev_name);
        return -1;
    }

    *fd = fdev;

    if(parm) {
        ioctl(fdev,TY_MOTOR_IOCTL_PARA_INIT_SET,parm);  
    }

    return 0;
}

int __attribute__((weak)) ptal_motor_deinit(int fd) 
{
    TB_LOGD();
    close(fd);
    return 0;
}

int __attribute__((weak)) ptal_motor_ops_state(int fd,int cmd,struct ty_motor_state *state)
{
    MOTOR_CMD_T param = {0};

    //TB_LOGD();

    if(cmd > PTZ_STATE_CMD_MAX) {
        TB_LOGE("invalid cmd");
        return -1;
    }

    param.state_cmd = cmd;

    if(ioctl(fd,TY_MOTOR_IOCTL_GET,&param)) {
        TB_LOGE("fd:%d motor get fail",fd);
        return -1;  
    }

    switch (cmd) {
        case GET_PAN_POS:
            state->pan_pos = param.state_value;
            break;
        case GET_TILT_POS:
            state->tilt_pos = param.state_value;
            break;
        case GET_LEFT_OPTO_FLAG:
            state->left_opto_flag = param.state_value;
            TB_LOGD("===param.state_value:%d",param.state_value);
            break;
        case GET_RIGHT_OPTO_FLAG:
            state->right_opto_flag = param.state_value;
            TB_LOGD("====param.state_value:%d",param.state_value);
            break;

        default:
            break;
    }

    return 0;
}

int __attribute__((weak)) ptal_motor_ops_cmd(int fd,int cmd,struct ty_motor_param_s *parm)
{
//  TB_LOGD();

    if(cmd == SET_CLEAR_OPTO_FLAG) {
        struct ty_motor_state state = {0};
        return ptal_motor_ops_state(fd,SET_CLEAR_OPTO_FLAG,&state);
    }
    
    if (cmd ==  MOTOR_MCMD_SET) {
        cmd = TY_MOTOR_IOCTL_SET;
    }else if(cmd == MOTOR_MCMD_STOP) {
        cmd = TY_MOTOR_IOCTL_STOP;
    }

    return ioctl(fd,cmd,parm);
}

