
# IRCUT API说明

## hal_ircut_switch

- 功能：
    启用/禁用ircut

- 语法：
    int hal_ircut_switch(char *name, hal_ircut_cmd_e cmd);

- 参数：

|名称|输入/输出|描述|
|----|----|----|
|name|输入|ircut名称。定义在hal_board.h。<br>如果只有一个ircut，name可设为NULL，表示使用默认名称|
|cmd|输入|控制命令：启用/禁用/休眠ircut|

- 返回值：
    成功返回0，失败返回Linux标准错误码。


    ### hal_ircut_cmd_e 类型

    - 类型定义

    ```
        typedef enum {
            IRCUT_CMD_NONE,         /* null command */
            IRCUT_CMD_ON,           /* enable ircut function */
            IRCUT_CMD_OFF,          /* disable ircut function */
            IRCUT_CMD_SLEEP,        /* put the ircut device to sleep state */
        } hal_ircut_cmd_e;
    ```

    - 类型说明

|成员|描述|
|----|----|
|IRCUT_CMD_NONE|空命令|
|IRCUT_CMD_ON|启用ircut|
|IRCUT_CMD_OFF|禁用ircut|
|IRCUT_CMD_SLEEP|休眠ircut|
