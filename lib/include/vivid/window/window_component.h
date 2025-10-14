#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <string>
#include <vector>

namespace VIVID {
namespace WINDOW {

// Pure data component - window configuration and state
struct WindowComponent {
  std::string title = "VIVID Application";
  int width = 800;
  int height = 600;
  int x = SDL_WINDOWPOS_CENTERED;
  int y = SDL_WINDOWPOS_CENTERED;
  SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  bool visible = true;
  bool should_close = false;
};

// GPU resource component - holds SDL window handle
struct WindowGpuComponent {
  SDL_Window* window_handle = nullptr;
  SDL_GLContext gl_context = nullptr;

  // Cache previous values to detect changes
  std::string cached_title;
  int cached_width = 0;
  int cached_height = 0;
  int cached_x = 0;
  int cached_y = 0;
  bool cached_visible = true;

  bool initialized = false;
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
    world.component<WindowComponent>();
    world.component<WindowGpuComponent>();
    world.component<WindowEventsComponent>();
  }
};

}  // namespace WINDOW
}  // namespace VIVID
