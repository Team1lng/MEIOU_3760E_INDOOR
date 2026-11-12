
# GPIO API说明

## hal_gpio_config

- 功能：
    配置GPIO管脚

- 语法：
    int hal_gpio_config(int num, hal_gpio_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|num|输入|GPIO管脚编号|
|attr|输入|GPIO管脚属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


    ### hal_gpio_attr_t 类型

    - 类型定义

    ```
        typedef struct {
            int direction;
            int value;
            bool irq_en;
            gpio_irq_type_e irq_type;
            int reserved[4];
        } hal_gpio_attr_t;
    ```

    - 类型说明

|成员|描述|
|----|----|
|direction|输入/输出方向。取值：GPIO_OUT 或 GPIO_IN|
|value|初始电平。只对GPIO输出管脚有效|
|irq_en|表示是否使能GPIO中断。只对GPIO输入管脚有效|
|irq_type|中断触发类型（上升沿/下降沿/双边沿触发）。只对GPIO输入管脚有效|


## hal_gpio_set_value

- 功能：
    设置GPIO电平。

- 语法：
    int hal_gpio_set_value(int num, int value);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|num|输入|GPIO管脚编号|
|value|输入|GPIO管脚电平。取值：GPIO_HIGH 或 GPIO_LOW|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

## hal_gpio_get_value

- 功能：
    读取GPIO管脚电平

- 语法：
    int hal_gpio_get_value(int num, int *value);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|num|输入|GPIO管脚编号|
|value|输出|保存读取的GPIO管脚电平|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

## hal_gpio_wait_for_irq

- 功能：
    等待GPIO管脚中断。

- 语法：
    int hal_gpio_wait_for_irq(int num, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|num|输入|GPIO管脚编号|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到中断触发<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|

# 示例

## 设置GPIO电平

```
    hal_gpio_attr_t attr = {0};
    int value;
    int ret;

    value = GPIO_HIGH;

    attr.direction = GPIO_OUT;
    attr.value = value;
    ret = hal_gpio_config(gpio_num, &attr);
    if (ret < 0) {
        // error
    }

    ret = hal_gpio_set_value(gpio_num, value);
```

## 读取GPIO电平

```
    hal_gpio_attr_t attr = {0};
    int ret;

    attr.direction = GPIO_IN;
    attr.irq_en = FALSE;
    ret = hal_gpio_config(gpio_num, &attr);
    if (ret != 0) {
        // error log
        return -1;
    }

    ret = hal_gpio_get_value(gpio_num, &value);
    if (ret != 0) {
        // error log
        return -1;
    }
```

## 等待GPIO中断

```
    hal_gpio_attr_t attr = {0};
    int ret;

    attr.direction = GPIO_IN;
    attr.irq_en = TRUE;
    if (gpio_hi_active) {
        attr.irq_type = GPIO_IRQ_EDGE_RISING;
    } else {
        attr.irq_type = GPIO_IRQ_EDGE_FALLING;
    }
    ret = hal_gpio_config(gpio_num, &attr);
    if (ret != 0) {
        // error log
        return -1;
    }

    ret = hal_gpio_wait_for_irq(gpio_num, timeout);

```
