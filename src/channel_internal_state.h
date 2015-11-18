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

#ifndef PINDROP_CHANNEL_INTERNAL_STATE_H_
#define PINDROP_CHANNEL_INTERNAL_STATE_H_

#include "intrusive_list.h"
#include "mathfu/vector_2.h"
#include "mathfu/vector_3.h"
#include "pindrop/pindrop.h"

namespace pindrop {

class SoundSource;

typedef int ChannelId;

// Special value representing an invalid stream.
const ChannelId kInvalidChannelId = -1;

enum ChannelState {
  kChannelStateStopped,
  kChannelStatePlaying,
  kChannelStateFadingOut,
  kChannelStatePaused,
};

// Represents a sample that is playing on a channel.
class ChannelInternalState {
 public:
  ChannelInternalState()
      : channel_id_(0),
        channel_state_(kChannelStateStopped),
        handle_(nullptr),
        sound_source_(nullptr),
        location_(),
        priority_node_(),
        bus_node_() {}

  // Updates the state enum based on whether this channel is stopped, playing,
  // etc.
  void UpdateState();

  // Returns true if this channel holds streaming data.
  bool IsStream() const;

  // Remove this channel from all lists that it is a part of.
  void Remove();

  // Get or set the sound handle playing on this channel. Note that when you set
  // the sound handle, you also add this channel to the bus list that
  // corresponds to that sound collection.
  void SetHandle(SoundHandle handle);
  SoundHandle handle() const { return handle_; }

  // Get or set the channel ID. Channel ID's greater than 0 represent real
  // channels.
  void set_channel_id(ChannelId channel_id) { channel_id_ = channel_id; }
  ChannelId channel_id() const { return channel_id_; }

  // Returns true if this is a real channel.
  bool is_real() const { return channel_id_ != kInvalidChannelId; }

  // Get the current state of this channel (playing, stopped, paused, etc). This
  // is tracked manually because not all ChannelInternalStates are backed by
  // real channels.
  ChannelState channel_state() const { return channel_state_; }

  // Get or set the location of this channel
  void SetLocation(const mathfu::Vector<float, 3>& location) {
    location_ = location;
  }
  mathfu::Vector<float, 3> Location() const {
    return mathfu::Vector<float, 3>(location_);
  }

  // Play a sound on this channel.
  bool Play(SoundHandle handle);

  // Check if this channel is currently playing on a real or virtual channel.
  bool Playing() const;

  // Check if this channel is currently stopped on a real or virtual channel.
  bool Stopped() const;

  // Check if this channel is currently paused on a real or virtual channel.
  bool Paused() const;

  // Set and query the user gain of this channel.
  void set_user_gain(const float user_gain) { user_gain_ = user_gain; }
  float user_gain() const { return user_gain_; }

  // Set and query the current gain of this channel.
  void set_gain(const float gain) { gain_ = gain; }
  float gain() const { return gain_; }

  // Immediately stop the audio. May cause clicking.
  void Halt();

  // Pauses this channel.
  void Pause();

  // Resumes this channel if it is paused.
  void Resume();

  // Fade out over the specified number of milliseconds.
  void FadeOut(int milliseconds);

  // Sets the pan based on a position in a unit circle.
  void SetPan(const mathfu::Vector<float, 2>& pan);

  // Devirtualizes a virtual channel. This transfers ownership of the given
  // channel's channel_id to this channel.
  void Devirtualize(ChannelInternalState* other);

  // Returns the priority of this channel based on its gain and priority
  // multiplier on the sound collection definition.
  float Priority() const;

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(priority_node_, priority_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(ChannelInternalState,
                                                 priority_node_,
                                                 GetInstanceFromPriorityNode);

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(free_node_, free_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(ChannelInternalState,
                                                 free_node_,
                                                 GetInstanceFromFreeNode);

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(bus_node_, bus_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(ChannelInternalState,
                                                 bus_node_,
                                                 GetInstanceFromBusNode);

  // TODO: Move RealChannel functions to a separate object.
  // Play the audio on the real channel.
  bool RealChannelPlay();

  // Halt the real channel so it may be re-used. However this virtual channel
  // may still be considered playing.
  void RealChannelHalt();

  // Pause the real channel.
  void RealChannelPause();

  // Resume the paused real channel.
  void RealChannelResume();

  // Check if this channel is currently playing on a real channel.
  bool RealChannelPlaying() const;

  // Check if this channel is currently paused on a real channel.
  bool RealChannelPaused() const;

  // Set and query the current gain of the real channel.
  void SetRealChannelGain(float gain);
  float RealChannelGain() const;

 private:
  ChannelId channel_id_;

  // Whether this channel is currently playing, stopped, fading out, etc.
  ChannelState channel_state_;

  // The handle of the sound being played on this channel.
  SoundHandle handle_;

  // The sound source that was chosen from the sound collection.
  SoundSource* sound_source_;

  // The gain set by the user.
  float user_gain_;

  // The gain of this channel.
  float gain_;

  // The location of this channel's sound.
  mathfu::VectorPacked<float, 3> location_;

  // The node that tracks the location in the priority list.
  IntrusiveListNode priority_node_;

  // The node that tracks the location in the free list.
  IntrusiveListNode free_node_;

  // The node that tracks the list of sounds playing on a given bus.
  IntrusiveListNode bus_node_;
};

}  // namespace pindrop

#endif  // PINDROP_CHANNEL_INTERNAL_STATE_H_

