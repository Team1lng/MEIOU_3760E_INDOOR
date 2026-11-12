#! /bin/bash 

if [ -z "$1" ];then
        echo -e "please input the file  signature"
        echo -e "example: ./signature kernel.bin"
        exit 1
fi

file=$1

FILE_HASH=`sha256sum $file`
HASH=${FILE_HASH:0:64}
echo -e "file sha256: $HASH"

#CLOUD_RESULT=`curl --request POST 'https://testbook-daily.tuya-inc.cn:7799/open/call' --header 'token:1uya!%2535@' --data 'region=AY' --data 'api=tuya.m.file.sign_1.0' --data 'postData={ "fileHash" : "'"$HASH"'" }' --data 'env=prod' > tmp.json`
CLOUD_RESULT=`curl --request POST 'https://testbook-ng.tuya-inc.top:7799/open/call'  --header 'token:1uya!%2535@' --data 'region=AY' --data 'api=tuya.m.file.sign_1.0' --data 'postData={ "fileHash" : "'"$HASH"'" }' --data 'env=prod'  > tmp.json`
SUCC=`jq '.data|fromjson|.success' tmp.json`
RESP=`jq '.data|fromjson|.result' tmp.json`
if [ $SUCC = "true" ];then
	rm tuya.sign -f
	echo -e ${RESP} | tr -d '"' > $file.sign 
else
	echo -e "cloud response fail"
fi
rm tmp.json
