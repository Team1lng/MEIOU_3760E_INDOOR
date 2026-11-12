#!/bin/sh

# ecprivate file is exsit
if [ ! -f ./tuyaos_ecprikey.pem ]; then
	echo "do not exit tuyaos_ecprikey.pem, exit"
	exit -1
fi

#file=$(cat tuyaos_ecprikey.pem)
file=`openssl ec -in tuyaos_ecprikey.pem -text`
filesize=`echo ${#file}`
rawstr1=$(echo ${file:0:${filesize}})
echo "rawstr1 is ${rawstr1}"

rawstr2=`echo ${rawstr1:134:196}`
echo "rawstr2 is ${rawstr2}"

rawstr3=`echo ${rawstr2// /}`
echo "rawstr3 is ${rawstr3}"

rawstr4=`echo ${rawstr3//:/}`
echo "rawstr4 is ${rawstr4}"

resultstr="${rawstr4}"

if [ -f "tuya_verify_pubkey.bin" ]; then
	echo "rm raw tuya_verify_pubkey.bin"
	rm tuya_verify_pubkey.bin
fi

touch tuya_verify_pubkey.bin

echo ${resultstr} >> tuya_verify_pubkey.bin

echo "make ok"

