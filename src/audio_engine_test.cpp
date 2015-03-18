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
#include <cmath>

#include "SDL_mixer.h"
#include "audio_engine_internal_state.h"
#include "channel_internal_state.h"
#include "gtest/gtest.h"
#include "intrusive_list.h"
#include "listener_internal_state.h"
#include "pindrop/audio_engine.h"
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
int Mix_Volume(int, int) { return MIX_MAX_VOLUME; }
int Mix_VolumeMusic(int) { return MIX_MAX_VOLUME; }
void Mix_CloseAudio() {}
void Mix_FreeChunk(Mix_Chunk*) {}
void Mix_FreeMusic(Mix_Music*) {}
void Mix_Pause(int) {}
void Mix_PauseMusic() {}
int Mix_FadeOutMusic(int) { return 0; }
void Mix_Resume(int) {}
void Mix_ResumeMusic() {}
}

static const float kEpsilon = 0.001f;

namespace pindrop {

class ChannelInternalStatePriorityTests : public ::testing::Test {
 protected:
  ChannelInternalStatePriorityTests() : collections_(), list_(), channels_() {}
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

    list_.InsertAfter(channels_[0].priority_node());
    channels_[0].priority_node()->InsertAfter(channels_[1].priority_node());
    channels_[1].priority_node()->InsertAfter(channels_[2].priority_node());
    channels_[2].priority_node()->InsertAfter(channels_[3].priority_node());

    channels_[0].SetHandle(&collections_[2]);
    channels_[1].SetHandle(&collections_[1]);
    channels_[2].SetHandle(&collections_[1]);
    channels_[3].SetHandle(&collections_[0]);

    channels_[0].SetGain(1.0f);
    channels_[1].SetGain(1.0f);
    channels_[2].SetGain(1.0f);
    channels_[3].SetGain(1.0f);
  }
  virtual void TearDown() {}

 protected:
  static const std::size_t kCollectionCount = 3;
  SoundCollection collections_[kCollectionCount];

  IntrusiveListNode list_;
  static const std::size_t kSoundCount = 4;
  ChannelInternalState channels_[kSoundCount];
};

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointAtHead) {
  // New sound's priority is greater than highest priority.
  EXPECT_EQ(
      ChannelInternalState::GetInstanceFromPriorityNode(list_.GetTerminator()),
      FindInsertionPoint(&list_, 2.5f));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointWithEqualPriority) {
  // New sound's priority is...
  // ...equal to highest priority.
  EXPECT_EQ(&channels_[0], FindInsertionPoint(&list_, 2.0f));
  // ...equal to a sound with the same priority as the previous sound.
  EXPECT_EQ(&channels_[2], FindInsertionPoint(&list_, 1.0f));
  // ...equal to the lowest priority.
  EXPECT_EQ(&channels_[3], FindInsertionPoint(&list_, 0));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointMiddlePriority) {
  // New sound's priority is...
  // ...between highest and second highest.
  EXPECT_EQ(&channels_[0], FindInsertionPoint(&list_, 1.5f));
  // ...lower than a sound with the same priority as the previous sound.
  EXPECT_EQ(&channels_[2], FindInsertionPoint(&list_, 0.5));
}

TEST_F(ChannelInternalStatePriorityTests, FindInsertionPointLowest) {
  // New sound's priority is less than the lowest prioity.
  EXPECT_EQ(&channels_[3], FindInsertionPoint(&list_, -1.0f));
}

TEST(BestListenerTests, BestListenerTests) {
  TypedIntrusiveListNode<ListenerInternalState> listener_list;
  ListenerInternalState listeners[4];
  listeners[0].set_location(mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f));
  listeners[1].set_location(mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f));
  listeners[2].set_location(mathfu::Vector<float, 3>(10.0f, 0.0f, 10.0f));
  listeners[3].set_location(mathfu::Vector<float, 3>(0.0f, 0.0f, 10.0f));
  listener_list.InsertAfter(&listeners[0]);
  listeners[0].InsertAfter(&listeners[1]);
  listeners[1].InsertAfter(&listeners[2]);
  listeners[2].InsertAfter(&listeners[3]);
  ListenerInternalState* listener;

  // Location overlaps with listener.
  EXPECT_EQ(0.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f)));
  EXPECT_EQ(&listeners[0], listener);
  EXPECT_EQ(0.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f)));
  EXPECT_EQ(&listeners[1], listener);
  EXPECT_EQ(0.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(10.0f, 0.0f, 10.0f)));
  EXPECT_EQ(&listeners[2], listener);
  EXPECT_EQ(0.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(0.0f, 0.0f, 10.0f)));
  EXPECT_EQ(&listeners[3], listener);

  // Nearest listener is 10 units away.
  EXPECT_EQ(100.0f, BestListener(&listener, listener_list,
                                 mathfu::Vector<float, 3>(0.0f, 10.0f, 10.0f)));
  EXPECT_EQ(&listeners[3], listener);
  EXPECT_EQ(100.0f, BestListener(&listener, listener_list,
                                 mathfu::Vector<float, 3>(20.0f, 0.0f, 10.0f)));
  EXPECT_EQ(&listeners[2], listener);

  // Location is in the center of multiple listeners.
  EXPECT_EQ(50.0f, BestListener(&listener, listener_list,
                                mathfu::Vector<float, 3>(5.0f, 0.0f, 5.0f)));

  // Location is close to one listener.
  EXPECT_EQ(2.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(1.0f, 0.0f, 1.0f)));
  EXPECT_EQ(&listeners[0], listener);
  EXPECT_EQ(8.0f, BestListener(&listener, listener_list,
                               mathfu::Vector<float, 3>(8.0f, 0.0f, 2.0f)));
  EXPECT_EQ(&listeners[1], listener);
  EXPECT_EQ(18.0f, BestListener(&listener, listener_list,
                                mathfu::Vector<float, 3>(7.0f, 0.0f, 7.0f)));
  EXPECT_EQ(&listeners[2], listener);
  EXPECT_EQ(32.0f, BestListener(&listener, listener_list,
                                mathfu::Vector<float, 3>(4.0f, 0.0f, 6.0f)));
  EXPECT_EQ(&listeners[3], listener);
}

TEST(CalculatePositionalAttenuation, RollOutCentered) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Positional);
  builder.add_roll_out_curve_factor(1.0f);
  builder.add_gain(1.0f);
  builder.add_max_audible_radius(10.0f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculatePositionalAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(100.0f, 2.0f), def));
}

TEST(CalculatePositionalAttenuation, RollOutOffCenter) {
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

  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculatePositionalAttenuation(std::pow(15.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(20.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(100.0f, 2.0f), def));
}

TEST(CalculatePositionalAttenuation, RollInOut) {
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

  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(0.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(5.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(10.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculatePositionalAttenuation(std::pow(15.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(20.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(25.0f, 2.0f), def));
  EXPECT_EQ(1.0f, CalculatePositionalAttenuation(std::pow(30.0f, 2.0f), def));
  EXPECT_EQ(0.5f, CalculatePositionalAttenuation(std::pow(35.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(40.0f, 2.0f), def));
  EXPECT_EQ(0.0f, CalculatePositionalAttenuation(std::pow(100.0f, 2.0f), def));
}

TEST(CalculateGain, Mode_Normal) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Nonpositional);
  builder.add_gain(0.75f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  TypedIntrusiveListNode<ListenerInternalState> listener_list;
  ListenerInternalState listener;
  listener.set_location(mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f));
  listener_list.InsertAfter(&listener);

  // Ensure the normal gain is always returned.
  EXPECT_EQ(0.75f, CalculateGain(listener_list, def,
                                 mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f)));
  EXPECT_EQ(0.75f, CalculateGain(listener_list, def, mathfu::Vector<float, 3>(
                                                         10.0f, 10.0f, 10.0f)));
  EXPECT_EQ(0.75f,
            CalculateGain(listener_list, def,
                          mathfu::Vector<float, 3>(-1000.0f, 200.0f, 123.0f)));
}

TEST(CalculateGain, Mode_Positional) {
  flatbuffers::FlatBufferBuilder fbb;
  SoundCollectionDefBuilder builder(fbb);
  builder.add_mode(Mode_Positional);
  builder.add_gain(1.0f);
  builder.add_roll_in_curve_factor(1.0f);
  builder.add_roll_out_curve_factor(1.0f);
  builder.add_max_audible_radius(10.0f);
  auto offset = builder.Finish();
  FinishSoundCollectionDefBuffer(fbb, offset);
  auto def = GetSoundCollectionDef(fbb.GetBufferPointer());

  TypedIntrusiveListNode<ListenerInternalState> listener_list;
  ListenerInternalState listener;
  listener.set_location(mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f));
  listener_list.InsertAfter(&listener);

  // Ensure the gain drops off with the proper curve.
  EXPECT_EQ(0.0f, CalculateGain(listener_list, def,
                                mathfu::Vector<float, 3>(0.0f, 0.0f, 0.0f)));
  EXPECT_EQ(0.5f, CalculateGain(listener_list, def,
                                mathfu::Vector<float, 3>(5.0f, 0.0f, 0.0f)));
  EXPECT_EQ(1.0f, CalculateGain(listener_list, def,
                                mathfu::Vector<float, 3>(10.0f, 0.0f, 0.0f)));
  EXPECT_EQ(0.0f, CalculateGain(listener_list, def,
                                mathfu::Vector<float, 3>(100.0f, 0.0f, 0.0f)));
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
