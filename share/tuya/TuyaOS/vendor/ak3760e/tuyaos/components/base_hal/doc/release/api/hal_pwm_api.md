
# PWM API说明

## hal_pwm_config

- 功能：
    配置pwm通道的属性

- 语法：
    int hal_pwm_config(int dev, int chn, hal_pwm_chn_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|pwm设备号。如果只有一个pwm设备，设置为0|
|chn|输入|pwm通道号|
|attr|输入|pwm属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_pwm_chn_attr_t 类型

    - 类型定义

        ```
        typedef struct {
            int freq;           /* frequency, in Hz */
            int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
            int polarity;       /* normal or inversed */
            int reserved[5];
        } hal_pwm_chn_attr_t;
        ```

    - 类型说明

|成员|描述|
|----|----|
|freq|pwm频率，单位：Hz|
|duty_cycle|占空比，取值范围：[0, 10000]，例如：1234 表示占空比为 12.34%|
|polarity|pwm极性，取值： HAL_PWM_POL_NORMAL 或 HAL_PWM_POL_INVERSED|


## hal_pwm_enable

- 功能：
    使能pwm通道

- 语法：
    int hal_pwm_enable(int dev, int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|pwm设备号。如果只有一个pwm设备，设置为0|
|chn|输入|pwm通道号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_pwm_disable

- 功能：
    禁止pwm通道。

- 语法：
    int hal_pwm_disable(int dev, int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|pwm设备号。如果只有一个pwm设备，设置为0|
|chn|输入|pwm通道号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
