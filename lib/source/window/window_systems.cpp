#include "vivid/window/window_systems.h"

#include <SDL3/SDL.h>
#include <vivid/log/log.h>

#include "vivid/app/App.h"

namespace vivid {
namespace window {

// Static member function implementations

// Window initialization system
void WindowSystems::windowInitImpl(flecs::entity entity, WindowContext& windowContext) {
  VividLogger::app_info("=== WindowInitialization system executing ===");
  VividLogger::app_info("Entity: %s", entity.name().c_str());
  VividLogger::app_info("Window handle before init: %p", windowContext.window_handle_);

  if (windowContext.window_handle_) {
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
  windowContext.window_handle_
      = SDL_CreateWindow(windowContext.title_.c_str(), windowContext.width_, windowContext.height_,
                         windowContext.flags_);

  if (!VividErrorHandler::check_sdl_pointer(windowContext.window_handle_, "SDL_CreateWindow")) {
    return;  // Error already logged
  }

  if (!VividErrorHandler::check_sdl_result(
          SDL_GetWindowSizeInPixels(windowContext.window_handle_, &windowContext.pixel_width_,
                                    &windowContext.pixel_height_),
          "SDL_GetWindowSizeInPixels")) {
    return;
  }

  // Set window position if specified
  if (windowContext.x_ != SDL_WINDOWPOS_CENTERED && windowContext.y_ != SDL_WINDOWPOS_CENTERED) {
    VividErrorHandler::check_sdl_result(
        SDL_SetWindowPosition(windowContext.window_handle_, windowContext.x_, windowContext.y_),
        "SDL_SetWindowPosition");
  }

  // Initialize cache with current values

  // Add events component
  entity.set<WindowEventsComponent>({});

  // Show window if visible
  if (windowContext.visible_) {
    VividErrorHandler::check_sdl_result(SDL_ShowWindow(windowContext.window_handle_),
                                        "SDL_ShowWindow");
  }

  VividLogger::app_info("Window created successfully: %s (%dx%d)", windowContext.title_.c_str(),
                        windowContext.width_, windowContext.height_);
}

// Window event processing system
void WindowSystems::processWindowEventsImpl(vivid::app::EventQueues& eventQueues,
                                            WindowContext& windowContext) {
  if (!windowContext.window_handle_) {
    return;
  }

  for (auto& event : eventQueues.raw_sdl_events_) {
    // Check if event belongs to this window
    if (!(event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)) {
      continue;
    }

    if (event.window.windowID != SDL_GetWindowID(windowContext.window_handle_)) {
      continue;
    }

    switch (event.type) {
      case SDL_EVENT_WINDOW_RESIZED:
        windowContext.width_ = event.window.data1;
        windowContext.height_ = event.window.data2;
        windowContext.MarkDirty(WindowContext::DirtyFlag::kSize);
        // SDL_Log("Window resize event received: %dx%d", event.window.data1, event.window.data2);
        break;

      case SDL_EVENT_WINDOW_MOVED:
        windowContext.x_ = event.window.data1;
        windowContext.y_ = event.window.data2;
        windowContext.MarkDirty(WindowContext::DirtyFlag::kPosition);
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
  if (!windowContext.window_handle_) {
    return;
  }

  // Only update properties that have actually changed
  if (windowContext.IsDirty(WindowContext::DirtyFlag::kTitle)) {
    SDL_SetWindowTitle(windowContext.window_handle_, windowContext.title_.c_str());
    windowContext.ClearDirty(WindowContext::DirtyFlag::kTitle);
  }

  if (windowContext.IsDirty(WindowContext::DirtyFlag::kSize)) {
    if (!VividErrorHandler::check_sdl_result(
            SDL_GetWindowSizeInPixels(windowContext.window_handle_, &windowContext.pixel_width_,
                                      &windowContext.pixel_height_),
            "SDL_GetWindowSizeInPixels")) {
      return;
    }
    windowContext.ClearDirty(WindowContext::DirtyFlag::kSize);
  }

  if (windowContext.IsDirty(WindowContext::DirtyFlag::kPosition)) {
    SDL_SetWindowPosition(windowContext.window_handle_, windowContext.x_, windowContext.y_);
    windowContext.ClearDirty(WindowContext::DirtyFlag::kPosition);
  }

  // Handle visibility changes
  if (windowContext.IsDirty(WindowContext::DirtyFlag::kVisibility)) {
    if (windowContext.visible_) {
      SDL_ShowWindow(windowContext.window_handle_);
    } else {
      SDL_HideWindow(windowContext.window_handle_);
    }
    windowContext.ClearDirty(WindowContext::DirtyFlag::kVisibility);
  }
}

// Clean events system
void WindowSystems::cleanEventsImpl(vivid::app::EventQueues& eventQueues) {
  eventQueues.raw_sdl_events_.clear();
}

// Window cleanup system
void WindowSystems::windowCleanupImpl(const flecs::entity entity, WindowContext& windowContext) {
  VividLogger::app_info("WindowCleanup system executing...");

  if (windowContext.window_handle_) {
    SDL_DestroyWindow(windowContext.window_handle_);
    windowContext.window_handle_ = nullptr;
    VividLogger::app_info("Window destroyed");
  }

  // Quit SDL video subsystem
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    VividLogger::app_info("SDL video subsystem shut down");
  }

  VividLogger::app_info("Window cleanup complete");
}

}  // namespace window
}  // namespace vivid