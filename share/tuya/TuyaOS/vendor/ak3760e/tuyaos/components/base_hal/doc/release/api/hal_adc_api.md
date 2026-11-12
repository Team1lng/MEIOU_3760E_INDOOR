
# ADC API说明

## hal_adc_get_value

- 功能：
    从ADC通道读取数据。

- 语法：
    int hal_adc_get_value(int dev, int chn, int *value);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|adc设备号。如果只有一个adc设备，设置为0|
|chn|输入|adc通道号|
|value|输出|保存从ADC通道读取的数据|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
