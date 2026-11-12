
# Sensor API说明

## hal_video_snr_init

- 功能：
    初始化sensor。

- 语法：
    int hal_video_snr_init(int id, hal_video_snr_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|attr|输入|sensor属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_deinit

- 功能：
    反初始化sensor

- 语法：
    int hal_video_snr_deinit(int id);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_enable

- 功能：
    使能sensor

- 语法：
    int hal_video_snr_enable(int id);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_disable

- 功能：
    禁止sensor

- 语法：
    int hal_video_snr_disable(int id);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_set_fps

- 功能：
    设置sensor帧率

- 语法：
    int hal_video_snr_set_fps(int id, int fps);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|fps|输入|帧率|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_get_fps

- 功能：
    获取sensor当前帧率

- 语法：
    int hal_video_snr_get_fps(int id, int *fps);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|fps|输出|保存sensor当前帧率|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_get_res_num

- 功能：
    获取sensor支持的分辨率个数。

- 语法：
    int hal_video_snr_get_res_num(int id, int *num);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|num|输出|保存sensor支持的分辨率个数|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_get_one_res

- 功能：
    从sensor支持的分辨率列表中获取一个分辨率。

- 语法：
    int hal_video_snr_get_one_res(int id, int index, hal_video_res_t *res);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|index|输入|分辨率编号，从0开始|
|res|输出|保存获取的分辨率|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_get_cur_res

- 功能：
    获取sensor当前的分辨率

- 语法：
    int hal_video_snr_get_cur_res(int id, int *index, hal_video_res_t *res);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|index|输出|sensor当前分辨率的编号|
|res|输出|保存sensor当前的分辨率|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_set_res

- 功能：
    设置新的分辨率

- 语法：
    int hal_video_snr_set_res(int id, int index);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|index|输入|分辨率编号|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_set_mirror

- 功能：
    设置sensor镜像/翻转

- 语法：
    int hal_video_snr_set_mirror(int id, hal_video_mirror_e mirror);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|mirror|输入|镜像/翻转类型|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_video_snr_get_mirror

- 功能：
    获取sensor当前镜像/翻转类型

- 语法：
    int hal_video_snr_get_mirror(int id, hal_video_mirror_e *mirror);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|sensor编号。如果只有一个sensor，设置为0|
|mirror|输出|保存获取的镜像/翻转类型|

- 返回值：
    成功返回0，失败返回Linux标准错误码。
