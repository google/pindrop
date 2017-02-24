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

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include "SDL.h"
#include "audio_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/vector.h"
#include "pindrop/pindrop.h"

const int kScreenWidth = 640;
const int kScreenHeight = 480;
const float kFramesPerSecond = 60.0f;
const Uint32 kDelayMilliseconds =
    static_cast<Uint32>(1000.0f * 1.0f / kFramesPerSecond);

const char* kWindowTitle = "Pindrop Sample";
const char* kAudioConfig = "assets/audio_config.pinconfig";
const char* kSoundBank = "assets/sound_banks/my_sound_bank.pinbank";
const char* kInstructionsTexture = "assets/textures/instructions.bmp";
const char* kChannelTexture = "assets/textures/channel.bmp";
const char* kListenerTexture = "assets/textures/listener.bmp";
const char* kSoundHandleName = "my_sounds";

typedef mathfu::Vector<float, 2> Vector2f;
typedef mathfu::Vector<int, 2> Vector2i;

struct IconState {
  IconState() : location(mathfu::kZeros2f), velocity(mathfu::kZeros2f) {}
  Vector2f location;
  Vector2f velocity;
};

struct ListenerIcon : public IconState {
  ListenerIcon() : listener(nullptr) {}
  pindrop::Listener listener;
};

struct ChannelIcon : public IconState {
  ChannelIcon() : channel(nullptr) {}
  pindrop::Channel channel;
};

class SampleState {
 public:
  SampleState()
      : quit_(false),
        audio_config_source_(),
        audio_engine_(),
        master_bus_(),
        window_(nullptr),
        renderer_(nullptr),
        sound_handle_(nullptr),
        instructions_texture_(nullptr),
        channel_icons_(),
        channel_texture_(nullptr),
        new_channel_location_(),
        listener_icons_(),
        listener_texture_(nullptr),
        new_listener_location_() {}

  ~SampleState();

  // Initialize the sample.
  bool Initialize();

  // Run the main loop.
  void Run();

 private:
  SDL_Texture* LoadTexture(const char* texture_path);
  void AdvanceFrame(float delta_time);
  void HandleInput();
  void UpdateIconState(IconState* icon_state, float delta_time);
  void UpdateIcons(float delta_time);
  void RemoveInvalidSounds();
  void DrawInstructions();
  void DrawIcon(const IconState& icon_state, SDL_Texture* texture);
  void DrawIcons();

  bool quit_;
  std::string audio_config_source_;
  pindrop::AudioEngine audio_engine_;
  pindrop::Bus master_bus_;
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  pindrop::SoundHandle sound_handle_;
  SDL_Texture* instructions_texture_;

  std::vector<ChannelIcon> channel_icons_;
  SDL_Texture* channel_texture_;
  Vector2f new_channel_location_;

  std::vector<ListenerIcon> listener_icons_;
  SDL_Texture* listener_texture_;
  Vector2f new_listener_location_;
};

SampleState::~SampleState() {
  SDL_DestroyTexture(instructions_texture_);
  SDL_DestroyTexture(channel_texture_);
  SDL_DestroyTexture(listener_texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

SDL_Texture* SampleState::LoadTexture(const char* texture_path) {
  SDL_Surface* surface = SDL_LoadBMP(texture_path);
  if (surface == nullptr) {
    fprintf(stderr, "Could not load `%s`: %s\n", texture_path, SDL_GetError());
    return nullptr;
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
  if (texture == nullptr) {
    fprintf(stderr, "Could not load `%s`: %s\n", texture_path, SDL_GetError());
    SDL_DestroyTexture(texture);
    return nullptr;
  }
  SDL_FreeSurface(surface);
  return texture;
}

bool SampleState::Initialize() {
  // Initialize SDL.
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
    return false;
  }

  // Initialize the window.
  window_ = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, kScreenWidth,
                             kScreenHeight, SDL_WINDOW_SHOWN);
  if (window_ == nullptr) {
    fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
    return false;
  }

  // Initialize the renderer.
  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
  if (renderer_ == nullptr) {
    fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
    return false;
  }
  SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);

  // Initialize images.
  if ((channel_texture_ = LoadTexture(kChannelTexture)) == nullptr ||
      (listener_texture_ = LoadTexture(kListenerTexture)) == nullptr ||
      (instructions_texture_ = LoadTexture(kInstructionsTexture)) == nullptr) {
    return false;
  }

  // Initialize Pindrop.
  if (!audio_engine_.Initialize(kAudioConfig) ||
      !audio_engine_.LoadSoundBank(kSoundBank)) {
    return false;
  }

  // Wait for the sound files to complete loading.
  audio_engine_.StartLoadingSoundFiles();
  while (!audio_engine_.TryFinalize()) {
    SDL_Delay(1);
  }

  // Cache the master bus so we can samplenstrate adjusting the gain.
  master_bus_ = audio_engine_.FindBus("master");

  // Cache the SoundHandle to the sound we want to play.
  sound_handle_ = audio_engine_.GetSoundHandle(kSoundHandleName);
  if (sound_handle_ == nullptr) {
    fprintf(stderr, "Could not find sound handle %s\n", kSoundHandleName);
    return false;
  }

  // Success!
  return true;
}

void SampleState::UpdateIconState(IconState* icon_state, float delta_time) {
  icon_state->location += icon_state->velocity * delta_time;
  if (icon_state->location.x < 0) {
    icon_state->location.x *= -1;
    icon_state->velocity.x *= -1;
  } else if (icon_state->location[0] > kScreenWidth) {
    icon_state->location.x -= icon_state->location[0] - kScreenWidth;
    icon_state->velocity.x *= -1;
  }
  if (icon_state->location.y < 0) {
    icon_state->location.y *= -1;
    icon_state->velocity.y *= -1;
  } else if (icon_state->location[1] > kScreenHeight) {
    icon_state->location.y -= icon_state->location[1] - kScreenHeight;
    icon_state->velocity.y *= -1;
  }
}

void SampleState::UpdateIcons(float delta_time) {
  for (size_t i = 0; i < channel_icons_.size(); ++i) {
    ChannelIcon& icon = channel_icons_[i];
    UpdateIconState(&icon, delta_time);
    icon.channel.SetLocation(mathfu::Vector<float, 3>(icon.location, 0.0f));
  }
  for (size_t i = 0; i < listener_icons_.size(); ++i) {
    ListenerIcon& icon = listener_icons_[i];
    UpdateIconState(&icon, delta_time);
    mathfu::Vector<float, 3> location(icon.location, 0.0f);
    icon.listener.SetOrientation(location, mathfu::kAxisY3f, -mathfu::kAxisZ3f);
  }
}

void TextureRect(SDL_Rect* rect, const Vector2f& location,
                 SDL_Texture* texture) {
  SDL_QueryTexture(texture, nullptr, nullptr, &rect->w, &rect->h);
  rect->x = static_cast<int>(location.x - rect->w / 2);
  rect->y = static_cast<int>(location.y - rect->h / 2);
}

void SampleState::DrawIcon(const IconState& icon_state, SDL_Texture* texture) {
  SDL_Rect rect = {0, 0, 0, 0};
  TextureRect(&rect, icon_state.location, texture);
  SDL_RenderCopy(renderer_, texture, nullptr, &rect);
}

void SampleState::RemoveInvalidSounds() {
  channel_icons_.erase(
      std::remove_if(channel_icons_.begin(), channel_icons_.end(),
                     [](const ChannelIcon& icon) {
                       return !icon.channel.Valid() || !icon.channel.Playing();
                     }),
      channel_icons_.end());
}

void SampleState::DrawInstructions() {
  SDL_Rect rect = {0, 0, 0, 0};
  SDL_QueryTexture(instructions_texture_, nullptr, nullptr, &rect.w, &rect.h);
  SDL_RenderCopy(renderer_, instructions_texture_, nullptr, &rect);
}

void SampleState::DrawIcons() {
  for (size_t i = 0; i < channel_icons_.size(); ++i) {
    DrawIcon(channel_icons_[i], channel_texture_);
  }
  for (size_t i = 0; i < listener_icons_.size(); ++i) {
    DrawIcon(listener_icons_[i], listener_texture_);
  }
}

bool RectContains(const SDL_Rect& rect, const Vector2f& point) {
  return point.x >= rect.x && point.x < rect.x + rect.w &&
         point.y >= rect.y && point.y < rect.y + rect.h;
}

void SampleState::HandleInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
      case SDL_QUIT: {
        quit_ = true;
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        Vector2f mouse_location(Vector2i(event.button.x, event.button.y));
        if (event.button.button == SDL_BUTTON_LEFT) {
          new_channel_location_ = mouse_location;
        } else {
          new_listener_location_ = mouse_location;
        }
        break;
      }
      case SDL_MOUSEBUTTONUP: {
        Vector2f mouse_location(Vector2i(event.button.x, event.button.y));
        SDL_Texture* texture;

        texture = channel_texture_;
        auto channel_iter = std::find_if(
            channel_icons_.begin(), channel_icons_.end(),
            [mouse_location, texture](const ChannelIcon& icon) -> bool {
              SDL_Rect rect;
              TextureRect(&rect, icon.location, texture);
              return RectContains(rect, mouse_location);
            });
        if (channel_iter != channel_icons_.end()) {
          channel_iter->channel.Stop();
          channel_icons_.erase(channel_iter);
          break;
        }

        texture = listener_texture_;
        auto listener_iter = std::find_if(
            listener_icons_.begin(), listener_icons_.end(),
            [mouse_location, texture](const ListenerIcon& icon) -> bool {
              SDL_Rect rect;
              TextureRect(&rect, icon.location, texture);
              return RectContains(rect, mouse_location);
            });
        if (listener_iter != listener_icons_.end()) {
          audio_engine_.RemoveListener(&listener_iter->listener);
          listener_icons_.erase(listener_iter);
          break;
        }

        if (event.button.button == SDL_BUTTON_LEFT) {
          pindrop::Channel channel = audio_engine_.PlaySound(sound_handle_);
          if (channel.Valid()) {
            channel_icons_.push_back(ChannelIcon());
            ChannelIcon& icon = channel_icons_.back();
            icon.location = new_channel_location_;
            icon.velocity = mouse_location - new_channel_location_;
            icon.channel = channel;
          }
          break;
        }

        if (event.button.button == SDL_BUTTON_RIGHT) {
          pindrop::Listener listener = audio_engine_.AddListener();
          if (listener.Valid()) {
            listener_icons_.push_back(ListenerIcon());
            ListenerIcon& icon = listener_icons_.back();
            icon.location = new_listener_location_;
            icon.velocity = mouse_location - new_listener_location_;
            icon.listener = listener;
          }
          break;
        }
        break;
      }
      case SDL_MOUSEMOTION: {
        // Set the master gain to be based on a x position of the mouse such
        // that at x = 0 the gain is 0% and at x = kScreenWidth the master gain
        // is at 100%.
        float percentage = static_cast<float>(event.motion.x) / kScreenWidth;
        master_bus_.SetGain(percentage);
      }
      default:;  // Do nothing.
    }
  }
}

void SampleState::AdvanceFrame(float delta_time) {
  HandleInput();
  UpdateIcons(delta_time);
  audio_engine_.AdvanceFrame(delta_time);
  RemoveInvalidSounds();
  SDL_RenderClear(renderer_);
  DrawInstructions();
  DrawIcons();
  SDL_RenderPresent(renderer_);
  SDL_Delay(kDelayMilliseconds);
}

void SampleState::Run() {
  Uint32 previous_time = 0;
  Uint32 time = 0;
  while (!quit_) {
    previous_time = time;
    time = SDL_GetTicks();
    float delta_time = (time - previous_time) / 1000.0f;
    AdvanceFrame(delta_time);
  }
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  SampleState sample;
  if (!sample.Initialize()) {
    fprintf(stderr, "Failed to initialize!\n");
    return 1;
  }
  sample.Run();
  return 0;
}
