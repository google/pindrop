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

#include "sound_collection.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "audio_engine_internal_state.h"
#include "file_loader.h"
#include "pindrop/log.h"
#include "sound.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

bool SoundCollection::LoadSoundCollectionDef(const std::string& source,
                                             AudioEngineInternalState* state) {
  source_ = source;
  const SoundCollectionDef* def = GetSoundCollectionDef();
  flatbuffers::uoffset_t sample_count =
      def->audio_sample_set() ? def->audio_sample_set()->Length() : 0;
  sounds_.resize(sample_count);
  for (flatbuffers::uoffset_t i = 0; i < sample_count; ++i) {
    const AudioSampleSetEntry* entry = def->audio_sample_set()->Get(i);
    const char* entry_filename = entry->audio_sample()->filename()->c_str();
    sum_of_probabilities_ += entry->playback_probability();

    Sound& sound = sounds_[i];
    sound.Initialize(this);
    sound.LoadFile(entry_filename, &state->loader);
  }
  if (!def->bus()) {
    CallLogFunc("Sound collection %s does not specify a bus", def->name());
    return false;
  }
  if (state) {
    bus_ = FindBusInternalState(state, def->bus()->c_str());
    if (!bus_) {
      CallLogFunc("Sound collection %s specifies an unknown bus: %s",
                  def->name(), def->bus()->c_str());
      return false;
    }
  }
  return true;
}

bool SoundCollection::LoadSoundCollectionDefFromFile(
    const std::string& filename, AudioEngineInternalState* state) {
  std::string source;
  return LoadFile(filename.c_str(), &source) &&
         LoadSoundCollectionDef(source, state);
}

const SoundCollectionDef* SoundCollection::GetSoundCollectionDef() const {
  assert(source_.size());
  return pindrop::GetSoundCollectionDef(source_.c_str());
}

Sound* SoundCollection::Select() {
  const SoundCollectionDef* sound_def = GetSoundCollectionDef();
  // Choose a random number between 0 and the sum of the probabilities, then
  // iterate over the list, subtracting the weight of each entry until 0 is
  // reached.
  float selection =
      rand() / static_cast<float>(RAND_MAX) * sum_of_probabilities_;
  for (size_t i = 0; i < sounds_.size(); ++i) {
    const AudioSampleSetEntry* entry = sound_def->audio_sample_set()->Get(
        static_cast<flatbuffers::uoffset_t>(i));
    selection -= entry->playback_probability();
    if (selection <= 0) {
      return &sounds_[i];
    }
  }
  // If we've reached here and didn't return a sound, assume there was some
  // floating point rounding error and just return the last one.
  return &sounds_.back();
}

}  // namespace pindrop
