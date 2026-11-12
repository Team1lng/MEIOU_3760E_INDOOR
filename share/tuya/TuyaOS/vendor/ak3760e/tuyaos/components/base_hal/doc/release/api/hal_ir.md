
# IR API说明


## hal_ir_rawcode_tx

- 功能：
    发送 IR rawcode

- 语法：
    int hal_ir_rawcode_tx(hal_board_id id, ir_raw_code *buf);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|board ID，已废弃，可传入任意值（比如：0）|
|buf|输入|保存待发送IR rawcode的buffer|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


## hal_ir_rawcode_rx

- 功能：
    接收 IR rawcode

- 语法：
    int hal_ir_rawcode_rx(hal_board_id id, ir_raw_code *buf, struct timeval *timeout);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|board ID，已废弃，可传入任意值（比如：0）|
|buf|输出|保存接收的IR rawcode|
|timeout|输入|超时时间。如果设为NULL，表示一直阻塞，直到接收到IR rawcode|

- 返回值：

|返回值|描述|
|----|----|
|0|成功|
|-ETIMEDOUT|超时|
|其它错误码|失败|


## hal_ir_set_tx_carrier

- 功能：
    设置 tx carrier frequency

- 语法：
    int hal_ir_set_tx_carrier(hal_board_id id, unsigned int frequency);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|id|输入|board ID，已废弃，可传入任意值（比如：0）|
|frequency|输入|carrier frequency, in Hz|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

