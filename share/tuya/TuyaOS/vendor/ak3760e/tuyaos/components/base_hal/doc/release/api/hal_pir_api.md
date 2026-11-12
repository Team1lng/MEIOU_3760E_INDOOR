
# PIR API说明

## hal_pir_enable

- 功能：
    使能/禁止PIR

- 语法：
    int hal_pir_enable(char *name, bool enable);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|pir名称。定义在hal_board.h。<br>如果只有一个pir，name可设为NULL，表示使用默认名称|
|enable|输入|表示是否使能PIR|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_pir_set_sens

- 功能：
    设置pir灵敏度

- 语法：
    int hal_pir_set_sens(char *name, int sens);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|pir名称。定义在hal_board.h。<br>如果只有一个pir，name可设为NULL，表示使用默认名称|
|sens|输入|灵敏度。可以传入 hal_pir_sens_e 枚举值|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_pir_sens_e 类型

    - 类型定义
        ```
        typedef enum {
            HAL_PIR_SENS_DEFAULT,   /* default sensitivity configured by hardware */
            HAL_PIR_SENS_LOW,       /* low level sensitivity */
            HAL_PIR_SENS_MIDDLE,    /* middle level sensitivity */
            HAL_PIR_SENS_HIGH,      /* high level sensitivity */
        } hal_pir_sens_e;
        ```
    - 类型说明

|成员|描述|
|----|----|
|HAL_PIR_SENS_DEFAULT|表示使用硬件默认的灵敏度，软件不配置灵敏度|
|HAL_PIR_SENS_LOW|低级别灵敏度|
|HAL_PIR_SENS_MIDDLE|中级别灵敏度|
|HAL_PIR_SENS_HIGH|高级别灵敏度|


## hal_pir_detect_state

- 功能：
    检测pir事件并返回pir状态。

- 语法：
    int hal_pir_detect_state(char *name, hal_pir_state_e *state, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|pir名称。定义在hal_board.h。<br>如果只有一个pir，name可设为NULL，表示使用默认名称|
|state|输出|保存获取的pir状态，取值：<br>HAL_PIR_TRIG 触发状态<br>HAL_PIR_NON_TRIG 未触发|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到检测到pir事件<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


# 示例

```
    hal_pir_state_e pir_state;
    int ret;

    ret = hal_pir_enable(NULL, TRUE);
    if (ret != 0) {
        //error
    }

    ret = hal_pir_set_sens(NULL, HAL_PIR_SENS_DEFAULT);
    if (ret != 0) {
        //error
    }

    ret = hal_pir_detect_state(NULL, &pir_state, timeout);
    if ((ret != 0) && (ret != -ETIMEDOUT)) {
        // error
    }

    if (pir_state == HAL_PIR_TRIG) {
        printf("pir triggered\n");
    } else {
        printf("pir not triggered\n");
    }

```
