#!/usr/bin/env python2
# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import os
import struct
import argparse
from PIL import Image
from glob import glob

def find_resource_files(root_dir):
    resource_files = []
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.ttf','.mp3')):
                full_path = os.path.join(dirpath, filename)
                resource_files.append(full_path)
    return sorted(resource_files)

def align_to_512(file_obj):
    """填充 0x00 直到文件指针对齐到 512 字节"""
    current_pos = file_obj.tell()
    padding = (512 - (current_pos % 512)) % 512
    if padding > 0:
        file_obj.write(b'\x00' * padding)
    return current_pos + padding  # 返回对齐后的位置

def process_resources(input_dir, output_bin, output_h):
    resource_entries = []
    
    with open(output_bin, 'wb') as bin_file, \
         open(output_h, 'w') as h_file:

        # 写入头文件头部
        h_file.write("""#ifndef __RESOURCE_DATA_H__
#define __RESOURCE_DATA_H__

#include <stdint.h>

typedef enum {
    RESOURCE_FORMAT_UNKNOWN = 0,
    RESOURCE_FORMAT_ARGB,
    RESOURCE_FORMAT_JPEG,
    RESOURCE_FORMAT_TTF
} ResourceFormat;

""")

        # 处理所有资源文件
        for file_path in find_resource_files(input_dir):
            try:
                filename = os.path.basename(file_path)
                rel_path = os.path.relpath(file_path, start=input_dir)
                var_name = rel_path.replace('/', '_').replace('.', '_').replace('-', '_').upper()
                
                # 记录起始位置
                start_pos = bin_file.tell()
                
                if file_path.lower().endswith('.png'):
                    # PNG处理
                    print("Processing PNG: {} ".format(rel_path))
                    with Image.open(file_path) as img:
                        width, height = img.size
                        bin_file.write(b'S@AC')
                        bin_file.write(struct.pack('<II', width, height))
                        
                        if img.mode != 'RGBA':
                            img = img.convert('RGBA')
                        argb_data = bytearray()
                        for r, g, b, a in img.getdata():
                            argb_data.extend([b, g, r, a])
                        bin_file.write(argb_data)
                        img_format = "RESOURCE_FORMAT_ARGB"

                        aligned_pos = align_to_512(bin_file)
                        data_size = aligned_pos - start_pos

                        # 添加到头文件
                        h_file.write("/* {} */\n".format(rel_path))
                        h_file.write("#define %s_OFFSET 0x%08X\n" % (var_name, start_pos))
                        h_file.write("#define %s_SIZE %d\n" % (var_name, data_size))
                        h_file.write("#define %s_WIDTH %d\n" % (var_name, width))
                        h_file.write("#define %s_HEIGHT %d\n" % (var_name, height))
                        h_file.write("#define %s_FORMAT %s\n\n" % (var_name, img_format))
                
                elif file_path.lower().endswith(('.jpg', '.jpeg')):
                    # JPEG处理
                    print("Processing JPEG: {} ".format(rel_path))
                    with Image.open(file_path) as img:
                        width, height = img.size
                        bin_file.write(b'S@JG')
                        bin_file.write(struct.pack('<II', width, height))
                        
                        with open(file_path, 'rb') as f:
                            jpeg_data = f.read()
                        bin_file.write(jpeg_data)
                        img_format = "RESOURCE_FORMAT_JPEG"
                        
                        aligned_pos = align_to_512(bin_file)
                        data_size = aligned_pos - start_pos
                        # 添加到头文件
                        h_file.write("/* {} */\n".format(rel_path))
                        h_file.write("#define %s_OFFSET 0x%08X\n" % (var_name, start_pos))
                        h_file.write("#define %s_SIZE %d\n" % (var_name, data_size))
                        h_file.write("#define %s_WIDTH %d\n" % (var_name, width))
                        h_file.write("#define %s_HEIGHT %d\n" % (var_name, height))
                        h_file.write("#define %s_FORMAT %s\n\n" % (var_name, img_format))
                
                elif file_path.lower().endswith('.ttf'):
                    # TTF处理
                    print("Processing TTF: {} ".format(rel_path))
                    with open(file_path, 'rb') as f:
                        ttf_data = f.read()
                    
                    # 写入TTF文件大小
                    # bin_file.write(struct.pack('<I', len(ttf_data)))
                    bin_file.write(ttf_data)
                    img_format = "RESOURCE_FORMAT_TTF"
                    
                    aligned_pos = align_to_512(bin_file)
                    data_size = aligned_pos - start_pos
                    # 添加到头文件
                    h_file.write("/* {} */\n".format(rel_path))
                    h_file.write("#define %s_OFFSET 0x%08X\n" % (var_name, start_pos))
                    h_file.write("#define %s_SIZE %d\n" % (var_name,data_size))
                    h_file.write("#define %s_FORMAT %s\n\n" % (var_name, img_format))
                
                elif file_path.lower().endswith('.mp3'):
                    # TTF处理
                    print("Processing mp3: {} ".format(rel_path))
                    with open(file_path, 'rb') as f:
                        ttf_data = f.read()
                    
                    # 写入TTF文件大小
                    # bin_file.write(struct.pack('<I', len(ttf_data)))
                    bin_file.write(ttf_data)
                    img_format = "RESOURCE_FORMAT_MP3"
                    
                    aligned_pos = align_to_512(bin_file)
                    data_size = aligned_pos - start_pos

                    # 添加到头文件
                    h_file.write("/* {} */\n".format(rel_path))
                    h_file.write("#define %s_OFFSET 0x%08X\n" % (var_name, start_pos))
                    h_file.write("#define %s_SIZE %d\n" % (var_name, data_size))
                    h_file.write("#define %s_FORMAT %s\n\n" % (var_name, img_format))
                # 记录结束位置
                end_pos = bin_file.tell()
                size = end_pos - start_pos
                
                # 保存条目信息
                resource_entries.append((var_name, rel_path))

            except Exception as e:
                print("Error processing {}: {}".format(file_path, str(e)))
                continue

        h_file.write("""
#endif /* __RESOURCE_DATA_H__ */
""")

    print("\nBinary output: {output_bin}")
    print("Header output: {output_h}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Pack images into binary + header')
    parser.add_argument('input_dir', help='Directory containing images')
    parser.add_argument('output_bin', help='Output binary file path')
    parser.add_argument('output_h', help='Output header file path')
    args = parser.parse_args()
    
    process_resources(args.input_dir, args.output_bin, args.output_h)