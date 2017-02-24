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

#include <cmath>
#include <vector>

#include "SDL_mixer.h"
#include "audio_engine_internal_state.h"
#include "channel_internal_state.h"
#include "fplutil/intrusive_list.h"
#include "gtest/gtest.h"
#include "listener_internal_state.h"
#include "pindrop/pindrop.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

// Stubs for SDL_mixer functions which are not actually part of the tests being
// run.
extern "C" {
Mix_Chunk* Mix_LoadWAV_RW(SDL_RWops*, int) { return NULL; }
Mix_Music* Mix_LoadMUS(const char*) { return NULL; }
int Mix_AllocateChannels(int) { return 0; }
int Mix_FadeOutChannel(int, int) { return 0; }
int Mix_HaltChannel(int) { return 0; }
int Mix_Init(int) { return 0; }
int Mix_OpenAudio(int, Uint16, int, int) { return 0; }
int Mix_Paused(int) { return 0; }
int Mix_Playing(int) { return 0; }
int Mix_SetPanning(int, Uint8, Uint8) { return 0; }
int Mix_Volume(int, int) { return MIX_MAX_VOLUME; }
void Mix_CloseAudio() {}
void Mix_FreeChunk(Mix_Chunk*) {}
void Mix_FreeMusic(Mix_Music*) {}
#ifdef PINDROP_MULTISTREAM
int Mix_FadeOutMusicCh(int, int) { return 0; }
int Mix_HaltMusicCh(int) { return 0; }
int Mix_PausedMusicCh(int) { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk*, int, int, int) { return 0; }
int Mix_PlayMusicCh(Mix_Music*, int, int) { return 0; }
int Mix_PlayingMusicCh(int) { return 0; }
int Mix_VolumeMusicCh(int, int) { return MIX_MAX_VOLUME; }
void Mix_HookMusicFinishedCh(void*, void (*)(void* userdata, Mix_Music* music,
                                             int channel)) {}
void Mix_PauseMusicCh(int) {}
void Mix_ResumeMusicCh(int) {}
#else
int Mix_FadeOutMusic(int) { return 0; }
int Mix_HaltMusic() { return 0; }
int Mix_PausedMusic() { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk*, int, int) { return 0; }
int Mix_PlayMusic(Mix_Music*, int) { return 0; }
int Mix_PlayingMusic() { return 0; }
int Mix_VolumeMusic(int) { return MIX_MAX_VOLUME; }
void Mix_HookMusicFinished(void (*)(void)) {}
void Mix_Pause(int) {}
void Mix_PauseMusic() {}
void Mix_Resume(int) {}
void Mix_ResumeMusic() {}
#endif  // PINDROP_MULTISTREAM
}

static const float kEpsilon = 0.001f;

namespace pindrop {

class ChannelInternalStatePriorityTests : public ::testing::Test {
 protected:
  ChannelInternalStatePriorityTests()
      : collections_(),
        list_(&ChannelInternalState::priority_node),
        channels_() {}
  virtual void SetUp() {
    // Make a bunch of sound defs with various priorities.
    for (size_t i = 0; i < static_cast<unsigned int>(kCollectionCount); ++i) {
      pindrop::SoundCollection& collection = collections_[i];

      flatbuffers::FlatBufferBuilder builder;
      auto name = builder.CreateString("");
      float priority = static_cast<float>(i);
      auto sound_def_buffer =
          pindrop::CreateSoundCollectionDef(builder, name, priority);
      builder.Finish(sound_def_buffer);
      collection.LoadSoundCollectionDef(
          std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                      builder.GetSize()),
          nullptr);
    }

    list_.push_back(channels_[0]);
    list_.push_back(channels_[1]);
    list_.push_back(channels_[2]);
    list_.push_back(channels_[3]);

    channels_[0].SetSoundCollection(&collections_[2]);
    channels_[1].SetSoundCollection(&collections_[1]);
    channels_[2].SetSoundCollection(&collections_[1]);
    channels_[3].SetSoundCollection(&collections_[0]);

    channels_[0].set_gain(1.0f);
    channels_[1].set_gain(1.0f);
    channels_[2].set_gain(1.0f);
    channels_[3].set_gain(1.0f);
  }
  virtual void TearDown() {}

 protected:
  static const std::size_t kCollectionCount = 3;
  SoundCollection collections_[kCollectionCount];

  PriorityList list_;
  static const std::size_t kSoundCount = 4;
  ChannelInternalState channels_[kSoundCount];
};

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointAtHead) {
  // New sound's priority is greater than highest priority.
  EXPECT_EQ(&channels_[0], &*FindInsertionPoint(&list_, 2.5f));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointWithEqualPriority) {
  // New sound's priority is...
  // ...equal to highest priority.
  EXPECT_EQ(&channels_[0], &*FindInsertionPoint(&list_, 2.0f));
  // ...equal to a sound with the same priority as the previous sound.
  EXPECT_EQ(&channels_[1], &*FindInsertionPoint(&list_, 1.0f));
  // ...equal to the lowest priority.
  EXPECT_EQ(&channels_[3], &*FindInsertionPoint(&list_, 0));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointMiddlePriority) {
  // New sound's priority is...
  // ...between highest and second highest.
  EXPECT_EQ(&channels_[1], &*FindInsertionPoint(&list_, 1.5f));
  // ...lower than a sound with the same priority as the previous sound.
  EXPECT_EQ(&channels_[3], &*FindInsertionPoint(&list_, 0.5));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointLowest) {
  // New sound's priority is less than the lowest prioity.
  EXPECT_EQ(list_.end(), FindInsertionPoint(&list_, -1.0f));
}

class BestListenerTests : public ::testing::Test {
 public:
  MATHFU_DEFINE_CLASS_SIMD_AWARE_NEW_DELETE

 protected:
  BestListenerTests()
      : listener_list_(&ListenerInternalState::node), listeners_(), listener_(), distance_squared_() {}
  virtual void SetUp() {
    listener_list_.push_back(listeners_[0]);
    listener_list_.push_back(listeners_[1]);
    listener_list_.push_back(listeners_[2]);
    listener_list_.push_back(listeners_[3]);

    Listener listener;
    listener = Listener(&listeners_[0]);
    listener.SetLocation(mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f));
    listener = Listener(&listeners_[1]);
    listener.SetLocation(mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f));
    listener = Listener(&listeners_[2]);
    listener.SetLocation(mathfu::Vector<float, 3>(10.0f, 0.0f, 10.0f));
    listener = Listener(&listeners_[3]);
    listener.SetLocation(mathfu::Vector<float, 3>(0.0f, 0.0f, 10.0f));
  }
  virtual void TearDown() {}

 protected:
  ListenerList listener_list_;
  ListenerInternalState listeners_[4];

  ListenerList::const_iterator listener_;
  float distance_squared_;
  mathfu::Vector<float, 3> transformed_location_;
};

// Location overlaps with listener.
TEST_F(BestListenerTests, OverlapsWithListener) {
  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f)));
  EXPECT_EQ(0.0f, distance_squared_);
  EXPECT_EQ(&listeners_[0], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f)));
  EXPECT_EQ(0.0f, distance_squared_);
  EXPECT_EQ(&listeners_[1], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(10.0f, 0.0f, 10.0f)));
  EXPECT_EQ(0.0f, distance_squared_);
  EXPECT_EQ(&listeners_[2], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(0.0f, 0.0f, 10.0f)));
  EXPECT_EQ(0.0f, distance_squared_);
  EXPECT_EQ(&listeners_[3], &*listener_);
}

// Nearest listener_ is 10 units away.
TEST_F(BestListenerTests, NearestIsTenUnitsAway) {
  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(0.0f, 10.0f, 10.0f)));
  EXPECT_EQ(100.0f, distance_squared_);
  EXPECT_EQ(&listeners_[3], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(20.0f, 0.0f, 10.0f)));
  EXPECT_EQ(100.0f, distance_squared_);
  EXPECT_EQ(&listeners_[2], &*listener_);
}

// Location is in the center of multiple listeners.
TEST_F(BestListenerTests, EquidistantListeners) {
  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(5.0f, 0.0f, 5.0f)));
  EXPECT_EQ(50.0f, distance_squared_);
}

// Location is close to one listener.
TEST_F(BestListenerTests, SingleClosestListener) {
  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(1.0f, 0.0f, 1.0f)));
  EXPECT_EQ(2.0f, distance_squared_);
  EXPECT_EQ(&listeners_[0], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(8.0f, 0.0f, 2.0f)));
  EXPECT_EQ(8.0f, distance_squared_);
  EXPECT_EQ(&listeners_[1], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(7.0f, 0.0f, 7.0f)));
  EXPECT_EQ(18.0f, distance_squared_);
  EXPECT_EQ(&listeners_[2], &*listener_);

  EXPECT_TRUE(BestListener(&listener_, &distance_squared_,
                           &transformed_location_, listener_list_,
                           mathfu::Vector<float, 3>(4.0f, 0.0f, 6.0f)));
  EXPECT_EQ(32.0f, distance_squared_);
  EXPECT_EQ(&listeners_[3], &*listener_);
}

TEST(CalculateDistanceAttenuation, RollOutCentered) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Positional);
  builder.add_roll_out_curve_factor(1.0f);
  builder.add_gain(1.0f);
  builder.add_max_audible_radius(10.0f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculateDistanceAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(100.0f, 2.0f), def));
}

TEST(CalculateDistanceAttenuation, RollOutOffCenter) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Positional);
  builder.add_gain(1.0f);
  builder.add_roll_out_curve_factor(1.0f);
  builder.add_roll_out_radius(10.0f);
  builder.add_max_audible_radius(20.0f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculateDistanceAttenuation(std::pow(15.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(20.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(100.0f, 2.0f), def));
}

TEST(CalculateDistanceAttenuation, RollInOut) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Positional);
  builder.add_roll_in_curve_factor(1.0f);
  builder.add_roll_out_curve_factor(1.0f);
  builder.add_min_audible_radius(10.0f);
  builder.add_roll_in_radius(20.f);
  builder.add_roll_out_radius(30.0f);
  builder.add_max_audible_radius(40.0f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculateDistanceAttenuation(std::pow(15.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(20.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(25.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculateDistanceAttenuation(std::pow(30.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculateDistanceAttenuation(std::pow(35.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(40.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculateDistanceAttenuation(std::pow(100.0f, 2.0f), def));
}

class ListenerSpaceTests : public ::testing::Test {
 public:
  MATHFU_DEFINE_CLASS_SIMD_AWARE_NEW_DELETE

 protected:
  ListenerSpaceTests()
      : listener_list_1_(&ListenerInternalState::node),
        listener_list_2_(&ListenerInternalState::node),
        listener_list_3_(&ListenerInternalState::node),
        state_1_(),
        state_2_(),
        state_3_(),
        best_state_(),
        distance_squared_(),
        transformed_location_() {}
  virtual void SetUp() {
    listener_list_1_.push_back(state_1_);
    listener_list_2_.push_back(state_2_);
    listener_list_3_.push_back(state_3_);

    Listener listener;
    // The Matrix::LookAt function is hardcoded to be left handed
    listener = Listener(&state_1_);
    listener.SetOrientation(mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f),
                            mathfu::Vector<float, 3>(0.0f, 1.0f, 0.0f),
                            -mathfu::kAxisZ3f);

    listener = Listener(&state_2_);
    listener.SetOrientation(mathfu::Vector<float, 3>(12.0f, 34.0f, 56.0f),
                            mathfu::Vector<float, 3>(0.0f, 0.0f, 78.0f),
                            -mathfu::kAxisX3f);

    listener = Listener(&state_3_);
    listener.SetOrientation(mathfu::Vector<float, 3>(10.0f, 10.0f, 10.0f),
                            mathfu::Vector<float, 3>(20.0f, 0.0f, 0.0f),
                            -mathfu::kAxisY3f);
  }
  virtual void TearDown() {}

 protected:
  ListenerList listener_list_1_;
  ListenerList listener_list_2_;
  ListenerList listener_list_3_;
  ListenerInternalState state_1_;
  ListenerInternalState state_2_;
  ListenerInternalState state_3_;
  ListenerList::const_iterator best_state_;
  float distance_squared_;
  mathfu::Vector<float, 3> transformed_location_;
};

TEST_F(ListenerSpaceTests, DirectlyToTheRight) {
  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_1_,
                           mathfu::Vector<float, 3>(1.0f, 0.0f, 0.0f)));
  EXPECT_LT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_2_,
                           mathfu::Vector<float, 3>(12.0f, 134.0f, 56.0f)));
  EXPECT_LT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_3_,
                           mathfu::Vector<float, 3>(10.0f, 10.0f, 100.0f)));
  EXPECT_LT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);
}

TEST_F(ListenerSpaceTests, DirectlyToTheLeft) {
  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_1_,
                           mathfu::Vector<float, 3>(-1.0f, 0.0f, 0.0f)));
  EXPECT_GT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_2_,
                           mathfu::Vector<float, 3>(12.0f, 0.0f, 56.0f)));
  EXPECT_GT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_3_,
                           mathfu::Vector<float, 3>(10.0f, 10.0f, 0.0f)));
  EXPECT_GT(0.0f, transformed_location_.x);
  EXPECT_NEAR(0.0f, transformed_location_.z, kEpsilon);
}

TEST_F(ListenerSpaceTests, DirectlyInFront) {
  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_1_,
                           mathfu::Vector<float, 3>(0.0f, 1.0f, 0.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_LT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_2_,
                           mathfu::Vector<float, 3>(12.0f, 34.0f, 156.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_LT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_3_,
                           mathfu::Vector<float, 3>(100.0f, 10.0f, 10.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_LT(0.0f, transformed_location_.z);
}

TEST_F(ListenerSpaceTests, DirectlyBehind) {
  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_1_,
                           mathfu::Vector<float, 3>(0.0f, -1.0f, 0.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_GT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_2_,
                           mathfu::Vector<float, 3>(12.0f, 34.0f, -56.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_GT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_3_,
                           mathfu::Vector<float, 3>(0.0f, 10.0f, 10.0f)));
  EXPECT_NEAR(0.0f, transformed_location_.x, kEpsilon);
  EXPECT_GT(0.0f, transformed_location_.z);
}

TEST_F(ListenerSpaceTests, Diagonal) {
  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_1_,
                           mathfu::Vector<float, 3>(1.0f, 1.0f, 0.0f)));
  EXPECT_LT(0.0f, transformed_location_.x);
  EXPECT_LT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_2_,
                           mathfu::Vector<float, 3>(12.0f, 0.0f, 156.0f)));
  EXPECT_GT(0.0f, transformed_location_.x);
  EXPECT_LT(0.0f, transformed_location_.z);

  EXPECT_TRUE(BestListener(&best_state_, &distance_squared_,
                           &transformed_location_, listener_list_3_,
                           mathfu::Vector<float, 3>(0.0f, 10.0f, 100.0f)));
  EXPECT_LT(0.0f, transformed_location_.x);
  EXPECT_GT(0.0f, transformed_location_.z);
}

TEST(AttenuationCurve, Linear) {
  EXPECT_EQ(0.0f, AttenuationCurve(0.0f, 0.0f, 1.0f, 1.0f));
  EXPECT_EQ(0.5f, AttenuationCurve(0.5f, 0.0f, 1.0f, 1.0f));
  EXPECT_EQ(1.0f, AttenuationCurve(1.0f, 0.0f, 1.0f, 1.0f));

  EXPECT_EQ(0.0f, AttenuationCurve(1.0f, 1.0f, 2.0f, 1.0f));
  EXPECT_EQ(0.5f, AttenuationCurve(1.5f, 1.0f, 2.0f, 1.0f));
  EXPECT_EQ(1.0f, AttenuationCurve(2.0f, 1.0f, 2.0f, 1.0f));

  EXPECT_EQ(0.0f, AttenuationCurve(100.0f, 100.0f, 200.0f, 1.0f));
  EXPECT_EQ(0.5f, AttenuationCurve(150.0f, 100.0f, 200.0f, 1.0f));
  EXPECT_EQ(1.0f, AttenuationCurve(200.0f, 100.0f, 200.0f, 1.0f));
}

TEST(AttenuationCurve, EaseOut) {
  EXPECT_NEAR(0.0f, AttenuationCurve(0.0f, 0.0f, 1.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(0.333f, AttenuationCurve(0.5f, 0.0f, 1.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(1.0f, 0.0f, 1.0f, 2.0f), kEpsilon);

  EXPECT_NEAR(0.0f, AttenuationCurve(1.0f, 1.0f, 2.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(0.333f, AttenuationCurve(1.5f, 1.0f, 2.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(2.0f, 1.0f, 2.0f, 2.0f), kEpsilon);

  EXPECT_NEAR(0.0f, AttenuationCurve(100.0f, 100.0f, 200.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(0.333f, AttenuationCurve(150.0f, 100.0f, 200.0f, 2.0f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(200.0f, 100.0f, 200.0f, 2.0f), kEpsilon);
}

TEST(AttenuationCurve, EaseIn) {
  EXPECT_NEAR(0.0f, AttenuationCurve(0.0f, 0.0f, 1.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(0.6666f, AttenuationCurve(0.5f, 0.0f, 1.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(1.0f, 0.0f, 1.0f, 0.5f), kEpsilon);

  EXPECT_NEAR(0.0f, AttenuationCurve(1.0f, 1.0f, 2.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(0.666f, AttenuationCurve(1.5f, 1.0f, 2.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(2.0f, 1.0f, 2.0f, 0.5f), kEpsilon);

  EXPECT_NEAR(0.0f, AttenuationCurve(100.0f, 100.0f, 200.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(0.666f, AttenuationCurve(150.0f, 100.0f, 200.0f, 0.5f), kEpsilon);
  EXPECT_NEAR(1.0f, AttenuationCurve(200.0f, 100.0f, 200.0f, 0.5f), kEpsilon);
}

}  // namespace pindrop

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
