#!/bin/bash
./configure \
--host=arm-linux-gnueabihf \
--disable-shared \
--enable-static \
CC=arm-anykav500-eabi-gcc \
cpp=arm-anykav500-eabi-g++ \