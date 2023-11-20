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

HIDE := @

LOCAL_PATH := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
ifeq (true, $(APP_DEBUG))
	BIN_DIR := $(LOCAL_PATH)/bin/debug
	OBJ_DIR := $(LOCAL_PATH)/obj/debug
else
	BIN_DIR := $(LOCAL_PATH)/bin/release
	OBJ_DIR := $(LOCAL_PATH)/obj/release
endif
SOURCE_DIR := $(LOCAL_PATH)/../source
THIRD_PARTY_DIR := $(LOCAL_PATH)/../thirdparty

CC := clang++

C_FLAGS := 
C_FLAGS += -Wall -Werror=return-type
C_FLAGS += -fPIC
C_FLAGS += -pthread
ifeq (true, $(APP_DEBUG))
	C_FLAGS += -g -O0 -UNDEBUG
else
	C_FLAGS += -O2 -DNDEBUG
endif

LD_FLAGS := 
LD_FLAGS += -pthread
LD_FLAGS += -Wl,--no-undefined
LD_FLAGS += -Wl,--enable-new-dtags 
LD_FLAGS += -Wl,-rpath,\$$ORIGIN
LD_FLAGS += -z now
LD_FLAGS += -z relro
ifneq (true, $(APP_DEBUG))
	LD_FLAGS += -s
endif

all :  \
	$(BIN_DIR)/libBRX-PAL.so

# Link
$(BIN_DIR)/libBRX-PAL.so: \
	$(LOCAL_PATH)/libBRX-PAL.map \
	$(OBJ_DIR)/BRX-PAL-brx_pal_device.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.o \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.o \
	$(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.o
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) $(CC) -shared $(LD_FLAGS) \
		-Wl,--version-script=$(LOCAL_PATH)/libBRX-PAL.map \
		$(OBJ_DIR)/BRX-PAL-brx_pal_device.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.o \
		$(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.o \
		$(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.o \
		-L $(THIRD_PARTY_DIR)/Vulkan-Loader/lib/linux/x64 -lvulkan \
		-o $(BIN_DIR)/libBRX-PAL.so

# Compile
$(OBJ_DIR)/BRX-PAL-brx_pal_device.o: $(SOURCE_DIR)/brx_pal_device.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_device.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_device.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_device.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.o: $(SOURCE_DIR)/brx_pal_vk_buffer.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_buffer.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.o: $(SOURCE_DIR)/brx_pal_vk_command_buffer.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_command_buffer.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.o: $(SOURCE_DIR)/brx_pal_vk_descriptor.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_descriptor.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.o: $(SOURCE_DIR)/brx_pal_vk_device.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_device.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.o: $(SOURCE_DIR)/brx_pal_vk_fence.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_fence.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.o: $(SOURCE_DIR)/brx_pal_vk_frame_buffer.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_frame_buffer.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.o: $(SOURCE_DIR)/brx_pal_vk_image.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_image.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.o: $(SOURCE_DIR)/brx_pal_vk_pipeline.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_pipeline.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.o: $(SOURCE_DIR)/brx_pal_vk_queue.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_queue.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.o: $(SOURCE_DIR)/brx_pal_vk_render_pass.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_render_pass.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.o: $(SOURCE_DIR)/brx_pal_vk_sampler.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_sampler.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.o: $(SOURCE_DIR)/brx_pal_vk_swap_chain.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_swap_chain.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.o

$(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.o: $(SOURCE_DIR)/brx_pal_vk_vma.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/brx_pal_vk_vma.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.d -o $(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.o

$(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.o: $(THIRD_PARTY_DIR)/McRT-Malloc/source/mcrt_malloc.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(THIRD_PARTY_DIR)/McRT-Malloc/source/mcrt_malloc.cpp -MD -MF $(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.d -o $(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.o


-include \
	$(OBJ_DIR)/BRX-PAL-brx_pal_device.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.d \
	$(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.d \
	$(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.d

clean:
	$(HIDE) rm -f $(BIN_DIR)/libBRX-PAL.so
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_device.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.o
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_device.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_buffer.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_command_buffer.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_descriptor.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_device.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_fence.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_frame_buffer.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_image.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_pipeline.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_queue.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_render_pass.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_sampler.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_swap_chain.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-brx_pal_vk_vma.d
	$(HIDE) rm -f $(OBJ_DIR)/BRX-PAL-thirdparty-McRT-Malloc-mcrt_malloc.d

.PHONY : \
	all \
	clean