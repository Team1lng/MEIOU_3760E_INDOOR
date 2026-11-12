#./configure --host=arm-fullhanv3-linux-uclibcgnueabi CFLAGS=-static --prefix=`pwd`/_install
host=$1
./configure --host=${host%?} CFLAGS=-static --prefix=`pwd`/_install
