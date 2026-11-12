#!/bin/bash

# CI系统传入的参数说明：
#
# $1 - SDK项目名称，如： tuya_iot_wifi_tls
# $2 - SDK产物版本，如： 4.1.3
# $3 - 开发环境名称，如： linux-ubuntu-6.2.0_64Bit
# $4 - SDK产物包路径，如： output/dist/tuya_iot_wifi_tls_linux-ubuntu-6.2.0_64Bit_4.1.3.tar.gz
#
# 例：
# ./ci_autobuild.sh tuya_iot_wifi_tls 4.1.3 linux-ubuntu-6.2.0_64Bit output/dist/tuya_iot_wifi_tls_linux-ubuntu-6.2.0_64Bit_4.1.3.tar.gz

function crt_hw_config()
{

while read line
do
if [[ "$line" =~ "CONFIG_TY_BSP_HW" ]] ;then
	 echo $line >> $2
fi
done < $1
}

function clear_hw_config()
{

while read line
do
if [[ "$line" =~ "CONFIG_TY_BSP_HW" ]] ;then
    echo "$line"
else 
    echo $line >> $2
fi
done < $1

}

function is_first_time()
{
    if [ -f $app_config ];then
        if [ -d ../release ] && [ ! -d ../../../output/bsp ];then
	    echo " first time create"
	    mkdir -p output/$CI_PROJECT_NAME
	    touch output/$CI_PROJECT_NAME/.build_success
        fi 
    fi
}

function hwconfig_is_same()
{
if [ -f $app_config ];then
    rm -rf hwconfig
    crt_hw_config $app_config hwconfig 

    if [ ! -f  hwconfig ];then
        touch  hwconfig 
    fi

    if [ -f .hwconfig.old ];then
            diff hwconfig .hwconfig.old > /dev/null
	    if [ $? == 0 ];then
	        echo "1"
	    fi
    fi
else
   echo "1"
   return;
fi
echo "0"
return;
}


if [ -z "$BASH_VERSION" ]
then
    exec bash "$0" "$@"
    exit
fi


cd `dirname $0`

CI_PROJECT_NAME=$1
CI_PROJECT_VERSION=$2
CI_PROJECT_PLATFORM=$3
CI_PACKAGE_NAME=$4

[ -z $CI_PROJECT_NAME ] && CI_PROJECT_NAME="fh8652"
[ -z $CI_PROJECT_VERSION ] && CI_PROJECT_VERSION="0.0.1-beta.2"
[ -z $CI_PROJECT_PLATFORM ] && CI_PROJECT_PLATFORM="arm-fullhanv3-linux-uclibcgnueabi-b6"
[ -z $CI_PACKAGE_NAME ] && CI_PACKAGE_NAME="output/dist/${CI_PROJECT_NAME}_${CI_PROJECT_VERSION}_${CI_PROJECT_PLATFORM}.tar.gz"

GIT_USER_ALL=$(git config --get user.email)
GIT_USER=${GIT_USER_ALL%@*}
BUILD_TIME=$(date +%H_%M_%S)
BUILD_DATE=$(date +%Y_%m_%d)
SDK_FULL_VER=$CI_PROJECT_VERSION
SDK_BETA_VER=`echo $CI_PROJECT_VERSION | grep -o 'beta\.[0-9]\+'`
IOT_SDK_VER=`echo $CI_PROJECT_VERSION | grep -o '[0-9]\+\.[0-9]\+\(\.[0-9]\+\)\?'`
IPC_SDK_VER=$IOT_SDK_VER
SDK_ID_TMP=`echo ${CI_PACKAGE_NAME%.tar.gz*}`
SDK_ID=`echo ${SDK_ID_TMP#*output/dist/}`

app_config=../../../build/tuya_app.config
echo "$app_config "

is_first_time

if [  ! -f .hwconfig.old ];then
        crt_hw_config $app_config .hwconfig.old
fi

#if [  ! -f .hwconfig.old ];then
#     touch .hwconfig.old
#fi

CONFIG_STATUS=`hwconfig_is_same`
echo "CONFIG_STATUS=$CONFIG_STATUS"

if [ -f output/$CI_PROJECT_NAME/.build_success ];then
       NEED_REBUILD=`find -H components/ tuyaos_adapter/ -cnewer output/$CI_PROJECT_NAME/.build_success -type f`
       if [ -z ${NEED_REBUILD} ]  && [ "$CONFIG_STATUS" != "0" ];then
               echo "###########################"
               echo "# No need to rebuild, skip!"
               echo "###########################"
               exit 0;
       fi
fi

set -e
# 检查编译环境的包依赖并安装缺少的包
./build/scripts/os-depends.sh

# 通过环境变量传递生成的产物包全路径名称
export CI_PACKAGE_FULLNAME="$(pwd)/$CI_PACKAGE_NAME"

# 不更新代码
# embcli update --platforms=$CI_PROJECT_PLATFORM
# python3 ./scripts/set_iotosconfig.py make.yaml build/IoTOSconfig $TARGET_PLATFORM

if [ -f platforms/$CI_PROJECT_PLATFORM/prepare.sh ]; then
    bash platforms/$CI_PROJECT_PLATFORM/prepare.sh
fi

# 保存CI参数
echo -e "CI_PROJECT_NAME=$CI_PROJECT_NAME" > .local.conf
echo -e "CI_PROJECT_VERSION=$CI_PROJECT_VERSION" >> .local.conf
echo -e "CI_PROJECT_PLATFORM=$CI_PROJECT_PLATFORM" >> .local.conf
echo -e "CI_PACKAGE_FULLNAME=$CI_PACKAGE_FULLNAME" >> .local.conf
echo -e "BUILD_DATE=$BUILD_DATE" >> .local.conf
echo -e "BUILD_TIME=$BUILD_TIME" >> .local.conf
echo -e "GIT_USER=$GIT_USER" >> .local.conf
echo -e "IOT_SDK_VER=$IOT_SDK_VER" >> .local.conf
echo -e "IPC_SDK_VER=$IPC_SDK_VER" >> .local.conf
echo -e "SDK_FULL_VER=$SDK_FULL_VER" >> .local.conf
echo -e "SDK_BETA_VER=$SDK_BETA_VER" >> .local.conf
echo -e "SDK_ID=$SDK_ID" >> .local.conf

. build/envsetup.sh
if [ -f build/tuya_iot.config ];then
    lunch default
else
    lunch $CI_PROJECT_NAME
fi

echo -e "Build SDK Begin"

if [ -d ../../../output/bsp ]; then
    rm -rf ../../../output/bsp
fi

if [ -d ./include ];then
    rm -rf include
fi

if [ -f $app_config ];then
    echo  "used externel hw config"
    clear_hw_config .config tmp.config
    cat tmp.config hwconfig > .config
    cp  hwconfig .hwconfig.old
fi

cat  tuyaos_kernel.config >>  .config

make clean
make -O -j4
make pack
touch output/$CI_PROJECT_NAME/.build_success
echo -e "Build Finish"

