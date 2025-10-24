#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <string>
#include <vector>

namespace VIVID {
namespace WINDOW {

// Pure data component - window configuration and state
struct WindowContext {
  std::string title = "VIVID Application";
  int width = 800;
  int height = 600;
  int x = SDL_WINDOWPOS_CENTERED;
  int y = SDL_WINDOWPOS_CENTERED;
  SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  bool visible = true;
  bool should_close = false;
  SDL_Window* window_handle = nullptr;

  // Dirty flags
  enum class DirtyFlag : uint8_t {
    Title = 1 << 0,
    Size = 1 << 1,
    Position = 1 << 2,
    Visibility = 1 << 3
  };
  uint8_t dirty_flags = 0;

  void markDirty(DirtyFlag flag) { dirty_flags |= static_cast<uint8_t>(flag); }

  bool isDirty(DirtyFlag flag) const { return (dirty_flags & static_cast<uint8_t>(flag)) != 0; }

  void clearDirty(DirtyFlag flag) { dirty_flags &= ~static_cast<uint8_t>(flag); }

  void clearAllDirty() { dirty_flags = 0; }
};

// Window events component - stores events for processing
struct WindowEventsComponent {
  std::vector<SDL_Event> events;
  bool quit_requested = false;
  bool close_requested = false;
  bool resized = false;
  bool moved = false;
};

// Window Components Module
struct WindowComponents {
  WindowComponents(flecs::world& world) {
    // Register module
    world.module<WindowComponents>();

    // Register components
    world.component<WindowContext>();
    world.component<WindowEventsComponent>();
  }
};

}  // namespace WINDOW
}  // namespace VIVID
