#!/bin/sh

FACDIR=`cd "$(dirname "$0")";pwd`

PATH="${PATH}:/opt/bin/:${FACDIR}/bin/:${RUNDIR}/bin/:/opt/skyeye/bin"
export PATH

echo "FAC MONITOR START" > /dev/console

while [ true ]
    do
	    pro0=$(ps | grep "tycam" | grep -v grep)
	    pro1=$(ps | grep "ty_monitor.sh" | grep -v grep)
    	if [ -n "$pro0" -o -n "$pro1" ]; then
			killall -q  ty_monitor.sh
			killall -q  -9 tycam
    	fi
		
		sleep 1
done




