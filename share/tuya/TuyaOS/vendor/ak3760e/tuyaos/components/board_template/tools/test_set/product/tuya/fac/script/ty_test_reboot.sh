#!/bin/sh



CURDIR=`cd "$(dirname "$0")";pwd`
# INIFILE=$1
DESTDOMAIN="www.baidu.com"
REBOOTRST=reboot.log


source ${CURDIR}/inifile.func
source ${CURDIR}/network.func


reboot_test_start()
{
	local rtl=""

	# rebootcnt=`ini_read_item ${INIFILE} RESULT rcnt`
	# echo ${rebootcnt} > /dev/console

	while [ 1 ]; do
		ping_network_check_connect ${DESTDOMAIN}

		rtl=$?

		if [ "$rtl" = "0" ]; then
			# rebootcnt=`expr "${rebootcnt}" + 1`
			# ini_write_item ${INIFILE} RESULT rcnt "${rebootcnt}"

			# echo "count: ${rebootcnt}, restart..." > /dev/console
			echo "network check ok, reboot system..." >> ${CURDIR}/${REBOOTRST}
			reboot
		else
			sleep 10
		fi
	done
}



reboot_test_start
