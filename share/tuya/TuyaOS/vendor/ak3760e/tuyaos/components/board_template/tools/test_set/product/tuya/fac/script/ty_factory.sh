#!/bin/sh

CURDIR=`cd "$(dirname "$0")";pwd`
FACDIR=${CURDIR}/..
RUNDIR=/tmp/tuya/

PATH="${PATH}:/opt/bin/:${FACDIR}/bin/:${RUNDIR}/fac/bin/:/opt/skyeye/bin:/tuya/app/skyeye/bin"

export PATH
echo "FAC START ..." > /dev/console
echo "FAC START ..." > /dev/console
echo "FAC START ..." > /dev/console
echo "FAC START ..." > /dev/console

factory_release_source()
{
	echo "prepare for factory environment ..." > /dev/console

	killall -q  iperf3
	killall -q  ty_monitor.sh
	killall -q  -9 tycam
	killall -q  tyfac
	killall -q  wpa_supplicant
	killall -q  udhcpc
	killall -q  wpa_cli
	killall -q  iwconfig
	killall -q  echo
    while [ true ]
    do
    	    sleep 2
	    pro0=$(ps | grep "tycam" | grep -v grep)
	    pro1=$(ps | grep "tyfac" | grep -v grep)
	    pro2=$(ps | grep "ty_monitor.sh" | grep -v grep)
    	if [ -z "$pro0" -a -z "$pro1" -a -z "$pro2"]; then
    		echo "process has been exited success ..."
    		break
    	fi  
#    	sleep 1
    	killall -q  iperf3
		killall -q  ty_monitor.sh
		killall -q  -9 tycam
		killall -q  tyfac
    	killall -q  wpa_supplicant
		killall -q  udhcpc
		killall -q  wpa_cli
		killall -q  iwconfig
		killall -q  echo
    	echo "wait release........" > /dev/console
	done

}

readIni()
{
    file=$1;section=$2;item=$3;
    val=$(awk -F '=' '/\['${section}'\]/{a=1} (a==1 && "'${item}'"==$1){a=0;print $2}' ${file}) 
    echo ${val}	
}
INI_FILE=${RUNDIR}/fac/cfg/config.ini

factory_release_source

[ ! -f ${RUNDIR} ] && mkdir -p ${RUNDIR}

sh /${RUNDIR}/fac/script/ty_wifi_link.sh $1
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_all

MODE=$(echo $(readIni ${INI_FILE} TEST aging_test) | sed -e 's/\r//g')
if [ -z $MODE ];then 
	MODE=0
fi
RUNTIME=$(echo $(readIni ${INI_FILE} TEST run_time) | sed -e 's/\r//g')
if [ -z $RUNTIME ];then 
	RUNTIME=12
fi

VQ=$(echo $(readIni ${INI_FILE} TEST video_quality) | sed -e 's/\r//g')
if [ -z $VQ ];then 
	VQ=0
fi

FP=$(echo $(readIni ${INI_FILE} TEST flip) | sed -e 's/\r//g')
if [ -z $FP ];then 
	FP=0
fi

LOG=$(echo $(readIni ${INI_FILE} TEST log) | sed -e 's/\r//g')
if [ -z $LOG ];then 
	LOG=0
fi

MOTOR=$(echo $(readIni ${INI_FILE} TEST motor_time) | sed -e 's/\r//g')
if [ -z $MOTOR ];then 
	MOTOR=-1
fi


PATH="${PATH}:/sbin:/system/sbin:/system/bin:/vendor/bin:/sbin:/opt/bin/:/opt/skyeye/bin:/tuya/app/skyeye/bin"
LD_LIBRARY_PATH="/lib:/system/lib:/vendor/lib:/ext/lib:/opt/lib:/tuya/app/lib/"

export PATH
export LD_LIBRARY_PATH
export OMX_LIB_PATH=/opt/lib

#${RUNDIR}/fac/bin/tyfac $1 -m $MODE -t $RUNTIME -q $VQ -f $FP -l $LOG -T $MOTOR > /dev/console 2>&1 &
#${RUNDIR}/fac/bin/tyfac
nvram set hal_config_file_path /tmp/test_set.json
export LD_LIBRARY_PATH=/mnt/sdcard/:$LD_LIBRARY_PATH

U_VALUE=$(nvram show | grep -w "\[STATUS\]" | awk '{print $2}' | grep -i active)
if [ "$U_VALUE" == "[active]" ]; then
	nvram set STATUS noactive
	nvram commit
	reboot
fi

${RUNDIR}/fac/bin/tyfac -m $MODE -t $RUNTIME

PCADDR=$(echo $(readIni ${INI_FILE} WIFI pc) | sed -e 's/\r//g')
if [ -n $PCADDR ];then 
	echo $PCADDR > /dev/console
	ping $PCADDR -c 20
fi

