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

#include "pindrop/log.h"
#include "file_loader.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

void Sound::Initialize(const SoundCollection* sound_collection) {
  stream_ = sound_collection->GetSoundCollectionDef()->stream();
}

void Sound::Load() {
  if (!stream_) {
    chunk_ = Mix_LoadWAV(filename().c_str());
    if (chunk_ == nullptr) {
      CallLogFunc("Could not load sound file: %s.", filename().c_str());
    }
  }
}

}  // namespace pindrop
