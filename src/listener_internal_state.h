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

#include "intrusive_list.h"
#include "mathfu/constants.h"
#include "mathfu/vector_3.h"
#include "mathfu/matrix_4x4.h"

namespace pindrop {

class ListenerInternalState
    : public TypedIntrusiveListNode<ListenerInternalState> {
 public:
  ListenerInternalState()
      : inverse_matrix_(mathfu::Matrix<float, 4>::Identity()) {}

  void set_inverse_matrix(const mathfu::Matrix<float, 4>& inverse_matrix) {
    inverse_matrix_ = inverse_matrix;
  }

  mathfu::Matrix<float, 4>& inverse_matrix() { return inverse_matrix_; }
  const mathfu::Matrix<float, 4>& inverse_matrix() const {
    return inverse_matrix_;
  }

 private:
  // We use an inverse matrix here rather than a regular matrix because the
  // inverse matrix is used to translate sounds into listener space, and
  // calculating the matrix every time would be wasteful.
  mathfu::Matrix<float, 4> inverse_matrix_;
};

}  // namespace pindrop
