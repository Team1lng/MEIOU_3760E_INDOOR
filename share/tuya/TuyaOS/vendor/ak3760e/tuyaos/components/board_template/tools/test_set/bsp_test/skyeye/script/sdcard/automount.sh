#!/bin/sh


destdir_sd=/mnt/${MDEV}
destdir_udisk=/mnt/udisk
SDCARDDIR=/opt/skyeye/script/sdcard/

check_any_usb_mounted()
{
	if grep "^/dev/sd[a-z][0-9]* " /proc/mounts; then
		return 1
	fi

	rmdir $destdir_udisk
	return 0
}

check_dev_before_action()
{
    ls /dev/mmc* > /dev/console
	if [ "$1" = "mmcblk0" ]; then
		files=$(ls /dev/mmcblk0p* 2> /dev/null | wc -l)

		if [ "$files" != "0" ]; then
			exit 1
		else
		    echo "wait 1 second ..." > /dev/console
		    sleep 1
            ls /dev/mmc* > /dev/console
		    files=$(ls /dev/mmcblk0p* 2> /dev/null | wc -l)
		    if [ "$files" != "0" ]; then
			    exit 1
			fi
		fi
	fi
}

do_sdck()
{
	if [ ! -f "/opt/skyeye/bin/sdck" ];then
		echo "####don't support sdck" > /dev/console
		return
	fi
	#pro=$(ps | grep "tycam" | grep -v grep | grep -v "$0")
	#if [ -z "$pro" ]; then
	echo "#####do sdck...." > /dev/console
	if [ -f "/tmp/sdck_log" ];then
		echo "####already checked" > /dev/console
		return
	fi
	touch /tmp/sdck_log
	echo $(df | grep $1 | awk  '{print $2"|"$3"|"$4}') > /tmp/sdck_info
	#/opt/skyeye/bin/sdck -m 1 -f /dev/$1 > /dev/console
	
	rm -f /tmp/sdck_info
	echo "#####finish sdck...." > /dev/console
	#fi
	
}

do_umount()
{
	# sdcard
	if echo $1 | grep "mmcblk" > /dev/null 2>&1; then
		if grep -qs "^/dev/$1 " /proc/mounts ; then
			#if ps | grep tycam > /dev/null 2>&1;then                        
                		#kill -10 $(ps | grep tycam | awk '{print $1}' | head -1)
			#fi                                                              
#           		sleep 1s                                                       
#           		fuser -mk "$destdir_sd";
			echo "force umount"
			umount -l "$destdir_sd"
		fi

		[ -d "${destdir_sd}" ] && rm -rf "${destdir_sd}"
	# udisk
	elif echo $1 | grep "sd" > /dev/null 2>&1; then
		if grep "^/dev/$1 " /proc/mounts; then
			tmp=`grep "^/dev/$1 " /proc/mounts | \
				awk -F \  '{print $2}'`
			umount $tmp
			[ -d $tmp ] && rmdir $tmp
		fi

		check_any_usb_mounted
	fi
}

do_mount()
{
	# sdcard
	if cat /proc/mounts | grep "${destdir_sd}" > /dev/null 2>&1; then               
                umount -l "$destdir_sd";                                    
        fi
	if echo $1 | grep "mmcblk" > /dev/null 2>&1; then
		mkdir -p "${destdir_sd}" || exit 1

		if ! mount -t auto "/dev/$1" "${destdir_sd}"; then
	        echo "### Fail to mount /dev/$1...." > /dev/console
			# failed to mount, clean up mountpoint
			# rmdir "${destdir_sd}"
            [ -L /mnt/sdcard ] && rm -fr /mnt/sdcard
            ln -s ${destdir_sd} /mnt/sdcard
			exit 1
		fi
		#do_sdck $1
		
        [ -L /mnt/sdcard ] && rm -fr /mnt/sdcard
        ln -s ${destdir_sd} /mnt/sdcard
        
		#[ -x ${SDCARDDIR}/ty_sdcard_check_upgrade.sh ] && ${SDCARDDIR}/ty_sdcard_check_upgrade.sh ${destdir_sd}
		
	# udisk
	elif echo $1 | grep "sd" > /dev/null 2>&1; then
		case "$1" in
		sd[a-z][0-9])
			;;
		sd[a-z])
			# check if has other partition, such as /dev/sda1...
			if grep " $1[0-9]$" /proc/partitions; then
				exit 1
			fi
			;;
		esac

		mkdir -p $destdir_udisk || exit 1
		tmp=`blkid /dev/$1 | awk -F \" '{print $2}'`
		if [ "$tmp" = "" ]; then
			check_any_usb_mounted
			exit 1
		fi
		mkdir $destdir_udisk/$tmp || exit 1
		if ! mount -t auto /dev/$1 $destdir_udisk/$tmp; then
			rmdir $destdir_udisk/$tmp
			check_any_usb_mounted
			exit 1
		fi
	fi
	if [  -e /tuya/app/skyeye/bin/ty_sign ]; then
		export LD_LIBRARY_PATH=/mnt/sdcard/:$LD_LIBRARY_PATH
		/tuya/app/skyeye/bin/ty_sign
	fi
}

case "${ACTION}" in
add|"")
        check_dev_before_action ${MDEV}
        do_umount ${MDEV}
        do_mount ${MDEV}
        ;;
remove)
        do_umount ${MDEV}
        ;;
esac
