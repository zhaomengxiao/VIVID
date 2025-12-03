#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <string>
#include <vector>

namespace vivid::window {

// Pure data component - window configuration and state
struct WindowContext {
  std::string title_ = "VIVID Application";
  int width_ = 800;
  int height_ = 600;
  int x_ = SDL_WINDOWPOS_CENTERED;
  int y_ = SDL_WINDOWPOS_CENTERED;
  SDL_WindowFlags flags_ = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  bool visible_ = true;
  bool should_close_ = false;
  SDL_Window* window_handle_ = nullptr;

  int pixel_width_ = 0;
  int pixel_height_ = 0;

  // Dirty flags
  enum class DirtyFlag : uint8_t {
    kTitle = 1 << 0,
    kSize = 1 << 1,
    kPosition = 1 << 2,
    kVisibility = 1 << 3
  };
  uint8_t dirty_flags_ = 0;

  void MarkDirty(DirtyFlag flag) { dirty_flags_ |= static_cast<uint8_t>(flag); }

  bool IsDirty(DirtyFlag flag) const { return (dirty_flags_ & static_cast<uint8_t>(flag)) != 0; }

  void ClearDirty(DirtyFlag flag) { dirty_flags_ &= ~static_cast<uint8_t>(flag); }

  void ClearAllDirty() { dirty_flags_ = 0; }
};

// Window events component - stores events for processing
struct WindowEventsComponent {
  std::vector<SDL_Event> events_;
  bool quit_requested_ = false;
  bool close_requested_ = false;
  bool resized_ = false;
  bool moved_ = false;
};

// Window Components Module
struct WindowComponents {
  explicit WindowComponents(flecs::world& world) {
    // Register module
    world.module<WindowComponents>();

    // Register components
    world.component<WindowContext>();
    world.component<WindowEventsComponent>();
  }
};

}  // namespace vivid::window
