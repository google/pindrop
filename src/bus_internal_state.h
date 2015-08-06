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

#ifndef PINDROP_BUS_INTERNAL_STATE_H_
#define PINDROP_BUS_INTERNAL_STATE_H_

#include <vector>

#include "channel_internal_state.h"
#include "fplutil/intrusive_list.h"

namespace pindrop {

struct BusDef;

typedef fplutil::intrusive_list<ChannelInternalState> BusList;

class BusInternalState {
 public:
  BusInternalState()
      : bus_def_(nullptr),
        user_gain_(1.0f),
        target_user_gain_(1.0f),
        target_user_gain_step_(0.0f),
        duck_gain_(1.0f),
        playing_sound_list_(&ChannelInternalState::bus_node),
        transition_percentage_(0.0f) {}

  void Initialize(const BusDef* bus_def);

  // Return the bus definition.
  const BusDef* bus_def() const { return bus_def_; }

  // Return the final gain after all modifiers have been applied (parent gain,
  // duck gain, bus gain, user gain).
  float gain() const { return gain_; }

  // Set the user gain.
  void set_user_gain(const float user_gain) {
    user_gain_ = user_gain;
    target_user_gain_ = user_gain;
    target_user_gain_step_ = 0.0f;
  }

  // Return the user gain.
  float user_gain() const { return user_gain_; }

  // Fade to the given gain over duration seconds.
  void FadeTo(float gain, float duration);

  // Resets the duck gain to 1.0f. Duck gain must be reset each frame before
  // modifying it.
  void ResetDuckGain() { duck_gain_ = 1.0f; }

  // Return the vector of child buses.
  std::vector<BusInternalState*>& child_buses() { return child_buses_; }

  // Return the vector of duck buses, the buses to be ducked when a sound is
  // playing on this bus.
  std::vector<BusInternalState*>& duck_buses() { return duck_buses_; }

  // When a sound begins playing or finishes playing, the sound counter should
  // be incremented or decremented appropriately to track whether or not to
  // apply the duck gain.
  BusList& playing_sound_list() { return playing_sound_list_; }
  const BusList& playing_sound_list() const { return playing_sound_list_; }

  // Apply appropriate duck gain to all ducked buses.
  void UpdateDuckGain(float delta_time);

  // Recursively update the final gain of the bus.
  void AdvanceFrame(float delta_time, float parent_gain);

 private:
  const BusDef* bus_def_;

  // Children of a given bus have their gain multiplied against their parent's
  // gain.
  std::vector<BusInternalState*> child_buses_;

  // When a sound is played on this bus, sounds played on these buses should be
  // ducked.
  std::vector<BusInternalState*> duck_buses_;

  // The current user gain of this bus.
  float user_gain_;

  // The target user gain of this bus (used for fading).
  float target_user_gain_;

  // How much to adjust the gain per second while fading.
  float target_user_gain_step_;

  // The current duck_gain_ of this bus to be applied to all buses in
  // duck_buses_.
  float duck_gain_;

  // The final gain to be applied to all sounds on this bus.
  float gain_;

  // Keeps track of how many sounds are being played on this bus.
  BusList playing_sound_list_;

  // If a sound is playing on this bus, all duck_buses_ should lower in volume
  // over time. This tracks how far we are into that transition.
  float transition_percentage_;
};

}  // namespace pindrop

#endif  // PINDROP_BUS_INTERNAL_STATE_H_
