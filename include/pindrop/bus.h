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

#ifndef PINDROP_BUS_H_
#define PINDROP_BUS_H_

namespace pindrop {

class BusInternalState;

/// @class Bus
///
/// @brief An object representing one in a tree of buses. Buses are used to
///        adjust a set of channel gains in tandem.
///
/// The Bus class is a lightweight reference to a BusInternalState which is
/// managed by the AudioEngine. There is always at least one bus, the master
/// bus, and any number of additional buses may be defined as well. Each bus can
/// be thought of as a node in a tree. The gain on a Bus is applied to all child
/// buses as well.
class Bus {
 public:
  /// @brief Construct an uninitialized Bus.
  ///
  /// An uninitialized Bus can not set or get any of it's fields.
  ///
  /// To initialize the Listener, use <code>AudioEngine::AddListener();</code>
  Bus() : state_(nullptr) {}

  explicit Bus(BusInternalState* state) : state_(state) {}

  /// @brief Uninitializes this Bus.
  ///
  /// Uninitializes this Bus. Note that this does not destroy the internal
  /// state it references; it just removes this reference to it.
  void Clear();

  /// @brief Checks whether this Bus has been initialized.
  ///
  /// @return Returns true if this Bus is initialized.
  bool Valid() const;

  /// @brief Sets the gain on this Bus.
  ///
  /// @param gain The new gain value.
  void SetGain(float gain);

  /// @brief Returns the user specified gain on this bus.
  ///
  /// @return Returns the user specified gain.
  float Gain() const;

  /// @brief Fades to <code>gain</code> over <code>duration</code> seconds.
  //
  /// @param gain The gain value to fade to.
  /// @param duration The amount of time to take to reach the target gain.
  void FadeTo(float gain, float duration);

  /// @brief Returns the final calculated gain on this bus.
  ///
  /// The FinalGain of a bus is the product of the gain specified in the Bus
  /// definition file, the gain specified by the user, and the final gain of
  /// this bus's parent bus.
  ///
  /// @return Returns the gain.
  float FinalGain() const;

  BusInternalState* state() { return state_; }

 private:
  BusInternalState* state_;
};

}  // namespace pindrop

#endif  // PINDROP_BUS_H_
