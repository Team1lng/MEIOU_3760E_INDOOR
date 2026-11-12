
# RTC API说明

## hal_rtc_init

- 功能：
    初始化RTC设备。

- 语法：
    int hal_rtc_init(void);

- 参数：无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。

## hal_rtc_deinit

- 功能：
    反初始化RTC设备。

- 语法：
    int hal_rtc_deinit(void);

- 参数：无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。

## hal_rtc_set_time

- 功能：
    设置RTC时间。

- 语法：
    int hal_rtc_set_time(hal_rtc_time_t *time);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|time|输入|rtc时间|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_rtc_time_t 类型

    - 类型定义
        ```
        typedef struct {
            int second;
            int minute;
            int hour;
            int day;
            int month;
            int year;
        } hal_rtc_time_t;
        ```
    - 类型说明

|成员|描述|
|----|----|
|second|秒|
|minute|分|
|hour|时|
|day|日|
|month|月|
|year|年|

## hal_rtc_get_time

- 功能：
    读取RTC时间。

- 语法：
    int hal_rtc_get_time(hal_rtc_time_t *time);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|time|输出|rtc时间|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
