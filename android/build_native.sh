#!/bin/sh

mkdir ../dist

# for arm
cd ..

export CC=arm-linux-androideabi-gcc

./configure --host=arm --enable-arm5e-asm --enable-fixed-point --disable-float-api

cd android

ndk-build -B V=1 -j4 APP_ABI=armeabi-v7a

ls -lR libs

cp ../config.h ../dist/config_arm.h
cp libs/armeabi-v7a/libspeex-jni.so ../dist/libspeex-jni_arm.so
cp obj/local/armeabi-v7a/libspeex.a ../dist/libspeex_arm.a

# for x86
cd ..

export CC=i686-linux-android-gcc

./configure --host=arm --enable-sse

cd android

ndk-build -B V=1 -j4 APP_ABI=x86

ls -lR libs

cp ../config.h ../dist/config_x86.h
cp libs/x86/libspeex-jni.so ../dist/libspeex-jni_x86.so
cp obj/local/x86/libspeex.a ../dist/libspeex_x86.a
