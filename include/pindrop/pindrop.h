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

#ifndef PINDROP_AUDIO_ENGINE_H_
#define PINDROP_AUDIO_ENGINE_H_

#include <string>

#include "mathfu/vector_3.h"

#define PINDROP_VERSION_MAJOR 1
#define PINDROP_VERSION_MINOR 0
#define PINDROP_VERSION_REVISION 0
#define PINDROP_STRING_EXPAND(X) #X
#define PINDROP_STRING(X) PINDROP_STRING_EXPAND(X)

namespace pindrop {

class Channel;
class ChannelInternalState;
class ListenerInternalState;
class SoundCollection;
struct AudioConfig;
struct AudioEngineInternalState;

typedef SoundCollection* SoundHandle;
typedef size_t ListenerId;

extern const Channel kInvalidChannel;

class Listener {
 public:
  Listener(ListenerInternalState* state) : state_(state) {}

  bool Valid() const;

  mathfu::Vector<float, 3> Location() const;
  void SetLocation(const mathfu::Vector<float, 3>& location);

  ListenerInternalState* state() { return state_; }

 private:
  ListenerInternalState* state_;
};

class Channel {
 public:
  Channel(ChannelInternalState* state) : state_(state) {}

  bool Valid() const;

  // Checks if the sound playing on a given channel is playing
  bool Playing() const;

  // Stop a channel.
  void Stop();

  // Sets or gets the location of a playing sound.
  const mathfu::Vector<float, 3> Location() const;
  void SetLocation(const mathfu::Vector<float, 3>& location);

 private:
  ChannelInternalState* state_;
};

class AudioEngine {
 public:
  AudioEngine() : state_(nullptr) {}
  ~AudioEngine();

  // Initialize the audio engine.
  // You may either initlize with a pointer to the AudioConfig structure or the
  // file containing the AudioConfig flatbuffer.
  bool Initialize(const char* config_file);
  bool Initialize(const AudioConfig* config);

  // Update audio volume per channel each frame.
  void AdvanceFrame(float delta_time);

  // Load a sound bank from a file.
  // Returns true on success.
  bool LoadSoundBank(const std::string& filename);

  // Unload a sound bank.
  void UnloadSoundBank(const std::string& filename);

  // Get a SoundHandle given its sound_id as defined in its flatbuffer.
  SoundHandle GetSoundHandle(const std::string& sound_id) const;

  // Get a SoundHandle given its filename.
  SoundHandle GetSoundHandleFromFile(const std::string& filename) const;

  // Adjusts the gain on the master bus.
  void set_master_gain(float master_gain);
  float master_gain();

  // Mutes the audio engine completely.
  void set_mute(bool mute);
  bool mute();

  // Pauses all playing sounds and streams.
  void Pause(bool pause);

  Listener AddListener();
  void RemoveListener(Listener* listener);

  // Play a sound associated with the given sound_handle.
  // Returns the channel the sound is played on.
  // Playing a SoundHandle is faster while passing the sound by sound_id incurs
  // a map lookup.
  Channel PlaySound(SoundHandle sound_handle);
  Channel PlaySound(SoundHandle sound_handle,
                    const mathfu::Vector<float, 3>& location);
  Channel PlaySound(const std::string& sound_id);
  Channel PlaySound(const std::string& sound_id,
                    const mathfu::Vector<float, 3>& location);

  const char* version_string() const;

  AudioEngineInternalState* state() { return state_; }

 private:
  AudioEngineInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_AUDIO_ENGINE_H_

