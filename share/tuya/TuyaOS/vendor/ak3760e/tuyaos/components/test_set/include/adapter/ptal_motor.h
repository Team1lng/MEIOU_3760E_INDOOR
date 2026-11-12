#ifndef _PTAL_MOTOR_H_
#define _PTAL_MOTOR_H_

#ifdef __cplusplus__
extern "C" {
#endif

#define TY_MOTOR_MAGIC 12
#define TY_MOTOR_IOCTL_STOP     _IO(TY_MOTOR_MAGIC, 0)
#define TY_MOTOR_IOCTL_GET      _IOR(TY_MOTOR_MAGIC, 1, struct ty_motor_param_s)
#define TY_MOTOR_IOCTL_SET      _IOW(TY_MOTOR_MAGIC, 2, struct ty_motor_param_s)
#define TY_MOTOR_IOCTL_PARA_INIT_SET    _IOW(TY_MOTOR_MAGIC, 3, struct ty_motor_param_init_s)




typedef enum {
    MOTOR_MCMD_INIT = 0x100,    
    MOTOR_MCMD_GET,
    MOTOR_MCMD_SET,
    MOTOR_MCMD_STOP,
}MOTO_MAIN_CMD;

typedef enum {
        GO_UP,
        RIGHT_UP,
        GO_RIGHT,
        RIGHT_DOWN,
        GO_DOWN,
        LEFT_DOWN,
        GO_LEFT = 6,
        LEFT_UP,
        PAN_STOP,
        PAN_SELF_CHECK,
        GO_POS,
        TILT_STOP,
        TILT_SELF_CHECK,
        SELF_CHECK,
        ALL_STOP,
        NONE
} PTZ_FUNC;

struct ty_motor_param_s {
        PTZ_FUNC motor_func;
    int goal_pan_speed;
        int goal_tilt_speed;
        int goal_pan_pos;
        int goal_tilt_pos;
};

struct ty_motor_param_init_s {
    int  _pan_roll_step;
    int  _pan_step_max;
    int  _tilt_step_max;
    int  _infi_rot_en;
};

typedef enum {
        GET_PAN_POS,
        GET_TILT_POS,
        GET_PAN_ANGLE,
        GET_TILT_ANGLE,
        GET_PAN_CHECK_STATE,
        GET_TILT_CHECK_STATE,
        GET_PRESET_STATE,
    GET_LEFT_OPTO_FLAG,
    GET_RIGHT_OPTO_FLAG,
    SET_CLEAR_OPTO_FLAG,
    PTZ_STATE_CMD_MAX,
} PTZ_STATE_CMD;

typedef struct motor_cmd_s {
        PTZ_STATE_CMD state_cmd;
        int           state_value;
}MOTOR_CMD_T;

struct ty_motor_state {
    int pan_pos;
        int tilt_pos;
    int right_opto_flag;
    int left_opto_flag;
};

extern int ptal_motor_init(char *dev_name,struct ty_motor_param_init_s *parm,int *fd);
extern int ptal_motor_deinit(int fd);
extern int ptal_motor_ops_cmd(int fd,int cmd,struct ty_motor_param_s *parm);
extern int ptal_motor_ops_state(int fd,int cmd,struct ty_motor_state *state);

#ifdef __cplusplus__
}
#endif

#endif
