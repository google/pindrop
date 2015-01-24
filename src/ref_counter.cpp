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

#include "ref_counter.h"

#include <cassert>

namespace pindrop {

int RefCounter::Increment() {
  // TODO(amablue): b/18809543
  // Track the code locations that are responsible for holding a reference to
  // the object
  return ++count_;
}

int RefCounter::Decrement() {
  assert(count_);
  return --count_;
}

}  // namespace pindrop
