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

#include "pindrop/pindrop.h"

#include <vector>
#include <map>

#include "backend.h"
#include "bus.h"
#include "channel_internal_state.h"
#include "mathfu/utilities.h"
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

typedef std::vector<ChannelInternalState> ChannelStateVector;

typedef std::vector<ListenerInternalState,
                    mathfu::simd_allocator<ListenerInternalState>>
    ListenerStateVector;

struct AudioEngineInternalState {
  Backend backend;

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

  // The preallocated pool of all ChannelInternalState objects
  ChannelStateVector channel_state_memory;

  // The lists that track currently playing channels and free channels.
  IntrusiveListNode playing_channel_list;
  IntrusiveListNode real_channel_free_list;
  IntrusiveListNode virtual_channel_free_list;

#ifndef PINDROP_MULTISTREAM
  // In single stream mode, track the currently playing stream.
  ChannelInternalState* stream_channel;
#endif  // PINDROP_MULTISTREAM

  // The list of listeners.
  TypedIntrusiveListNode<ListenerInternalState> listener_list;
  ListenerStateVector listener_state_memory;
  std::vector<ListenerInternalState*> listener_state_free_list;

  // The current frame, i.e. the number of times AdvanceFrame has been called.
  unsigned int current_frame;

  const char* version_string;
};

// Find a bus with the given name.
Bus* FindBus(AudioEngineInternalState* state, const char* name);

// Given a playing sound, find where a new sound with the given priority should
// be inserted into the list.
IntrusiveListNode* FindInsertionPoint(IntrusiveListNode* list, float priority);

// Given a list of listeners and a location, find which listener is closest.
// Additionally, return the square of the distance between the closest listener
// and the location, as well as the given location translated into listener
// space.  Returns true on success, or false if the list was empty.
bool BestListener(
    ListenerInternalState** best_listener, float* distance_squared,
    mathfu::Vector<float, 3>* listener_space_location,
    const TypedIntrusiveListNode<ListenerInternalState>& listeners,
    const mathfu::Vector<float, 3>& location);

// This will take a point between lower_bound and upper_bound and use it to
// calculate an attenuation multiplier. The curve_factor can be adjusted based
// on how rapidly the attenuation should change with distance.
//
// A curve_factor of 1.0 means the attenuation will adjust linearly with
// distance.
//
// A curve_factor greater than 1.0 means the attenuation will change gently at
// first, then rapidly approach its target.
//
// A fractional curve_factor between 0.0 and 1.0 means the attenuation will
// change rapidly at first, then gently approach its target.
float AttenuationCurve(float point, float lower_bound, float upper_bound,
                       float curve_factor);

// Determine whether the sound can be heard at all. If it can, apply the roll
// in gain, roll out gain, or norminal gain appropriately.
float CalculateDistanceAttenuation(float distance_squared,
                                   const SoundCollectionDef* def);

// Given a vector in listener space, return a vector inside a unit circle
// representing the direction from the listener to the sound. A value of (-1, 0)
// means the sound is directly to the listener's left, while a value of (1, 0)
// means the sound is directly to the listener's right. Likewise, values of
// (0, 1) and (0, -1) mean the sound is directly in front or behind the
// listener, respectively.
mathfu::Vector<float, 2> CalculatePan(
    const mathfu::Vector<float, 3>& listener_space_location);

bool LoadFile(const char* filename, std::string* dest);

}  // namespace pindrop

#endif  // PINDROP_AUDIO_ENGINE_INTERNAL_STATE_H_

