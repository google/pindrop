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

#ifndef PINDROP_ASYNCHRONOUS_LOADER_FILE_LOADER_H_
#define PINDROP_ASYNCHRONOUS_LOADER_FILE_LOADER_H_

#include "fplbase/async_loader.h"

namespace pindrop {

class FileLoader;

class Resource : public fplbase::AsyncAsset {
 public:
  virtual ~Resource() {}

  void LoadFile(const char* filename, FileLoader* loader);

 private:
  virtual bool Finalize() { return true; };
  virtual bool IsValid() { return true; };
};

class FileLoader {
 public:
  void StartLoading();

  bool TryFinalize();

  void QueueJob(Resource* resource);

 private:
  fplbase::AsyncLoader loader;
};

}  // namespace pindrop

#endif  // PINDROP_ASYNCHRONOUS_LOADER_FILE_LOADER_H_
