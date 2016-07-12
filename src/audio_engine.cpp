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

#include "pindrop/audio_engine.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "SDL.h"
#include "audio_config_generated.h"
#include "audio_engine_internal_state.h"
#include "bus_internal_state.h"
#include "buses_generated.h"
#include "channel_internal_state.h"
#include "file_loader.h"
#include "listener_internal_state.h"
#include "mathfu/constants.h"
#include "pindrop/log.h"
#include "pindrop/version.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

typedef flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>
    BusNameList;

bool LoadFile(const char* filename, std::string* dest) {
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) {
    CallLogFunc("LoadFile fail on %s", filename);
    return false;
  }
  size_t len = static_cast<size_t>(SDL_RWsize(handle));
  dest->assign(len + 1, 0);
  size_t rlen = SDL_RWread(handle, &(*dest)[0], 1, len);
  SDL_RWclose(handle);
  return len == rlen && len > 0;
}

AudioEngine::~AudioEngine() { delete state_; }

BusInternalState* FindBusInternalState(AudioEngineInternalState* state,
                                       const char* name) {
  auto it =
      std::find_if(state->buses.begin(), state->buses.end(),
                   [name](const BusInternalState& bus) {
                     return strcmp(bus.bus_def()->name()->c_str(), name) == 0;
                   });
  if (it != state->buses.end()) {
    return &*it;
  } else {
    return nullptr;
  }
}

static bool PopulateBuses(AudioEngineInternalState* state,
                          const char* list_name,
                          const BusNameList* child_name_list,
                          std::vector<BusInternalState*>* output) {
  for (flatbuffers::uoffset_t i = 0;
       child_name_list && i < child_name_list->Length(); ++i) {
    const char* bus_name = child_name_list->Get(i)->c_str();
    BusInternalState* bus = FindBusInternalState(state, bus_name);
    if (bus) {
      output->push_back(bus);
    } else {
      CallLogFunc("Unknown bus \"%s\" listed in %s.\n", bus_name, list_name);
      return false;
    }
  }
  return true;
}

// The InternalChannelStates have three lists they are a part of: The engine's
// priority list, the bus's playing sound list, and which free list they are in.
// Initially, all nodes are in a free list becuase nothing is playing. Seperate
// free lists are kept for real channels and virtual channels (where 'real'
// channels are channels that have a channel_id
static void InitializeChannelFreeLists(
    FreeList* real_channel_free_list, FreeList* virtual_channel_free_list,
    std::vector<ChannelInternalState>* channels, unsigned int virtual_channels,
    unsigned int real_channels) {
  // We do our own tracking of audio channels so that when a new sound is
  // played we can determine if one of the currently playing channels is lower
  // priority so that we can drop it.
  unsigned int total_channels = real_channels + virtual_channels;
  channels->resize(total_channels);
  for (size_t i = 0; i < total_channels; ++i) {
    ChannelInternalState& channel = (*channels)[i];

    // Track real channels separately from virtual channels.
    if (i < real_channels) {
      channel.real_channel().Initialize(static_cast<int>(i));
      real_channel_free_list->push_front(channel);
    } else {
      virtual_channel_free_list->push_front(channel);
    }
  }
}

static void InitializeListenerFreeList(
    std::vector<ListenerInternalState*>* listener_state_free_list,
    ListenerStateVector* listener_list, unsigned int list_size) {
  listener_list->resize(list_size);
  listener_state_free_list->reserve(list_size);
  for (size_t i = 0; i < list_size; ++i) {
    ListenerInternalState& listener = (*listener_list)[i];
    listener_state_free_list->push_back(&listener);
  }
}

bool AudioEngine::Initialize(const char* config_file) {
  std::string audio_config_source;
  if (!LoadFile(config_file, &audio_config_source)) {
    CallLogFunc("Could not load audio config file.\n");
    return false;
  }
  return Initialize(GetAudioConfig(audio_config_source.c_str()));
}

bool AudioEngine::Initialize(const AudioConfig* config) {
  // Construct internals.
  state_ = new AudioEngineInternalState();
  state_->version = &Version();

  // Initialize audio engine.
  if (!state_->mixer.Initialize(config)) {
    return false;
  }

  // Initialize the channel internal data.
  InitializeChannelFreeLists(
      &state_->real_channel_free_list, &state_->virtual_channel_free_list,
      &state_->channel_state_memory, config->mixer_virtual_channels(),
      config->mixer_channels());

  // Initialize the listener internal data.
  InitializeListenerFreeList(&state_->listener_state_free_list,
                             &state_->listener_state_memory,
                             config->listeners());

  // Load the audio buses.
  if (!LoadFile(config->bus_file()->c_str(), &state_->buses_source)) {
    CallLogFunc("Could not load audio bus file.\n");
    return false;
  }
  const BusDefList* bus_def_list =
      pindrop::GetBusDefList(state_->buses_source.c_str());
  state_->buses.resize(bus_def_list->buses()->Length());
  for (flatbuffers::uoffset_t i = 0; i < bus_def_list->buses()->Length(); ++i) {
    state_->buses[i].Initialize(bus_def_list->buses()->Get(i));
  }

  // Set up the children and ducking pointers.
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    BusInternalState& bus = state_->buses[i];
    const BusDef* def = bus.bus_def();
    if (!PopulateBuses(state_, "child_buses", def->child_buses(),
                       &bus.child_buses())) {
      return false;
    }
    if (!PopulateBuses(state_, "duck_buses", def->duck_buses(),
                       &bus.duck_buses())) {
      return false;
    }
  }

  state_->master_bus = FindBusInternalState(state_, "master");
  if (!state_->master_bus) {
    CallLogFunc("No master bus specified.\n");
    return false;
  }

  state_->paused = false;
  state_->mute = false;
  state_->master_gain = 1.0f;

  return true;
}

bool AudioEngine::LoadSoundBank(const std::string& filename) {
  bool success = true;
  auto iter = state_->sound_bank_map.find(filename);
  if (iter == state_->sound_bank_map.end()) {
    auto& sound_bank = state_->sound_bank_map[filename];
    sound_bank.reset(new SoundBank());
    success = sound_bank->Initialize(filename, this);
    if (success) {
      sound_bank->ref_counter()->Increment();
    }
  } else {
    iter->second->ref_counter()->Increment();
  }
  return success;
}

void AudioEngine::UnloadSoundBank(const std::string& filename) {
  auto iter = state_->sound_bank_map.find(filename);
  if (iter == state_->sound_bank_map.end()) {
    CallLogFunc(
        "Error while deinitializing SoundBank %s - sound bank not loaded.\n",
        filename.c_str());
    assert(0);
  }
  if (iter->second->ref_counter()->Decrement() == 0) {
    iter->second->Deinitialize(this);
  }
}

void AudioEngine::StartLoadingSoundFiles() { state_->loader.StartLoading(); }

bool AudioEngine::TryFinalize() { return state_->loader.TryFinalize(); }

bool BestListener(ListenerList::const_iterator* best_listener,
                  float* distance_squared,
                  mathfu::Vector<float, 3>* listener_space_location,
                  const ListenerList& listener_list,
                  const mathfu::Vector<float, 3>& location) {
  if (listener_list.empty()) {
    return false;
  }
  ListenerList::const_iterator listener = listener_list.cbegin();
  mathfu::Matrix<float, 4, 4> mat = listener->inverse_matrix();
  *listener_space_location = mat * location;
  *distance_squared = listener_space_location->LengthSquared();
  *best_listener = listener;
  for (++listener; listener != listener_list.cend(); ++listener) {
    mathfu::Vector<float, 3> transformed_location =
        listener->inverse_matrix() * location;
    float magnitude_squared = transformed_location.LengthSquared();
    if (magnitude_squared < *distance_squared) {
      *best_listener = listener;
      *distance_squared = magnitude_squared;
      *listener_space_location = transformed_location;
    }
  }
  return true;
}

mathfu::Vector<float, 2> CalculatePan(
    const mathfu::Vector<float, 3>& listener_space_location) {
  // Zero length vectors just end up with NaNs when normalized. Return a zero
  // vector instead.
  const float kEpsilon = 0.0001f;
  if (listener_space_location.LengthSquared() <= kEpsilon) {
    return mathfu::kZeros2f;
  }
  mathfu::Vector<float, 3> direction = listener_space_location.Normalized();
  return mathfu::Vector<float, 2>(
      mathfu::Vector<float, 3>::DotProduct(mathfu::kAxisX3f, direction),
      mathfu::Vector<float, 3>::DotProduct(mathfu::kAxisZ3f, direction));
}

float AttenuationCurve(float point, float lower_bound, float upper_bound,
                       float curve_factor) {
  assert(lower_bound <= point && point <= upper_bound && curve_factor >= 0.0f);
  float distance = point - lower_bound;
  float range = upper_bound - lower_bound;
  return distance / ((range - distance) * (curve_factor - 1.0f) + range);
}

inline float Square(float f) { return f * f; }

float CalculateDistanceAttenuation(float distance_squared,
                                   const SoundCollectionDef* def) {
  if (distance_squared < Square(def->min_audible_radius()) ||
      distance_squared > Square(def->max_audible_radius())) {
    return 0.0f;
  }
  float distance = std::sqrt(distance_squared);
  if (distance < def->roll_in_radius()) {
    return AttenuationCurve(distance, def->min_audible_radius(),
                            def->roll_in_radius(), def->roll_in_curve_factor());
  } else if (distance > def->roll_out_radius()) {
    return 1.0f - AttenuationCurve(distance, def->roll_out_radius(),
                                   def->max_audible_radius(),
                                   def->roll_out_curve_factor());
  } else {
    return 1.0f;
  }
}

static void CalculateGainAndPan(float* gain, mathfu::Vector<float, 2>* pan,
                                SoundCollection* collection,
                                const mathfu::Vector<float, 3>& location,
                                const ListenerList& listener_list,
                                float user_gain) {
  const SoundCollectionDef* def = collection->GetSoundCollectionDef();
  *gain = def->gain() * collection->bus()->gain() * user_gain;
  if (def->mode() == Mode_Positional) {
    ListenerList::const_iterator listener;
    float distance_squared;
    mathfu::Vector<float, 3> listener_space_location;
    if (BestListener(&listener, &distance_squared, &listener_space_location,
                     listener_list, location)) {
      *gain *= CalculateDistanceAttenuation(distance_squared, def);
      *pan = CalculatePan(listener_space_location);
    } else {
      *gain = 0.0f;
      *pan = mathfu::kZeros2f;
    }
  } else {
    *pan = mathfu::kZeros2f;
  }
}

// Given the priority of a node, and the list of ChannelInternalStates sorted by
// priority, find the location in the list where the node would be inserted.
// Note that the node should be inserted using InsertAfter. If the node you want
// to insert turns out the be the highest priority node, this will return the
// list terminator (and inserting after the terminator will put it at the front
// of the list).
PriorityList::iterator FindInsertionPoint(PriorityList* list, float priority) {
  PriorityList::reverse_iterator iter;
  for (iter = list->rbegin(); iter != list->rend(); ++iter) {
    float p = iter->Priority();
    if (p > priority) {
      break;
    }
  }
  return iter.base();
}

// Given a location to insert a node, take an InternalChannelState from the
// appropritate list and insert it there. Return the new InternalChannelState.
//
// There are three places an InternalChannelState may be taken from. First, if
// there are any real channels available in the real channel free list, use one
// of those so that your channel can play.
//
// If there are no real channels, then use a free virtual channel instead so
// that your channel can at least be tracked.
//
// If there are no real or virtual channels, use the node in the priority list,
// remove it from the list, and insert it in the new insertion point. This
// causes the lowest priority sound to stop being tracked.
//
// If the node you are trying to insert is the lowest priority, do nothing and
// return a nullptr.
//
// This function could use some unit tests b/20752976
static ChannelInternalState* FindFreeChannelInternalState(
    PriorityList::iterator insertion_point, PriorityList* list,
    FreeList* real_channel_free_list, FreeList* virtual_channel_free_list,
    bool paused) {
  ChannelInternalState* new_channel = nullptr;
  // Grab a free ChannelInternalState if there is one and the engine is not
  // paused. The engine is paused, grab a virtual channel for now, and it will
  // fix itself when the engine is unpaused.
  if (!paused && !real_channel_free_list->empty()) {
    new_channel = &real_channel_free_list->front();
    real_channel_free_list->pop_front();
    PriorityList::insert_before(*insertion_point, *new_channel,
                               &ChannelInternalState::priority_node);
  } else if (!virtual_channel_free_list->empty()) {
    new_channel = &virtual_channel_free_list->front();
    virtual_channel_free_list->pop_front();
    PriorityList::insert_before(*insertion_point, *new_channel,
                               &ChannelInternalState::priority_node);
  } else if (&*insertion_point != &list->back()) {
    // If there are no free sounds, and the new sound is not the lowest priority
    // sound, evict the lowest priority sound.
    new_channel = &list->back();
    new_channel->Halt();

    // Move it to a new spot in the list if it needs to be moved.
    if (&*insertion_point != new_channel) {
      list->pop_back();
      list->insert(insertion_point, *new_channel);
    }
  }
  return new_channel;
}

// Returns this channel to the free appropriate free list based on whether it's
// backed by a real channel or not.
static void InsertIntoFreeList(AudioEngineInternalState* state,
                               ChannelInternalState* channel) {
  channel->Remove();
  FreeList* list = channel->is_real()
                       ? &state->real_channel_free_list
                       : &state->virtual_channel_free_list;
  list->push_front(*channel);
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle) {
  return PlaySound(sound_handle, mathfu::kZeros3f, 1.0f);
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle,
                               const mathfu::Vector<float, 3>& location) {
  return PlaySound(sound_handle, location, 1.0f);
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle,
                               const mathfu::Vector<float, 3>& location,
                               float user_gain) {
  SoundCollection* collection = sound_handle;
  if (!collection) {
    CallLogFunc("Cannot play sound: invalid sound handle\n");
    return Channel(nullptr);
  }

  // Find where it belongs in the list.
  float gain;
  mathfu::Vector<float, 2> pan;
  CalculateGainAndPan(&gain, &pan, collection, location, state_->listener_list,
                      user_gain);
  float priority = gain * sound_handle->GetSoundCollectionDef()->priority();
  PriorityList::iterator insertion_point =
      FindInsertionPoint(&state_->playing_channel_list, priority);

  // Decide which ChannelInternalState object to use.
  ChannelInternalState* new_channel = FindFreeChannelInternalState(
      insertion_point, &state_->playing_channel_list,
      &state_->real_channel_free_list, &state_->virtual_channel_free_list,
      state_->paused);

  // The sound could not be added to the list; not high enough priority.
  if (new_channel == nullptr) {
    return Channel(nullptr);
  }

  // Now that we have our new sound, set the data on it and update the next
  // pointers.
  new_channel->SetSoundCollection(sound_handle);
  new_channel->set_user_gain(user_gain);

  // Attempt to play the sound if the engine is not paused.
  if (!state_->paused) {
    if (!new_channel->Play(sound_handle)) {
      // Error playing the sound, put it back in the free list.
      InsertIntoFreeList(state_, new_channel);
      return Channel(nullptr);
    }
  }

  new_channel->set_gain(gain);
  new_channel->SetLocation(location);
  if (new_channel->is_real()) {
    new_channel->real_channel().SetGain(gain);
    new_channel->real_channel().SetPan(pan);
  }

  return Channel(new_channel);
}

Channel AudioEngine::PlaySound(const std::string& sound_name) {
  return PlaySound(sound_name, mathfu::kZeros3f, 1.0f);
}

Channel AudioEngine::PlaySound(const std::string& sound_name,
                               const mathfu::Vector<float, 3>& location) {
  return PlaySound(sound_name, location, 1.0f);
}

Channel AudioEngine::PlaySound(const std::string& sound_name,
                               const mathfu::Vector<float, 3>& location,
                               float user_gain) {
  SoundHandle handle = GetSoundHandle(sound_name);
  if (handle) {
    return PlaySound(handle, location, user_gain);
  } else {
    CallLogFunc("Cannot play sound: invalid name (%s)\n", sound_name.c_str());
    return Channel(nullptr);
  }
}

SoundHandle AudioEngine::GetSoundHandle(const std::string& sound_name) const {
  auto iter = state_->sound_collection_map.find(sound_name);
  if (iter == state_->sound_collection_map.end()) {
    return nullptr;
  }
  return iter->second.get();
}

SoundHandle AudioEngine::GetSoundHandleFromFile(
    const std::string& filename) const {
  auto iter = state_->sound_id_map.find(filename);
  if (iter == state_->sound_id_map.end()) {
    return nullptr;
  }
  return GetSoundHandle(iter->second);
}

Listener AudioEngine::AddListener() {
  if (state_->listener_state_free_list.empty()) {
    return Listener(nullptr);
  }
  ListenerInternalState* listener = state_->listener_state_free_list.back();
  state_->listener_state_free_list.pop_back();
  state_->listener_list.push_back(*listener);
  return Listener(listener);
}

void AudioEngine::RemoveListener(Listener* listener) {
  assert(listener->Valid());
  listener->state()->node.remove();
  state_->listener_state_free_list.push_back(listener->state());
}

Bus AudioEngine::FindBus(const char* bus_name) {
  return Bus(FindBusInternalState(state_, bus_name));
}

void AudioEngine::Pause(bool pause) {
  state_->paused = pause;

  PriorityList& list = state_->playing_channel_list;
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    if (!iter->Paused() && iter->is_real()) {
      if (pause) {
        // Pause the real channel underlying this virutal channel. This freezes
        // playback of the channel without marking it as paused from the audio
        // engine's point of view, so that we know to restart it when the audio
        // engine is unpaused.
        iter->real_channel().Pause();
      } else {
        // Unpause all channels that were not explicitly paused.
        iter->real_channel().Resume();
      }
    }
  }
}

static void EraseFinishedSounds(AudioEngineInternalState* state) {
  PriorityList& list = state->playing_channel_list;
  for (auto iter = list.begin(); iter != list.end();) {
    auto current = iter++;
    current->UpdateState();
    if (current->Stopped()) {
      InsertIntoFreeList(state, &*current);
    }
  }
}

static void UpdateChannel(ChannelInternalState* channel,
                          AudioEngineInternalState* state) {
  float gain;
  mathfu::Vector<float, 2> pan;
  CalculateGainAndPan(&gain, &pan, channel->sound_collection(),
                      channel->Location(), state->listener_list,
                      channel->user_gain());
  channel->set_gain(gain);
  if (channel->is_real()) {
    channel->real_channel().SetGain(gain);
    channel->real_channel().SetPan(pan);
  }
}

// If there are any free real channels, assign those to virtual channels that
// need them. If the priority list has gaps (i.e. if there are real channels
// that are lower priority than virtual channels) then move the lower priority
// real channels to the higher priority virtual channels.
// TODO(amablue): Write unit tests for this function. b/20696606
static void UpdateRealChannels(PriorityList* priority_list,
                               FreeList* real_free_list,
                               FreeList* virtual_free_list) {
  PriorityList::reverse_iterator reverse_iter = priority_list->rbegin();
  for (auto iter = priority_list->begin(); iter != priority_list->end();
       ++iter) {
    if (!iter->is_real()) {
      // First check if there are any free real channels.
      if (!real_free_list->empty()) {
        // We have a free real channel. Assign this channel id to the channel
        // that is trying to resume, clear the free channel, and push it into
        // the virtual free list.
        ChannelInternalState* free_channel = &real_free_list->front();
        iter->Devirtualize(free_channel);
        virtual_free_list->push_front(*free_channel);
        iter->Resume();
      } else {
        // If there aren't any free channels, then scan from the back of the
        // list for low priority real channels.
        reverse_iter =
            std::find_if(reverse_iter, PriorityList::reverse_iterator(iter),
                         [](const ChannelInternalState& channel) {
                           return channel.real_channel().Valid();
                         });
        if (reverse_iter == priority_list->rend()) {
          // There is no more swapping that can be done. Return.
          return;
        }
        // Found a real channel that we can give to the higher priority
        // channel.
        iter->Devirtualize(&*reverse_iter);
      }
    }
  }
}

void AudioEngine::AdvanceFrame(float delta_time) {
  ++state_->current_frame;
  EraseFinishedSounds(state_);
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    state_->buses[i].ResetDuckGain();
  }
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    state_->buses[i].UpdateDuckGain(delta_time);
  }
  if (state_->master_bus) {
    float master_gain = state_->mute ? 0.0f : state_->master_gain;
    state_->master_bus->AdvanceFrame(delta_time, master_gain);
  }
  PriorityList& list = state_->playing_channel_list;
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    UpdateChannel(&*iter, state_);
  }
  list.sort(
      [](const ChannelInternalState& a, const ChannelInternalState& b) -> bool {
        return a.Priority() < b.Priority();
      });
  // No point in updating which channels are real and virtual when paused.
  if (!state_->paused) {
    UpdateRealChannels(&state_->playing_channel_list,
                       &state_->real_channel_free_list,
                       &state_->virtual_channel_free_list);
  }
}

const PindropVersion* AudioEngine::version() const { return state_->version; }

}  // namespace pindrop
