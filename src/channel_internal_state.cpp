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
#define _USE_MATH_DEFINES
#endif  // !defined(_USE_MATH_DEFINES)
#endif  // _WIN32

#include <algorithm>
#include <cmath>

#include "bus_internal_state.h"
#include "fplutil/intrusive_list.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

bool ChannelInternalState::IsStream() const {
  return collection_->GetSoundCollectionDef()->stream() != 0;
}

// Removes this channel state from all lists.
void ChannelInternalState::Remove() {
  free_node.remove();
  priority_node.remove();
  bus_node.remove();
}

void ChannelInternalState::SetSoundCollection(SoundCollection* collection) {
  if (collection_ && collection_->bus()) {
    bus_node.remove();
  }
  collection_ = collection;
  if (collection_ && collection_->bus()) {
    collection_->bus()->playing_sound_list().push_front(*this);
  }
}

bool ChannelInternalState::Play(SoundCollection* collection) {
  collection_ = collection;
  sound_ = collection->Select();
  channel_state_ = kChannelStatePlaying;
  return real_channel_.Valid() ? real_channel_.Play(collection_, sound_) : true;
}

bool ChannelInternalState::Playing() const {
  return channel_state_ == kChannelStatePlaying;
}

bool ChannelInternalState::Stopped() const {
  return channel_state_ == kChannelStateStopped;
}

bool ChannelInternalState::Paused() const {
  return channel_state_ == kChannelStatePaused;
}

void ChannelInternalState::Halt() {
  // If this channel loops, we may want to resume it later. If this is a one
  // shot sound that does not loop, just halt it now.
  // TODO(amablue): What we really want is for one shot sounds to change to the
  // stopped state when the sound would have finished. However, SDL mixer does
  // not give good visibility into the length of loaded audio, which makes this
  // difficult. b/20697050
  if (!collection_->GetSoundCollectionDef()->loop()) {
    channel_state_ = kChannelStateStopped;
  }
  if (real_channel_.Valid()) {
    real_channel_.Halt();
  }
  channel_state_ = kChannelStateStopped;
}

void ChannelInternalState::Pause() {
  if (real_channel_.Valid()) {
    real_channel_.Pause();
  }
  channel_state_ = kChannelStatePaused;
}

void ChannelInternalState::Resume() {
  if (real_channel_.Valid()) {
    real_channel_.Resume();
  }
  channel_state_ = kChannelStatePlaying;
}

void ChannelInternalState::FadeOut(int milliseconds) {
  if (real_channel_.Valid()) {
    real_channel_.FadeOut(milliseconds);
  }
  channel_state_ = kChannelStateFadingOut;
}

void ChannelInternalState::SetPan(const mathfu::Vector<float, 2>& pan) {
  if (real_channel_.Valid()) {
    real_channel_.SetPan(pan);
  }
}

void ChannelInternalState::Devirtualize(ChannelInternalState* other) {
  assert(!real_channel_.Valid());
  assert(other->real_channel_.Valid());

  // Transfer the real channel id to this channel.
  std::swap(real_channel_, other->real_channel_);

  if (Playing()) {
    // Resume playing the audio.
    real_channel_.Play(collection_, sound_);
  } else if (Paused()) {
    // The audio needs to be playing to pause it.
    real_channel_.Play(collection_, sound_);
    real_channel_.Pause();
  }
}

float ChannelInternalState::Priority() const {
  assert(collection_);
  return gain() * collection_->GetSoundCollectionDef()->priority();
}

void ChannelInternalState::UpdateState() {
  switch (channel_state_) {
    case kChannelStatePaused:
    case kChannelStateStopped: {
      break;
    }
    case kChannelStatePlaying:
      if (real_channel_.Valid() && !real_channel_.Playing()) {
        channel_state_ = kChannelStateStopped;
      }
      break;
    case kChannelStateFadingOut: {
      if (!real_channel_.Valid() || !real_channel_.Playing()) {
        channel_state_ = kChannelStateStopped;
      }
      break;
    }
    default: { assert(false); }
  }
}

}  // namespace pindrop
