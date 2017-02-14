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

#ifndef PINDROP_LISTENER_H_
#define PINDROP_LISTENER_H_

#include "mathfu/matrix.h"
#include "mathfu/matrix_4x4.h"
#include "mathfu/vector.h"

namespace pindrop {

class ListenerInternalState;

/// @class Listener
///
/// @brief An object whose distance from sounds determines their gain.
///
/// The Listener class is a lightweight reference to a ListenerInternalState
/// which is managed by the AudioEngine. Multiple Listener objects may point to
/// the same underlying data.
class Listener {
 public:
  /// @brief Construct an uninitialized Listener.
  ///
  /// An uninitialized Listener can not have its location set or queried.
  ///
  /// To initialize the Listener, use <code>AudioEngine::AddListener();</code>
  Listener() : state_(nullptr) {}

  explicit Listener(ListenerInternalState* state) : state_(state) {}

  /// @brief Uninitializes this Listener.
  ///
  /// Uninitializes this Listener. Note that this does not destroy the internal
  /// state it references; it just removes this reference to it.
  /// To destroy the Listener, use <code>AudioEngine::RemoveListener();</code>
  void Clear();

  /// @brief Checks whether this Listener has been initialized.
  ///
  /// @return Returns true if this Listener is initialized.
  bool Valid() const;

  /// @brief Get the location of this Listener.
  ///
  /// @return The location of this Listener.
  mathfu::Vector<float, 3> Location() const;

  /// @brief Set the location of this Listener.
  ///
  /// @param location The new location of the Listener.
  void SetLocation(const mathfu::Vector<float, 3>& location);

  /// @brief Set the location, direction and up vector of this Listener.
  ///
  /// @param location The location of this listener.
  /// @param direction The direction this listener is facing.
  /// @param up The up vector of this listener.
  void SetOrientation(const mathfu::Vector<float, 3>& location,
                      const mathfu::Vector<float, 3>& direction,
                      const mathfu::Vector<float, 3>& up);

  /// @brief Set the location and orientation of this Listener using a matrix.
  ///
  /// @param matrix The matrix representating the location and orientation of
  ///               this listener.
  void SetMatrix(const mathfu::Matrix<float, 4>& matrix);

  /// @brief Get the matrix of this Listener.
  ///
  /// @return The matrix of this Listener.
  const mathfu::Matrix<float, 4> Matrix() const;

  ListenerInternalState* state() { return state_; }

 private:
  ListenerInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_LISTENER_H_
