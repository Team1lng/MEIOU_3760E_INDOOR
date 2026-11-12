# tuyaOS 开发环境

## tuyaOS开发环境目录 
### 开发环境顶层目录 
| 文件名 | 描述 |
|ci_autobuild.sh| 目前只用来拉取最新构建代码和prepare.sh|
|ci_autopack.sh| 用于CDE上的打包发布
|docs|发布的文档|
|prepare.sh|用于拉取代码|
|README.md|使用接受
|toolchain|对应平台的编译工具链|
|toolchain.yaml|各个组件的最新构建版本，用于拉取最新代码|
|tuyaos| tuyaos bsp 开发环境编译目录|

### tuyaOS 开发环境编译目录 
| 文件名 | 描述 |
|build| BSP 构建规则、脚本和工具目录|
|build.conf|预定义的编译配置|
|build_path|编译工具链路径和一些环境变量设置|
|build.sh| 自动编译开发环境主入口|
|components|BSP 提供的一些功能组件|
|download|第三方下载的一些代码和工具|
|include| 开发环境默认配置头文件|
|IoTOSconfig|编译顶层配置|
|Makefile|顶层make文件|
|scripts|编译脚本|

## 开发环境编译 

### 开发环境一键编译 

通过调用 tuyaos/build.sh 编译完成

build.sh 参数：
$1 - SDK产品名，如：sc015wu4v2
$2 - SDK产物版本，如： 4.1.3
$3 - 发布平台，如： arm-anykav500-linux-uclibcgnueabi-gcc-4.9.4
$4 - 产物发布包名，如：anyka3918.zip
./build.sh $1 $2 $3 $4 ，后面3个产测做扩展用，目前暂时没有用到。
例如：./build.sh sc015wu4v2

tuyaos/builds.sh 的处理流程：

1. 根据传入的参数初始化环境并加载对应的 BSP 配置
2. 根据 Makefile 编译组件，生成SDK产物包
3. 将SDK产物包存放在指定位置

### 线下开发常用命令

- 手动初始化环境并选择 BSP 配置

```shell
/# cd tuyaos  
/# . build/envsetup.sh
/# lunch

You're building on Linux

Lunch menu... pick a board config:
     1. tuya_aaa_sdk
     2. tuya_bbb_sdk
     3. tuya_ccc_sdk
     4. tuya_xxx_sdk

Which would you like?: 4
```

- 手动编译SDK

```shell
/# make -j 4
```

- 手动打包生成产物

```shell
/# make pack
```

- 修改 BSP 配置
```shell
/# make menuconfig
```

- 保存 BSP 配置（保存到sdk/board_xxx/config/yyy_defconfig）
```shell
/# make savedefconfig
```

- 单独编译某个包/模块
```shell
/# cd sdk/xxx/xxx
/sdk/xxx/xxx # mm
```
或者
```shell
/# make xxx
```

- 跳转到 SDK 根目录
```shell
/sdk/xxx/xxx # croot
/#
```

- 清除软件包xxx的编译中间产物
```shell
/# make xxx-clean
```

- 清除所有编译中间产物
```shell
/# make clean
```

- 清除所有编译中间产物和配置
```shell
/# make distclean
```

- 更新模板
```shell
/# ./build/scripts/xmake_update.sh
```



## 产物包文件结果
编译后的产物包位于../vendor/output 下面为了方便品类工程的管理，SDK产物包内的文件按照一定的标准进行组织。

```
└── bsp
    ├── boot.bin
    ├── bsp_sdk
    ├── build.sh
    ├── cfg.bin
    ├── drivers
    ├── drv
    ├── images
    ├── kernel.bin
    ├── mtd.bin
    ├── partitions.json
    ├── rootfs
    ├── testfs
    ├── tools
    ├── tuya_t31_normal_power_product_20211219.zip
    └── tuyaos_ecprikey.pem
```
## 添加新的产品型号

产品配置存储路径：
vendor/对应平台/build/config/

产品配置名命名规则：
产品平台名+.config
例如：template.config 

开发环境中添加新的产品型号只需要拷贝产品配置目录下的“template.config”为 “需要添加的产品名.config”

```shell
/# cd tuyaos  
/# . build/envsetup.sh
/# lunch 
``` 
能看到你添加的产品
