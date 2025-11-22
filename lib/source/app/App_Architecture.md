## VIVID App 架构导读：SDL3 回调入口 + Flecs 模块/系统/管线 + Bevy 风格链式构建

本文基于项目中的以下文件：`hello_sdl3/source/main.cpp`、`lib/include/vivid/app/App.h`、`lib/include/vivid/app/SDL3App.h`、`lib/source/app/SDL3App.cpp`，系统介绍 VIVID 的应用层架构：如何用 SDL3 的 Main Callback 作为应用入口，如何在 `App` 中整合 Flecs 的世界（world）、系统（system）、模块（module）与管线（pipeline），以及如何通过 Bevy 风格的链式 API 构建应用。

---

### 架构总览

- **SDL3 回调式入口**：使用 `SDL_AppInit/SDL_AppIterate/SDL_AppEvent/SDL_AppQuit` 接管应用生命周期，替代传统 `main`。
- **App 容器**：封装 `flecs::world`，统一初始化、逐帧推进、事件转发、优雅关停；集成日志、断言、SDL 元数据。
- **Flecs 模块/系统/管线**：以模块组织功能域；在 `OnStart` 做一次性初始化；用自定义关停管线集中执行清理逻辑。
- **Bevy 风格链式构建**：`SDL3AppBuilder` + `VIVID_SDL3_MAIN` 宏，提供声明式、链式的应用装配体验。

---

### SDL3 Main Callback：回调就是应用入口

在 `lib/source/app/SDL3App.cpp` 中，实现了 SDL3 的回调入口，贯通了构建、逐帧、事件与关停四个阶段：

```cpp
extern "C" {

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  // Create application state and build app via builder
  auto* state = new SDL3AppState();
  auto builder = CreateAppInstance();
  auto bundle = builder.ReleaseAppBundle();
  state->app = std::move(bundle.app);
  state->metadata = std::move(bundle.metadata);
  state->log_config = std::move(bundle.log_config);
  state->assert_config = std::move(bundle.assert_config);

  // Initialize logging/assertion/SDL and apply metadata
  VividLogger::initialize(state->log_config);
  VividAssertManager::initialize(state->assert_config);
  apply_sdl3_metadata(state->metadata);
  if (!VividErrorHandler::check_sdl_result(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO), "SDL_Init")) {
    delete state; return SDL_APP_FAILURE;
  }

  // Initialize app (runs startup systems once)
  if (state->app && state->app->Initialize(argc, argv)) {
    state->initialized = true; *appstate = state; return SDL_APP_CONTINUE;
  }
  delete state; return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  // Advance one frame; return CONTINUE or SUCCESS to exit
  auto* state = static_cast<SDL3AppState*>(appstate);
  return state->app->Iterate() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  // Push events into App-managed queues; handle quit
  auto* state = static_cast<SDL3AppState*>(appstate);
  if (event->type == SDL_EVENT_QUIT) { state->app->Exit(); return SDL_APP_SUCCESS; }
  return state->app->HandleEvent(event) ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  // Run shutdown pipeline and cleanly quit SDL
  auto* state = static_cast<SDL3AppState*>(appstate);
  if (state && state->app && state->initialized) {
    state->app->Shutdown();
    SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    SDL_Quit();
  }
  delete state;
}

} // extern "C"
```

同时提供 `ApplySdl3Metadata` 将应用名、版本、标识符、类型、URL、版权以及自定义键值对齐 SDL3 规范设置到系统。

---

### App：Flecs 世界与生命周期的宿主

在 `lib/include/vivid/app/App.h` 中，`App` 封装了 `flecs::world` 与两套管线：

```cpp
class App {
  flecs::world world_;
  bool running_ = true;
  bool initialized_ = false;
  flecs::entity builtin_pipeline_;
  flecs::entity shutdown_pipeline_;
public:
  App() {
    // Store default pipeline and build shutdown-only pipeline
    builtin_pipeline_ = world_.get_pipeline();
    shutdown_pipeline_ = world_.pipeline()
      .with(flecs::System)    // must match systems
      .with<ShutdownPhase>()  // match systems tagged for shutdown
      .build();
  }

  template <typename T, typename... Args>
  App& InsertResource(Args&&... args) { world_.set<T>({std::forward<Args>(args)...}); return *this; }

  bool Initialize(int argc, char** argv) {
    // Ensure event queues exist and run OnStart systems once
    world_.set<EventQueues>({});
    world_.progress(0);
    initialized_ = true; return true;
  }

  bool Iterate() { if (!initialized_ || !running_) return false; world_.progress(); return running_; }

  bool HandleEvent(SDL_Event* event) {
    // Push raw SDL events into queues for systems to consume later
    auto& queues = world_.get_mut<EventQueues>();
    queues.raw_sdl_events.push(*event);
    return running_;
  }

  void Shutdown() {
    if (!initialized_) return;
    world_.set_pipeline(shutdown_pipeline_);
    world_.progress();
  }
};
```

- **事件即资源**：以 `EventQueues` 作为资源（单例）收集 SDL 事件，便于在 Event 阶段的系统中统一消费。
- **关停有管线**：切换到 `shutdown_pipeline_`，仅运行带 `ShutdownPhase` 的系统，实现可控、集中式清理。

---

### Flecs：模块、系统与管线的实践

在 `hello_sdl3/source/main.cpp` 中，推荐将场景初始化逻辑放入模块，并用 `OnStart` 在第一帧前执行一次：

```cpp
struct Setup {
  Setup(flecs::world& world) {
    // Register module namespace
    world.module<Setup>();
    // Import components used by this scene
    world.import<VIVID::RENDER::RenderComponents>();
    // Run once at startup to create entities
    world.system("SceneInitialization").kind(flecs::OnStart).run(sceneInitializationImpl);
  }
  static void sceneInitializationImpl(flecs::iter& it) {
    auto world = it.world();
    // Create cube/light/camera entities here
  }
};
```

- **module/import**：通过模块组织组件与系统；在应用侧按需引入。
- **OnStart**：一次性建场景；持续逻辑放到 `Update`；退出清理给系统打上 `ShutdownPhase` 标签即可纳入关停管线。

---

### Bevy 风格链式构建：Builder + 宏

在 `lib/include/vivid/app/SDL3App.h` 提供 `SDL3AppBuilder` 与 `VIVID_SDL3_MAIN` 宏：

```cpp
class SDL3AppBuilder {
  std::unique_ptr<App> app_;
  SDL3AppMetadata metadata_;
  SDL3LogConfig log_config_;
public:
  SDL3AppBuilder() : app_(std::make_unique<App>()) {}

  template <typename T, typename... Args>
  SDL3AppBuilder& import_module(Args&&... args) { app_->ImportModule<T>(std::forward<Args>(args)...); return *this; }
  template <typename T, typename... Args>
  SDL3AppBuilder& insert_resource(Args&&... args) { app_->InsertResource<T>(std::forward<Args>(args)...); return *this; }
  SDL3AppBuilder& set_app_info(const std::string& name, const std::string& version, const std::string& id) { metadata_.name=name; metadata_.version=version; metadata_.identifier=id; return *this; }
  SDL3AppBuilder& set_metadata(SDL3MetadataProperty prop, const std::string& value) { /* set fields */ return *this; }
  SDL3AppBuilder& set_default_log_level(SDL3LogLevel level) { log_config_.default_level = level; return *this; }
  SDL3AppBundle ReleaseAppBundle();
};

#define VIVID_SDL3_MAIN(chain_calls)                         \
  namespace VIVID { namespace APP {                          \
  SDL3AppBuilder CreateAppInstance() {                        \
    auto builder = CreateSdl3App();                          \
    return std::move(builder chain_calls);                   \
  } }                                                        \

```

在用户侧，仅需一次宏调用即可完成“声明式装配”：

```cpp
VIVID_SDL3_MAIN(
  .set_app_info("VIVID Hello SDL3 with WebGPU Rendering", "1.0.0", "com.vivid.hello_sdl3")
  .set_metadata(SDL3MetadataProperty::Type, SDL3AppType::kApplication)
  .set_default_log_level(VividLogLevel::Debug)
  .insert_resource<MyResource>(100)
  .import_module<vivid::window::WindowSystems>()
  .import_module<Setup>()
  .import_module<VIVID::RENDER::RenderSystems>()
  .import_module<VIVID::UI::UISystems>()
)
```

- **声明式与顺序化**：按依赖顺序声明模块与资源，清晰表达初始化意图。
- **零样板入口**：SDL3 回调侧自动调用 `CreateAppInstance()`，无需手写 `main()`。

---

### 生命周期与事件流（速览）

- **Init**：构建器产出 `App`/元数据/日志断言配置 → 应用 SDL 元数据 → 初始化 SDL 子系统 → `App::Initialize()` 确保事件资源与一次性 `OnStart` 系统。
- **Iterate**：`App::Iterate()` → `world_.progress()` 推进帧调度。
- **Event**：`SDL_AppEvent` → `App::HandleEvent()` 将 `SDL_Event` 入队，系统可在事件阶段统一消费。
- **Quit**：切换到 `shutdown_pipeline_` 仅运行带 `ShutdownPhase` 的系统 → 干净关闭 SDL。

---

### 可扩展性建议

- 使用“模块”对齐功能域（渲染/窗口/UI/物理），在应用侧按需 `.import_module<T>()`。
- 用资源（单例）承载跨系统数据（例如事件队列、配置、设备句柄）。
- `OnStart` 做一次性准备；常规逻辑放 `Update`；退出清理挂 `ShutdownPhase`，纳入关停管线。
- 通过 `VIVID_SDL3_MAIN` + Builder 保持入口文件即“配置清单”，降低样板与耦合。

---

### 结语

这套组合让 VIVID 的应用入口简洁、扩展点明确、关停可控：

- **SDL3 回调**提供稳定生命周期；
- **App 容器**统一调度与资源；
- **模块/系统/管线**带来清晰阶段化；
- **链式构建**对齐 Bevy 风格开发体验。

无论面向桌面还是 Web（Emscripten），都能以相同的装配范式快速搭建渲染应用。
