# 板级/平台相关组件移植开发指南
------------------------
一个最简单的板级组件必须包含如下的文件：

|文件名|描述|
|----|----|
|Config.in|menuconfig 配置文件|
|config/|配置文件存放目录|
|vendorsetup.sh|配置文件选择脚本|
|local.mk|xmake 编译脚本|


## Config.in
如下的配置必须在 `Config.in` 中包含：

|配置名|描述|
|----|----|
|TUYA_KERNEL_LOADADDR|内核加载地址|
|TUYA_KERNEL_CONFIG|内核配置文件|

详细的配置请参考模板[Config.board.in.template](Config.board.in.template)

## config/
`config` 目录用于存放 BSP 配置文件、kernel 配置文件、bootloader配置文件等。  
其中 BSP 配置文件名必须以 `_defconfig` 来结尾，名称需要和 CI 上的 SDK 名称保持一致。


## vendorsetup.sh
`vendorsetup.sh` 脚本用于选择 `config` 目录中的 BSP 配置文件，其格式如下：
```shell
#!/bin/bash
add_lunch_combo `dirname $BASH_SOURCE` tuya_model_a_sdk
add_lunch_combo `dirname $BASH_SOURCE` tuya_model_b_sdk
add_lunch_combo `dirname $BASH_SOURCE` tuya_model_c_sdk
```
tuya_model_a_sdk 为去除 `_defconfig` 的 BSP 配置文件名，每个配置一行。

## local.mk
`local.mk` 为 xmake 的编译脚本，用于构建平台相关的程序和模块。  
为了使功能权责更清晰简明，建议将平台相关组件的构建划分到如下几个子 `.mk` 中：
```makefile
LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/rootfs.mk
include $(LOCAL_PATH)/image_builder.mk
include $(shell find $(LOCAL_PATH)/ -name app.mk)
```

`rootfs.mk` 用于调整生成的根文件系统，拷贝平台相关的预编译库、程序和脚本，删除不需要的文件等。  
`image_builder.mk` 用于平台相关的镜像制作，如DDR配置、bootloader签名等。  
`app.mk` 用于编译平台相关的软件。

## 模板

在新平台移植时，可以使用 `build/scripts/create_board.sh` 脚本来创建板级包的模板。
```shell
chenfn@embed:~/WorkSpace/template_sdk_bsp$ ./build/scripts/create_board.sh 
Usage:
  create_board.sh create a board support package skeleton

Example:
  create_board.sh board_generic
```