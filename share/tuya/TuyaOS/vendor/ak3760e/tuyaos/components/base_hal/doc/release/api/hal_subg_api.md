
# SUB-G API说明

## hal_subg_send

- 功能：
    发送数据。

- 语法：
    yint hal_subg_send(void *buf, int size);yy

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|buf|输入|保存待发送数据的buffer|
|size|输入|数据大小（bytes）|

- 返回值：
    成功，返回发送的数据大小（bytes）；失败返回-1。


## hal_subg_receive

- 功能：
    接收数据。

- 语法：
    int hal_subg_receive(void *buf, int size);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|buf|输出|保存接收数据的buffer|
|size|输入|buffer大小（bytes）|

- 返回值：
    成功，返回接收的数据大小（bytes）；失败返回-1。
