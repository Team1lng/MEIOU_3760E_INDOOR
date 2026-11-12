#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import shutil
import json
import os
import hashlib

# $1 - 产品目录，如： apps/product1
# $2 - 产品名称，如： product1
# $3 - 产品版本，如： 1.0.0
# $4 - 产物包路径，如： output/dist/product1_1.0.0
outputPath = sys.argv[4]
productName = sys.argv[2]
productVersion = sys.argv[3]


# 计算MD5
def getMD5(file_path):
    md5 = None
    if os.path.isfile(file_path):
        f = open(file_path, 'rb')
        md5_obj = hashlib.md5()
        md5_obj.update(f.read())
        hash_code = md5_obj.hexdigest()
        f.close()
        md5 = str(hash_code).lower()
    return md5


def qio2prod():
    # 搜索目录，构造json字符串
    qio = ""
    for f in os.listdir(outputPath):
        file_path = os.path.join(outputPath, f)
        if not os.path.isfile(file_path):
            continue
        if f.find("_PROD_") != -1:
            return
        if f.find("_QIO_") != -1:
            qio = f
            qio_path = file_path
            continue

    prod = qio.replace("_QIO_", "_PROD_", 1)
    prod_path = os.path.join(outputPath, prod)
    shutil.copyfile(qio_path, prod_path)
    pass


def main():
    if not productName:
        print("please input the product name!")
        return

    if not outputPath:
        print("please input the path of product output")
        return

    qio2prod()
    binFileKeys = {
        "_QIO_": "fwQIO",
        "_QOUT_": "fwQOUT",
        "_DIO_": "fwDIO",
        "_DOUT_": "fwDOUT",
        "_UA_": "fwUserArea",
        "_UG_": "fwUpgrade",
        "_STU_": "fwSTU",
        "_PROD_": "fwPROD",
    }
    packageInfoFile = productName + '_' + productVersion + '.json'
    packageInfo = []

    # 搜索目录，构造json字符串
    for f in os.listdir(outputPath):
        file_path = os.path.join(outputPath, f)
        if not os.path.isfile(file_path):
            continue
        for key in binFileKeys:
            if f.find(key) != -1:
                bin_info = {
                    "name": f,
                    "MD5":  getMD5(outputPath + '/' + f),
                }
                packageInfo.append({binFileKeys[key]: bin_info})
                del binFileKeys[key]
                break
    jsonStr = json.dumps(packageInfo, indent=2, ensure_ascii=False)
    print(jsonStr)

    # 写入文件
    fp = open(outputPath + '/' + packageInfoFile, 'w')
    fp.write(jsonStr)
    fp.close()
    return


if __name__ == '__main__':
    main()
