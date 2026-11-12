#!/usr/bin/env python3
# coding=utf-8
##
# @file write_verid_to_bin.py
# @brief 使用本脚本可以在bin文件结尾添加json形式的TUYAOS_VERSION_ID
#        ID值通过环境变量(TUYAOS_VERSION_ID)获取
#        在json数据前拼入标记符"firmwareMetaData"
#        并使用最后四个字节(大端)表示拼入数据的总长度
# @example python3 write_verid_to_bin.py ./output/xxx.bin
# @author huatuo
# @version 1.0.0
# @date 2025-01-14


import sys
import os
import json
import struct

ENV_ID = "TUYAOS_VERSION_ID"
KEY_WORD = "firmwareMetaData"


def write_verid_to_bin(ver_id, bin_path):
    json_data = {
        "TUYAOS_VERSION_ID": ver_id,
    }
    json_str = KEY_WORD + json.dumps(json_data, separators=(',', ':'))
    json_len = len(json_str) + 4
    len_encode = struct.pack('>I', json_len)
    json_encode = json_str.encode('utf-8')
    json_len = len(json_encode)
    with open(bin_path, 'ab') as f:
        f.write(json_encode)
        f.write(len_encode)
    pass


def get_verid():
    ver_id = os.environ.get(ENV_ID)
    return ver_id


def main():
    if len(sys.argv) < 2:
        print("Error: write versionID need [bin path].")
        exit(1)
    bin_path = sys.argv[1]
    if not os.path.exists(bin_path):
        print(f"Error: not found bin path [{bin_path}].")
        exit(1)
    ver_id = get_verid()
    if ver_id is None:
        print(f"Error: not found environment variable [{ENV_ID}].")
        exit(1)
    write_verid_to_bin(ver_id, bin_path)
    pass


if __name__ == "__main__":
    main()
    pass
