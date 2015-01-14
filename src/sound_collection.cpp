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

#include "SDL_log.h"
#include "audio_engine_internal_state.h"
#include "flatbuffers/util.h"
#include "pindrop/audio_engine.h"
#include "sound.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

bool SoundCollection::LoadSoundCollectionDef(const std::string& source,
                                             AudioEngineInternalState* state) {
  source_ = source;
  const SoundCollectionDef* def = GetSoundCollectionDef();
  unsigned int sample_count =
      def->audio_sample_set() ? def->audio_sample_set()->Length() : 0;
  sound_sources_.resize(sample_count);
  for (unsigned int i = 0; i < sample_count; ++i) {
    const AudioSampleSetEntry* entry = def->audio_sample_set()->Get(i);
    const char* entry_filename = entry->audio_sample()->filename()->c_str();
    auto& sound_source = sound_sources_[i];
    if (def->stream()) {
      sound_source.reset(new SoundStream(entry));
    } else {
      sound_source.reset(new SoundBuffer(entry));
    }
    if (!sound_source->LoadFile(entry_filename)) {
      return false;
    }
    sum_of_probabilities_ += entry->playback_probability();
  }
  if (!def->bus()) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Sound collection %s does not specify a bus", def->name());
    return false;
  }
  if (state) {
    bus_ = FindBus(state, def->bus()->c_str());
    if (!bus_) {
      return false;
    }
  }
  return true;
}

bool SoundCollection::LoadSoundCollectionDefFromFile(
    const std::string& filename, AudioEngineInternalState* state) {
  std::string source;
  return flatbuffers::LoadFile(filename.c_str(), false, &source) &&
      LoadSoundCollectionDef(source, state);
}

const SoundCollectionDef* SoundCollection::GetSoundCollectionDef() const {
  assert(source_.size());
  return pindrop::GetSoundCollectionDef(source_.c_str());
}

SoundSource* SoundCollection::Select() const {
  const SoundCollectionDef* sound_def = GetSoundCollectionDef();
  // Choose a random number between 0 and the sum of the probabilities, then
  // iterate over the list, subtracting the weight of each entry until 0 is
  // reached.
  float selection =
      rand() / static_cast<float>(RAND_MAX) * sum_of_probabilities_;
  for (unsigned int i = 0; i < sound_sources_.size(); ++i) {
    const AudioSampleSetEntry* entry = sound_def->audio_sample_set()->Get(i);
    selection -= entry->playback_probability();
    if (selection <= 0) {
      return sound_sources_[i].get();
    }
  }
  // If we've reached here and didn't return a sound, assume there was some
  // floating point rounding error and just return the last one.
  return sound_sources_.back().get();
}

bool SoundCollectionDefComparator(const SoundCollectionDef& a,
                                  const SoundCollectionDef& b) {
  // Since there can only be one stream playing, and that stream is independent
  // of the buffer channels, always make it highest priority.
  if (a.stream() != b.stream()) {
    return a.stream();
  } else {
    return a.priority() > b.priority();
  }
}

}  // namespace pindrop

