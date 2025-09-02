#pragma once

// SDL3 应用集成头文件
//
// 使用方法：
// 1. 包含此头文件：
//    #include "vivid/app/SDL3App.h"
// 2. 使用 VIVID_SDL3_MAIN 宏创建应用（无需其他设置）

#include <SDL3/SDL.h>

#include <memory>

#include "App.h"

// SDL3 应用状态管理结构
struct SDL3AppState {
  std::unique_ptr<App> app;
  bool initialized = false;

  SDL3AppState() = default;
  ~SDL3AppState() = default;

  // 禁用拷贝和移动，确保状态唯一性
  SDL3AppState(const SDL3AppState&) = delete;
  SDL3AppState& operator=(const SDL3AppState&) = delete;
  SDL3AppState(SDL3AppState&&) = delete;
  SDL3AppState& operator=(SDL3AppState&&) = delete;

  // 状态验证
  bool is_valid() const { return app && initialized; }
};

// SDL3 应用构建器类，保持链式调用风格
class SDL3AppBuilder {
private:
  std::unique_ptr<App> app_;

public:
  SDL3AppBuilder() : app_(std::make_unique<App>()) {}

  // 禁用拷贝，启用移动
  SDL3AppBuilder(const SDL3AppBuilder&) = delete;
  SDL3AppBuilder& operator=(const SDL3AppBuilder&) = delete;
  SDL3AppBuilder(SDL3AppBuilder&&) = default;
  SDL3AppBuilder& operator=(SDL3AppBuilder&&) = default;

  // 添加插件
  template <typename T, typename... Args> SDL3AppBuilder& add_plugin(Args&&... args) {
    app_->add_plugin<T>(std::forward<Args>(args)...);
    return *this;
  }

  // 添加系统
  template <typename Fn> SDL3AppBuilder& add_system(ScheduleLabel label, Fn&& fn) {
    app_->add_system(label, std::forward<Fn>(fn));
    return *this;
  }

  // 添加启动系统
  template <typename Fn> SDL3AppBuilder& add_startup_system(Fn&& fn) {
    app_->add_startup_system(std::forward<Fn>(fn));
    return *this;
  }

  // 插入资源
  template <typename T, typename... Args> SDL3AppBuilder& insert_resource(Args&&... args) {
    app_->insert_resource<T>(std::forward<Args>(args)...);
    return *this;
  }

  // 注意：SDL3应用通过回调系统自动运行，不需要显式的run方法

  // 获取内部App实例（用于SDL3回调）
  std::unique_ptr<App> release_app() { return std::move(app_); }
};

// SDL3 回调函数声明
extern "C" {
SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv);
SDL_AppResult SDL_AppIterate(void* appstate);
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event);
void SDL_AppQuit(void* appstate, SDL_AppResult result);
}

// 静态工厂函数 - 返回临时对象支持移动语义
SDL3AppBuilder create_sdl3_app();

// =============================================================================
// SDL3 主入口宏
// =============================================================================

// 便捷宏，用于创建和运行SDL3应用
// 用法: VIVID_SDL3_MAIN(.add_plugin<DefaultPlugin>().add_system(...))
// 示例: VIVID_SDL3_MAIN(.add_plugin<DefaultPlugin>().add_startup_system(my_system))
#define VIVID_SDL3_MAIN(chain_calls)                                       \
  SDL3AppBuilder create_app_instance() {                                   \
    try {                                                                  \
      auto builder = create_sdl3_app();                                    \
      return std::move(builder chain_calls);                               \
    } catch (const std::exception& e) {                                    \
      std::cerr << "Failed to create SDL3 app: " << e.what() << std::endl; \
      throw;                                                               \
    }                                                                      \
  }                                                                        \
  extern "C" {                                                             \
  SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv);       \
  SDL_AppResult SDL_AppIterate(void* appstate);                            \
  SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event);            \
  void SDL_AppQuit(void* appstate, SDL_AppResult result);                  \
  }

// 注意：SDL3应用通过回调系统自动运行
// 用户只需要使用 VIVID_SDL3_MAIN 宏即可
//
// 完整示例：
//   #include "vivid/app/SDL3App.h"
//
//   VIVID_SDL3_MAIN(
//       .add_plugin<DefaultPlugin>()
//       .add_startup_system(my_system)
//   )
