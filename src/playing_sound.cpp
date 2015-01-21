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

void PlayingSound::Clear() {
  if (handle_ && handle_->bus()) {
    handle_->bus()->DecrementSoundCounter();
    handle_ = nullptr;
  }
  Remove();
}

void PlayingSound::SetHandle(AudioEngine::SoundHandle handle) {
  if (handle_ && handle_->bus()) {
    handle_->bus()->DecrementSoundCounter();
  }
  handle_ = handle;
  if (handle_ && handle_->bus()) {
    handle_->bus()->IncrementSoundCounter();
  }
}

}  // namespace pindrop
