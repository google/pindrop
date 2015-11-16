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
#include "pindrop/pindrop.h"

namespace pindrop {

const int kLoopForever = -1;
const int kPlayOnce = 0;

#ifdef PINDROP_MULTISTREAM
void FreeFinishedMusicMultistream(void* /*userdata*/, Mix_Music* music,
                                  int /*channel*/) {
  Mix_FreeMusic(music);
}
#else
static Mix_Music* playing_music;

void FreeFinishedMusic() {
  Mix_FreeMusic(playing_music);
  playing_music = nullptr;
}
#endif  // PINDROP_MULTISTREAM

SoundBuffer::~SoundBuffer() {
  if (data_) {
    Mix_FreeChunk(data_);
  }
}

void SoundBuffer::Load() {
  data_ = Mix_LoadWAV(filename().c_str());
  if (data_ == nullptr) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load sound file %s.",
                 filename().c_str());
  }
}

bool SoundBuffer::Play(ChannelId channel_id, bool loop) {
  assert(data_ != nullptr);
  int loops = loop ? kLoopForever : kPlayOnce;
  return Mix_PlayChannel(channel_id, data_, loops) != kInvalidChannelId;
}

void SoundStream::Load() {}

bool SoundStream::Play(ChannelId channel_id, bool loop) {
  ChannelId result;
  int loops = loop ? kLoopForever : kPlayOnce;
#ifdef PINDROP_MULTISTREAM
  result = Mix_PlayMusicCh(Mix_LoadMUS(filename().c_str()), loops, channel_id);
#else
  (void)channel_id;
  FreeFinishedMusic();
  result = Mix_PlayMusic(Mix_LoadMUS(filename().c_str()), loops);
#endif
  return result != kInvalidChannelId;
}

}  // namespace pindrop
