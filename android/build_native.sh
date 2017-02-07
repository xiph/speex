#!/bin/sh

mkdir ../dist
mkdir ../dist/arm
mkdir ../dist/x86

# for arm
cd ..

export CC=arm-linux-androideabi-gcc

./configure --host=arm --enable-arm5e-asm --enable-fixed-point --disable-float-api

cd android

ndk-build -B V=1 -j4 APP_ABI=armeabi-v7a

ls -lR libs

cp ../config.h ../dist/arm/config.h
cp libs/armeabi-v7a/libspeex-jni.so ../dist/arm/libspeex-jni.so
cp obj/local/armeabi-v7a/libspeex.a ../dist/arm/libspeex.a

# for x86
cd ..

export CC=i686-linux-android-gcc

./configure --host=arm --enable-sse

cd android

ndk-build -B V=1 -j4 APP_ABI=x86

ls -lR libs

cp ../config.h ../dist/x86/config.h
cp libs/armeabi-v7a/libspeex-jni.so ../dist/x86/libspeex-jni.so
cp obj/local/armeabi-v7a/libspeex.a ../dist/x86/libspeex.a
