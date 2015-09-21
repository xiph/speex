#!/bin/sh

cd ..

./configure --host=arm --enable-arm5e-asm -enable-fixed-point -disable-float-api

cd android

# force rebuild libs
ndk-build -B V=1 -j4

ls -lR libs
