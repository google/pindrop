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

/// @file pindrop/log.h
///
/// @brief This file contains functions to register and call logging function.
///        The log function is used for various debug logging.

#ifndef PINDROP_LOG_H_
#define PINDROP_LOG_H_

#include <cstdarg>

namespace pindrop {

/// @typedef LogFunc
///
/// @brief The function signature of the logging function.
///
/// In order to perform logging, the library needs to be provided with a logging
/// function that fits this type signature.
typedef void (*LogFunc)(const char* fmt, va_list args);

/// @brief Register a logging function with the library.
///
/// @param[in] The function to use for logging.
void RegisterLogFunc(LogFunc log_func);

/// @brief Call the registered log function with the provided format string.
///
/// This does nothing if no logging function has been registered.
///
/// @param[in] format The format string to print.
/// @param[in] ... The arguments to format.
void CallLogFunc(const char* format, ...);

}  // namespace pindrop

#endif  // PINDROP_LOG_H_
