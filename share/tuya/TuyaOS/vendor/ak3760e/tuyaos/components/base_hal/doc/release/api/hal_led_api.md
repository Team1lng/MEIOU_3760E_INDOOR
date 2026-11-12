
# LED API说明

## hal_led_switch

- 功能：
    打开/关闭 开关型LED

- 语法：
    int hal_led_switch(char *name, int on);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|LED名称。定义在hal_board.h|
|on|输入|表示打开/关闭LED。取值：LED_ON 或 LED_OFF |

- 返回值：
    成功返回0，失败返回Linux标准错误码。

- 注意事项：
    该API用于控制开关型LED（比如用单个GPIO控制的LED），不能控制亮度渐变型LED（比如PWM控制的LED）


## hal_led_pwm_control

- 功能：
    控制 亮度渐变型LED（通过pwm控制）的亮度。

- 语法：
    int hal_led_pwm_control(char *name, hal_led_pwm_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|LED名称。定义在hal_board.h|
|attr|输入|配置pwm属性（频率、占空比等）|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


    ### hal_led_pwm_attr_t 类型

    - 类型定义

    ```
        typedef struct {
            int freq;           /* frequency, in Hz */
            int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
            int reserved[2];
        } hal_led_pwm_attr_t;
    ```

    - 类型说明

|成员|描述|
|----|----|
|freq|pwm频率，单位：Hz|
|duty_cycle|占空比，取值范围：[0, 10000]，例如：1234 表示占空比为 12.34%|


## hal_led_control

- 功能：
    控制 开关型LED（gpio led） 或 亮度渐变型LED（pwm led）。

- 语法：
    int hal_led_control(char *name, hal_led_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|LED名称。定义在hal_board.h|
|attr|输入|配置pwm属性（频率、占空比等）|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

- 注意事项：
    对于某些LED（比如红外LED），存在这样的情况：在某些平台用GPIO控制，在某些平台用pwm控制。
    应用可以通过该API控制此类LED，以简化应用代码。

    ### hal_led_attr_t 类型

    - 类型定义

    ```
        typedef struct {
            int freq;           /* frequency, in Hz */
            int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
            int reserved[2];
        } hal_led_attr_t;
    ```

    - 类型说明

|成员|描述|
|----|----|
|freq|pwm频率，单位：Hz<br>对于gpio led，设置为任意值<br>对于pwm led，设置为pwm频率|
|duty_cycle|占空比，取值范围：[0, 10000]，例如：1234 表示占空比为 12.34%<br>对于gpio led，设为0表示关闭LED，设为非零值表示打开LED|
