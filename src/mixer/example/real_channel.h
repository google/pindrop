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

#ifndef PINDROP_MIXER_EXAMPLE_REAL_CHANNEL_H_
#define PINDROP_MIXER_EXAMPLE_REAL_CHANNEL_H_

#include "mathfu/vector.h"

namespace pindrop {

class SoundCollection;
class Sound;

// A RealChannel represents a channel of audio on the mixer.
//
// Not all Pindrop channels are backed by RealChannels. If there are more
// channels of audio being played simultaneously than the mixer can handle,
// the lowest priority channels will be virtualized. That is, they will no
// longer have their audio mixed, but their gain value and position (and a few
// other properties) will continue to be tracked.
//
// This class represents the real channel interface to the underlying audio
// mixer backend being used.
class RealChannel {
 public:
  // Initialize this channel.
  void Initialize(int index);

  // Play the audio on the real channel.
  bool Play(SoundCollection* handle, Sound* sound);

  // Halt the real channel so it may be re-used. However this virtual channel
  // may still be considered playing.
  void Halt();

  // Pause the real channel.
  void Pause();

  // Resume the paused real channel.
  void Resume();

  // Check if this channel is currently playing on a real channel.
  bool Playing() const;

  // Check if this channel is currently paused on a real channel.
  bool Paused() const;

  // Set the current gain of the real channel.
  void SetGain(float gain);

  // Get the current gain of the real channel.
  float Gain() const;

  // Set the pan for the sound. This should be a unit vector.
  void SetPan(const mathfu::Vector<float, 2>& pan);

  // Fade this channel out over the given number of milliseconds.
  void FadeOut(int milliseconds);

  // Return true if this is a valid real channel.
  bool Valid() const;
};

}  // namespace pindrop

#endif  // PINDROP_MIXER_EXAMPLE_REAL_CHANNEL_H_
