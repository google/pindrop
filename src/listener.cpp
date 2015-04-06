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

#include "listener_internal_state.h"
#include "pindrop/pindrop.h"

namespace pindrop {

void Listener::Clear() { state_ = nullptr; }

bool Listener::Valid() const { return state_ != nullptr && state_->InList(); }

mathfu::Vector<float, 3> Listener::Location() const {
  assert(Valid());
  return state_->Location();
}

void Listener::SetLocation(const mathfu::Vector<float, 3>& location) {
  assert(Valid());
  state_->SetLocation(location);
}

}  // namespace pindrop

