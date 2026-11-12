
# VIF API说明

## hal_video_vif_init

- 功能：
    初始化VIF

- 语法：
    int hal_video_vif_init(int dev, int chn, int port, hal_video_vif_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|VIF设备编号。如果只有一个VIF设备，设置为0|
|chn|输入|VIF通道编号|
|port|输入|VIF端口编号|
|attr|输入|VIF属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vif_deinit

- 功能：
    反初始化VIF

- 语法：
    int hal_video_vif_deinit(int dev, int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|VIF设备编号。如果只有一个VIF设备，设置为0|
|chn|输入|VIF通道编号|
|port|输入|VIF端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vif_enable

- 功能：
    使能VIF

- 语法：
    int hal_video_vif_enable(int dev, int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|VIF设备编号。如果只有一个VIF设备，设置为0|
|chn|输入|VIF通道编号|
|port|输入|VIF端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vif_disable

- 功能：
    禁止VIF

- 语法：
    int hal_video_vif_disable(int dev, int chn, int port);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|VIF设备编号。如果只有一个VIF设备，设置为0|
|chn|输入|VIF通道编号|
|port|输入|VIF端口编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_vif_get_info

- 功能：
    获取VIF属性

- 语法：
    int hal_video_vif_get_info(int dev, int chn, int port, hal_video_vif_info_t *info);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|dev|输入|VIF设备编号。如果只有一个VIF设备，设置为0|
|chn|输入|VIF通道编号|
|port|输入|VIF端口编号|
|info|输出|保存VIF属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
