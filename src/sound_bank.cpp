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

#include "sound_bank.h"

#include "SDL_log.h"
#include "audio_engine.h"
#include "flatbuffers/util.h"
#include "sound_bank_def_generated.h"

namespace fpl {

static bool InitializeSoundCollection(const std::string& filename,
                                      AudioEngine* audio_engine) {
  // Find the ID.
  AudioEngine::SoundHandle handle =
      audio_engine->GetSoundHandleFromFile(filename);
  if (handle) {
    // We've seen this ID before, update it.
    handle->ref_counter()->Increment();
  } else {
    // This is a new sound collection, load it and update it.
    std::unique_ptr<SoundCollection> collection(new SoundCollection());
    if (!collection->LoadSoundCollectionDefFromFile(filename, audio_engine)) {
      return false;
    }
    collection->ref_counter()->Increment();

    AudioEngine::SoundCollectionMap& collection_map =
        *audio_engine->sound_collection_map();
    std::string name = collection->GetSoundCollectionDef()->name()->c_str();
    collection_map[name] = std::move(collection);
  }
  return true;
}

bool SoundBank::Initialize(const std::string& filename,
                           AudioEngine* audio_engine) {
  bool success = true;
  if (!flatbuffers::LoadFile(filename.c_str(), false,
                             &sound_bank_def_source_)) {
    return false;
  }
  sound_bank_def_ = GetSoundBankDef(sound_bank_def_source_.c_str());

  // Load each SoundCollection named in the sound bank.
  for (size_t i = 0; i < sound_bank_def_->filenames()->size(); ++i) {
    const char* sound_filename = sound_bank_def_->filenames()->Get(i)->c_str();
    success &= InitializeSoundCollection(sound_filename, audio_engine);
  }
  return success;
}

static bool DeinitializeSoundCollection(
    const char* filename, AudioEngine* audio_engine) {
  auto id_iter = audio_engine->sound_id_map()->find(filename);
  if (id_iter == audio_engine->sound_id_map()->end()) {
    return false;
  }

  std::string id = id_iter->second;
  auto collection_iter = audio_engine->sound_collection_map()->find(id);
  if (collection_iter == audio_engine->sound_collection_map()->end()) {
    return false;
  }

  if (collection_iter->second->ref_counter()->Decrement() == 0) {
    audio_engine->sound_collection_map()->erase(collection_iter);
  }
  return true;
}

void SoundBank::Deinitialize(AudioEngine* audio_engine) {
  for (size_t i = 0; i < sound_bank_def_->filenames()->size(); ++i) {
    const char* filename = sound_bank_def_->filenames()->Get(i)->c_str();
    if (!DeinitializeSoundCollection(filename, audio_engine)) {
      SDL_LogError(
          SDL_LOG_CATEGORY_ERROR,
          "Error while deinitializing SoundCollection %s in SoundBank.\n",
          filename);
      assert(0);
    }
  }
}

}  // namespace fpl

