
# 电机 API说明

## hal_motor_init

- 功能：
    初始化电机。

- 语法：
    int hal_motor_init(int dev, hal_motor_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|电机设备编号。如果只有一个电机，设置为0|
|attr|输入|电机设备属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_motor_attr_t 类型

    - 类型定义
        ```
        typedef struct {
            int pan_step_max;   /* maximum number of horizontal steps */
            int tilt_step_max;  /* maximum number of vertical steps */
            int pan_roll_step;  /* horizontal rollback position (in steps) */
            bool infi_rot_en;   /* infinity rotation enable */

        } __attribute__((packed, aligned(64))) hal_motor_attr_t;
        ```
    - 类型说明

|成员|描述|
|----|----|
|pan_step_max|最大水平步长|
|tilt_step_max|最大垂直步长|
|pan_roll_step|水平rollback位置，单位：步长|
|infi_rot_en|表示是否使能无极旋转|


## hal_motor_deinit

- 功能：
    反初始化电机。

- 语法：
    int hal_motor_deinit(int dev);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|电机设备编号。如果只有一个电机，设置为0|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_motor_op

- 功能：
    执行电机操作。

- 语法：
    int hal_motor_op(int dev, hal_motor_cmd_t *cmd);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|电机设备编号。如果只有一个电机，设置为0|
|cmd|输入|电机操作命令|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

### hal_motor_cmd_t 类型

- 类型定义
    ```
    typedef struct {
        hal_motor_cmd_id_e id;
        hal_motor_cmd_param_t param;
    } hal_motor_cmd_t;
    ```
- 类型说明

|成员|描述|
|----|----|
|id|命令ID|
|param|命令参数|

#### hal_motor_cmd_id_e 类型

- 类型说明

|成员|描述|
|----|----|
|HAL_MOTOR_CMD_NONE|空命令。不执行任何操作|
|HAL_MOTOR_CMD_GO_UP|向上运动|
|HAL_MOTOR_CMD_GO_DOWN|向下运动|
|HAL_MOTOR_CMD_GO_LEFT|向左运动|
|HAL_MOTOR_CMD_GO_RIGHT|向右运动|
|HAL_MOTOR_CMD_GO_LEFT_UP|同时向左向上运动|
|HAL_MOTOR_CMD_GO_LEFT_DOWN|同时向左向下运动|
|HAL_MOTOR_CMD_GO_RIGHT_UP|同时向右向上运动|
|HAL_MOTOR_CMD_GO_RIGHT_DOWN|同时向右向下运动|
|HAL_MOTOR_CMD_GO_POS|向指定位置运动|
|HAL_MOTOR_CMD_STOP_ALL|同时停止水平和垂直运动|
|HAL_MOTOR_CMD_STOP_PAN|停止水平运动|
|HAL_MOTOR_CMD_STOP_TILT|停止垂直运动|
|HAL_MOTOR_CMD_SELF_CHECK_ALL|执行水平和垂直自检|
|HAL_MOTOR_CMD_SELF_CHECK_PAN|执行水平自检|
|HAL_MOTOR_CMD_SELF_CHECK_TILT|执行垂直自检|

#### hal_motor_cmd_param_t 类型

- 类型定义
    ```
    typedef struct {
        /* for HAL_MOTOR_CMD_GO_xxxxxx */
        int pan_step_period;    /* horizontal step period (in milliseconds) */
        int tilt_step_period;   /* vertical step period (in milliseconds) */

        /* for HAL_MOTOR_CMD_GO_POS */
        int pan_pos;    /* horizontal position (in steps) */
        int tilt_pos;   /* vertical position (in steps) */

    } __attribute__((packed, aligned(64))) hal_motor_cmd_param_t;
    ```
- 类型说明

|成员|描述|
|----|----|
|pan_step_period|水平步长周期（毫秒），只对 HAL_MOTOR_CMD_GO_xxx 有效|
|tilt_step_period|垂直步长周期（毫秒），只对 HAL_MOTOR_CMD_GO_xxx 有效|
|pan_pos|水平位置，单位：步长。只用于 HAL_MOTOR_CMD_GO_POS 命令|
|tilt_pos|垂直位置，单位：步长。只用于 HAL_MOTOR_CMD_GO_POS 命令|


## hal_motor_get_state

- 功能：
    获取电机状态。

- 语法：
    int hal_motor_get_state(int dev, hal_motor_stat_t *stat);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|电机设备编号。如果只有一个电机，设置为0|
|stat|输出|保存电机状态|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_motor_stat_t 类型

    - 类型定义
        ```
        typedef struct {
            int pan_pos;    /* horizontal position (in steps) */
            int tilt_pos;   /* vertical position (in steps) */

        } __attribute__((packed, aligned(128))) hal_motor_stat_t;
        ```

    - 类型说明

|成员|描述|
|----|----|
|pan_pos|水平位置，单位：步长|
|tilt_pos|垂直位置，单位：步长|


# 示例

```
static void motor_func_example(int dev)
{
    hal_motor_cmd_t cmd;
    hal_motor_stat_t motor_stat;
    int pan_step_period;    /* horizontal step period (in milliseconds) */
    int tilt_step_period;   /* vertical step period (in milliseconds) */
    int pan_pos;            /* horizontal position (in steps) */
    int tilt_pos;           /* vertical position (in steps) */
    int func_num = 0;
    int ret;

    ret = hal_motor_init(dev, NULL);
    if (ret != 0) {
        printf("error: hal_motor_init failed\n");
        return;
    }

    cmd.param.pan_step_period = 5;
    cmd.param.tilt_step_period = 4;

    while (!sg_flag) {
        printf("0  go up\n");
        printf("1  go down\n");
        printf("2  go left\n");
        printf("3  go right\n");
        printf("4  go left_up\n");
        printf("5  go left_down\n");
        printf("6  go right_up\n");
        printf("7  go right_down\n");
        printf("8  go specific position\n");
        printf("----------\n");

        printf("9  all stop\n");
        printf("10 pan stop\n");
        printf("11 tilt stop\n");
        printf("----------\n");

        printf("12 all self check\n");
        printf("13 pan self check\n");
        printf("14 tilt self check\n");
        printf("----------\n");

        printf("15 set pan-tilt speed\n");
        printf("16 get pan-tilt position\n");
        printf("17 exit\n");
        printf("please enter the function number: \n");

        ret = scanf("%d", &func_num);
        if (ret != 1) {
            printf("scanf error %d\n", ret);
        }
        getchar();

        switch(func_num) {
        case 0:
            cmd.id = HAL_MOTOR_CMD_GO_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 1:
            cmd.id = HAL_MOTOR_CMD_GO_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 2:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 3:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 4:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 5:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 6:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 7:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 8:
            printf("please input the pan position:");
            ret = scanf("%d", &pan_pos);
            getchar();
            if (ret != 1) {
                printf("scanf error %d\n", ret);
                break;
            }

            printf("please input the tilt position:");
            ret = scanf("%d", &tilt_pos);
            getchar();
            if (ret != 1) {
                printf("scanf error %d\n", ret);
                break;
            }

            cmd.id = HAL_MOTOR_CMD_GO_POS;
            cmd.param.pan_pos = pan_pos;
            cmd.param.tilt_pos = tilt_pos;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 9:
            cmd.id = HAL_MOTOR_CMD_STOP_ALL;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 10:
            cmd.id = HAL_MOTOR_CMD_STOP_PAN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 11:
            cmd.id = HAL_MOTOR_CMD_STOP_TILT;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 12:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_ALL;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 13:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_PAN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 14:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_TILT;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 15:
            printf("please input the pan_step_period:");
            ret = scanf("%d", &pan_step_period);
            getchar();
            if (ret != 1) {
                printf("scanf pan speed erro %d\n", ret);
                break;
            }

            printf("please input the tilt_step_period:");
            ret = scanf("%d", &tilt_step_period);
            getchar();
            if (ret != 1) {
                printf("scanf tilt speed erro %d\n", ret);
                break;
            }

            cmd.param.pan_step_period = pan_step_period;
            cmd.param.tilt_step_period = tilt_step_period;
            printf("new pan_step_period: %d\n", pan_step_period);
            printf("new tilt_step_period: %d\n", tilt_step_period);
            break;

        case 16:
            ret = hal_motor_get_state(dev, &motor_stat);
            if (ret != 0) {
                printf("hal_motor_get_state failed\n");
                break;
            }

            printf("pan pos: %d\n", motor_stat.pan_pos);
            printf("tilt pos: %d\n", motor_stat.tilt_pos);
            break;

        case 17:
        default:
            sg_flag = 1;
            break;
        }
    }

    hal_motor_deinit(dev);
    return;
}
```
