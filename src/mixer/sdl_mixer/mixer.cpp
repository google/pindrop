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

#include "mixer.h"

#include "SDL_mixer.h"
#include "audio_config_generated.h"
#include "pindrop/log.h"
#include "real_channel.h"

namespace pindrop {

Mixer::Mixer() : initialized_(false) {}

Mixer::~Mixer() {
  if (initialized_) {
    Mix_CloseAudio();
  }
}

bool Mixer::Initialize(const AudioConfig* config) {
  if (initialized_) {
    CallLogFunc("SDL_Mixer has already been initialized.\n");
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
    CallLogFunc("Could not open audio stream\n");
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
    CallLogFunc("Error initializing Ogg support\n");
  }

  return true;
}

}  // namespace pindrop
