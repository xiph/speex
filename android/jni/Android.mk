LOCAL_PATH := $(call my-dir)

SPEEX_PATH=$(realpath $(LOCAL_PATH)/../..)
#$(info $(SPEEX_PATH))

ifeq ($(NDK_DEBUG),1)

# for profiler when debug!
include $(CLEAR_VARS)

LOCAL_CFLAGS := -fvisibility=hidden -DDEBUG
LOCAL_MODULE    := android-ndk-profiler
LOCAL_SRC_FILES := profiler/gnu_mcount.S profiler/prof.c profiler/read_maps.c

include $(BUILD_STATIC_LIBRARY)
endif

# for speex-jni
include $(CLEAR_VARS)

LOCAL_MODULE := libspeex-jni

LOCAL_CFLAGS += -Wall -fvisibility=hidden
LOCAL_C_INCLUDES := $(SPEEX_PATH)/include

LOCAL_SRC_FILES := speex-jni.cpp
LOCAL_STATIC_LIBRARIES := libspeex

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--as-needed

include $(BUILD_SHARED_LIBRARY)

# for speexenc
include $(CLEAR_VARS)

LOCAL_MODULE := speexenc

LOCAL_CFLAGS += -Wall -fvisibility=hidden

# add debug info
ifeq ($(NDK_DEBUG),1)
LOCAL_CFLAGS += -pg -fno-omit-frame-pointer -DENABLE_PROFILER
endif

LOCAL_C_INCLUDES := $(SPEEX_PATH)/include

LOCAL_SRC_FILES := $(SPEEX_PATH)/src/speexenc.c \
	$(SPEEX_PATH)/src/skeleton.c \
	$(SPEEX_PATH)/src/wav_io.c

LOCAL_STATIC_LIBRARIES := libspeex
# add profiler static library
ifeq ($(NDK_DEBUG),1)
LOCAL_STATIC_LIBRARIES += libandroid-ndk-profiler
endif

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--as-needed -logg

include $(BUILD_EXECUTABLE)

# for speexdec
include $(CLEAR_VARS)

LOCAL_MODULE := speexdec

LOCAL_CFLAGS += -Wall -fvisibility=hidden

# add debug info
ifeq ($(NDK_DEBUG),1)
LOCAL_CFLAGS += -pg -fno-omit-frame-pointer -DENABLE_PROFILER
endif

LOCAL_C_INCLUDES := $(SPEEX_PATH)/include

LOCAL_SRC_FILES := $(SPEEX_PATH)/src/speexdec.c \
	$(SPEEX_PATH)/src/skeleton.c \
	$(SPEEX_PATH)/src/wav_io.c

LOCAL_STATIC_LIBRARIES := libspeex 
# add profiler static library
ifeq ($(NDK_DEBUG),1)
LOCAL_STATIC_LIBRARIES += libandroid-ndk-profiler
endif

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--as-needed -logg

include $(BUILD_EXECUTABLE)

include $(SPEEX_PATH)/Android.mk
