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

#include "sound.h"

#include "SDL_log.h"
#include "SDL_mixer.h"
#include "pindrop/audio_engine.h"

namespace pindrop {

const int kPlayStreamError = -1;
const int kLoopForever = -1;
const int kPlayOnce = 0;

SoundBuffer::~SoundBuffer() {
  if (data_) {
    Mix_FreeChunk(data_);
  }
}

bool SoundBuffer::LoadFile(const char* filename) {
  data_ = Mix_LoadWAV(filename);
  return data_ != nullptr;
}

bool SoundBuffer::Play(ChannelId channel_id, bool loop) {
  int loops = loop ? kLoopForever : kPlayOnce;
  return Mix_PlayChannel(channel_id, data_, loops) != kInvalidChannelId;

}

SoundStream::~SoundStream() {
  if (data_) {
    Mix_FreeMusic(data_);
  }
}

bool SoundStream::LoadFile(const char* filename) {
  data_ = Mix_LoadMUS(filename);
  return data_ != nullptr;
}

bool SoundStream::Play(ChannelId channel_id, bool loop) {
  (void)channel_id;  // SDL_mixer does not currently support
                     // more than one channel of streaming audio.
  int loops = loop ? kLoopForever : kPlayOnce;
  return Mix_PlayMusic(data_, loops) != kPlayStreamError;

}

}  // namespace pindrop
