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

#include "SDL_mixer.h"
#include "file_loader.h"
#include "pindrop/log.h"
#include "real_channel.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

namespace pindrop {

static const int kLoopForever = -1;
static const int kPlayOnce = 0;
static const int kInvalidChannelId = -1;

#ifdef PINDROP_MULTISTREAM
void FreeFinishedMusicMultistream(void* /*userdata*/, Mix_Music* music,
                                  int /*channel*/) {
  Mix_FreeMusic(music);
}
#else
static int s_music_channel_id;

static Mix_Music* s_playing_music;

void FreeFinishedMusic() {
  Mix_FreeMusic(s_playing_music);
  s_playing_music = nullptr;
}
#endif  // PINDROP_MULTISTREAM

RealChannel::RealChannel() : channel_id_(kInvalidChannelId) {}

void RealChannel::Initialize(int i) { channel_id_ = i; }

bool RealChannel::Valid() const { return channel_id_ != kInvalidChannelId; }

bool RealChannel::Play(SoundCollection* collection, Sound* sound) {
  assert(Valid());
  const SoundCollectionDef* def = collection->GetSoundCollectionDef();
  int loops = def->loop() ? kLoopForever : kPlayOnce;
  stream_ = def->stream();

  // Play the audio using the appropriate Mix_Play* function.
  int result;
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    result = Mix_PlayMusicCh(Mix_LoadMUS(sound->filename().c_str()), loops,
                             channel_id);
#else
    s_music_channel_id = channel_id_;
    FreeFinishedMusic();
    result = Mix_PlayMusic(Mix_LoadMUS(sound->filename().c_str()), loops);
#endif
  } else {
    result = Mix_PlayChannel(channel_id_, sound->chunk(), loops);
  }

  // Check if playing the sound was successful, and display the error if it was
  // not.
  bool success = result != kInvalidChannelId;
  if (!success) {
    CallLogFunc("Could not play sound %s\n", Mix_GetError());
  }
  return success;
}

bool RealChannel::Playing() const {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    return Mix_PlayingMusicCh(channel_id_) != 0;
#else
    return Mix_PlayingMusic() != 0 && channel_id_ == s_music_channel_id;
#endif  // PINDROP_MULTISTREAM
  } else {
    return Mix_Playing(channel_id_) != 0;
  }
}

bool RealChannel::Paused() const {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    return Mix_PausedMusicCh(channel_id_) != 0;
#else
    return Mix_PausedMusic() != 0;
#endif  // PINDROP_MULTISTREAM
  } else {
    return Mix_Paused(channel_id_) != 0;
  }
}

void RealChannel::SetGain(const float gain) {
  assert(Valid());
  int mix_volume = static_cast<int>(gain * MIX_MAX_VOLUME);
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    Mix_VolumeMusicCh(channel_id_, mix_volume);
#else
    Mix_VolumeMusic(mix_volume);
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_Volume(channel_id_, mix_volume);
  }
}

float RealChannel::Gain() const {
  assert(Valid());
  // Special value to query volume rather than set volume.
  static const int kQueryVolume = -1;
  int volume;
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    volume = Mix_VolumeMusicCh(channel_id_, kQueryVolume);
#else
    volume = Mix_VolumeMusic(kQueryVolume);
#endif  // PINDROP_MULTISTREAM
  } else {
    volume = Mix_Volume(channel_id_, kQueryVolume);
  }
  return volume / static_cast<float>(MIX_MAX_VOLUME);
}

void RealChannel::Halt() {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    Mix_HaltMusicCh(channel_id_);
#else
    Mix_HaltMusic();
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_HaltChannel(channel_id_);
  }
}

void RealChannel::Pause() {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    Mix_PauseMusicCh(channel_id_);
#else
    Mix_PauseMusic();
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_Pause(channel_id_);
  }
}

void RealChannel::Resume() {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    Mix_ResumeMusicCh(channel_id_);
#else
    Mix_ResumeMusic();
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_Resume(channel_id_);
  }
}

void RealChannel::FadeOut(int milliseconds) {
  assert(Valid());
  if (stream_) {
#ifdef PINDROP_MULTISTREAM
    Mix_FadeOutMusicCh(channel_id_, milliseconds);
#else
    Mix_FadeOutMusic(milliseconds);
#endif  // PINDROP_MULTISTREAM
  } else {
    Mix_FadeOutChannel(channel_id_, milliseconds);
  }
}

void RealChannel::SetPan(const mathfu::Vector<float, 2>& pan) {
  assert(Valid());
  static const unsigned char kMaxPanValue = 255;
  if (!stream_) {
    // This formula is explained in the following paper:
    // http://www.rs-met.com/documents/tutorials/PanRules.pdf
    float p = static_cast<float>(M_PI) * (pan.x + 1.0f) / 4.0f;
    unsigned char left = static_cast<unsigned char>(cos(p) * kMaxPanValue);
    unsigned char right = static_cast<unsigned char>(sin(p) * kMaxPanValue);
    Mix_SetPanning(channel_id_, left, right);
  }
}

}  // namespace pindrop
