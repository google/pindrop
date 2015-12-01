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

#ifndef PINDROP_SOUND_COLLECTION_H_
#define PINDROP_SOUND_COLLECTION_H_

#include <memory>
#include <string>
#include <vector>

#include "real_channel.h"
#include "ref_counter.h"
#include "sound.h"

namespace pindrop {

class BusInternalState;
struct AudioEngineInternalState;
struct SoundCollectionDef;

// SoundCollection represent an abstract sound (like a 'whoosh'), which contains
// a number of pieces of audio with weighted probabilities to choose between
// randomly when played. It holds objects of type `Audio`, which can be either
// Sounds or Music
class SoundCollection {
 public:
  SoundCollection()
      : bus_(nullptr),
        source_(),
        sounds_(),
        sum_of_probabilities_(0.0f),
        ref_counter_() {}

  // Load the given flatbuffer data representing a SoundCollectionDef.
  bool LoadSoundCollectionDef(const std::string& source,
                              AudioEngineInternalState* state);

  // Load the given flatbuffer binary file containing a SoundDef.
  bool LoadSoundCollectionDefFromFile(const std::string& filename,
                                      AudioEngineInternalState* state);

  // Return the SoundDef.
  const SoundCollectionDef* GetSoundCollectionDef() const;

  // Return a random piece of audio from the set of audio for this sound.
  Sound* Select();

  // Return the bus this SoundCollection will play on.
  BusInternalState* bus() { return bus_; }

  RefCounter* ref_counter() { return &ref_counter_; }

 private:
  // The bus this SoundCollection will play on.
  BusInternalState* bus_;

  std::string source_;
  std::vector<Sound> sounds_;
  float sum_of_probabilities_;

  RefCounter ref_counter_;
};

}  // namespace pindrop

#endif  // PINDROP_SOUND_COLLECTION_H_
