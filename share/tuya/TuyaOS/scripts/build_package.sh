#!/bin/bash

# CI系统传入的参数说明：
#
# $1 - 产品目录，如： apps/product1
# $2 - 产品名称，如： product1
# $3 - 产品版本，如： 1.0.0

[ -z $CI_PACKAGE_PATH ] && exit 12
python3 ./scripts/build_package_info.py "$1" "$2" "$3" "$CI_PACKAGE_PATH" || exit 13
tar -czf ${CI_PACKAGE_PATH}.tar.gz -C "$(dirname ${CI_PACKAGE_PATH})" "$(basename ${CI_PACKAGE_PATH})"
