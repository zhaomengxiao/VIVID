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

  if (!VividErrorHandler::check_sdl_result(
          SDL_GetWindowSizeInPixels(windowContext.window_handle, &windowContext.pixel_width,
                                    &windowContext.pixel_height),
          "SDL_GetWindowSizeInPixels")) {
    return;
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
void WindowSystems::processWindowEventsImpl(VIVID::APP::EventQueues& eventQueues,
                                            WindowContext& windowContext) {
  if (!windowContext.window_handle) {
    return;
  }

  for (auto& event : eventQueues.raw_sdl_events) {
    // Check if event belongs to this window
    if (!(event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)) {
      continue;
    }

    if (event.window.windowID != SDL_GetWindowID(windowContext.window_handle)) {
      continue;
    }

    switch (event.type) {
      case SDL_EVENT_WINDOW_RESIZED:
        windowContext.width = event.window.data1;
        windowContext.height = event.window.data2;
        windowContext.markDirty(WindowContext::DirtyFlag::Size);
        // SDL_Log("Window resize event received: %dx%d", event.window.data1, event.window.data2);
        break;

      case SDL_EVENT_WINDOW_MOVED:
        windowContext.x = event.window.data1;
        windowContext.y = event.window.data2;
        windowContext.markDirty(WindowContext::DirtyFlag::Position);
        // SDL_Log("Window moved event received: (%d, %d)", event.window.data1, event.window.data2);
        break;

      default:
        // SDL_Log("UnHandled window event received: %d", event.type);
        break;
    }
  }
}

// Window update system
void WindowSystems::windowUpdateImpl(const flecs::entity entity, WindowContext& windowContext) {
  if (!windowContext.window_handle) {
    return;
  }

  // Only update properties that have actually changed
  if (windowContext.isDirty(WindowContext::DirtyFlag::Title)) {
    SDL_SetWindowTitle(windowContext.window_handle, windowContext.title.c_str());
    windowContext.clearDirty(WindowContext::DirtyFlag::Title);
  }

  if (windowContext.isDirty(WindowContext::DirtyFlag::Size)) {
    if (!VividErrorHandler::check_sdl_result(
            SDL_GetWindowSizeInPixels(windowContext.window_handle, &windowContext.pixel_width,
                                      &windowContext.pixel_height),
            "SDL_GetWindowSizeInPixels")) {
      return;
    }
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

// Clean events system
void WindowSystems::cleanEventsImpl(APP::EventQueues& eventQueues) {
  eventQueues.raw_sdl_events.clear();
}

// Window cleanup system
void WindowSystems::windowCleanupImpl(const flecs::entity entity, WindowContext& windowContext) {
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