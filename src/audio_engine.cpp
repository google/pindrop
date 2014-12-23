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

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "audio_engine_internal_state.h"
#include "audio_config_generated.h"
#include "bus.h"
#include "buses_generated.h"
#include "flatbuffers/util.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

const int kChannelFadeOutRateMs = 10;

// Special value for SDL_Mixer that indicates an operation should be applied to
// all channels.
const AudioEngine::ChannelId kAllChannels = -1;

// Special value representing an audio stream.
static const AudioEngine::ChannelId kStreamChannel = -100;

const AudioEngine::ChannelId AudioEngine::kInvalidChannel = -1;

AudioEngine::~AudioEngine() {
  delete state_;
  Mix_CloseAudio();
}

Bus* FindBus(AudioEngineInternalState* state, const char* name) {
  auto it = std::find_if(state->buses.begin(), state->buses.end(),
                         [name](const Bus& bus) {
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
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Unknown bus \"%s\" listed in %s.\n", bus_name, list_name);
      return false;
    }
  }
  return true;
}

bool AudioEngine::Initialize(const AudioConfig* config) {
  // Construct internals.
  state_ = new AudioEngineInternalState();

  // Initialize audio engine.
  if (Mix_OpenAudio(config->output_frequency(),
                    AUDIO_S16LSB,
                    config->output_channels(),
                    config->output_buffer_size()) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't open audio stream\n");
    return false;
  }

  // Initialize Ogg support. Returns a bitmask of formats that were successfully
  // initialized, so make sure ogg support was successfully loaded.
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing Ogg support\n");
  }

  // Number of sound that can be played simutaniously.
  Mix_AllocateChannels(config->mixer_channels());

  // We do our own tracking of audio channels so that when a new sound is
  // played we can determine if one of the currently playing channels is lower
  // priority so that we can drop it.
  state_->playing_sounds.reserve(config->mixer_channels());

  // Load the audio buses.
  if (!flatbuffers::LoadFile("buses.bin", false, &state_->buses_source)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't load audio bus file.\n");
    return false;
  }
  const BusDefList* bus_def_list =
      pindrop::GetBusDefList(state_->buses_source.c_str());
  state_->buses.reserve(bus_def_list->buses()->Length());
  for (size_t i = 0; i < bus_def_list->buses()->Length(); ++i) {
    state_->buses.push_back(Bus(bus_def_list->buses()->Get(i)));
  }

  // Set up the children and ducking pointers.
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    Bus& bus = state_->buses[i];
    const BusDef* def = bus.bus_def();
    if (!PopulateBuses(
            state_, "child_buses", def->child_buses(), &bus.child_buses())) {
      return false;
    }
    if (!PopulateBuses(
            state_, "duck_buses", def->duck_buses(), &bus.duck_buses())) {
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

static int AllocatedChannelCount() {
  // Passing negative values returns the number of allocated channels.
  return Mix_AllocateChannels(-1);
}

static int PlayingChannelCount() {
  // Passing negative values returns the number of playing channels.
  return Mix_Playing(-1);
}

static AudioEngine::ChannelId FindFreeChannel(bool stream) {
  if (stream) {
    return kStreamChannel;
  }
  const int allocated_channels = AllocatedChannelCount();
  if (PlayingChannelCount() < allocated_channels) {
    for (int i = 0; i < allocated_channels; ++i) {
      if (!Mix_Playing(i)) {
        return i;
      }
    }
  }
  return AudioEngine::kInvalidChannel;
}

static int SoundCollectionDefComparitor(const SoundCollectionDef* a,
                                        const SoundCollectionDef* b) {
  // Since there can only be one stream playing, and that stream is independent
  // of the buffer channels, always make it highest priority.
  if (a->stream() || b->stream()) {
    return b->stream() - a->stream();
  } else {
    return (b->priority()- a->priority()) < 0 ? -1 : 1;
  }
}

// Sort by priority. In the case of two sounds with the same priority, sort
// the newer one as being higher priority. Higher priority elements have lower
// indicies.
static int PriorityComparitor(const PlayingSound& a, const PlayingSound& b) {
  int result = SoundCollectionDefComparitor(
      a.handle->GetSoundCollectionDef(),
      b.handle->GetSoundCollectionDef());
  if (result == 0) {
    return b.start_time - a.start_time;
  }
  return result;
}

// Sort channels with highest priority first.
void PrioritizeChannels(std::vector<PlayingSound>* playing_sounds) {
  std::sort(playing_sounds->begin(), playing_sounds->end(), PriorityComparitor);
}

// Remove all sounds that are no longer playing.
static void EraseFinishedSounds(AudioEngineInternalState* state) {
  state->playing_sounds.erase(std::remove_if(
      state->playing_sounds.begin(), state->playing_sounds.end(),
      [](const PlayingSound& playing_sound) {
        return !AudioEngine::Playing(playing_sound.channel_id);
      }),
      state->playing_sounds.end());
}

// Remove all streams.
static void EraseStreams(AudioEngineInternalState* state) {
  state->playing_sounds.erase(std::remove_if(
      state->playing_sounds.begin(), state->playing_sounds.end(),
      [](const PlayingSound& playing_sound) {
        return playing_sound.channel_id == kStreamChannel;
      }),
      state->playing_sounds.end());
}

static bool PlayCollection(const SoundCollection& collection,
                           AudioEngine::ChannelId channel_id) {
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

static void Halt(AudioEngine::ChannelId channel_id) {
  assert(channel_id != AudioEngine::kInvalidChannel);
  if (channel_id == kStreamChannel) {
    Mix_HaltMusic();
  } else {
    Mix_HaltChannel(channel_id);
  }
}

bool AudioEngine::Playing(ChannelId channel_id) {
  assert(channel_id != kInvalidChannel);
  if (channel_id == kStreamChannel) {
    return Mix_PlayingMusic() != 0;
  } else {
    return Mix_Playing(channel_id) != 0;
  }
}

static void SetChannelGain(AudioEngine::ChannelId channel_id, float volume) {
  assert(channel_id != AudioEngine::kInvalidChannel);
  int mix_volume = static_cast<int>(volume * MIX_MAX_VOLUME);
  if (channel_id == kStreamChannel) {
    Mix_VolumeMusic(mix_volume);
  } else {
    Mix_Volume(channel_id, mix_volume);
  }
}

AudioEngine::ChannelId AudioEngine::PlaySound(SoundHandle sound_handle) {
  SoundCollection* collection = sound_handle;
  if (!collection) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Cannot play sound: invalid sound handle\n");
    return kInvalidChannel;
  }

  // Prune sounds that have finished playing.
  EraseFinishedSounds(state_);

  bool stream = collection->GetSoundCollectionDef()->stream() != 0;
  ChannelId new_channel = FindFreeChannel(stream);

  // If there are no empty channels, clear out the one with the lowest
  // priority.
  if (new_channel == kInvalidChannel) {
    PrioritizeChannels(&state_->playing_sounds);
    // If the lowest priority sound is lower than the new one, halt it and
    // remove it from our list. Otherwise, do nothing.
    const SoundCollectionDef* new_def = collection->GetSoundCollectionDef();
    const SoundCollectionDef* back_def =
        state_->playing_sounds.back().handle->GetSoundCollectionDef();
    if (SoundCollectionDefComparitor(new_def, back_def) < 0) {
      // Use the channel of the sound we're replacing.
      new_channel = state_->playing_sounds.back().channel_id;
      // Dispose of the lowest priority sound.
      Halt(new_channel);
      state_->playing_sounds.pop_back();
    } else {
      // The sound was lower priority than all currently playing sounds; do
      // nothing.
      return kInvalidChannel;
    }
  } else if (new_channel == kStreamChannel) {
    // Halt the sound the may currently be playing on this channel.
    if (Playing(new_channel)) {
      Halt(new_channel);
      EraseStreams(state_);
    }
  }

  // At this point we should not have an invalid channel.
  assert(new_channel != kInvalidChannel);

  // Attempt to play the sound.
  if (PlayCollection(*collection, new_channel)) {
    state_->playing_sounds.push_back(
        PlayingSound(collection, new_channel, state_->world_time));
  }

  return new_channel;
}

AudioEngine::ChannelId AudioEngine::PlaySound(const std::string& sound_name) {
  SoundHandle handle = GetSoundHandle(sound_name);
  if (handle) {
    return PlaySound(handle);
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Cannot play sound: invalid name (%s)\n",
                 sound_name.c_str());
    return kInvalidChannel;
  }
}

AudioEngine::SoundHandle AudioEngine::GetSoundHandle(
    const std::string& sound_name) const {
  auto iter = state_->sound_collection_map.find(sound_name);
  if (iter != state_->sound_collection_map.end()) {
    return iter->second.get();
  } else {
    return nullptr;
  }
}

AudioEngine::SoundHandle AudioEngine::GetSoundHandleFromFile(
    const std::string& filename) const {
  auto iter = state_->sound_id_map.find(filename);
  if (iter != state_->sound_id_map.end()) {
    return GetSoundHandle(iter->second);
  } else {
    return nullptr;
  }
}

void AudioEngine::Stop(ChannelId channel_id) {
  assert(channel_id != kInvalidChannel);
  // Fade out rather than halting to avoid clicks.
  if (channel_id == kStreamChannel) {
    int return_value = Mix_FadeOutMusic(kChannelFadeOutRateMs);
    if (return_value != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Error stopping music: %s\n", Mix_GetError());
    }
  } else {
    int return_value = Mix_FadeOutChannel(channel_id, kChannelFadeOutRateMs);
    if (return_value != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Error stopping channel %d: %s\n", channel_id,
                   Mix_GetError());
    }
  }
}

void AudioEngine::Pause(bool pause) {
  if (pause) {
    Mix_Pause(kAllChannels);
    Mix_PauseMusic();
  } else {
    Mix_Resume(kAllChannels);
    Mix_ResumeMusic();
  }
}

void AudioEngine::AdvanceFrame(WorldTime world_time) {
  WorldTime delta_time = world_time - state_->world_time;
  state_->world_time = world_time;
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    state_->buses[i].ResetDuckGain();
  }
  for (size_t i = 0; i < state_->buses.size(); ++i) {
    state_->buses[i].UpdateDuckGain(delta_time);
  }
  if (state_->master_bus) {
    state_->master_bus->UpdateGain(state_->mute ? 0.0f : state_->master_gain);
  }
  for (size_t i = 0; i < state_->playing_sounds.size(); ++i) {
    PlayingSound& playing_sound = state_->playing_sounds[i];
    SetChannelGain(playing_sound.channel_id,
                   playing_sound.handle->bus()->gain());
  }
}

}  // namespace pindrop

