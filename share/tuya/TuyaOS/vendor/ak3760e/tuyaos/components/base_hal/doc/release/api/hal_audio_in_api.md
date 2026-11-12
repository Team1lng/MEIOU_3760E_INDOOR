
# Audio_In API说明

## hal_aud_in_init

- 功能：
    初始化音频输入通道。

- 语法：
    int hal_aud_in_init(int dev, int chn, hal_aud_in_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|attr|输入|音频设备/通道属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_deinit

- 功能：
    反初始化音频输入通道。

- 语法：
    int hal_aud_in_deinit(int dev, int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_wait_for_frame

- 功能：
    等待音频帧就绪。

- 语法：
    int hal_aud_in_wait_for_frame(int dev, int chn, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到音频帧就绪<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_aud_in_get_frame

- 功能：
    获取音频帧。

- 语法：
    int hal_aud_in_get_frame(int dev, int chn, hal_aud_frame_s *frame, int timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|frame|输出|保存音频帧|
|timeout|输入|超时时间（毫秒）<br>-1: 一直阻塞，直到音频帧就绪<br>0: 不阻塞<br>>0: 超时时间|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_aud_in_release_frame

- 功能：
    释放音频帧。

- 语法：
    int hal_aud_in_release_frame(int dev, int chn, hal_aud_frame_s *frame);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|frame|输入|待释放的音频帧|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_set_volume

- 功能：
    设置音量。

- 语法：
    int hal_aud_in_set_volume(int dev, int chn, int vol);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|vol|输入|音量（in dB）|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_get_volume

- 功能：
    获取当前音量。

- 语法：
    int hal_aud_in_get_volume(int dev, int chn, int *vol);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|
|vol|输出|保存获取的音量（in dB）|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_mute

- 功能：
    音频输入设备静音。

- 语法：
    int hal_aud_in_mute(int dev, int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_aud_in_unmute

- 功能：
    音频设备取消静音。

- 语法：
    int hal_aud_in_unmute(int dev, int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|音频输入设备号。如果只有一个音频输入设备，设置为0|
|chn|输入|音频通道号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
