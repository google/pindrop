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

#include "mathfu/matrix.h"
#include "mathfu/matrix_4x4.h"
#include "mathfu/vector_3.h"

#define PINDROP_VERSION_MAJOR 1
#define PINDROP_VERSION_MINOR 0
#define PINDROP_VERSION_REVISION 0
#define PINDROP_STRING_EXPAND(X) #X
#define PINDROP_STRING(X) PINDROP_STRING_EXPAND(X)

/// @file pindrop/pindrop.h
/// @brief The public API consisting of the AudioEngine, Channel, and Listener
///        classes.

namespace pindrop {

class Channel;
class ChannelInternalState;
class ListenerInternalState;
class SoundCollection;
struct AudioConfig;
struct AudioEngineInternalState;

typedef SoundCollection* SoundHandle;
typedef size_t ListenerId;

/// @class Listener
///
/// @brief An object whose distance from sounds determines their gain.
///
/// The Listener class is a lightweight reference to a ListenerInternalState
/// which is managed by the AudioEngine. Multiple Listener objects may point to
/// the same underlying data.
class Listener {
 public:
  /// @brief Construct an uninitialized Listener.
  ///
  /// An uninitialized Listener can not have its location set or queried.
  ///
  /// To initialize the Listener, use <code>AudioEngine::AddListener();</code>
  Listener() : state_(nullptr) {}

  explicit Listener(ListenerInternalState* state) : state_(state) {}

  /// @brief Uninitializes this Listener.
  ///
  /// Uninitializes this Listener. Note that this does not destroy the internal
  /// state it references; it just removes this reference to it.
  /// To destroy the Listener, use <code>AudioEngine::RemoveListener();</code>
  void Clear();

  /// @brief Checks whether this Listener has been initialized.
  ///
  /// @return Returns true if this Listener is initialized.
  bool Valid() const;

  /// @brief Get the location of this Listener.
  ///
  /// @return The location of this Listener.
  mathfu::Vector<float, 3> Location() const;

  /// @brief Set the location of this Listener.
  ///
  /// @param location The new location of the Listener.
  void SetLocation(const mathfu::Vector<float, 3>& location);

  /// @brief Set the location, direction and up vector of this Listener.
  ///
  /// @param location The location of this listener.
  /// @param direction The direction this listener is facing.
  /// @param up The up vector of this listener.
  void SetOrientation(const mathfu::Vector<float, 3>& location,
                      const mathfu::Vector<float, 3>& direction,
                      const mathfu::Vector<float, 3>& up);

  /// @brief Set the location and orientation of this Listener using a matrix.
  ///
  /// @param matrix The matrix representating the location and orientation of
  ///               this listener.
  void SetMatrix(const mathfu::Matrix<float, 4>& matrix);

  /// @brief Get the matrix of this Listener.
  ///
  /// @return The matrix of this Listener.
  const mathfu::Matrix<float, 4>& Matrix() const;

  ListenerInternalState* state() { return state_; }

 private:
  ListenerInternalState* state_;
};

/// @class Channel
///
/// @brief An object that represents a single channel of audio.
///
/// The Channel class is a lightweight reference to a ChannelInternalState
/// which is managed by the AudioEngine. Multiple Channel objects may point to
/// the same underlying data.
class Channel {
 public:
  /// @brief Construct an uninitialized Listener.
  ///
  /// An uninitialized Construct can not have its location set or queried.
  ///
  /// To initialize the Channel, use <code>AudioEngine::PlaySound();</code>
  Channel() : state_(nullptr) {}

  explicit Channel(ChannelInternalState* state) : state_(state) {}

  /// @brief Uninitializes this Channel.
  ///
  /// Uninitializes this Channel. Note that this does not stop the audio or
  /// destroy the internal state it references; it just removes this reference
  /// to it. To stop the Channel use <code>Channel::Stop();</code>
  void Clear();

  /// @brief Checks whether this Channel has been initialized.
  bool Valid() const;

  /// @brief Checks if the sound playing on a given Channel is playing.
  ///
  /// @return Whether the Channel is currently playing.
  bool Playing() const;

  /// @brief Stop a channel.
  ///
  /// Stop this channel from playing. A sound will stop on its own if it not set
  /// to loop. Looped audio must be explicitly stopped.
  void Stop();

  /// @brief Get the location of this Channel.
  ///
  /// If the audio on this channel is not set to be Positional this property
  /// does nothing.
  ///
  /// @return The location of this Channel.
  const mathfu::Vector<float, 3> Location() const;

  /// @brief Set the location of this Channel.
  ///
  /// If the audio on this channel is not set to be Positional this property
  /// does nothing.
  ///
  /// @param location The new location of the Channel.
  void SetLocation(const mathfu::Vector<float, 3>& location);

 private:
  ChannelInternalState* state_;
};

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

  /// @brief Load a sound bank from a file.
  ///
  /// @param filename The file containing the SoundBank flatbuffer binary data.
  /// @return Returns true on success
  bool LoadSoundBank(const std::string& filename);

  /// @brief Unload a sound bank.
  ///
  /// @param filename The file to unload.
  void UnloadSoundBank(const std::string& filename);

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

  /// @brief Get the version string.
  ///
  /// @return The version string.
  const char* version_string() const;

  AudioEngineInternalState* state() { return state_; }

 private:
  AudioEngineInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_AUDIO_ENGINE_H_

