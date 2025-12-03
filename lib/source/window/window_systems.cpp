#include "vivid/window/window_systems.h"

#include <SDL3/SDL.h>
#include <vivid/log/log.h>

#include "vivid/app/App.h"

namespace vivid {
namespace window {

// Static member function implementations

// Window initialization system
void WindowSystems::windowInitImpl([[maybe_unused]] flecs::entity entity,
                                   WindowContext& window_context) {
  VividLogger::app_info("=== WindowInitialization system executing ===");
  VividLogger::app_info("Entity: %s", entity.name().c_str());
  VividLogger::app_info("Window handle before init: %p", window_context.window_handle_);

  if (window_context.window_handle_ != nullptr) {
    VividLogger::app_warn("Window already initialized");
    return;
  }

  // Initialize SDL video subsystem
  if (!VividErrorHandler::check_sdl_result(static_cast<int>(SDL_Init(SDL_INIT_VIDEO)),
                                           "SDL_Init")) {
    VividLogger::app_error("Failed to initialize SDL video subsystem");
    return;
  }

  // Create SDL window
  window_context.window_handle_
      = SDL_CreateWindow(window_context.title_.c_str(), window_context.width_,
                         window_context.height_, window_context.flags_);

  if (!VividErrorHandler::check_sdl_pointer(window_context.window_handle_, "SDL_CreateWindow")) {
    return;  // Error already logged
  }

  if (!VividErrorHandler::check_sdl_result(
          static_cast<int>(SDL_GetWindowSizeInPixels(window_context.window_handle_,
                                                     &window_context.pixel_width_,
                                                     &window_context.pixel_height_)),
          "SDL_GetWindowSizeInPixels")) {
    return;
  }

  // Set window position if specified
  if (window_context.x_ != SDL_WINDOWPOS_CENTERED && window_context.y_ != SDL_WINDOWPOS_CENTERED) {
    VividErrorHandler::check_sdl_result(
        static_cast<int>(SDL_SetWindowPosition(window_context.window_handle_, window_context.x_,
                                               window_context.y_)),
        "SDL_SetWindowPosition");
  }

  // Initialize cache with current values

  // Add events component
  entity.set<WindowEventsComponent>({});

  // Show window if visible
  if (window_context.visible_) {
    VividErrorHandler::check_sdl_result(
        static_cast<int>(SDL_ShowWindow(window_context.window_handle_)), "SDL_ShowWindow");
  }

  VividLogger::app_info("Window created successfully: %s (%dx%d)", window_context.title_.c_str(),
                        window_context.width_, window_context.height_);
}

// Window event processing system
void WindowSystems::processWindowEventsImpl(vivid::app::EventQueues& event_queues,
                                            WindowContext& window_context) {
  if (window_context.window_handle_ == nullptr) {
    return;
  }

  for (auto& event : event_queues.raw_sdl_events_) {
    // Check if event belongs to this window
    if (event.type < SDL_EVENT_WINDOW_FIRST || event.type > SDL_EVENT_WINDOW_LAST) {
      continue;
    }

    if (event.window.windowID != SDL_GetWindowID(window_context.window_handle_)) {
      continue;
    }

    switch (event.type) {
      case SDL_EVENT_WINDOW_RESIZED:
        window_context.width_ = event.window.data1;
        window_context.height_ = event.window.data2;
        window_context.MarkDirty(WindowContext::DirtyFlag::kSize);
        // SDL_Log("Window resize event received: %dx%d", event.window.data1, event.window.data2);
        break;

      case SDL_EVENT_WINDOW_MOVED:
        window_context.x_ = event.window.data1;
        window_context.y_ = event.window.data2;
        window_context.MarkDirty(WindowContext::DirtyFlag::kPosition);
        // SDL_Log("Window moved event received: (%d, %d)", event.window.data1, event.window.data2);
        break;

      default:
        // SDL_Log("UnHandled window event received: %d", event.type);
        break;
    }
  }
}

// Window update system
void WindowSystems::windowUpdateImpl([[maybe_unused]] flecs::entity entity,
                                     WindowContext& window_context) {
  if (window_context.window_handle_ == nullptr) {
    return;
  }

  // Only update properties that have actually changed
  if (window_context.IsDirty(WindowContext::DirtyFlag::kTitle)) {
    SDL_SetWindowTitle(window_context.window_handle_, window_context.title_.c_str());
    window_context.ClearDirty(WindowContext::DirtyFlag::kTitle);
  }

  if (window_context.IsDirty(WindowContext::DirtyFlag::kSize)) {
    if (!VividErrorHandler::check_sdl_result(
            static_cast<int>(SDL_GetWindowSizeInPixels(window_context.window_handle_,
                                                       &window_context.pixel_width_,
                                                       &window_context.pixel_height_)),
            "SDL_GetWindowSizeInPixels")) {
      return;
    }
    window_context.ClearDirty(WindowContext::DirtyFlag::kSize);
  }

  if (window_context.IsDirty(WindowContext::DirtyFlag::kPosition)) {
    VividErrorHandler::check_sdl_result(
        static_cast<int>(SDL_SetWindowPosition(window_context.window_handle_, window_context.x_,
                                               window_context.y_)),
        "SDL_SetWindowPosition");
    window_context.ClearDirty(WindowContext::DirtyFlag::kPosition);
  }

  // Handle visibility changes
  if (window_context.IsDirty(WindowContext::DirtyFlag::kVisibility)) {
    if (window_context.visible_) {
      VividErrorHandler::check_sdl_result(
          static_cast<int>(SDL_ShowWindow(window_context.window_handle_)), "SDL_ShowWindow");
    } else {
      VividErrorHandler::check_sdl_result(
          static_cast<int>(SDL_HideWindow(window_context.window_handle_)), "SDL_HideWindow");
    }
    window_context.ClearDirty(WindowContext::DirtyFlag::kVisibility);
  }
}

// Clean events system
void WindowSystems::cleanEventsImpl(vivid::app::EventQueues& event_queues) {
  event_queues.raw_sdl_events_.clear();
}

// Window cleanup system
void WindowSystems::windowCleanupImpl([[maybe_unused]] flecs::entity entity,
                                      WindowContext& window_context) {
  VividLogger::app_info("WindowCleanup system executing...");

  if (window_context.window_handle_ != nullptr) {
    SDL_DestroyWindow(window_context.window_handle_);
    window_context.window_handle_ = nullptr;
    VividLogger::app_info("Window destroyed");
  }

  // Quit SDL video subsystem
  if (static_cast<bool>(SDL_WasInit(SDL_INIT_VIDEO))) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    VividLogger::app_info("SDL video subsystem shut down");
  }

  VividLogger::app_info("Window cleanup complete");
}

}  // namespace window
}  // namespace vivid