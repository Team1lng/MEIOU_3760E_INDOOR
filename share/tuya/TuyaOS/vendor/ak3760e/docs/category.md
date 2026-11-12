## BSP SDk 产物结构

BSP SDK 的产物包含如下文件

|文件名|描述|
|----|----|
|boot.bin|bootloader镜像|
|kernel.bin|内核镜像|
|rootfs.squashfs.bin|根文件系统镜像|
|partitions.json|固件打包配置文件|
|flash_json_update.py|配置文件更新脚本|
|tuya_pack|固件打包程序|
|build.sh|固件打包脚本|


## BSP SDK 使用说明

BSP SDK 作为附件 SDK 添加后，CI 在编译时会将产物解压到 `accessorySDK/release` 目录下。

使用方法也比较简单，将生成的镜像文件和版本信息文件拷贝到 `accessorySDK/release` 目录下并调用 `build.sh` 脚本打包即可。

具体可以参考如下的 `pack.sh` 脚本。

```shell
#!/bin/bash
APP_DIR=${PWD}
TOP_DIR=${APP_DIR}/../../
BSP_SDK=${TOP_DIR}/accessorySDK/release

# 将user.bin等拷贝到 $BSP_SDK 目录
cp -v ${APP_DIR}/user.bin $BSP_SDK
cp -v ${APP_DIR}/drv.squashfs.bin $BSP_SDK
cp -v ${APP_DIR}/mtd.bin $BSP_SDK
cp -v ${APP_DIR}/cfg.bin $BSP_SDK

# 获取user.bin等镜像的版本信息
USER_VER=1.3.0
DRV_VER=1.0.1
NVRAM_VER=1.1.0
MTD_VER=1.0.0

# 生成user.bin等文件的版本信息文件并拷贝到 $BSP_SDK
cat << EOF > ${BSP_SDK}/version.ini
[user]
version=$USER_VER
filename=user.bin

[drv]
version=$DRV_VER
filename=drv.squashfs.bin

[nvram]
version=$NVRAM_VER
filename=cfg.bin

[mtd]
version=$MTD_VER
filename=mtd.bin
EOF

# 打包固件
cd $BSP_SDK && ./build.sh

PRODUCT_NAME=`grep product_name ${BSP_SDK}/partitions.json | cut -d '"' -f 4`
# 获取基线的BV/PV
BV=`strings ${APP_DIR}/demo_soc_dev_wired | grep -m1 TUYA_SDK_BV`
PV=`strings ${APP_DIR}/demo_soc_dev_wired | grep -m1 TUYA_SDK_PV`

# 将固件按tuya的命名规则重新命名
cp ${BSP_SDK}/${PRODUCT_NAME}_all.img ${BSP_SDK}/ty_ipc_${PRODUCT_NAME}_fw_QIO_${APP_VERSION}.bin
# 将基线的PV/BV信息附加到OTA固件的尾部，供云端获取版本信息
echo ${BV} > ${BSP_SDK}/bv_pv.bin
echo ${PV} >> ${BSP_SDK}/bv_pv.bin
cat ${BSP_SDK}/${PRODUCT_NAME}_ota.img ${BSP_SDK}/bv_pv.bin > ${BSP_SDK}/ty_ipc_${PRODUCT_NAME}_fw_UG_${APP_VERSION}.bin

if [ ! -z $CI_PACKAGE_PATH ];then
        mkdir -p $CI_PACKAGE_PATH
        cp $BSP_SDK/ty_ipc_*.bin $CI_PACKAGE_PATH/
fi
```