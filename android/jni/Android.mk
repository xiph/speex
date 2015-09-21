LOCAL_PATH := $(call my-dir)

SPEEX_PATH=$(LOCAL_PATH)/../..

include $(CLEAR_VARS)

LOCAL_MODULE := libspeex-jni

LOCAL_CFLAGS += -Wall -fvisibility=hidden
LOCAL_C_INCLUDES := $(SPEEX_PATH)/include

LOCAL_SRC_FILES := speex-jni.cpp
LOCAL_STATIC_LIBRARIES := libspeex

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--as-needed

include $(BUILD_SHARED_LIBRARY)

include $(SPEEX_PATH)/Android.mk