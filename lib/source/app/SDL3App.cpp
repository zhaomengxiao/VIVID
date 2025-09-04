#include "vivid/app/SDL3App.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <iostream>

// 前向声明用户定义的应用创建函数
extern SDL3AppBuilder create_app_instance();

// SDL3 callback functions implementation

extern "C" {

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  std::cout << "SDL_AppInit called with " << argc << " arguments" << std::endl;

  // 创建应用状态
  auto* state = new SDL3AppState();

  // 使用宏定义的函数创建应用
  auto builder = create_app_instance();
  state->app = builder.release_app();

  // 初始化应用
  if (state->app && state->app->initialize(argc, argv)) {
    state->initialized = true;
    *appstate = state;
    std::cout << "SDL3 application initialized successfully" << std::endl;
    return SDL_APP_CONTINUE;
  } else {
    std::cerr << "Failed to initialize SDL3 application" << std::endl;
    delete state;
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  if (!state || !state->app || !state->initialized) {
    return SDL_APP_FAILURE;
  }

  // 执行一帧迭代
  if (state->app->iterate()) {
    return SDL_APP_CONTINUE;
  } else {
    // 应用请求退出
    return SDL_APP_SUCCESS;
  }
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  if (!state || !state->app || !state->initialized) {
    return SDL_APP_FAILURE;
  }

  // 处理特殊事件
  if (event->type == SDL_EVENT_QUIT) {
    std::cout << "Received SDL_EVENT_QUIT" << std::endl;
    state->app->exit();
    return SDL_APP_SUCCESS;
  }

  // 让应用处理事件
  if (state->app->handle_event(event)) {
    return SDL_APP_CONTINUE;
  } else {
    return SDL_APP_SUCCESS;
  }
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  if (state) {
    std::cout << "SDL_AppQuit called with result: "
              << (result == SDL_APP_SUCCESS   ? "SUCCESS"
                  : result == SDL_APP_FAILURE ? "FAILURE"
                                              : "CONTINUE")
              << std::endl;

    if (state->app && state->initialized) {
      state->app->shutdown();
    }

    delete state;
  }
}

}  // extern "C"

SDL3AppBuilder create_sdl3_app() { return SDL3AppBuilder{}; }
