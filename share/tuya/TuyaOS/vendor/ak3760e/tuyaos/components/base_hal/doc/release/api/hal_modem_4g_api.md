
# 4G Modem API说明

## hal_modem_4g_get_dev_attr

- 功能：
    获取4G modem设备属性。

- 语法：
    int hal_modem_4g_get_dev_attr(hal_modem_4g_dev_attr_t *attr);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|attr|输出|保存4G modem设备属性|

- 返回值：
    成功返回0，失败返回Linux标准错误码。

    ### hal_modem_4g_dev_attr_t 类型

    - 类型定义
        ```
        typedef struct {
            char net_dev[64];               /* network device name, e.g. USB0 */
            char at_port[64];               /* AT command port, e.g. /dev/ttyUSB0 */
            hal_serial_attr_t at_port_attr;     /* AT command port attribute */
        } __attribute__((packed, aligned(4))) hal_modem_4g_dev_attr_t;
        ```
    - 类型说明

|成员|描述|
|----|----|
|net_dev|4G modem设备对应的网络设备名，例如：USB0 |
|at_port|AT命令串口名称，例如：/dev/ttyUSB0|
|at_port_attr|AT命令串口属性，包括：波特率、数据位、奇偶校验、停止位、流控|
