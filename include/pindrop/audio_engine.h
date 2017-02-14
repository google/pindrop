// Copyright 2015 Google Inc. All rights reserved.
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

#include "mathfu/matrix.h"
#include "mathfu/matrix_4x4.h"
#include "mathfu/vector.h"
#include "pindrop/bus.h"
#include "pindrop/channel.h"
#include "pindrop/listener.h"
#include "pindrop/version.h"

// In windows.h, PlaySound is #defined to be either PlaySoundW or PlaySoundA.
// We need to undef this macro or AudioEngine::PlaySound() won't compile.
// TODO(amablue): Change our PlaySound to have a different name (b/30090037).
#if defined(PlaySound)
#undef PlaySound
#endif  // defined(PlaySound)

namespace pindrop {

class SoundCollection;
struct AudioConfig;
struct AudioEngineInternalState;

typedef SoundCollection* SoundHandle;

/// @class AudioEngine
///
/// @brief The central class of the library that manages the Listeners,
/// Channels, and tracks all of the internal state.
class AudioEngine {
 public:
  /// @brief Construct an uninitialized AudioEngine.
  AudioEngine() : state_(nullptr) {}

  ~AudioEngine();

  /// @brief Initialize the audio engine.
  ///
  /// @param config_file the path to the file containing an AudioConfig
  /// Flatbuffer binary.
  /// @return Whether initialization was successful.
  bool Initialize(const char* config_file);

  /// @brief Initialize the audio engine.
  ///
  /// @param config A pointer to a loaded AudioConfig object.
  /// @return Whether initialization was successful.
  bool Initialize(const AudioConfig* config);

  /// @brief Update audio volume per channel each frame.
  ///
  /// @param delta_time the number of elapsed seconds since the last frame.
  void AdvanceFrame(float delta_time);

  /// @brief Load a sound bank from a file. Queue the sound files in that sound
  ///        bank for loading. Call StartLoadingSoundFiles() to trigger loading
  ///        of the sound files on a separate thread.
  ///
  /// @param filename The file containing the SoundBank flatbuffer binary data.
  /// @return Returns true on success
  bool LoadSoundBank(const std::string& filename);

  /// @brief Unload a sound bank.
  ///
  /// @param filename The file to unload.
  void UnloadSoundBank(const std::string& filename);

  /// @brief Kick off loading thread to load all sound files queued with
  ///        LoadSoundBank().
  void StartLoadingSoundFiles();

  /// @brief Return true if all sound files have been loaded. Must call
  ///        StartLoadingSoundFiles() first.
  bool TryFinalize();

  /// @brief Get a SoundHandle given its name as defined in its JSON data.
  ///
  /// @param name The unique name as defined in the JSON data.
  SoundHandle GetSoundHandle(const std::string& name) const;

  /// @brief Get a SoundHandle given its SoundCollectionDef filename.
  ///
  /// @param name The filename containing the flatbuffer binary data.
  SoundHandle GetSoundHandleFromFile(const std::string& filename) const;

  /// @brief Adjusts the gain on the master bus.
  ///
  /// @param master_gain the gain to apply to all buses.
  void set_master_gain(float master_gain);

  /// @brief Get the master bus's current gain.
  ///
  /// @return the master bus's current gain.
  float master_gain();

  /// @brief Mutes the AudioEngine completely.
  ///
  /// @param mute whether to mute or unmute the AudioEngine.
  void set_mute(bool mute);

  /// @brief Whether the AudioEngine is currently muted.
  ///
  /// @return Whether the AudioEngine is currently muted.
  bool mute();

  /// @brief Pauses all playing sounds and streams.
  ///
  /// @param pause Whether to pause or unpause the AudioEngine.
  void Pause(bool pause);

  /// @brief Initialize and return a Listener.
  ///
  /// @return An initialized Listener.
  Listener AddListener();

  /// @brief Remove a Listener.
  ///
  /// @param The Listener to be removed.
  void RemoveListener(Listener* listener);

  /// @brief Returns the named bus.
  ///
  /// @return The named bus.
  Bus FindBus(const char* bus_name);

  /// @brief Play a sound associated with the given sound_handle.
  ///
  /// @param sound_handle A handle to the sound to play.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(SoundHandle sound_handle);

  /// @brief Play a sound associated with the given sound_handle at the given
  ///        location.
  ///
  /// @param sound_handle A handle to the sound to play.
  /// @param location The location of the sound.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(SoundHandle sound_handle,
                    const mathfu::Vector<float, 3>& location);

  /// @brief Play a sound associated with the given sound_handle at the given
  ///        location with the given gain.
  ///
  /// @param sound_handle A handle to the sound to play.
  /// @param location The location of the sound.
  /// @param gain The gain of the sound.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(SoundHandle sound_handle,
                    const mathfu::Vector<float, 3>& location, float gain);

  /// @brief Play a sound associated with the given sound name.
  ///
  /// Note: Playing a sound with its SoundHandle is faster than using the sound
  /// name as using the name requires a map lookup internally.
  ///
  /// @param sound_name A handle to the sound to play.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(const std::string& sound_name);

  /// @brief Play a sound associated with the given sound name at the given
  ///        location.
  ///
  /// Note: Playing a sound with its SoundHandle is faster than using the sound
  /// name as using the name requires a map lookup internally.
  ///
  /// @param sound_name A handle to the sound to play.
  /// @param location The location of the sound.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(const std::string& sound_name,
                    const mathfu::Vector<float, 3>& location);

  /// @brief Play a sound associated with the given sound name at the given
  ///        location with the given gain.
  ///
  /// Note: Playing a sound with its SoundHandle is faster than using the sound
  /// name as using the name requires a map lookup internally.
  ///
  /// @param sound_name A handle to the sound to play.
  /// @param location The location of the sound.
  /// @param gain The gain of the sound.
  /// @return The channel the sound is played on. If the sound could not be
  ///         played, an invalid Channel is returned.
  Channel PlaySound(const std::string& sound_name,
                    const mathfu::Vector<float, 3>& location, float gain);

  /// @brief Get the version structure.
  ///
  /// @return The version string structure
  const PindropVersion* version() const;

  AudioEngineInternalState* state() { return state_; }

 private:
  AudioEngineInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_AUDIO_ENGINE_H_
