#!/bin/sh
CURDIR=`cd "$(dirname "$0")";pwd`
INIFILE=${CURDIR}/../cfg/cat.ini

#copy ini
if [ -e /mnt/sdcard/config.ini ]; then
	cp /mnt/sdcard/config.ini ${CURDIR}/../cfg/
else
	echo "sd card nonexist config.ini" > /dev/console
fi

if [ -e /mnt/sdcard/test.pcm ]; then
	cp /mnt/sdcard/test.pcm /tmp
fi

if [ -e /mnt/sdcard/test_set.json ]; then
	cp /mnt/sdcard/test_set.json /tmp
fi

source ${CURDIR}/inifile.func

name=`ini_read_item ${INIFILE} TEST name`

echo "start to auto test ${name} ..." > /dev/console


case $name in
	reboot)
		${CURDIR}/ty_test_reboot.sh > /dev/console 2>&1 &
	;;

	stability)
		${CURDIR}/ty_test_stability.sh > /dev/console 2>&1 &
	;;

	fac)
		${CURDIR}/ty_factory.sh > /dev/console 2>&1 &
	;;

	wifi)
	;;

	*)
		echo "no item need to test, please check it..." > /dev/console
	;;
esac
