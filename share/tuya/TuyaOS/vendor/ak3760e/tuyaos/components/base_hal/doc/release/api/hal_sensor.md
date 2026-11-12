
# Sensor API说明

## hal_sensors_init

- 功能：
    初始化系统中所有的sensor。

- 语法：
    int hal_sensors_init(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_sensors_exit

- 功能：
    反初始化系统中所有的sensor。

- 语法：
    int hal_sensors_exit(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_sensor_get

- 功能：
    根据sensor类型和索引获取sensor句柄。

- 语法：
    hal_sensor_t hal_sensor_get(enum hal_sensor_type type, int index);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|type|输入|sensor类型|
|index|输入|sensor索引|

- 返回值：
    成功返回sensor句柄，失败返回0。


## hal_sensor_put

- 功能：
    释放sensor句柄。

- 语法：
    void hal_sensor_put(hal_sensor_t sensor);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|sensor|输入|sensor句柄|

- 返回值：
    无。


## hal_sensor_read

- 功能：
    读取sensor数据。

- 语法：
    float hal_sensor_read(hal_sensor_t sensor);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|sensor|输入|sensor句柄|

- 返回值：
    成功返回sensor数据，失败返回负数。
