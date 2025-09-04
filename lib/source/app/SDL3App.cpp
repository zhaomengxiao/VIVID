#include "vivid/app/SDL3App.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_init.h>
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
  auto [app, metadata] = builder.release_app_with_metadata();
  state->app = std::move(app);
  state->metadata = std::move(metadata);

  // 应用SDL3元数据
  apply_sdl3_metadata(state->metadata);

  SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);

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
      SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
      SDL_Quit();
    }

    delete state;
  }
}

}  // extern "C"

SDL3AppBuilder create_sdl3_app() { return SDL3AppBuilder{}; }

// 应用SDL3元数据的辅助函数实现
// 完全按照SDL3官方规范设置所有支持的元数据属性
void apply_sdl3_metadata(const SDL3AppMetadata& metadata) {
  // 设置基本元数据（name, version, identifier）
  // 根据SDL3规范：name有默认值"SDL Application"，version和identifier无默认值
  if (metadata.has_basic_info()) {
    // 用户提供了完整的基本信息
    SDL_SetAppMetadata(metadata.name.c_str(), metadata.version.c_str(),
                       metadata.identifier.c_str());
    std::cout << "SDL3 App Info: " << metadata.name << " v" << metadata.version << " ("
              << metadata.identifier << ")" << std::endl;
  } else {
    // 缺少必要信息，使用可用的信息和合理的默认值
    std::string final_name = metadata.name.empty() ? "SDL Application" : metadata.name;
    std::string final_version = metadata.version.empty() ? "1.0.0" : metadata.version;
    std::string final_identifier
        = metadata.identifier.empty() ? "com.example.sdlapp" : metadata.identifier;

    if (metadata.version.empty() || metadata.identifier.empty()) {
      std::cout << "Warning: App metadata incomplete. "
                << "Missing " << (metadata.version.empty() ? "version " : "")
                << (metadata.identifier.empty() ? "identifier " : "") << std::endl;
    }

    SDL_SetAppMetadata(final_name.c_str(), final_version.c_str(), final_identifier.c_str());
  }

  // 设置扩展属性（按SDL3规范，仅在非空时设置）
  // SDL_PROP_APP_METADATA_CREATOR_STRING - 创建者信息
  if (!metadata.creator.empty()) {
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, metadata.creator.c_str());
  }

  // SDL_PROP_APP_METADATA_COPYRIGHT_STRING - 版权信息
  if (!metadata.copyright.empty()) {
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, metadata.copyright.c_str());
  }

  // SDL_PROP_APP_METADATA_URL_STRING - 应用网址
  if (!metadata.url.empty()) {
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, metadata.url.c_str());
  }

  // SDL_PROP_APP_METADATA_TYPE_STRING - 应用类型
  // 根据SDL3规范，默认值为"application"，总是设置此属性
  SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, metadata.type.c_str());

  // 设置自定义属性（非SDL3官方规范）
  if (!metadata.custom_properties.empty()) {
    std::cout << "Setting " << metadata.custom_properties.size() << " custom metadata properties"
              << std::endl;
    for (const auto& [key, value] : metadata.custom_properties) {
      SDL_SetAppMetadataProperty(key.c_str(), value.c_str());
    }
  }
}
