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

#include "pindrop/log.h"

namespace pindrop {

LogFunc g_log_func;

// Register a logging function with the library.
void RegisterLogFunc(LogFunc log_func) { g_log_func = log_func; }

// Call the registered log function with the provided format string. This does
// nothing if no logging function has been registered.
void CallLogFunc(const char* format, ...) {
  if (g_log_func) {
    va_list args;
    va_start(args, format);
    g_log_func(format, args);
    va_end(args);
  }
}

}  // namespace pindrop
