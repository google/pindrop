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

#include "pindrop/pindrop.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "SDL.h"
#include "audio_config_generated.h"
#include "audio_engine_internal_state.h"
#include "bus.h"
#include "buses_generated.h"
#include "channel_internal_state.h"
#include "intrusive_list.h"
#include "listener_internal_state.h"
#include "mathfu/constants.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

#define PINDROP_VERSION_MAJOR 1
#define PINDROP_VERSION_MINOR 0
#define PINDROP_VERSION_REVISION 0
#define PINDROP_STRING_EXPAND(X) #X
#define PINDROP_STRING(X) PINDROP_STRING_EXPAND(X)

namespace pindrop {

// clang-format off
static const char* kPindropVersionString =
    "pindrop "
    PINDROP_STRING(PINDROP_VERSION_MAJOR) "."
    PINDROP_STRING(PINDROP_VERSION_MINOR) "."
    PINDROP_STRING(PINDROP_VERSION_REVISION);
// clang-format on

bool LoadFile(const char* filename, std::string* dest) {
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "LoadFile fail on %s", filename);
    return false;
  }
  size_t len = static_cast<size_t>(SDL_RWsize(handle));
  dest->assign(len + 1, 0);
  size_t rlen = SDL_RWread(handle, &(*dest)[0], 1, len);
  SDL_RWclose(handle);
  return len == rlen && len > 0;
}

AudioEngine::~AudioEngine() { delete state_; }

Bus* FindBus(AudioEngineInternalState* state, const char* name) {
  auto it = std::find_if(
      state->buses.begin(), state->buses.end(), [name](const Bus& bus) {
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
                          std::vector<Bus*>* output) {
  for (flatbuffers::uoffset_t i = 0;
       child_name_list && i < child_name_list->Length(); ++i) {
    const char* bus_name = child_name_list->Get(i)->c_str();
    Bus* bus = FindBus(state, bus_name);
    if (bus) {
      output->push_back(bus);
    } else {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown bus \"%s\" listed in %s.\n",
                   bus_name, list_name);
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
    IntrusiveListNode* real_channel_free_list,
    IntrusiveListNode* virtual_channel_free_list,
    std::vector<ChannelInternalState>* channels, unsigned int virtual_channels,
    unsigned int real_channels) {
  // We do our own tracking of audio channels so that when a new sound is
  // played we can determine if one of the currently playing channels is lower
  // priority so that we can drop it.
  unsigned int total_channels = real_channels + virtual_channels;
  channels->resize(total_channels);
  for (size_t i = 0; i < total_channels; ++i) {
    ChannelInternalState& channel = (*channels)[i];
    // Because the channels are in a vector they may have been copy constructed.
    // Intrusive lists misbehave in these cases, so they need to be
    // reinitialized.
    channel.priority_node()->Initialize();
    channel.free_node()->Initialize();
    channel.bus_node()->Initialize();

    // Track real channels separately from virtual channels.
    if (i < real_channels) {
      channel.set_channel_id(static_cast<ChannelId>(i));
      real_channel_free_list->InsertAfter(channel.free_node());
    } else {
      channel.set_channel_id(static_cast<ChannelId>(kInvalidChannelId));
      virtual_channel_free_list->InsertAfter(channel.free_node());
    }
  }
}

static void InitializeListenerFreeList(
    std::vector<ListenerInternalState*>* listener_state_free_list,
    ListenerStateVector* listeners, unsigned int list_size) {
  listeners->resize(list_size);
  listener_state_free_list->reserve(list_size);
  for (size_t i = 0; i < list_size; ++i) {
    ListenerInternalState& listener = (*listeners)[i];
    listener_state_free_list->push_back(&listener);
    // Because the listeners are in a vector they may have been copy
    // constructed.  Intrusive lists misbehave in these cases, so they need to
    // be reinitialized.
    listener.GetListNode()->Initialize();
  }
}

bool AudioEngine::Initialize(const char* config_file) {
  std::string audio_config_source;
  if (!LoadFile(config_file, &audio_config_source)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load audio config file.\n");
    return false;
  }
  return Initialize(GetAudioConfig(audio_config_source.c_str()));
}

bool AudioEngine::Initialize(const AudioConfig* config) {
  // Construct internals.
  state_ = new AudioEngineInternalState();
  state_->version_string = kPindropVersionString;

  // Initialize audio engine.
  if (!state_->backend.Initialize(config)) {
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
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load audio bus file.\n");
    return false;
  }
  const BusDefList* bus_def_list =
      pindrop::GetBusDefList(state_->buses_source.c_str());
  state_->buses.resize(bus_def_list->buses()->Length());
  for (size_t i = 0; i < bus_def_list->buses()->Length(); ++i) {
    state_->buses[i].Initialize(bus_def_list->buses()->Get(i));
  }

  // Set up the children and ducking pointers.
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    Bus& bus = state_->buses[i];
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

  state_->master_bus = FindBus(state_, "master");
  if (!state_->master_bus) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No master bus specified.\n");
    return false;
  }

#ifndef PINDROP_MULTISTREAM
  state_->stream_channel = nullptr;
#endif  // PINDROP_MULTISTREAM

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
    SDL_LogError(
        SDL_LOG_CATEGORY_ERROR,
        "Error while deinitializing SoundBank %s - sound bank not loaded.\n",
        filename.c_str());
    assert(0);
  }
  if (iter->second->ref_counter()->Decrement() == 0) {
    iter->second->Deinitialize(this);
  }
}

bool BestListener(
    ListenerInternalState** best_listener, float* distance_squared,
    mathfu::Vector<float, 3>* listener_space_location,
    const TypedIntrusiveListNode<ListenerInternalState>& listeners,
    const mathfu::Vector<float, 3>& location) {
  if (listeners.IsEmpty()) {
    return false;
  }

  ListenerInternalState* listener = listeners.GetNext();
  *listener_space_location = listener->matrix() * location;
  *distance_squared = listener_space_location->LengthSquared();
  *best_listener = listener;
  for (listener = listener->GetNext(); listener != listeners.GetTerminator();
       listener = listener->GetNext()) {
    mathfu::Vector<float, 3> transformed_location =
        listener->matrix() * location;
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

static void CalculateGainAndPan(
    float* gain, mathfu::Vector<float, 2>* pan, SoundCollection* collection,
    const mathfu::Vector<float, 3>& location,
    const TypedIntrusiveListNode<ListenerInternalState>& listener_list) {
  const SoundCollectionDef* def = collection->GetSoundCollectionDef();
  *gain = def->gain() * collection->bus()->gain();
  if (def->mode() == Mode_Positional) {
    ListenerInternalState* listener;
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
IntrusiveListNode* FindInsertionPoint(IntrusiveListNode* list, float priority) {
  IntrusiveListNode* node;
  for (node = list->GetPrevious(); node != list->GetTerminator();
       node = node->GetPrevious()) {
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    if (channel->Priority() >= priority) {
      break;
    }
  }
  return node;
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
    IntrusiveListNode* insertion_point, IntrusiveListNode* list,
    IntrusiveListNode* real_channel_free_list,
    IntrusiveListNode* virtual_channel_free_list, bool paused) {
  ChannelInternalState* new_channel = nullptr;
  // Grab a free ChannelInternalState if there is one and the engine is not
  // paused. The engine is paused, grab a virtual channel for now, and it will
  // fix itself when the engine is unpaused.
  if (!paused && !real_channel_free_list->IsEmpty()) {
    IntrusiveListNode* node = real_channel_free_list->GetNext()->Remove();
    new_channel = ChannelInternalState::GetInstanceFromFreeNode(node);
    insertion_point->InsertAfter(new_channel->priority_node());
  } else if (!virtual_channel_free_list->IsEmpty()) {
    IntrusiveListNode* node = virtual_channel_free_list->GetNext()->Remove();
    new_channel = ChannelInternalState::GetInstanceFromFreeNode(node);
    insertion_point->InsertAfter(new_channel->priority_node());
  } else if (insertion_point != list->GetTerminator()->GetPrevious()) {
    // If there are no free sounds, and the new sound is not the lowest priority
    // sound, evict the lowest priority sound.
    IntrusiveListNode* node = list->GetPrevious();
    new_channel = ChannelInternalState::GetInstanceFromPriorityNode(node);
    new_channel->Halt();

    // Move it to a new spot in the list if it needs to be moved.
    if (insertion_point != node) {
      insertion_point->InsertAfter(node->Remove());
    }
  }
  return new_channel;
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle) {
  return PlaySound(sound_handle, mathfu::kZeros3f);
}

// Returns this channel to the free appropriate free list based on whether it's
// backed by a real channel or not.
static void InsertIntoFreeList(AudioEngineInternalState* state,
                               ChannelInternalState* channel) {
  channel->Remove();
  IntrusiveListNode* list = channel->is_real()
                                ? &state->real_channel_free_list
                                : &state->virtual_channel_free_list;
  list->InsertAfter(channel->free_node());
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle,
                               const mathfu::Vector<float, 3>& location) {
  SoundCollection* collection = sound_handle;
  if (!collection) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Cannot play sound: invalid sound handle\n");
    return Channel(nullptr);
  }

  // Find where it belongs in the list.
  float gain;
  mathfu::Vector<float, 2> pan;
  CalculateGainAndPan(&gain, &pan, collection, location, state_->listener_list);
  float priority = gain * sound_handle->GetSoundCollectionDef()->priority();
  IntrusiveListNode* insertion_point =
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
  new_channel->SetHandle(sound_handle);

  // Attempt to play the sound if the engine is not paused.
  if (!state_->paused) {
    if (!new_channel->Play(sound_handle)) {
      // Error playing the sound, put it back in the free list.
      InsertIntoFreeList(state_, new_channel);
      return Channel(nullptr);
    }
  }

  new_channel->set_gain(gain);
  if (new_channel->is_real()) {
    new_channel->SetRealChannelGain(gain);
    new_channel->SetPan(pan);
  }

#ifndef PINDROP_MULTISTREAM
  // If we only support a single stream, stop tracking the channel assigned to
  // the old stream.
  if (new_channel->IsStream()) {
    if (state_->stream_channel) {
      InsertIntoFreeList(state_, state_->stream_channel);
    }
    state_->stream_channel = new_channel;
  }
#endif  // PINDROP_MULTISTREAM

  return Channel(new_channel);
}

Channel AudioEngine::PlaySound(const std::string& sound_name,
                               const mathfu::Vector<float, 3>& location) {
  SoundHandle handle = GetSoundHandle(sound_name);
  if (handle) {
    return PlaySound(handle, location);
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Cannot play sound: invalid name (%s)\n", sound_name.c_str());
    return Channel(nullptr);
  }
}

Channel AudioEngine::PlaySound(const std::string& sound_name) {
  return PlaySound(sound_name, mathfu::kZeros3f);
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
  state_->listener_list.InsertBefore(listener);
  return Listener(listener);
}

void AudioEngine::RemoveListener(Listener* listener) {
  assert(listener->Valid());
  listener->state()->Remove();
  state_->listener_state_free_list.push_back(listener->state());
}

void AudioEngine::Pause(bool pause) {
  state_->paused = pause;
  if (pause) {
    ChannelInternalState::PauseAll();
  } else {
    ChannelInternalState::ResumeAll();
  }
}

static void EraseFinishedSounds(AudioEngineInternalState* state) {
  IntrusiveListNode& list = state->playing_channel_list;
  IntrusiveListNode* next;
  for (IntrusiveListNode* node = list.GetNext(); node != list.GetTerminator();
       node = next) {
    next = node->GetNext();
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    channel->UpdateState();
    if (channel->Stopped()) {
      InsertIntoFreeList(state, channel);
    }
  }
}

static void UpdateChannel(ChannelInternalState* channel,
                          AudioEngineInternalState* state) {
  float gain;
  mathfu::Vector<float, 2> pan;
  CalculateGainAndPan(&gain, &pan, channel->handle(), channel->Location(),
                      state->listener_list);
  channel->set_gain(gain);
  if (channel->is_real()) {
    channel->SetRealChannelGain(gain);
    channel->SetPan(pan);
  }
}

// Searches backwards through the list from start to finish looking for a real
// channel.
static IntrusiveListNode* FindRealChannel(IntrusiveListNode* start,
                                          IntrusiveListNode* finish) {
  IntrusiveListNode* node;
  for (node = start; node != finish; node = node->GetPrevious()) {
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    if (channel->is_real()) {
      return node;
    }
  }
  return nullptr;
}

// If there are any free real channels, assign those to virtual channels that
// need them. If the priority list has gaps (i.e. if there are real channels
// that are lower priority than virtual channels) then move the lower priority
// real channels to the higher priority virtual channels.
// TODO(amablue): Write unit tests for this function. b/20696606
static void UpdateRealChannels(IntrusiveListNode* priority_list,
                               IntrusiveListNode* real_free_list,
                               IntrusiveListNode* virtual_free_list) {
  IntrusiveListNode* reverse_node = priority_list->GetPrevious();
  IntrusiveListNode* node = priority_list->GetNext();
  for (; node != priority_list->GetTerminator(); node = node->GetNext()) {
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    if (!channel->is_real()) {
      // First check if there are any free real channels.
      if (!real_free_list->IsEmpty()) {
        // We have a free real channel. Assign this channel id to the channel
        // that is trying to resume, clear the free channel, and push it into
        // the virtual free list.
        IntrusiveListNode* free_node = real_free_list->GetNext()->Remove();
        ChannelInternalState* free_channel =
            ChannelInternalState::GetInstanceFromFreeNode(free_node);
        channel->set_channel_id(free_channel->channel_id());
        free_channel->invalidate();
        virtual_free_list->InsertAfter(free_node);
        channel->Resume();
      } else {
        // If there aren't any free channels, then scan from the back of the
        // list for low priority real channels.
        reverse_node = FindRealChannel(reverse_node, node);
        if (reverse_node == nullptr) {
          // There is no more swapping that can be done. Return.
          return;
        }
        // Found a real channel that we can give to the higher priority
        // channel.
        ChannelInternalState* reverse_channel =
            ChannelInternalState::GetInstanceFromPriorityNode(reverse_node);
        channel->set_channel_id(reverse_channel->channel_id());
        reverse_channel->RealChannelHalt();
        reverse_channel->invalidate();
        channel->Resume();
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
    state_->master_bus->UpdateGain(state_->mute ? 0.0f : state_->master_gain);
  }
  IntrusiveListNode& list = state_->playing_channel_list;
  for (IntrusiveListNode* node = list.GetNext(); node != list.GetTerminator();
       node = node->GetNext()) {
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    UpdateChannel(channel, state_);
  }
  list.Sort([](const IntrusiveListNode& a, const IntrusiveListNode& b) -> bool {
    const ChannelInternalState* channel_a =
        ChannelInternalState::GetInstanceFromPriorityNode(&a);
    const ChannelInternalState* channel_b =
        ChannelInternalState::GetInstanceFromPriorityNode(&b);
    return channel_b->Priority() < channel_a->Priority();
  });
  // No point in updating which channels are real and virtual when paused.
  if (!state_->paused) {
    UpdateRealChannels(&state_->playing_channel_list,
                       &state_->real_channel_free_list,
                       &state_->virtual_channel_free_list);
  }
}

const char* AudioEngine::version_string() const {
  return state_->version_string;
}

}  // namespace pindrop
