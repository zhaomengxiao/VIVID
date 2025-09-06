#include "vivid/window/window_systems.h"

#include <SDL3/SDL.h>

#include "vivid/app/Schedule.h"

namespace VIVID::Window {

  // Window initialization system for Startup schedule
  void window_initialization_system(Resources& resources, entt::registry& registry) {
    // Find all entities with WindowComponent but without WindowGpuComponent (uninitialized windows)
    auto view = registry.view<WindowComponent>(entt::exclude<WindowGpuComponent>);

    view.each([&](auto entity, auto& window_comp) {
      // Initialize SDL video subsystem if not already initialized
      if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
          SDL_Log("Failed to initialize SDL video subsystem: %s", SDL_GetError());
          return;
        }
      }

      // Create SDL window
      SDL_Window* window_handle = SDL_CreateWindow(window_comp.title.c_str(), window_comp.width,
                                                   window_comp.height, window_comp.flags);

      if (!window_handle) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return;
      }

      // Set window position if specified
      if (window_comp.x != SDL_WINDOWPOS_CENTERED && window_comp.y != SDL_WINDOWPOS_CENTERED) {
        SDL_SetWindowPosition(window_handle, window_comp.x, window_comp.y);
      }

      // Add GPU component to mark as initialized
      auto& gpu_comp = registry.emplace<WindowGpuComponent>(entity);
      gpu_comp.window_handle = window_handle;
      gpu_comp.initialized = true;

      // Initialize cache with current values
      gpu_comp.cached_title = window_comp.title;
      gpu_comp.cached_width = window_comp.width;
      gpu_comp.cached_height = window_comp.height;
      gpu_comp.cached_x = window_comp.x;
      gpu_comp.cached_y = window_comp.y;
      gpu_comp.cached_visible = window_comp.visible;

      // Add events component
      registry.emplace<WindowEventsComponent>(entity);

      // Show window if visible
      if (window_comp.visible) {
        SDL_ShowWindow(window_handle);
      }

      SDL_Log("Window created successfully: %s (%dx%d)", window_comp.title.c_str(),
              window_comp.width, window_comp.height);
    });
  }

  // Window event processing system for Update schedule
  void window_event_processing_system(Resources& resources, entt::registry& registry) {
    auto view = registry.view<WindowComponent, WindowGpuComponent, WindowEventsComponent>();

    view.each([&](auto entity, auto& window_comp, auto& gpu_comp, auto& events_comp) {
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

  // Window update system for Update schedule
  void window_update_system(Resources& resources, entt::registry& registry) {
    auto view = registry.view<WindowComponent, WindowGpuComponent>();

    view.each([&](auto entity, auto& window_comp, auto& gpu_comp) {
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

  // Window cleanup system for Shutdown schedule
  void window_cleanup_system(Resources& resources, entt::registry& registry) {
    auto view = registry.view<WindowGpuComponent>();

    view.each([&](auto entity, auto& gpu_comp) {
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
  }

  void WindowPlugin::build(App& app) {
    // Create a default window entity if none exists
    // Note: This could be made configurable in the future
    auto& registry = app.world();

    // Check if there's already a window entity
    auto view = registry.view<WindowComponent>();
    if (view.empty()) {
      // Create default window entity
      auto window_entity = registry.create();
      registry.emplace<WindowComponent>(window_entity);  // Uses default values
      SDL_Log("Created default window entity");
    }

    // Add window systems to appropriate schedules
    app.add_system(ScheduleLabel::Startup, window_initialization_system);
    app.add_system(ScheduleLabel::Update, window_event_processing_system);
    app.add_system(ScheduleLabel::Update, window_update_system);
    app.add_system(ScheduleLabel::Shutdown, window_cleanup_system);
  }

  std::string WindowPlugin::name() const { return "WindowPlugin"; }

}  // namespace VIVID::Window