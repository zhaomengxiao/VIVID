#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>
#include <vivid/log/log.h>

#include <vector>

struct ShutdownPhase {};

namespace VIVID {
namespace APP {

struct EventQueues {
  std::vector<SDL_Event> raw_sdl_events;  // 原始SDL事件
  // std::queue<InputEvent> input_events;            // 输入事件
  // std::queue<WindowEvent> window_events;          // 窗口事件
  // std::queue<SystemEvent> system_events;          // 系统事件
  // ... 更多特定事件队列
};

// Main application class
class App {
private:
  flecs::world world_;
  bool running_ = true;
  bool initialized_ = false;

  flecs::entity
      builtin_pipeline_;  // see:
                          // https://www.flecs.dev/flecs/md_docs_2Systems.html#builtin-pipeline
  flecs::entity
      shutdown_pipeline_;  // see:
                           // https://www.flecs.dev/flecs/md_docs_2Systems.html#custom-pipeline

public:
  App() {
    // store default pipeline
    builtin_pipeline_ = world_.get_pipeline();
    // setup custom ShutDown pipeline,Create a pipeline that matches systems with
    // Shutdown
    shutdown_pipeline_ = world_.pipeline()
                             .with(flecs::System)    // Mandatory, must always match systems
                             .with<ShutdownPhase>()  // or .with<Foo>() if a type
                             .build();
  }

  // Chain call entry point
  static App& new_app() {
    static App instance;
    return instance;
  }

  // Import flecs native module
  // Supports standard flecs module system with world.module<T>()
  // Usage: app.import_module<MyModule>(args...)
  template <typename Module, typename... Args> App& ImportModule(Args&&... args) {
    VividLogger::app_info("Importing flecs module: %s", typeid(Module).name());
    world_.import <Module>(std::forward<Args>(args)...);
    return *this;
  }

  // // Import pre-built system bundle
  // // SystemBundle groups related systems together for modular organization
  // // Usage: app.import_systems<MySystemBundle>(args...)
  // template <typename T, typename... Args> App& ImportSystems(Args&&... args) {
  //   static_assert(std::is_base_of_v<SystemBundle, T>, "T must inherit from
  //   SystemBundle"); T bundle(std::forward<Args>(args)...); std::cout << "Importing
  //   system bundle: " << bundle.name() << std::endl; bundle.build(world_); return *this;
  // }

  // Insert resource (singleton)
  template <typename T, typename... Args> App& InsertResource(Args&&... args) {
    world_.set<T>({std::forward<Args>(args)...});
    return *this;
  }

  // Get resource (singleton) - const version
  template <typename T> const T* GetResource() const { return world_.get<T>(); }

  // Get mutable resource (singleton)
  template <typename T> T* GetResourceMut() { return world_.get_mut<T>(); }

  // Get world
  flecs::world& GetWorld() { return world_; }
  const flecs::world& GetWorld() const { return world_; }

  // Exit application
  void Exit() { running_ = false; }

  // Traditional run mode (backward compatible)
  void Run() {
    VividLogger::app_info("Starting application...");

    // Run startup systems
    world_.progress(0);
    initialized_ = true;

    // Main loop
    while (running_) {
      world_.progress();
    }

    // Application shutdown
    VividLogger::app_info("Application shutting down...");
    // Shutdown systems using custom pipeline
    world_.set_pipeline(shutdown_pipeline_);
    world_.progress();
    VividLogger::app_info("Application finished.");
  }

  // Set enable REST server
  void EnableRestServer() {
#ifndef __EMSCRIPTEN__

    world_.set<flecs::Rest>({});
    VividLogger::app_info("Rest Server Running, navigate to this URL: https://flecs.dev/explorer");
#else
    // Note: REST server not supported in Emscripten browser environment
    VividLogger::app_warn("Skipping flecs::Rest in Emscripten (not supported in browser)");
#endif
  }

  // Enable flecs::stats
  void EnableStats() {
    world_.import <flecs::stats>();
    VividLogger::app_info("Stats Enabled");
  }

  // SDL3 Callback mode support

  // Initialize application (corresponds to SDL_AppInit)
  bool Initialize(int argc, char** argv) {
    if (initialized_) return true;
    VividLogger::app_info("Initializing SDL3 application...");

    // ensure ensure resource exists, if not, create one
    world_.set<EventQueues>({});

    // Run startup systems
    world_.progress(0);
    initialized_ = true;

    return true;
  }

  // Single iteration (corresponds to SDL_AppIterate)
  bool Iterate() {
    if (!initialized_ || !running_) return false;

    static int frame_count = 0;
    if (frame_count == 0) {
      VividLogger::app_info("Starting main loop...");
    }
    frame_count++;

    // Run one frame of system schedule
    world_.progress();

    return running_;
  }

  // Handle event (corresponds to SDL_AppEvent)
  bool HandleEvent(SDL_Event* event) {
    // Event handling can be implemented through systems in EventPhase
    // Or handled directly here

    // push event to event queues
    auto& event_queues = world_.get_mut<EventQueues>();
    event_queues.raw_sdl_events.push_back(*event);
    // VividLogger::app_info("SDL_AppEvent: %d", event->type);
    // VividLogger::app_info("event_queues size: %zu", event_queues.raw_sdl_events.size());
    return running_;
  }

  // Close application (corresponds to SDL_AppQuit)
  void Shutdown() {
    if (!initialized_) return;

    VividLogger::app_info("SDL3 application shutting down...");
    // Shutdown systems using custom pipeline
    world_.set_pipeline(shutdown_pipeline_);
    world_.progress();

    VividLogger::app_info("SDL3 application finished.");
  }

  // Check if application is running
  bool IsRunning() const { return running_; }

  // Check if application is initialized
  bool IsInitialized() const { return initialized_; }

private:
};

}  // namespace APP
}  // namespace VIVID
