#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(call my-dir)

# Platform Abstraction Layer

include $(CLEAR_VARS)

ifeq (armeabi-v7a, $(TARGET_ARCH_ABI))
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := BRX-PAL

LOCAL_SRC_FILES := \
	$(LOCAL_PATH)/../source/brx_pal_device.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_buffer.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_command_buffer.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_descriptor.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_device.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_fence.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_frame_buffer.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_image.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_pipeline.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_queue.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_render_pass.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_sampler.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_swap_chain.cpp \
	$(LOCAL_PATH)/../source/brx_pal_vk_vma.cpp \
	$(LOCAL_PATH)/../thirdparty/McRT-Malloc/source/mcrt_malloc.cpp 

LOCAL_CFLAGS :=
LOCAL_CFLAGS += -Wall
LOCAL_CFLAGS += -Werror=return-type

LOCAL_CPPFLAGS :=

LOCAL_C_INCLUDES :=

LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -Wl,--enable-new-dtags
LOCAL_LDFLAGS += -Wl,-rpath,\$$ORIGIN
LOCAL_LDFLAGS += -Wl,--version-script,$(LOCAL_PATH)/libBRX-PAL.map

LOCAL_LDLIBS :=
LOCAL_LDLIBS += -lvulkan
ifeq (true, $(APP_DEBUG))
	LOCAL_LDLIBS += -llog
endif

include $(BUILD_SHARED_LIBRARY)
