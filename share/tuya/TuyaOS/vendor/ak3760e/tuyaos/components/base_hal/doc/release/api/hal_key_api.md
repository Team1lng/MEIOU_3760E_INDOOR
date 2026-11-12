
# 按键 API说明

## hal_gpio_key_get_state

- 功能：
    获取GPIO按键状态。
    该API只适用于单个GPIO控制的不上报键值的按键。
    对于上报键值的按键，请使用 hal_input_event_xxx API。

- 语法：
    int hal_gpio_key_get_state(char *name, hal_gpio_key_state_e *state);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|按键名称。定义在hal_board.h|
|state|输出|保存获取的按键状态。取值： HAL_GPIO_KEY_DOWN 或 HAL_GPIO_KEY_UP|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|错误码|失败|


## hal_input_event_init

- 功能：
    打开并监听所有的input event设备（/dev/input/eventX）。

- 语法：
    int hal_input_event_init(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回-1。


## hal_input_event_recv

- 功能：
    获取input event。

- 语法：
    int hal_input_event_recv(struct input_event *ev, int *count, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|ev|输出|保存获取的input_event|
|count|输出|获取到的input_event个数|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到检测到按键事件<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_input_event_exit

- 功能：
    关闭所有已打开的input event设备。

- 语法：
    void hal_input_event_exit(void);

- 参数：
    无。

- 返回值：
    无。
