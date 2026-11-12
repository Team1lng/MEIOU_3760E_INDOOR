#!/bin/sh/


date
ulimit -c unlimited
ulimit -s unlimited 
echo "/mnt/sdcard/core_%e_%p_%t" > /proc/sys/kernel/core_pattern
count=0

while [ 1 ]
do
	ps | grep tycam | grep -v grep
	if [ $? -ne 0 ]; then
		break
	else
		killall tycam
	fi

	sleep 1
	killall udhcpc
	sleep 1
	killall wpa_supplicant

	let count=count+1
	echo "count is $count"
	sleep 15

	/opt/skyeye/bin/tycam &
	sleep 20
	cat /sys/devices/platform/rts_soc_camera/streaminfo
	sleep 1
	cat /sys/devices/platform/rts_soc_camera/streaminfo
	sleep 3
done

echo "realtek is end"

