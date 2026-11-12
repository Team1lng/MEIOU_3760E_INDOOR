#!/bin/sh



CURDIR=`cd "$(dirname "$0")";pwd`
DESTDOMAIN="www.baidu.com"
STREAMINFO=streaminfo.txt
TOPINFO="top.info"
DATE=`date "+%s"`
STLITYLOG=${CURDIR}/${DATE}_stability.log
stream0=
stream1=
mqstatus=0

source ${CURDIR}/network.func


stability_test_stream()
{
	cat /sys/devices/platform/rts_soc_camera/streaminfo > ${STREAMINFO}

	s0tmp=`sed -n "2p" ${STREAMINFO} | awk -F" " '{ print $2 }'`
	s1tmp=`sed -n "3p" ${STREAMINFO} | awk -F" " '{ print $2 }'`

	if [ $s0tmp -gt $stream0 ]; then
		stream0=${s0tmp}
	else
		echo "`date` stream0 error" >> ${STLITYLOG}
	fi

	if [ ${s1tmp} -gt ${stream1} ]; then
		stream1=${s1tmp}
	else
		echo "`date` stream1 error" >> ${STLITYLOG}
	fi

	rm -f ${STREAMINFO}

	echo "`date` stream info: ${stream0} ${stream1}" >> ${STLITYLOG}
}

stability_test_mqtt()
{
	if [ ! -e /tmp/.mq_status ]; then
		return
	fi

	mqtmp=`cat /tmp/.mq_status`

	if [ x"$mqtmp" = x"0" ]; then
		echo "`date` mqtt offline checked" >> ${STLITYLOG}
	fi

	if [ x"$mqtmp" != x"$mqstatus" ]; then
		echo "`date` mqtt old($mqstatus), new($mqtmp)" >> ${STLITYLOG}
		mqstatus=$mqtmp
	fi
}

stability_test_top_info()
{
	top -b -d 1 -n 1 > ${TOPINFO}

	sed -n "2p" ${TOPINFO} >> ${STLITYLOG}
	cat ${TOPINFO} | grep tycam | grep -v ty_monitor.sh >> ${STLITYLOG}

	rm -f ${TOPINFO}
}

stability_test_start()
{
	local rtl=""

	echo "`date` start stability test" > ${STLITYLOG}

	sleep 10

	cat /sys/devices/platform/rts_soc_camera/streaminfo > ${STREAMINFO}

	stream0=`sed -n "2p" ${STREAMINFO} | awk -F" " '{ print $2 }'`
	stream1=`sed -n "3p" ${STREAMINFO} | awk -F" " '{ print $2 }'`

	cat /sys/devices/platform/rts_soc_camera/meminfo >> ${STLITYLOG}
	cat /sys/devices/platform/rts_isp_mem/memlist >> ${STLITYLOG}
	cat sys/devices/virtual/rts_camera/rtslock/lockinfo >> ${STLITYLOG}
	cat /sys/devices/platform/rts_isp_mem/memctrl >> ${STLITYLOG}

	while [ 1 ]; do
		stability_test_stream

		ping_network_check_connect ${DESTDOMAIN}
		rtl=$?
		if [ "$rtl" = "1" ]; then
		 	echo "`date` network not ok" >> ${STLITYLOG}
		fi

		stability_test_mqtt
		stability_test_top_info

		sleep 5
	done
}


stability_test_start

