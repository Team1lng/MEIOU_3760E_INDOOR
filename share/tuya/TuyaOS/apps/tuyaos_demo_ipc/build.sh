#!/bin/bash

TOPDIR=$(pwd)/../../
PRODUCT_NAME=$1
VENDOR=tuya
FW_VERSION=`sed '/^CONFIG_TY_APP_FW_VERSION *=/!d;s/.*=\"//' build/tuya_app.config | sed $'s/\"//g'`
FLASH_SIZE=`sed '/^CONFIG_TY_BSP_HW_FLASH_SIZE *=/!d;s/.*=//'  build/tuya_app.config`
TARGET_PLATFORM=`sed '/^TARGET_PLATFORM *=/!d;s/.*=//' $TOPDIR/build/build_param`
TOOLCHAIN_PREFIX=`sed '/^COMPILE_PREX *=/!d;s/.*=//' $TOPDIR/build/build_param`
ROOTDIR=`sed '/^ROOT_DIR *=/!d;s/.*=//' $TOPDIR/build/build_param`
OPERATING_SYSTEM=`grep -E 'CONFIG_OPERATING_SYSTEM\s*=\s*' $TOPDIR/vendor/$TARGET_PLATFORM/tuyaos/tuyaos_kernel.config | cut -d'=' -f2 | tr -d '\r' | xargs`
BSP_DIR=output/bsp/
if [ "${TARGET_PLATFORM}" = "x86_64" ];then
BSP_PRODUCT_CONFIG=x86_64
elif [ "${TARGET_PLATFORM}" = "fh8652" ];then
BSP_PRODUCT_CONFIG=sc116wz4
else
BSP_PRODUCT_CONFIG=template
fi

BSP_BUILD_SUCCESS_CONFIG=${TOPDIR}/vendor/${TARGET_PLATFORM}/tuyaos/output/${BSP_PRODUCT_CONFIG}/.build_success
APP_CONFIG_COPY=${TOPDIR}/build/tuya_app.config

echo $FW_VERSION
echo $TARGET_PLATFORM
echo $TOOLCHAIN_PREFIX
echo $FLASH_SIZE
echo $BSP_PRODUCT_CONFIG

#copy for bsp parse config
cp  build/tuya_app.config ${APP_CONFIG_COPY} -f
cd ${TOPDIR}/vendor/${TARGET_PLATFORM}/tuyaos/
    #call bsp build every time
    echo "call bsp sh build.sh start"
    bash build.sh ${BSP_PRODUCT_CONFIG}
    echo "call bsp sh build.sh end"
cd -

#copy release bsp
if [ ! -d ${TOPDIR}output/bsp ] && [ -f ${BSP_BUILD_SUCCESS_CONFIG} ];then
    echo "app copy release bsp file"
    cp ${TOPDIR}/vendor/${TARGET_PLATFORM}/release/bsp/ ${TOPDIR}/output -a
    # 删除冲突的头文件 bsp_sdk中这两个文件较老，后续bsp从include中移除
    rm ${TOPDIR}/output/bsp/bsp_sdk/include/tuya_iot_config.h
    rm ${TOPDIR}/output/bsp/bsp_sdk/include/tuya_cloud_types.h
fi

#link
if [ $OPERATING_SYSTEM -eq 100 ];then #linux
    make -f link.mk
else #non-Linux
    echo -e "\033[0;31mIf it is a non-Linux development environment, the link is implemented by the developers.\033[0m"
fi

#pack
if [ $? -eq 0 ] && [ $OPERATING_SYSTEM -eq 100 ];then
    echo "pack img"
    #echo "${TOPDIR}/scripts/ty_pack.sh ${TOPDIR} ${VENDOR} ${TARGET_PLATFORM} ${PRODUCT_NAME} ${FW_VERSION} ${BSP_DIR} ${FLASH_SIZE} ${TOOLCHAIN_PREFIX}"
	#bash ${TOPDIR}/apps/${PRODUCT_NAME}/ty_pack.sh ${TOPDIR} ${VENDOR} ${TARGET_PLATFORM} ${PRODUCT_NAME} ${FW_VERSION} ${BSP_DIR} ${FLASH_SIZE} ${TOOLCHAIN_PREFIX}
	echo -e "\033[32mFIRMWARE PATH : ${ROOTDIR}/output/${TARGET_PLATFORM}_${PRODUCT_NAME}/firmware/tuyaos_demo_ipc\033[0m"
else
	rm -rf ${TOPDIR}/output/${TARGET_PLATFORM}_${PRODUCT_NAME}/firmware/*
fi
