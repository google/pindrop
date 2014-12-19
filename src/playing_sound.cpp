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

#include "audio_engine/audio_engine.h"
#include "bus.h"
#include "sound_collection.h"

namespace fpl {

PlayingSound::PlayingSound(AudioEngine::SoundHandle sound_handle,
                           AudioEngine::ChannelId cid,
                           WorldTime time)
    : handle(sound_handle),
      channel_id(cid),
      start_time(time) {
  Bus* bus = handle->bus();
  if (bus) {
    bus->IncrementSoundCounter();
  }
}

PlayingSound::PlayingSound(const PlayingSound& other)
    : handle(other.handle),
      channel_id(other.channel_id),
      start_time(other.start_time) {
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
  start_time = other.start_time;
  return *this;
}

}  // namespace fpl

