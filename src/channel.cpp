// Copyright 2014 Google Inc. All rights reserved.
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

#include "pindrop/audio_engine.h"

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "channel_internal_state.h"

namespace pindrop {

const Channel kInvalidChannel(nullptr);

const int kChannelFadeOutRateMs = 10;

bool Channel::Playing() const {
  assert(valid());
  if (state_->channel_id() == kStreamChannelId) {
    return Mix_PlayingMusic() != 0;
  } else {
    return Mix_Playing(state_->channel_id()) != 0;
  }
}

void Channel::Stop() {
  assert(valid());
  // Fade out rather than halting to avoid clicks.
  if (state_->channel_id() == kStreamChannelId) {
    // SDL_Mixer will not fade out channels with a volume of 0.
    // Manually halt channels in this case.
    int volume = Mix_VolumeMusic(-1);
    if (volume == 0) {
      Mix_HaltMusic();
    } else {
      int result = Mix_FadeOutMusic(kChannelFadeOutRateMs);
      if (result == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error stopping music: %s\n",
                     Mix_GetError());
      }
    }
  } else {
    // SDL_Mixer will not fade out channels with a volume of 0.
    // Manually halt channels in this case.
    int volume = Mix_Volume(state_->channel_id(), -1);
    if (volume == 0) {
      Mix_HaltChannel(state_->channel_id());
    } else {
      int result =
          Mix_FadeOutChannel(state_->channel_id(), kChannelFadeOutRateMs);
      if (result == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error stopping channel %d: %s\n",
                     state_->channel_id(), Mix_GetError());
      }
    }
  }
}

const mathfu::Vector<float, 3> Channel::location() const {
  assert(valid());
  return state_->location();
}

void Channel::set_location(const mathfu::Vector<float, 3>& location) {
  assert(valid());
  state_->set_location(location);
}

}  // namespace pindrop

