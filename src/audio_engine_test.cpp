// Copyright (c) 2014 Google, Inc.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <vector>

#include "SDL_mixer.h"
#include "audio_engine_internal_state.h"
#include "gtest/gtest.h"
#include "intrusive_list.h"
#include "pindrop/audio_engine.h"
#include "playing_sound.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

// Stubs for SDL_mixer functions which are not actually part of the tests being
// run.
extern "C" {
Mix_Chunk* Mix_LoadWAV_RW(SDL_RWops*, int) { return NULL; }
Mix_Music* Mix_LoadMUS(const char*) { return NULL; }
int Mix_AllocateChannels(int) { return 0; }
int Mix_HaltChannel(int) { return 0; }
int Mix_HaltMusic() { return 0; }
int Mix_Init(int) { return 0; }
int Mix_OpenAudio(int, Uint16, int, int) { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk*, int, int) { return 0; }
int Mix_FadeOutChannel(int, int) { return 0; }
int Mix_PlayMusic(Mix_Music*, int) { return 0; }
int Mix_Playing(int) { return 0; }
int Mix_PlayingMusic() { return 0; }
int Mix_Volume(int, int) { return 0; }
int Mix_VolumeMusic(int) { return 0; }
void Mix_CloseAudio() {}
void Mix_FreeChunk(Mix_Chunk*) {}
void Mix_FreeMusic(Mix_Music*) {}
void Mix_Pause(int) {}
void Mix_PauseMusic() {}
int Mix_FadeOutMusic(int) { return 0; }
void Mix_Resume(int) {}
void Mix_ResumeMusic() {}
}

namespace pindrop {

bool LoadFile(const char*, std::string*) { return false; }

class AudioEngineTests : public ::testing::Test {
 protected:
  AudioEngineTests() : collections_(), list_(), sounds_() {}
  virtual void SetUp() {
    // Make a bunch of sound defs with various priorities.
    for (uint16_t i = 0; i < kCollectionCount; ++i) {
      pindrop::SoundCollection& collection = collections_[i];

      flatbuffers::FlatBufferBuilder builder;
      auto name = builder.CreateString(std::to_string(i));
      float priority = static_cast<float>(i);
      auto sound_def_buffer =
          pindrop::CreateSoundCollectionDef(builder, name, priority);
      builder.Finish(sound_def_buffer);
      collection.LoadSoundCollectionDef(
          std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                      builder.GetSize()),
          nullptr);
    }

    list_.InsertAfter(&sounds_[0]);
    sounds_[0].InsertAfter(&sounds_[1]);
    sounds_[1].InsertAfter(&sounds_[2]);
    sounds_[2].InsertAfter(&sounds_[3]);

    sounds_[0].SetHandle(&collections_[2]);
    sounds_[1].SetHandle(&collections_[1]);
    sounds_[2].SetHandle(&collections_[1]);
    sounds_[3].SetHandle(&collections_[0]);
  }
  virtual void TearDown() {}

 protected:
  static const std::size_t kCollectionCount = 3;
  SoundCollection collections_[kCollectionCount];

  TypedIntrusiveListNode<PlayingSound> list_;
  static const std::size_t kSoundCount = 4;
  PlayingSound sounds_[kSoundCount];
};

TEST_F(AudioEngineTests, FindInsertionPointAtHead) {
  // New sound's priority is greater than highest priority.
  EXPECT_EQ(list_.GetTerminator(), FindInsertionPoint(&list_, 2.5f));
}

TEST_F(AudioEngineTests, FindInsertionPointWithEqualPriority) {
  // New sound's priority is...
  // ...equal to highest priority.
  EXPECT_EQ(&sounds_[0], FindInsertionPoint(&list_, 2.0f));
  // ...equal to a sound with the same priority as the previous sound.
  EXPECT_EQ(&sounds_[2], FindInsertionPoint(&list_, 1.0f));
  // ...equal to the lowest priority.
  EXPECT_EQ(&sounds_[3], FindInsertionPoint(&list_, 0));
}

TEST_F(AudioEngineTests, FindInsertionPointMiddlePriority) {
  // New sound's priority is...
  // ...between highest and second highest.
  EXPECT_EQ(&sounds_[0], FindInsertionPoint(&list_, 1.5f));
  // ...lower than a sound with the same priority as the previous sound.
  EXPECT_EQ(&sounds_[2], FindInsertionPoint(&list_, 0.5));
}

TEST_F(AudioEngineTests, FindInsertionPointLowest) {
  // New sound's priority is less than the lowest prioity.
  EXPECT_EQ(&sounds_[3], FindInsertionPoint(&list_, -1.0f));
}

}  // namespace pindrop

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
