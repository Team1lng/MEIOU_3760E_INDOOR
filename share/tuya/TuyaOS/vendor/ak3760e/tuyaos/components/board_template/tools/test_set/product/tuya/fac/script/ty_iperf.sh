#!/bin/sh



FACDIR=`cd "$(dirname "$0")";pwd`
INI_FILE=${FACDIR}/../cfg/config.ini
file=/usr/local/bin/

LD_LIBRARY_PATH="./:/usr/local/lib:/usr/lib:${FACDIR}/../bin/"
export LD_LIBRARY_PATH

readIni()
{
    file=$1;section=$2;item=$3;
    val=$(awk -F '=' '/\['${section}'\]/{a=1} (a==1 && "'${item}'"==$1){a=0;print $2}' ${file}) 
    echo ${val}	
}
PCADDR=$(echo $(readIni ${INI_FILE} WIFI pc) | sed -e 's/\r//g')
echo $PCADDR > /dev/console

if [ "$PCADDR" == "" ];then
	echo "pc ip is not in config.ini" > /dev/console
	PCADDR=$1
fi

echo "server ip is $PCADDR"> /dev/console

sleep 1
if [ "$2" == "" ] ;then
    ${FACDIR}/../bin/iperf3 -c $PCADDR -w 64k -t 10 > /dev/console
else
    echo "out file is $2"> /dev/console
    ${FACDIR}/../bin/iperf3 -c $PCADDR -w 64k -t 10 -J > $2    
fi

echo "result is $? " > /dev/console
