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
  ListenerInternalState() : matrix_(mathfu::Matrix<float, 4>::Identity()) {}

  void set_matrix(const mathfu::Matrix<float, 4>& matrix) { matrix_ = matrix; }

  mathfu::Matrix<float, 4>& matrix() { return matrix_; }
  const mathfu::Matrix<float, 4>& matrix() const { return matrix_; }

 private:
  mathfu::Matrix<float, 4> matrix_;
};

}  // namespace pindrop

