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

#include "channel_internal_state.h"

#include "bus.h"
#include "intrusive_list.h"
#include "pindrop/audio_engine.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

void ChannelInternalState::Clear() {
  handle_ = nullptr;
  priority_node_.Remove();
  bus_node_.Remove();
}

void ChannelInternalState::SetHandle(SoundHandle handle) {
  if (handle_ && handle_->bus()) {
    bus_node_.Remove();
  }
  handle_ = handle;
  if (handle_ && handle_->bus()) {
    handle_->bus()->playing_sound_list().InsertAfter(&bus_node_);
  }
}

float ChannelInternalState::Priority() const {
  assert(handle_);
  return gain_ * handle_->GetSoundCollectionDef()->priority();
}

}  // namespace pindrop

