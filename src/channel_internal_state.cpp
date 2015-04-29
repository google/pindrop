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

#ifdef _WIN32
#if !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES  // For M_PI.
#endif                     // !defined(_USE_MATH_DEFINES)
#endif                     // _WIN32

#include <math.h>

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "bus.h"
#include "intrusive_list.h"
#include "pindrop/pindrop.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

bool ChannelInternalState::IsStream() const {
  return handle_->GetSoundCollectionDef()->stream() != 0;
}

// Removes this channel state from all lists.
void ChannelInternalState::Remove() {
  free_node_.Remove();
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

bool ChannelInternalState::Play(SoundHandle handle) {
  handle_ = handle;
  sound_source_ = handle->Select();
  channel_state_ = kChannelStatePlaying;
  return is_real() ? Resume() : true;
}

static bool SoundHandleLoops(SoundHandle handle) {
  SoundCollection* collection = handle;
  const SoundCollectionDef* def = collection->GetSoundCollectionDef();
  return def->loop() != 0;
}

bool ChannelInternalState::Resume() {
  assert(is_real() && channel_state_ == kChannelStatePlaying);
  if (!sound_source_->Play(channel_id_, SoundHandleLoops(handle_))) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not play sound %s\n",
                 Mix_GetError());
    return false;
  }
  return true;
}

bool ChannelInternalState::RealChannelPlaying() const {
  assert(is_real());
  if (IsStream()) {
#ifdef PINDROP_MULTISTREAM
    return Mix_PlayingMusicCh(channel_id_) != 0;
#else
    return Mix_PlayingMusic() != 0;
#endif  // PINDROP_MULTISTREAM
  } else {
    return Mix_Playing(channel_id_) != 0;
  }
}

bool ChannelInternalState::Playing() const {
  return channel_state_ == kChannelStatePlaying;
}

void ChannelInternalState::SetRealChannelGain(const float gain) {
  assert(is_real());
  int mix_volume = static_cast<int>(gain * MIX_MAX_VOLUME);
  if (IsStream()) {
#ifdef PINDROP_MULTISTREAM
    Mix_VolumeMusicCh(channel_id_, mix_volume);
#else
    Mix_VolumeMusic(mix_volume);
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_Volume(channel_id_, mix_volume);
  }
}

float ChannelInternalState::RealChannelGain() const {
  assert(is_real());
  // Special value to query volume rather than set volume.
  static const int kQueryVolume = -1;
  int volume;
  if (IsStream()) {
#ifdef PINDROP_MULTISTREAM
    volume = Mix_VolumeMusicCh(channel_id_, kQueryVolume);
#else
    volume = Mix_VolumeMusic(kQueryVolume);
#endif  // PINDROP_MULTISTREAM
  } else {
    volume = Mix_Volume(channel_id_, kQueryVolume);
  }
  return volume / static_cast<float>(MIX_MAX_VOLUME);
}

void ChannelInternalState::RealChannelHalt() {
  assert(is_real());
  // If this channel loops, we may want to resume it later. If this is a one
  // shot sound that does not loop, just halt it now.
  // TODO(amablue): What we really want is for one shot sounds to change to the
  // stopped state when the sound would have finished. However, SDL mixer does
  // not give good visibility into the length of loaded audio, which makes this
  // difficult. b/20697050
  if (!SoundHandleLoops(handle_)) {
    channel_state_ = kChannelStateStopped;
  }
  if (IsStream()) {
#ifdef PINDROP_MULTISTREAM
    Mix_HaltMusicCh(channel_id_);
#else
    Mix_HaltMusic();
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_HaltChannel(channel_id_);
  }
}

void ChannelInternalState::Halt() {
  if (is_real()) {
    RealChannelHalt();
  }
  channel_state_ = kChannelStateStopped;
}

void ChannelInternalState::FadeOut(int milliseconds) {
  int channels_halted;
  if (IsStream()) {
#ifdef PINDROP_MULTISTREAM
    channels_halted = Mix_FadeOutMusicCh(channel_id_, milliseconds);
#else
    channels_halted = Mix_FadeOutMusic(milliseconds);
#endif  // PINDROP_MULTISTREAM
  } else {
    channels_halted = Mix_FadeOutChannel(channel_id_, milliseconds);
  }
  if (channels_halted == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error halting channel %i\n");
  } else {
    channel_state_ = kChannelStateFadingOut;
  }
}

void ChannelInternalState::SetPan(const mathfu::Vector<float, 2>& pan) {
  if (!is_real()) {
    return;
  }
  static const Uint8 kMaxPanValue = 255;

  // This formula is explained in the following paper:
  // http://www.rs-met.com/documents/tutorials/PanRules.pdf
  float p = static_cast<float>(M_PI) * (pan.x() + 1.0f) / 4.0f;
  Uint8 left = static_cast<Uint8>(cos(p) * kMaxPanValue);
  Uint8 right = static_cast<Uint8>(sin(p) * kMaxPanValue);
  Mix_SetPanning(channel_id_, left, right);
}

float ChannelInternalState::Priority() const {
  assert(handle_);
  return gain() * handle_->GetSoundCollectionDef()->priority();
}

void ChannelInternalState::UpdateState() {
  switch (channel_state_) {
    case kChannelStateStopped: {
      break;
    }
    case kChannelStatePlaying:
    case kChannelStateFadingOut: {
      if (is_real() && !RealChannelPlaying()) {
        channel_state_ = kChannelStateStopped;
      }
      break;
    }
    default: {
      assert(false);
    }
  }
}

// Special value for SDL_Mixer that indicates an operation should be applied
// to all channels.
static const ChannelId kAllChannels = -1;

void ChannelInternalState::PauseAll() {
  Mix_Pause(kAllChannels);
#ifdef PINDROP_MULTISTREAM
  Mix_PauseMusicCh(kAllChannels);
#else
  Mix_PauseMusic();
#endif  // PINDROP_MULTISTREAM
}

void ChannelInternalState::ResumeAll() {
  Mix_Resume(kAllChannels);
#ifdef PINDROP_MULTISTREAM
  Mix_ResumeMusicCh(kAllChannels);
#else
  Mix_ResumeMusic();
#endif  // PINDROP_MULTISTREAM
}

}  // namespace pindrop

