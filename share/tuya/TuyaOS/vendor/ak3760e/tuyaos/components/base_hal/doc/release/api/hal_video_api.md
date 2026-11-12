
# Video System API说明

## hal_video_sys_init

- 功能：
    视频HAL全局 初始化。调用视频HAL其它API之前，必须先调用该API。

- 语法：
    int hal_video_sys_init(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_sys_deinit

- 功能：
    视频HAL全局 反初始化。当不再使用视频HAL时，调用该API。

- 语法：
    int hal_video_sys_deinit(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。
