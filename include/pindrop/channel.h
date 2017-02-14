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

#ifndef PINDROP_CHANNEL_H_
#define PINDROP_CHANNEL_H_

#include "mathfu/matrix.h"
#include "mathfu/matrix_4x4.h"
#include "mathfu/vector.h"

namespace pindrop {

class ChannelInternalState;

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

  /// @brief Pause a channel.
  ///
  /// Pause this channel. A paused channel may be resumed where it left off.
  void Pause();

  /// @brief Resumes a paused channel.
  ///
  /// Resume this channel. If this channel was paused it will continue where it
  /// left off.
  void Resume();

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

  /// @brief Sets the gain on this chanel.
  ///
  /// @param gain The new gain value.
  void SetGain(float gain);

  /// @brief Returns the gain on this chanel.
  ///
  /// @return Returns the gain.
  float Gain() const;

 private:
  ChannelInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_CHANNEL_H_
