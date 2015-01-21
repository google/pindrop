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

#ifndef PINDROP_AUDIO_ENGINE_INTERNAL_STATE_H_
#define PINDROP_AUDIO_ENGINE_INTERNAL_STATE_H_

#include <vector>
#include <map>

#include "SDL_log.h"
#include "pindrop/audio_engine.h"
#include "bus.h"
#include "playing_sound.h"
#include "sound.h"
#include "sound_bank.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

struct AudioConfig;
struct BusDefList;
struct SoundBankDef;

typedef SoundCollection* SoundHandle;

typedef std::map<std::string, std::unique_ptr<SoundCollection>>
    SoundCollectionMap;

typedef std::map<std::string, std::string> SoundIdMap;

typedef std::map<std::string, std::unique_ptr<SoundBank>> SoundBankMap;

typedef flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>
    BusNameList;

struct AudioEngineInternalState {
  // Hold the audio bus list.
  std::string buses_source;

  // The state of the buses.
  std::vector<Bus> buses;

  // The master bus, cached to prevent needless lookups.
  Bus* master_bus;

  // The gain applied to all buses.
  float master_gain;

  // If true, the master gain is ignored and all channels have a gain of 0.
  bool mute;

  // A map of sound names to SoundCollections.
  SoundCollectionMap sound_collection_map;

  // A map of file names to sound ids to determine if a file needs to be loaded.
  SoundIdMap sound_id_map;

  // Hold the sounds banks.
  SoundBankMap sound_bank_map;

  // A list of the currently playing sounds.
  TypedIntrusiveListNode<PlayingSound> playing_sounds_list;
  std::vector<PlayingSound> playing_sounds;
  std::vector<PlayingSound*> playing_sounds_free_list;

  // The current frame, i.e. the number of times AdvanceFrame has been called.
  unsigned int current_frame;
};

// Find a bus with the given name.
Bus* FindBus(AudioEngineInternalState* state, const char* name);

// Given a playing sound, find where a new sound with the given priority should
// be inserted into the list.
PlayingSound* FindInsertionPoint(TypedIntrusiveListNode<PlayingSound>* list,
                                 float priority);

}  // namespace pindrop

#endif  // PINDROP_AUDIO_ENGINE_INTERNAL_STATE_H_
