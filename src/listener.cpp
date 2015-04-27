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

void Listener::SetOrientation(const mathfu::Vector<float, 3>& location,
                              const mathfu::Vector<float, 3>& direction,
                              const mathfu::Vector<float, 3>& up) {
  state_->set_matrix(
      mathfu::Matrix<float, 4>::LookAt(location + direction, location, up));
}

mathfu::Vector<float, 3> Listener::Location() const {
  return state_->matrix().Inverse().TranslationVector3D();
}

void Listener::SetLocation(const mathfu::Vector<float, 3>& location) {
  state_->set_matrix(
      mathfu::Matrix<float, 4>::FromTranslationVector(-location));
}

void Listener::SetMatrix(const mathfu::Matrix<float, 4>& matrix) {
  assert(Valid());
  state_->set_matrix(matrix);
}

const mathfu::Matrix<float, 4>& Listener::Matrix() const {
  return state_->matrix();
}

}  // namespace pindrop

