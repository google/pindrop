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

#ifndef PINDROP_PLAYING_SOUND_H_
#define PINDROP_PLAYING_SOUND_H_

#include "pindrop/audio_engine.h"
#include "intrusive_list.h"

namespace pindrop {

// Represents a sample that is playing on a channel.
class PlayingSound {
 public:
  PlayingSound()
      : handle_(nullptr), channel_id_(0), priority_node_(), bus_node_() {}

  void Clear();

  void SetHandle(AudioEngine::SoundHandle handle);
  AudioEngine::SoundHandle handle() const { return handle_; }

  void set_channel_id(AudioEngine::ChannelId channel_id) {
    channel_id_ = channel_id;
  }
  AudioEngine::ChannelId channel_id() const { return channel_id_; }

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(priority_node_, priority_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(PlayingSound, priority_node_,
                                                 GetInstanceFromPriorityNode);

  PINDROP_INTRUSIVE_GET_NODE_ACCESSOR(bus_node_, bus_node);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(PlayingSound, bus_node_,
                                                 GetInstanceFromBusNode);

 private:
  AudioEngine::SoundHandle handle_;
  AudioEngine::ChannelId channel_id_;

  IntrusiveListNode priority_node_;
  IntrusiveListNode bus_node_;
};

}  // namespace pindrop

#endif  // PINDROP_PLAYING_SOUND_H_

