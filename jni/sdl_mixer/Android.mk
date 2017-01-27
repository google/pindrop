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

# This forks the SDL_mixer makefile to workaround the following limitations:
#   1. Build with arm rather than thumb code.
#   2. Disable MP3 SMPEG, MOD music loading / playback plug-ins
#   3. Add a static library build target.
#   4. Import SDL module into this makefile.
#
# Lines that are patches are marked up with (X) where X corresponds to an issue
# described above.


LOCAL_PATH:=$(call my-dir)

# Project directory relative to this file.
PINDROP_DIR:=$(LOCAL_PATH)/../..
include $(PINDROP_DIR)/jni/android_config.mk

# Modify the local path to point to the SDL mixer library source.
LOCAL_PATH:=$(DEPENDENCIES_SDL_MIXER_DIR)

include $(CLEAR_VARS)

LOCAL_MODULE := SDL2_mixer

LOCAL_ARM_MODE := arm  # (1) arm code generation.

# Enable this if you want to support loading MOD music via modplug
# The library path should be a relative path to this directory.
SUPPORT_MOD_MODPLUG := false  # (2) Disable modplug
MODPLUG_LIBRARY_PATH := external/libmodplug-0.8.8.4

# Enable this if you want to support loading MOD music via mikmod
# The library path should be a relative path to this directory.
SUPPORT_MOD_MIKMOD := false  # (2) Disable mikmod.
MIKMOD_LIBRARY_PATH := external/libmikmod-3.1.12

# Enable this if you want to support loading MP3 music via SMPEG
# The library path should be a relative path to this directory.
SUPPORT_MP3_SMPEG := false   # (2) Disable SMPEG.
SMPEG_LIBRARY_PATH := external/smpeg2-2.0.0

# Enable this if you want to support loading OGG Vorbis music via Tremor
# The library path should be a relative path to this directory.
SUPPORT_OGG := true
OGG_LIBRARY_PATH := $(DEPENDENCIES_LIBOGG_REL_SDL_MIXER)
TREMOR_LIBRARY_PATH := $(DEPENDENCIES_TREMOR_REL_SDL_MIXER)


LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := -DWAV_MUSIC

LOCAL_SRC_FILES := $(notdir $(filter-out %/playmus.c %/playwave.c, $(wildcard $(LOCAL_PATH)/*.c)))

LOCAL_LDLIBS :=
LOCAL_SHARED_LIBRARIES := SDL2

ifeq ($(SUPPORT_MOD_MODPLUG),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(MODPLUG_LIBRARY_PATH)/src $(LOCAL_PATH)/$(MODPLUG_LIBRARY_PATH)/src/libmodplug
    LOCAL_CFLAGS += -DMODPLUG_MUSIC -DHAVE_SETENV -DHAVE_SINF
    LOCAL_SRC_FILES += \
        $(MODPLUG_LIBRARY_PATH)/src/fastmix.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_669.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_abc.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_amf.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_ams.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_dbm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_dmf.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_dsm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_far.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_it.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_j2b.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_mdl.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_med.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_mid.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_mod.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_mt2.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_mtm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_okt.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_pat.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_psm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_ptm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_s3m.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_stm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_ult.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_umx.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_wav.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/load_xm.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/mmcmp.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/modplug.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/snd_dsp.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/snd_flt.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/snd_fx.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/sndfile.cpp \
        $(MODPLUG_LIBRARY_PATH)/src/sndmix.cpp
endif

ifeq ($(SUPPORT_MOD_MIKMOD),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(MIKMOD_LIBRARY_PATH)/include
    LOCAL_CFLAGS += -DMOD_MUSIC
    LOCAL_SHARED_LIBRARIES += mikmod
endif

ifeq ($(SUPPORT_MP3_SMPEG),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SMPEG_LIBRARY_PATH)
    LOCAL_CFLAGS += -DMP3_MUSIC
    LOCAL_SHARED_LIBRARIES += smpeg2
endif

ifeq ($(SUPPORT_OGG),true)
    LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/$(OGG_LIBRARY_PATH)/include \
        $(LOCAL_PATH)/$(TREMOR_LIBRARY_PATH) \
        $(FPL_ABSOLUTE_INCLUDE_DIR)
    # Some versions of Tremor are missing #include <endian.h> in misc.h.
    # This generates "union magic already defined" errors because BYTE_ORDER
    # is undefined. We force include it here.
    LOCAL_CFLAGS += -DOGG_MUSIC -DOGG_USE_TREMOR -include "endian.h"
    LOCAL_SRC_FILES += \
        $(TREMOR_LIBRARY_PATH)/mdct.c \
        $(TREMOR_LIBRARY_PATH)/block.c \
        $(TREMOR_LIBRARY_PATH)/window.c \
        $(TREMOR_LIBRARY_PATH)/synthesis.c \
        $(TREMOR_LIBRARY_PATH)/info.c \
        $(TREMOR_LIBRARY_PATH)/floor1.c \
        $(TREMOR_LIBRARY_PATH)/floor0.c \
        $(TREMOR_LIBRARY_PATH)/vorbisfile.c \
        $(TREMOR_LIBRARY_PATH)/res012.c \
        $(TREMOR_LIBRARY_PATH)/mapping0.c \
        $(TREMOR_LIBRARY_PATH)/registry.c \
        $(TREMOR_LIBRARY_PATH)/codebook.c \
        $(TREMOR_LIBRARY_PATH)/sharedbook.c \
        $(OGG_LIBRARY_PATH)/src/framing.c \
        $(OGG_LIBRARY_PATH)/src/bitwise.c
endif

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

# (3) Save all variables required to build the shared library.
SDL_MIXER_C_INCLUDES := $(LOCAL_C_INCLUDES)
SDL_MIXER_CFLAGS := $(LOCAL_CFLAGS)
SDL_MIXER_SRC_FILES := $(LOCAL_SRC_FILES)
SDL_MIXER_LDLIBS := $(LOCAL_LDLIBS)
SDL_MIXER_EXPORT_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDES)
# (3) Save all variables required to build the shared library: end.

include $(BUILD_SHARED_LIBRARY)

# (3) Build a static library.
include $(CLEAR_VARS)
LOCAL_MODULE := SDL2_mixer_static
LOCAL_MODULE_FILENAME := libSDL2_mixer
LOCAL_ARM_MODE := arm  # (1) arm code generation.
LOCAL_C_INCLUDES := $(SDL_MIXER_C_INCLUDES)
LOCAL_CFLAGS := $(SDL_MIXER_CFLAGS)
LOCAL_SRC_FILES := $(SDL_MIXER_SRC_FILES)
LOCAL_EXPORT_LDLIBS := $(SDL_MIXER_LDLIBS)
LOCAL_EXPORT_C_INCLUDES := $(SDL_MIXER_EXPORT_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := SDL2_static
include $(BUILD_STATIC_LIBRARY)
# (3) Build a static library: end.

# (4) Import the SDL module to satisfy the dependency.
# NOTE: $(DEPENDENCIES_SDL_DIR) is not included here as we have patched the
# makefile in fplbase/jni/sdl/Android.mk
$(call import-add-path,$(DEPENDENCIES_FPLBASE_DIR))
# NOTE: We're including $(FPLBASE_DIR)/jni/sdl/Android.mk here.
$(call import-module,fplbase/jni/sdl)
# (4) Import the SDL module to satisfy the dependency: end.
