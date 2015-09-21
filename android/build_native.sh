#!/bin/sh

cd ..

export CC=arm-linux-androideabi-gcc

./configure --host=arm --enable-arm5e-asm -enable-fixed-point -disable-float-api

cd android

# force rebuild libs
ndk-build -B V=1 -j4

ls -lR libs
