#include "vivid/window/window_systems.h"

#include <SDL3/SDL.h>
#include <vivid/log/log.h>

#include <iostream>

#include "vivid/app/App.h"

namespace VIVID {
namespace WINDOW {

// Static member function implementations

// Window initialization system
void WindowSystems::windowInitImpl(flecs::entity entity, WindowContext& windowContext) {
  VividLogger::app_info("=== WindowInitialization system executing ===");
  VividLogger::app_info("Entity: %s", entity.name());
  VividLogger::app_info("Window handle before init: %p", windowContext.window_handle);

  if (windowContext.window_handle) {
    VividLogger::app_warn("Window already initialized");
    return;
  }

  // Initialize SDL video subsystem
  if (!VividErrorHandler::check_sdl_result(SDL_Init(SDL_INIT_VIDEO), "SDL_Init")) {
    VividLogger::app_error("Failed to initialize SDL video subsystem");
    return;
  }

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());  // FIXME-WGPU: Test this?
  SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

  // Create SDL window
  windowContext.window_handle = SDL_CreateWindow(windowContext.title.c_str(), windowContext.width,
                                                 windowContext.height, windowContext.flags);

  if (!VividErrorHandler::check_sdl_pointer(windowContext.window_handle, "SDL_CreateWindow")) {
    return;  // Error already logged
  }

  // Set window position if specified
  if (windowContext.x != SDL_WINDOWPOS_CENTERED && windowContext.y != SDL_WINDOWPOS_CENTERED) {
    VividErrorHandler::check_sdl_result(
        SDL_SetWindowPosition(windowContext.window_handle, windowContext.x, windowContext.y),
        "SDL_SetWindowPosition");
  }

  // Initialize cache with current values

  // Add events component
  entity.set<WindowEventsComponent>({});

  // Show window if visible
  if (windowContext.visible) {
    VividErrorHandler::check_sdl_result(SDL_ShowWindow(windowContext.window_handle),
                                        "SDL_ShowWindow");
  }

  VividLogger::app_info("Window created successfully: %s (%dx%d)", windowContext.title.c_str(),
                        windowContext.width, windowContext.height);
}

// Window event processing system
// void WindowSystems::windowEventProcessingImpl(flecs::iter& it) {
//   auto world = it.world();

//   // Query for windows with all three components
//   auto query = world.query<WindowContext, WindowGpuComponent, WindowEventsComponent>();

//   query.each([&](flecs::entity entity, WindowContext& windowContext,
//                  WindowGpuComponent& windowContext, WindowEventsComponent& events_comp) {
//     if (!windowContext.initialized || !windowContext.window_handle) {
//       return;
//     }

//     // Clear previous frame events
//     events_comp.events.clear();
//     events_comp.quit_requested = false;
//     events_comp.close_requested = false;
//     events_comp.resized = false;
//     events_comp.moved = false;

//     SDL_Event event;
//     while (SDL_PollEvent(&event)) {
//       bool is_window_event = false;

//       // Check if event belongs to this window
//       if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
//         is_window_event = (event.window.windowID ==
//         SDL_GetWindowID(windowContext.window_handle));
//       }

//       switch (event.type) {
//         case SDL_EVENT_QUIT:
//           events_comp.quit_requested = true;
//           events_comp.events.push_back(event);
//           SDL_Log("Quit event received");
//           break;

//         case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
//           if (is_window_event) {
//             events_comp.close_requested = true;
//             windowContext.should_close = true;
//             events_comp.events.push_back(event);
//             SDL_Log("Window close requested");
//           }
//           break;

//         case SDL_EVENT_WINDOW_RESIZED:
//           if (is_window_event) {
//             windowContext.width = event.window.data1;
//             windowContext.height = event.window.data2;
//             // Update cache to match the new size
//             windowContext.cached_width = event.window.data1;
//             windowContext.cached_height = event.window.data2;
//             events_comp.resized = true;
//             events_comp.events.push_back(event);
//             // SDL_Log("Window resized to %dx%d", event.window.data1, event.window.data2);
//           }
//           break;

//         case SDL_EVENT_WINDOW_MOVED:
//           if (is_window_event) {
//             windowContext.x = event.window.data1;
//             windowContext.y = event.window.data2;
//             // Update cache to match the new position
//             windowContext.cached_x = event.window.data1;
//             windowContext.cached_y = event.window.data2;
//             events_comp.moved = true;
//             events_comp.events.push_back(event);
//             // SDL_Log("Window moved to (%d, %d)", event.window.data1, event.window.data2);
//           }
//           break;

//         default:
//           // Store all events for potential use by other systems
//           events_comp.events.push_back(event);
//           break;
//       }
//     }
// });
// }

// Window update system
void WindowSystems::windowUpdateImpl(flecs::entity entity, WindowContext& windowContext) {
  if (!windowContext.window_handle) {
    return;
  }

  // Only update properties that have actually changed
  if (windowContext.isDirty(WindowContext::DirtyFlag::Title)) {
    SDL_SetWindowTitle(windowContext.window_handle, windowContext.title.c_str());
    windowContext.clearDirty(WindowContext::DirtyFlag::Title);
  }

  if (windowContext.isDirty(WindowContext::DirtyFlag::Size)) {
    SDL_SetWindowSize(windowContext.window_handle, windowContext.width, windowContext.height);
    windowContext.clearDirty(WindowContext::DirtyFlag::Size);
  }

  if (windowContext.isDirty(WindowContext::DirtyFlag::Position)) {
    SDL_SetWindowPosition(windowContext.window_handle, windowContext.x, windowContext.y);
    windowContext.clearDirty(WindowContext::DirtyFlag::Position);
  }

  // Handle visibility changes
  if (windowContext.isDirty(WindowContext::DirtyFlag::Visibility)) {
    if (windowContext.visible) {
      SDL_ShowWindow(windowContext.window_handle);
    } else {
      SDL_HideWindow(windowContext.window_handle);
    }
    windowContext.clearDirty(WindowContext::DirtyFlag::Visibility);
  }
}

// Window cleanup system
void WindowSystems::windowCleanupImpl(flecs::entity entity, WindowContext& windowContext) {
  VividLogger::app_info("WindowCleanup system executing...");

  if (windowContext.window_handle) {
    SDL_DestroyWindow(windowContext.window_handle);
    windowContext.window_handle = nullptr;
    VividLogger::app_info("Window destroyed");
  }

  // Quit SDL video subsystem
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    VividLogger::app_info("SDL video subsystem shut down");
  }

  VividLogger::app_info("Window cleanup complete");
}

}  // namespace WINDOW
}  // namespace VIVID