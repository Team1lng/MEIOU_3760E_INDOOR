
# VPE API说明

## hal_video_vpe_init

- 功能：
    初始化VPE

- 语法：
    int hal_video_vpe_init(hal_video_vpe_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|attr|输入|VPE属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_deinit

- 功能：
    反初始化VPE

- 语法：
    int hal_video_vpe_deinit(void);

- 参数：
    无。

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_create_channel

- 功能：
    创建VPE通道。

- 语法：
    int hal_video_vpe_create_channel(int chn, int port, hal_video_vpe_chn_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPE通道编号|
|port|输入|VPE端口编号|
|attr|输入|VPE通道属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_destroy_channel

- 功能：
    销毁VPE通道。

- 语法：
    int hal_video_vpe_destroy_channel(int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPE通道编号|
|port|输入|VPE端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_start_channel

- 功能：
    启动VPE通道

- 语法：
    int hal_video_vpe_start_channel(int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPE通道编号|
|port|输入|VPE端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_stop_channel

- 功能：
    停止VPE通道

- 语法：
    int hal_video_vpe_stop_channel(int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|chn|输入|VPE通道编号|
|port|输入|VPE端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vpe_isp_load_para_file

- 功能：
    加载ISP参数文件

- 语法：
    int hal_video_vpe_isp_load_para_file(int dev, char *path, unsigned int magic);

- 参数：

chn, int port

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|ISP设备编号。如果只有一个ISP设备，设置为0|
|path|输入|ISP参数文件路径|
|magic|输入|自定义参数|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
