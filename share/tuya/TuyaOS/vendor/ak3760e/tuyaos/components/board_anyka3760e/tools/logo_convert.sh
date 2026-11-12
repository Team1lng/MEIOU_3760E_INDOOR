#!/bin/sh
tmpres=./pix_fmt_tmp.rgb
filename=`echo $1 | sed  "s/\.[^.]*$//"`
echo $filename
ffmpeg -pix_fmts

ffmpeg -i $1 -pix_fmt rgb24 $tmpres
mv $tmpres $filename.rgb888

ffmpeg -i $1 -pix_fmt bgr24 $tmpres
mv $tmpres $filename.bgr888

ffmpeg -i $1 -pix_fmt rgb565 $tmpres
mv $tmpres $filename.rgb565

ffmpeg -i $1 -pix_fmt bgr565 $tmpres
mv $tmpres $filename.bgr565

ffmpeg -i $1 -pix_fmt yuv420p $tmpres
mv $tmpres $filename.yuv420p

ffmpeg -i $1 -pix_fmt nv12 $tmpres
mv $tmpres $filename.yuv420sp

ffmpeg -i $1 -pix_fmt argb $tmpres
mv $tmpres $filename.argb8888

ffmpeg -i $1 -pix_fmt rgba $tmpres
mv $tmpres $filename.rgba8888

ffmpeg -i $1 -pix_fmt abgr $tmpres
mv $tmpres $filename.abgr8888

ffmpeg -i $1 -pix_fmt bgra $tmpres
mv $tmpres $filename.bgra8888
