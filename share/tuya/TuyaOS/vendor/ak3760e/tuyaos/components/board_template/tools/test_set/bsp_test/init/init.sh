#!/bin/sh

INI_FILE=/tuya/data/config.ini
INTERFACE=wlan0
DRV=nl80211
USB=

PATH="/usr/bin:/usr/sbin:/bin:/sbin:/opt/bin/:/opt/skyeye/bin/"
LD_LIBRARY_PATH="./:/usr/local/lib:/usr/lib:/opt/lib/:/tuya/data"

export PATH
export LD_LIBRARY_PATH

install_drv() 
{
    echo "excute platform script..." > /dev/console                
}

readIni()
{
    file=$1;section=$2;item=$3;
    val=$(awk -F '=' '/\['${section}'\]/{a=1} (a==1 && "'${item}'"==$1){a=0;print $2}' ${file}) 
    echo ${val}	
}

writeIni()
{
    file=$1;section=$2;item=$3;val=$4
    awk -F '=' '/\['${section}'\]/{a=1} (a==1 && "'${item}'"==$1){gsub($2,"\"'${val}'\"");a=0} {print $0}' ${file} 1<>${file}
}

readIniSections()
{
    file=$1;
    val=$(awk '/\[/{printf("%s ",$1)}' ${file} | sed 's/\[//g' | sed 's/\]//g')
    echo ${val}
}

install_drv

if [ -f ${INI_FILE} ]; then
	echo "Load Config from $INI_FILE" > /dev/console
	
	SSID=$(echo $(readIni ${INI_FILE} WIFI ssid) | sed -e 's/\r//g')
	PSK=$(echo $(readIni ${INI_FILE} WIFI password) | sed -e 's/\r//g')
	IPADDR=$(echo $(readIni ${INI_FILE} WIFI ipaddr) | sed -e 's/\r//g')
	NETMASK=$(echo $(readIni ${INI_FILE} WIFI netmask) | sed -e 's/\r//g')
	INTERFACE=$(echo $(readIni ${INI_FILE} WIFI interface) | sed -e 's/\r//g')
	USB=$(echo $(readIni ${INI_FILE} WIFI usbethernet) | sed -e 's/\r//g')
	ETH=$(echo $(readIni ${INI_FILE} WIFI ethip) | sed -e 's/\r//g')
	
	WPA_CONF=/tmp/tmp.conf 
	#临时文件生成
	echo "network={" > $WPA_CONF
	echo "ssid="\""$SSID"\"" " >> $WPA_CONF
	if [ "$PSK" == "" ];then
		echo  "key_mgmt=NONE" >> $WPA_CONF
	else
		echo "psk=\"${PSK}\"">> $WPA_CONF
	fi
	echo "}">> $WPA_CONF
else
	echo "Use default wpa config#" > /dev/console
fi

sleep 1
echo "done"

ifconfig $INTERFACE up
iwconfig $INTERFACE mode managed
killall -q wpa_cli
killall -q wpa_supplicant 
killall -q udhcpc
wpa_supplicant -B -D${DRV} -i $INTERFACE -c $WPA_CONF

if [ -z $IPADDR ]; then
	echo "auto ip addr " > /dev/console
	udhcpc -i $INTERFACE &
	sleep 3
else
	echo "static ip addr " > /dev/console
	echo "ifconfig $INTERFACE $IPADDR netmask $NETMASK " > /dev/console
	ifconfig $INTERFACE $IPADDR netmask $NETMASK &

fi

ifconfig usb0 up
ifconfig usb0 $USB netmask 255.255.255.0
ifconfig eth0 up
ifconfig eth0 $ETH netmask 255.255.255.0
