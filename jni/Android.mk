# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)

LOCAL_MODULE := pindrop
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := libmathfu SDL2_mixer_static

PINDROP_DIR := $(LOCAL_PATH)

include $(PINDROP_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

# realpath-portable From flatbuffers/android/jni/include.mk
LOCAL_PATH := $(call realpath-portable,$(LOCAL_PATH))
PINDROP_DIR := $(LOCAL_PATH)

PINDROP_ASYNC_LOADING ?= 0

PINDROP_MIXER ?= sdl_mixer

PINDROP_MIXER_DIR ?= $(PINDROP_DIR)/src/mixer/$(PINDROP_MIXER)

ifneq (0,$(PINDROP_ASYNC_LOADING))
  PINDROP_FILE_LOADER_DIR ?= $(PINDROP_DIR)/src/asynchronous_loader
else
  PINDROP_FILE_LOADER_DIR ?= $(PINDROP_DIR)/src/synchronous_loader
endif

ifeq ("$(PINDROP_MIXER)",sdl_mixer)
  PINDROP_SDL_MIXER_MULTISTREAM ?= 0
endif

PINDROP_GENERATED_OUTPUT_DIR := $(PINDROP_DIR)/gen/include

LOCAL_EXPORT_C_INCLUDES := \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/include \
  $(PINDROP_DIR)/include \
  $(PINDROP_GENERATED_OUTPUT_DIR)

LOCAL_C_INCLUDES := \
  $(LOCAL_EXPORT_C_INCLUDES) \
  $(PINDROP_DIR)/src \
  $(PINDROP_FILE_LOADER_DIR) \
  $(PINDROP_MIXER_DIR) \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/include \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_SDL_DIR) \
  $(DEPENDENCIES_SDL_DIR)/include \
  $(DEPENDENCIES_SDL_MIXER_DIR)

ifneq (0,$(PINDROP_ASYNC_LOADING))
LOCAL_STATIC_LIBRARIES += libfplbase
LOCAL_C_INCLUDES += $(DEPENDENCIES_FPLBASE_DIR)/include
endif

LOCAL_SRC_FILES := \
  src/audio_engine.cpp \
  src/bus.cpp \
  src/bus_internal_state.cpp \
  src/channel.cpp \
  src/channel_internal_state.cpp \
  src/listener.cpp \
  src/log.cpp \
  src/ref_counter.cpp \
  src/sound_bank.cpp \
  src/sound_collection.cpp \
  src/version.cpp \
  $(PINDROP_MIXER_DIR)/mixer.cpp \
  $(PINDROP_MIXER_DIR)/real_channel.cpp \
  $(PINDROP_MIXER_DIR)/sound.cpp \
  $(PINDROP_FILE_LOADER_DIR)/file_loader.cpp

PINDROP_SCHEMA_DIR := $(PINDROP_DIR)/schemas
PINDROP_SCHEMA_INCLUDE_DIRS :=

PINDROP_SCHEMA_FILES := \
  $(PINDROP_SCHEMA_DIR)/audio_config.fbs \
  $(PINDROP_SCHEMA_DIR)/buses.fbs \
  $(PINDROP_SCHEMA_DIR)/sound_bank_def.fbs \
  $(PINDROP_SCHEMA_DIR)/sound_collection_def.fbs

ifeq (,$(PINDROP_RUN_ONCE))
PINDROP_RUN_ONCE := 1
$(call flatbuffers_header_build_rules, \
  $(PINDROP_SCHEMA_FILES), \
  $(PINDROP_SCHEMA_DIR), \
  $(PINDROP_GENERATED_OUTPUT_DIR), \
  $(PINDROP_SCHEMA_INCLUDE_DIRS), \
  $(LOCAL_SRC_FILES), \
  pindrop_generated_includes)
endif

include $(BUILD_STATIC_LIBRARY)

$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(PINDROP_DIR)/..)

$(call import-module,flatbuffers/android/jni)
$(call import-module,mathfu/jni)
$(call import-module,pindrop/jni/sdl_mixer)
