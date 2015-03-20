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

#ifndef PINDROP_SOUND_H_
#define PINDROP_SOUND_H_

#include "pindrop/audio_engine.h"
#include "channel_internal_state.h"

struct Mix_Chunk;
typedef struct _Mix_Music Mix_Music;

namespace pindrop {

struct AudioSampleSetEntry;

// SoundSource is a base class for both SoundStreams and SoundBuffers.
class SoundSource {
 public:
  explicit SoundSource(const AudioSampleSetEntry* entry)
      : audio_sample_set_entry_(entry) {}
  virtual ~SoundSource() {}

  // Load the sound from the given filename.
  virtual bool LoadFile(const char* filename) = 0;

  // Play this sound on the given channel, and loop if necessary.
  virtual bool Play(ChannelId channel_id, bool loop) = 0;

  const AudioSampleSetEntry& audio_sample_set_entry() {
    return *audio_sample_set_entry_;
  }

 private:
  const AudioSampleSetEntry* audio_sample_set_entry_;
};

// A SoundBuffer is a piece of buffered audio that is completely loaded into
// memory.
class SoundBuffer : public SoundSource {
 public:
  explicit SoundBuffer(const AudioSampleSetEntry* entry) : SoundSource(entry) {}
  virtual ~SoundBuffer();

  virtual bool LoadFile(const char* filename);

  virtual bool Play(ChannelId channel_id, bool loop);

 private:
  Mix_Chunk* data_;
};

// A SoundStream is audio that is streamed from disk rather than loaded into
// memory.
class SoundStream : public SoundSource {
 public:
  explicit SoundStream(const AudioSampleSetEntry* entry) : SoundSource(entry) {}
  virtual ~SoundStream();

  virtual bool LoadFile(const char* filename);

  virtual bool Play(ChannelId channel_id, bool loop);

 private:
  Mix_Music* data_;
};

}  // namespace pindrop

#endif  // PINDROP_SOUND_H_
