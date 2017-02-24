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
if(fpl_ios)
  cmake_minimum_required(VERSION 3.3.1)
else()
  cmake_minimum_required(VERSION 2.8.12)
endif()

add_custom_target(pindrop_sample_assets
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_assets.py
  --flatc $<TARGET_FILE:flatc>
  --output ${CMAKE_BINARY_DIR}/bin/assets DEPENDS flatc)

set(pindrop_sample_SRCS main.cpp)

add_executable(pindrop_sample ${pindrop_sample_SRCS})
target_link_libraries(pindrop_sample
  pindrop
  ${SDL_LIBRARIES}
  ${FPLBASE_LIBRARY}
  sdl_mixer
  libvorbis
  libogg)

mathfu_configure_flags(pindrop_sample)
add_dependencies(pindrop_sample pindrop pindrop_sample_assets)
