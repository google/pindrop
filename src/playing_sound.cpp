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

#include "playing_sound.h"

#include "bus.h"
#include "pindrop/audio_engine.h"
#include "sound_collection.h"

namespace pindrop {

PlayingSound::PlayingSound(AudioEngine::SoundHandle sound_handle,
                           AudioEngine::ChannelId cid, unsigned int frame)
    : handle(sound_handle), channel_id(cid), frame_created(frame) {
  Bus* bus = handle->bus();
  if (bus) {
    bus->IncrementSoundCounter();
  }
}

PlayingSound::PlayingSound(const PlayingSound& other)
    : handle(other.handle),
      channel_id(other.channel_id),
      frame_created(other.frame_created) {
  Bus* bus = handle->bus();
  if (bus) {
    bus->IncrementSoundCounter();
  }
}

PlayingSound::~PlayingSound() {
  Bus* bus = handle->bus();
  if (bus) {
    bus->DecrementSoundCounter();
  }
}

PlayingSound& PlayingSound::operator=(const PlayingSound& other) {
  Bus* bus = handle->bus();
  if (bus) {
    bus->DecrementSoundCounter();
  }
  Bus* other_bus = other.handle->bus();
  if (other_bus) {
    other_bus->IncrementSoundCounter();
  }
  handle = other.handle;
  channel_id = other.channel_id;
  frame_created = other.frame_created;
  return *this;
}

bool PlayingSoundComparator(const PlayingSound& a, const PlayingSound& b) {
  const auto& a_def = *a.handle->GetSoundCollectionDef();
  const auto& b_def = *b.handle->GetSoundCollectionDef();
  // Check if `a_def` is higher priority.
  if (SoundCollectionDefComparator(a_def, b_def)) {
    return true;
    // `a_def` is not higher priority, it could be equal or lower.
  } else if (SoundCollectionDefComparator(b_def, a_def)) {
    return false;
    // `a_def` and `b_def` are equal prioity, compare frames created.
  } else {
    return a.frame_created < b.frame_created;
  }
}

}  // namespace pindrop
