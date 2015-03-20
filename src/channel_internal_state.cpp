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

#include "channel_internal_state.h"

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "bus.h"
#include "intrusive_list.h"
#include "pindrop/audio_engine.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

const int kChannelFadeOutDurationMs = 10;

void ChannelInternalState::Clear() {
  handle_ = nullptr;
  priority_node_.Remove();
  bus_node_.Remove();
}

void ChannelInternalState::SetHandle(SoundHandle handle) {
  if (handle_ && handle_->bus()) {
    bus_node_.Remove();
  }
  handle_ = handle;
  if (handle_ && handle_->bus()) {
    handle_->bus()->playing_sound_list().InsertAfter(&bus_node_);
  }
}

bool ChannelInternalState::Play(SoundSource* source, bool loop) {
  if (!source->Play(channel_id_, loop)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not play sound %s\n",
                 Mix_GetError());
    return false;
  }
  return true;
}

bool ChannelInternalState::Playing() const {
  if (channel_id_ == kStreamChannelId) {
    return Mix_PlayingMusic() != 0;
  } else {
    return Mix_Playing(channel_id_) != 0;
  }
}

void ChannelInternalState::SetGain(const float gain) {
  int mix_volume = static_cast<int>(gain * MIX_MAX_VOLUME);
  if (channel_id_ == kStreamChannelId) {
    Mix_VolumeMusic(mix_volume);
  } else {
    Mix_Volume(channel_id_, mix_volume);
  }
}

float ChannelInternalState::Gain() const {
  // Special value to query volume rather than set volume.
  static const int kQueryVolume = -1;
  int volume;
  if (channel_id_ == kStreamChannelId) {
    volume = Mix_VolumeMusic(kQueryVolume);
  } else {
    volume = Mix_Volume(channel_id_, kQueryVolume);
  }
  return volume / static_cast<float>(MIX_MAX_VOLUME);
}

void ChannelInternalState::Halt() {
  if (channel_id_ == kStreamChannelId) {
    Mix_HaltMusic();
  } else {
    Mix_HaltChannel(channel_id_);
  }
}

void ChannelInternalState::FadeOut(int milliseconds) {
  int channels_halted;
  if (channel_id_ == kStreamChannelId) {
    channels_halted = Mix_FadeOutMusic(milliseconds);
  } else {
    channels_halted = Mix_FadeOutChannel(channel_id_, milliseconds);
  }
  if (channels_halted == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error halting channel %i\n");
  }
}

float ChannelInternalState::Priority() const {
  assert(handle_);
  return Gain() * handle_->GetSoundCollectionDef()->priority();
}

}  // namespace pindrop

