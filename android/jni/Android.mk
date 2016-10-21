LOCAL_PATH := $(call my-dir)

SPEEX_PATH=$(realpath $(LOCAL_PATH)/../..)
SPX_ANDROID_PATH=$(realpath $(LOCAL_PATH)/..)
$(info SPEEX_PATH = $(SPEEX_PATH))

##########################
#    libspeex      static library       #
 ##########################

include $(CLEAR_VARS)
 
LOCAL_MODULE := libspeex
LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_C_INCLUDES := $(SPX_ANDROID_PATH)
LOCAL_C_INCLUDES += $(SPEEX_PATH)/include 

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES :=  \
	$(SPEEX_PATH)/libspeex/bits.c \
	$(SPEEX_PATH)/libspeex/cb_search.c \
	$(SPEEX_PATH)/libspeex/exc_10_32_table.c \
	$(SPEEX_PATH)/libspeex/exc_10_16_table.c \
	$(SPEEX_PATH)/libspeex/exc_20_32_table.c \
	$(SPEEX_PATH)/libspeex/exc_5_256_table.c \
	$(SPEEX_PATH)/libspeex/exc_5_64_table.c \
	$(SPEEX_PATH)/libspeex/exc_8_128_table.c \
	$(SPEEX_PATH)/libspeex/filters.c \
	$(SPEEX_PATH)/libspeex/gain_table.c \
	$(SPEEX_PATH)/libspeex/hexc_table.c \
	$(SPEEX_PATH)/libspeex/high_lsp_tables.c \
	$(SPEEX_PATH)/libspeex/lsp.c \
	$(SPEEX_PATH)/libspeex/ltp.c \
	$(SPEEX_PATH)/libspeex/speex.c \
	$(SPEEX_PATH)/libspeex/stereo.c \
	$(SPEEX_PATH)/libspeex/vbr.c \
	$(SPEEX_PATH)/libspeex/vq.c \
	$(SPEEX_PATH)/libspeex/gain_table_lbr.c \
	$(SPEEX_PATH)/libspeex/hexc_10_32_table.c \
	$(SPEEX_PATH)/libspeex/lpc.c \
	$(SPEEX_PATH)/libspeex/lsp_tables_nb.c \
	$(SPEEX_PATH)/libspeex/modes.c \
	$(SPEEX_PATH)/libspeex/modes_wb.c \
	$(SPEEX_PATH)/libspeex/nb_celp.c \
	$(SPEEX_PATH)/libspeex/quant_lsp.c \
	$(SPEEX_PATH)/libspeex/sb_celp.c \
	$(SPEEX_PATH)/libspeex/speex_callbacks.c \
	$(SPEEX_PATH)/libspeex/speex_header.c \
	$(SPEEX_PATH)/libspeex/window.c

include $(BUILD_STATIC_LIBRARY)

##########################
#    libspeex-jni shared library       #
 ##########################

include $(CLEAR_VARS)

LOCAL_MODULE := libspeex-jni

LOCAL_CFLAGS += -Wall -fvisibility=hidden -DHAVE_CONFIG_H
LOCAL_C_INCLUDES := $(SPX_ANDROID_PATH)
LOCAL_C_INCLUDES += $(SPEEX_PATH)/include 

LOCAL_SRC_FILES := speex-jni.cpp
LOCAL_STATIC_LIBRARIES := libspeex

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -Wl,--as-needed

include $(BUILD_SHARED_LIBRARY)
