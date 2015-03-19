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
#include <map>
#include <iostream>

#include "SDL_log.h"
#include "SDL_mixer.h"
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

AudioEngine::~AudioEngine() {
  delete state_;
  Mix_CloseAudio();
}

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
  for (size_t i = 0; child_name_list && i < child_name_list->Length(); ++i) {
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

static void InitializeChannelFreeList(
    std::vector<ChannelInternalState*>* channel_state_free_list,
    std::vector<ChannelInternalState>* channels, unsigned int list_size) {
  // We do our own tracking of audio channels so that when a new sound is
  // played we can determine if one of the currently playing channels is lower
  // priority so that we can drop it.
  channels->resize(list_size);
  channel_state_free_list->reserve(list_size);
  for (size_t i = 0; i < list_size; ++i) {
    ChannelInternalState& channel = (*channels)[i];
    channel_state_free_list->push_back(&channel);
    channel.set_channel_id(static_cast<ChannelId>(i));
    // Because the channels are in a vector they may have been copy constructed.
    // Intrusive lists misbehave in these cases, so they need to be
    // reinitialized.
    channel.bus_node()->Initialize();
    channel.priority_node()->Initialize();
  }
}

static void InitializeListenerFreeList(
    std::vector<ListenerInternalState*>* listener_state_free_list,
    std::vector<ListenerInternalState>* listeners, unsigned int list_size) {
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
  if (Mix_OpenAudio(config->output_frequency(), AUDIO_S16LSB,
                    config->output_channels(),
                    config->output_buffer_size()) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not open audio stream\n");
    return false;
  }

  // Initialize Ogg support. Returns a bitmask of formats that were successfully
  // initialized, so make sure ogg support was successfully loaded.
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing Ogg support\n");
  }

  // Initialize the channel internal data.
  Mix_AllocateChannels(config->mixer_channels());
  InitializeChannelFreeList(&state_->channel_state_free_list,
                            &state_->channel_state_memory,
                            config->mixer_channels());
  state_->stream_channel_state.set_channel_id(kStreamChannelId);

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

static bool PlayCollection(const SoundCollection& collection,
                           ChannelId channel_id) {
  SoundSource* source = collection.Select();
  const SoundCollectionDef& def = *collection.GetSoundCollectionDef();
  const float gain =
      source->audio_sample_set_entry().audio_sample()->gain() * def.gain();
  source->SetGain(channel_id, gain);
  if (source->Play(channel_id, def.loop() != 0)) {
    return true;
  }
  return false;
}

static void SetChannelGain(ChannelId channel_id, float volume) {
  assert(channel_id != kInvalidChannelId);
  int mix_volume = static_cast<int>(volume * MIX_MAX_VOLUME);
  if (channel_id == kStreamChannelId) {
    Mix_VolumeMusic(mix_volume);
  } else {
    Mix_Volume(channel_id, mix_volume);
  }
}

static ChannelInternalState* PlayStream(AudioEngineInternalState* state,
                                        SoundHandle sound_handle, float gain) {
  // TODO: Add prioritization by gain for streams, like we have for buffers.
  (void)gain;
  if (!PlayCollection(*sound_handle, kStreamChannelId)) {
    state->stream_channel_state.Clear();
    return nullptr;
  }
  state->stream_channel_state.SetHandle(sound_handle);
  return &state->stream_channel_state;
}

ChannelInternalState* FindInsertionPoint(IntrusiveListNode* list,
                                         float priority) {
  IntrusiveListNode* node;
  for (node = list->GetPrevious(); node != list->GetTerminator();
       node = node->GetPrevious()) {
    ChannelInternalState* channel =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    if (channel->Priority() >= priority) {
      break;
    }
  }
  return ChannelInternalState::GetInstanceFromPriorityNode(node);
}

static ChannelInternalState* FindFreeChannelInternalState(
    IntrusiveListNode* list, ChannelInternalState* insertion_point,
    std::vector<ChannelInternalState*>& free_list) {
  ChannelInternalState* new_channel = nullptr;
  // Grab a free ChannelInternalState if there is one.
  if (free_list.size()) {
    new_channel = free_list.back();
    free_list.pop_back();
    insertion_point->priority_node()->InsertAfter(new_channel->priority_node());
  } else if (insertion_point->priority_node() != list->GetTerminator()) {
    // If there are no free sounds, and the new sound is not the lowest priority
    // sound, evict the lowest priority sound.
    IntrusiveListNode* node = list->GetPrevious();
    new_channel = ChannelInternalState::GetInstanceFromPriorityNode(node);
    Mix_HaltChannel(new_channel->channel_id());

    // Move it to a new spot in the list if it needs to be moved.
    if (insertion_point->priority_node() != node) {
      insertion_point->priority_node()->InsertAfter(node->Remove());
    }
  }
  return new_channel;
}

static ChannelInternalState* PlayBuffer(AudioEngineInternalState* state,
                                        SoundHandle sound_handle, float gain) {
  // Find where it belongs in the list.
  float priority = gain * sound_handle->GetSoundCollectionDef()->priority();
  ChannelInternalState* insertion_point =
      FindInsertionPoint(&state->playing_channel_list, priority);

  // Decide which ChannelInternalState object to use.
  ChannelInternalState* new_channel = FindFreeChannelInternalState(
      &state->playing_channel_list, insertion_point,
      state->channel_state_free_list);

  // The sound could not be added to the list; not high enough priority.
  if (new_channel == nullptr) {
    return nullptr;
  }

  // Now that we have our new sound, set the data on it and update the next
  // pointers.
  new_channel->SetHandle(sound_handle);

  // Attempt to play the sound.
  if (!PlayCollection(*sound_handle, new_channel->channel_id())) {
    // Error playing the sound, put it back in the free list.
    state->channel_state_free_list.push_back(new_channel);
    new_channel->Clear();
    return nullptr;
  }
  return new_channel;
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle) {
  return PlaySound(sound_handle, mathfu::kZeros3f);
}

Channel AudioEngine::PlaySound(SoundHandle sound_handle,
                               const mathfu::Vector<float, 3>& location) {
  SoundCollection* collection = sound_handle;
  if (!collection) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Cannot play sound: invalid sound handle\n");
    return Channel(nullptr);
  }
  bool stream = collection->GetSoundCollectionDef()->stream() != 0;
  float channel_gain = CalculateGain(
      state_->listener_list, sound_handle->GetSoundCollectionDef(), location);
  float bus_gain = sound_handle->bus()->gain();
  float final_gain = channel_gain * bus_gain;
  ChannelInternalState* new_channel;
  if (stream) {
    new_channel = PlayStream(state_, sound_handle, final_gain);
  } else {
    new_channel = PlayBuffer(state_, sound_handle, final_gain);
  }
  if (new_channel) {
    new_channel->set_gain(final_gain);
    SetChannelGain(new_channel->channel_id(), new_channel->gain());
  }
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
  // Special value for SDL_Mixer that indicates an operation should be applied
  // to all channels.
  const ChannelId kAllChannels = -1;
  if (pause) {
    Mix_Pause(kAllChannels);
    Mix_PauseMusic();
  } else {
    Mix_Resume(kAllChannels);
    Mix_ResumeMusic();
  }
}

static void EraseFinishedSounds(AudioEngine* engine) {
  AudioEngineInternalState* state = engine->state();
  IntrusiveListNode& list = state->playing_channel_list;
  IntrusiveListNode* next;
  for (IntrusiveListNode* node = list.GetNext(); node != list.GetTerminator();
       node = next) {
    next = node->GetNext();
    ChannelInternalState* channel_state =
        ChannelInternalState::GetInstanceFromPriorityNode(node);
    Channel channel(channel_state);
    if (!channel.Playing()) {
      state->channel_state_free_list.push_back(channel_state);
      channel_state->Clear();
    }
  }
}

float BestListener(ListenerInternalState** best_listener,
                   TypedIntrusiveListNode<ListenerInternalState>& listeners,
                   const mathfu::Vector<float, 3>& location) {
  assert(!listeners.IsEmpty());

  ListenerInternalState* listener = listeners.GetNext();
  mathfu::Vector<float, 3> delta = listener->location() - location;
  float distance_squared = delta.LengthSquared();
  *best_listener = listener;

  for (listener = listener->GetNext(); listener != listeners.GetTerminator();
       listener = listener->GetNext()) {
    delta = listener->location() - location;
    float delta_magnitude = delta.LengthSquared();
    if (delta_magnitude < distance_squared) {
      *best_listener = listener;
      distance_squared = delta_magnitude;
    }
  }
  return distance_squared;
}

inline float Square(float f) { return f * f; }

float AttenuationCurve(float point, float lower_bound, float upper_bound,
                       float curve_factor) {
  assert(lower_bound <= point && point <= upper_bound && curve_factor >= 0.0f);
  float distance = point - lower_bound;
  float range = upper_bound - lower_bound;
  return distance / ((range - distance) * (curve_factor - 1.0f) + range);
}

float CalculatePositionalAttenuation(float distance_squared,
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

float CalculateGain(TypedIntrusiveListNode<ListenerInternalState>& listeners,
                    const SoundCollectionDef* def,
                    const mathfu::Vector<float, 3>& location) {
  if (def->mode() == Mode_Nonpositional) {
    return def->gain();
  } else if (!listeners.IsEmpty()) {
    ListenerInternalState* listener;
    float distance_squared = BestListener(&listener, listeners, location);
    return def->gain() * CalculatePositionalAttenuation(distance_squared, def);
  } else {
    return 0.0f;
  }
}

static void UpdateChannel(ChannelInternalState* channel,
                          AudioEngineInternalState* state) {
  float channel_gain = CalculateGain(state->listener_list,
                                     channel->handle()->GetSoundCollectionDef(),
                                     channel->location());
  float bus_gain = channel->handle()->bus()->gain();
  channel->set_gain(channel_gain * bus_gain);
  SetChannelGain(channel->channel_id(), channel->gain());
}

void AudioEngine::AdvanceFrame(float delta_time) {
  ++state_->current_frame;
  EraseFinishedSounds(this);
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
  if (state_->stream_channel_state.handle()) {
    UpdateChannel(&state_->stream_channel_state, state_);
  }
  list.Sort([](const IntrusiveListNode& a, const IntrusiveListNode& b) -> bool {
    const ChannelInternalState* channel_a =
        ChannelInternalState::GetInstanceFromPriorityNode(&a);
    const ChannelInternalState* channel_b =
        ChannelInternalState::GetInstanceFromPriorityNode(&b);
    return channel_a->Priority() < channel_b->Priority();
  });
}

const char* AudioEngine::version_string() const {
  return state_->version_string;
}

}  // namespace pindrop

