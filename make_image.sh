#!/bin/bash
if [ ! -d "build/" ]; then
    mkdir build/
fi

python script/mkbin.py project/resource/  build/res.bin include/res.h

cd build/
set -e 
cmake ../;make
cd -
   