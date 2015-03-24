// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "backend.h"

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "audio_config_generated.h"
#include "sound.h"

namespace pindrop {

Backend::Backend() : initialized_(false) {}

Backend::~Backend() {
  if (initialized_) {
    Mix_CloseAudio();
  }
}

bool Backend::Initialize(const AudioConfig* config) {
  if (initialized_) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "SDL_Mixer has already been initialized.\n");
    return false;
  }

#ifdef PINDROP_MULTISTREAM
  Mix_HookMusicFinishedCh(nullptr, FreeFinishedMusicMultistream);
#else
  Mix_HookMusicFinished(FreeFinishedMusic);
#endif  // PINDROP_MULTISTREAM

  if (Mix_OpenAudio(config->output_frequency(), AUDIO_S16LSB,
                    config->output_channels(),
                    config->output_buffer_size()) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not open audio stream\n");
    return false;
  }
  initialized_ = true;

  // Initialize the channels.
  Mix_AllocateChannels(config->mixer_channels());

  // Initialize Ogg support. Returns a bitmask of formats that were successfully
  // initialized, so make sure ogg support was successfully loaded.
  // Even without Ogg support we can still play .wav files, so don't return
  // false.
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing Ogg support\n");
  }

  return true;
}

}  // namespace pindrop

