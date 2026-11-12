#! /bin/sh
if [ -f ../.build_product ];then
TUYAOS_PRODUCT_NAME=`cat ../.build_product`
else
TUYAOS_PRODUCT_NAME=fh8636
fi 

echo "product=${TUYAOS_PRODUCT_NAME}"

./build.sh ${TUYAOS_PRODUCT_NAME}


