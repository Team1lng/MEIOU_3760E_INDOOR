
# VENC API说明

## hal_video_venc_init

- 功能：
    初始化VENC

- 语法：
    int hal_video_venc_init(hal_video_venc_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|attr|输入|VENC属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_deinit

- 功能：
    反初始化VENC

- 语法：
    int hal_video_venc_deinit(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_create_channel

- 功能：
    创建VENC通道

- 语法：
    int hal_video_venc_create_channel(int chn, hal_video_venc_chn_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|
|attr|输入|VENC通道属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_destroy_channel

- 功能：
    销毁VENC通道

- 语法：
    int hal_video_venc_destroy_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_start_channel

- 功能：
    启动VENC通道，开始编码。

- 语法：
    int hal_video_venc_start_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_stop_channel

- 功能：
    停止VENC通道

- 语法：
    int hal_video_venc_stop_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_wait_for_stream

- 功能：
    等待编码的码流就绪。

- 语法：
    int hal_video_venc_wait_for_stream(int chn, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到码流就绪<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_video_venc_get_packet_num

- 功能：
    获取码流包个数

- 语法：
    int hal_video_venc_get_packet_num(int chn, int *num);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|
|num|输出|保存码流包个数|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_venc_get_stream

- 功能：
    获取码流

- 语法：
    int hal_video_venc_get_stream(int chn, hal_video_venc_stream_t *stream, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|
|stream|输出|保存码流数据|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到读取到码流<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_video_venc_release_stream

- 功能：
    释放码流

- 语法：
    int hal_video_venc_release_stream(int chn, hal_video_venc_stream_t *stream);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VENC通道编号|
|stream|输入|待释放的码流数据|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
