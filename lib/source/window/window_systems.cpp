#include "vivid/window/window_systems.h"

#include <SDL3/SDL.h>
#include <vivid/log/log.h>

#include <iostream>

#include "vivid/app/App.h"

namespace VIVID {
namespace WINDOW {

// Static member function implementations

// Window initialization system
void WindowSystems::windowInitializationImpl(flecs::iter& it) {
  auto world = it.world();

  VividLogger::app_debug("WindowInitialization system executing...");

  // Find all entities with WindowComponent but without WindowGpuComponent (uninitialized windows)
  auto query = world.query_builder<WindowComponent>().without<WindowGpuComponent>().build();

  query.each([&](flecs::entity entity, WindowComponent& window_comp) {
    // Initialize SDL video subsystem if not already initialized
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL video subsystem: %s", SDL_GetError());
        return;
      }
    }
    float main_scale
        = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());  // FIXME-WGPU: Test this?
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

    // Create SDL window
    SDL_Window* window_handle = SDL_CreateWindow(window_comp.title.c_str(), window_comp.width,
                                                 window_comp.height, window_comp.flags);

    if (!VividErrorHandler::check_sdl_pointer(window_handle, "SDL_CreateWindow")) {
      return;  // Error already logged
    }

    // TODO: =======move to render==========================================================
    // SDL_GLContext gl_context = SDL_GL_CreateContext(window_handle);
    //
    // if (!VividErrorHandler::check_sdl_pointer(gl_context, "SDL_GL_CreateContext")) {
    //   return;  // Error already logged
    // }
    //
    // SDL_GL_MakeCurrent(window_handle, gl_context);
    // SDL_GL_SetSwapInterval(1);  // Enable vsync
    //=====================================================================================

    // Set window position if specified
    if (window_comp.x != SDL_WINDOWPOS_CENTERED && window_comp.y != SDL_WINDOWPOS_CENTERED) {
      VividErrorHandler::check_sdl_result(
          SDL_SetWindowPosition(window_handle, window_comp.x, window_comp.y),
          "SDL_SetWindowPosition");
    }

    // Add GPU component to mark as initialized
    WindowGpuComponent gpu_comp{};
    gpu_comp.window_handle = window_handle;
    // gpu_comp.gl_context = gl_context;
    gpu_comp.initialized = true;

    // Initialize cache with current values
    gpu_comp.cached_title = window_comp.title;
    gpu_comp.cached_width = window_comp.width;
    gpu_comp.cached_height = window_comp.height;
    gpu_comp.cached_x = window_comp.x;
    gpu_comp.cached_y = window_comp.y;
    gpu_comp.cached_visible = window_comp.visible;

    entity.set<WindowGpuComponent>(gpu_comp);

    // Add events component
    entity.set<WindowEventsComponent>({});

    // Show window if visible
    if (window_comp.visible) {
      VividErrorHandler::check_sdl_result(SDL_ShowWindow(window_handle), "SDL_ShowWindow");
    }

    VividLogger::app_info("Window created successfully: %s (%dx%d)", window_comp.title.c_str(),
                          window_comp.width, window_comp.height);
    VividLogger::app_debug("WindowGpuComponent added to entity: %llu, handle: %p", entity.id(),
                           gpu_comp.window_handle);
  });
}

// Window event processing system
void WindowSystems::windowEventProcessingImpl(flecs::iter& it) {
  auto world = it.world();

  // Query for windows with all three components
  auto query = world.query<WindowComponent, WindowGpuComponent, WindowEventsComponent>();

  query.each([&](flecs::entity entity, WindowComponent& window_comp, WindowGpuComponent& gpu_comp,
                 WindowEventsComponent& events_comp) {
    if (!gpu_comp.initialized || !gpu_comp.window_handle) {
      return;
    }

    // Clear previous frame events
    events_comp.events.clear();
    events_comp.quit_requested = false;
    events_comp.close_requested = false;
    events_comp.resized = false;
    events_comp.moved = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      bool is_window_event = false;

      // Check if event belongs to this window
      if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        is_window_event = (event.window.windowID == SDL_GetWindowID(gpu_comp.window_handle));
      }

      switch (event.type) {
        case SDL_EVENT_QUIT:
          events_comp.quit_requested = true;
          events_comp.events.push_back(event);
          SDL_Log("Quit event received");
          break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          if (is_window_event) {
            events_comp.close_requested = true;
            window_comp.should_close = true;
            events_comp.events.push_back(event);
            SDL_Log("Window close requested");
          }
          break;

        case SDL_EVENT_WINDOW_RESIZED:
          if (is_window_event) {
            window_comp.width = event.window.data1;
            window_comp.height = event.window.data2;
            // Update cache to match the new size
            gpu_comp.cached_width = event.window.data1;
            gpu_comp.cached_height = event.window.data2;
            events_comp.resized = true;
            events_comp.events.push_back(event);
            SDL_Log("Window resized to %dx%d", event.window.data1, event.window.data2);
          }
          break;

        case SDL_EVENT_WINDOW_MOVED:
          if (is_window_event) {
            window_comp.x = event.window.data1;
            window_comp.y = event.window.data2;
            // Update cache to match the new position
            gpu_comp.cached_x = event.window.data1;
            gpu_comp.cached_y = event.window.data2;
            events_comp.moved = true;
            events_comp.events.push_back(event);
            SDL_Log("Window moved to (%d, %d)", event.window.data1, event.window.data2);
          }
          break;

        default:
          // Store all events for potential use by other systems
          events_comp.events.push_back(event);
          break;
      }
    }
  });
}

// Window update system
void WindowSystems::windowUpdateImpl(flecs::iter& it) {
  auto world = it.world();

  // Query for windows with WindowComponent and WindowGpuComponent
  auto query = world.query<WindowComponent, WindowGpuComponent>();

  query.each([&](flecs::entity entity, WindowComponent& window_comp, WindowGpuComponent& gpu_comp) {
    if (!gpu_comp.initialized || !gpu_comp.window_handle) {
      return;
    }

    // Only update properties that have actually changed
    if (window_comp.title != gpu_comp.cached_title) {
      SDL_SetWindowTitle(gpu_comp.window_handle, window_comp.title.c_str());
      gpu_comp.cached_title = window_comp.title;
    }

    if (window_comp.width != gpu_comp.cached_width
        || window_comp.height != gpu_comp.cached_height) {
      SDL_SetWindowSize(gpu_comp.window_handle, window_comp.width, window_comp.height);
      gpu_comp.cached_width = window_comp.width;
      gpu_comp.cached_height = window_comp.height;
    }

    if (window_comp.x != gpu_comp.cached_x || window_comp.y != gpu_comp.cached_y) {
      SDL_SetWindowPosition(gpu_comp.window_handle, window_comp.x, window_comp.y);
      gpu_comp.cached_x = window_comp.x;
      gpu_comp.cached_y = window_comp.y;
    }

    // Handle visibility changes
    if (window_comp.visible != gpu_comp.cached_visible) {
      if (window_comp.visible) {
        SDL_ShowWindow(gpu_comp.window_handle);
      } else {
        SDL_HideWindow(gpu_comp.window_handle);
      }
      gpu_comp.cached_visible = window_comp.visible;
    }
  });
}

// Window cleanup system
void WindowSystems::windowCleanupImpl(flecs::iter& it) {
  auto world = it.world();

  std::cout << "Cleaning up windows..." << std::endl;

  // Query for all windows with GPU components
  auto query = world.query<WindowGpuComponent>();

  query.each([&](flecs::entity entity, WindowGpuComponent& gpu_comp) {
    if (gpu_comp.gl_context) {
      SDL_GL_DestroyContext(gpu_comp.gl_context);
      gpu_comp.gl_context = nullptr;
    }

    if (gpu_comp.window_handle) {
      SDL_DestroyWindow(gpu_comp.window_handle);
      gpu_comp.window_handle = nullptr;
      SDL_Log("Window destroyed");
    }

    gpu_comp.initialized = false;
  });

  // Quit SDL video subsystem
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Log("SDL video subsystem shut down");
  }

  std::cout << "Window cleanup complete" << std::endl;
}

}  // namespace WINDOW
}  // namespace VIVID