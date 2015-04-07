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
#include "mathfu/vector_3.h"
#include "pindrop/pindrop.h"

namespace pindrop {

class SoundSource;

typedef int ChannelId;

// Special value representing an invalid stream.
const ChannelId kInvalidChannelId = -1;

// Represents a sample that is playing on a channel.
class ChannelInternalState {
 public:
  ChannelInternalState()
      : handle_(nullptr),
        channel_id_(0),
        location_(),
        priority_node_(),
        bus_node_() {}

  bool IsStream() const;

  void Remove();

  void SetHandle(SoundHandle handle);
  SoundHandle handle() const { return handle_; }

  void set_channel_id(ChannelId channel_id) { channel_id_ = channel_id; }
  ChannelId channel_id() const { return channel_id_; }

  void SetLocation(const mathfu::Vector<float, 3>& location) {
    location_ = location;
  }
  mathfu::Vector<float, 3> Location() const {
    return mathfu::Vector<float, 3>(location_);
  }

  // Play a sound on this channel.
  bool Play(SoundSource* source, bool loop);

  // Check if this channel is currently playing audio.
  bool Playing() const;

  // Set and query the current gain of this channel.
  void SetGain(const float gain);
  float Gain() const;

  // Immediately stop the audio. May cause clicking.
  void Halt();

  // Fade out over the specified number of milliseconds.
  void FadeOut(int milliseconds);

  float Priority() const;

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(priority_node_, priority_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(ChannelInternalState,
                                                 priority_node_,
                                                 GetInstanceFromPriorityNode);

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(bus_node_, bus_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(ChannelInternalState,
                                                 bus_node_,
                                                 GetInstanceFromBusNode);

  static void PauseAll();
  static void ResumeAll();

 private:
  SoundHandle handle_;
  ChannelId channel_id_;

  mathfu::VectorPacked<float, 3> location_;

  IntrusiveListNode priority_node_;
  IntrusiveListNode bus_node_;
};

}  // namespace pindrop

#endif  // PINDROP_CHANNEL_INTERNAL_STATE_H_

