
# VPP API说明

## hal_video_vpp_init

- 功能：
    初始化VPP

- 语法：
    int hal_video_vpp_init(hal_video_vpp_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|attr|输入|VPP属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpp_deinit

- 功能：
    反初始化VPP

- 语法：
    int hal_video_vpp_deinit(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpp_create_channel

- 功能：
    创建VPP通道

- 语法：
    int hal_video_vpp_create_channel(int chn, hal_video_vpp_chn_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPP通道编号|
|attr|输入|VPP通道属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpp_destroy_channel

- 功能：
    销毁VPP通道

- 语法：
    int hal_video_vpp_destroy_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPP通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpp_start_channel

- 功能：
    启动VPP通道

- 语法：
    int hal_video_vpp_start_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPP通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpp_stop_channel

- 功能：
    停止VPP通道

- 语法：
    int hal_video_vpp_stop_channel(int chn);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPP通道编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
