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
  // 创建应用状态
  auto* state = new SDL3AppState();

  try {
    // 使用宏定义的函数创建应用
    auto builder = create_app_instance();
    auto bundle = builder.release_app_bundle();
    state->app = std::move(bundle.app);
    state->metadata = std::move(bundle.metadata);
    state->log_config = std::move(bundle.log_config);
    state->assert_config = std::move(bundle.assert_config);

    // 初始化日志系统
    VividLogger::initialize(state->log_config);
    VividLogger::app_info("SDL_AppInit called with %d arguments", argc);

    // 初始化断言系统
    VividAssertManager::initialize(state->assert_config);

    // 清除任何之前的错误
    VividErrorHandler::clear_error();

    // 应用SDL3元数据
    apply_sdl3_metadata(state->metadata);

    // 初始化SDL子系统
    if (!VividErrorHandler::check_sdl_result(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO),
                                             "SDL_Init")) {
      VividLogger::app_error("Failed to initialize SDL");
      delete state;
      return SDL_APP_FAILURE;
    }

    // 初始化应用
    VIVID_ASSERT(state->app != nullptr);
    if (state->app && state->app->initialize(argc, argv)) {
      state->initialized = true;
      *appstate = state;
      VividLogger::app_info("SDL3 application initialized successfully");
      return SDL_APP_CONTINUE;
    } else {
      VividLogger::app_error("Failed to initialize SDL3 application");
      delete state;
      return SDL_APP_FAILURE;
    }
  } catch (const std::exception& e) {
    VividLogger::app_critical("Exception during SDL_AppInit: %s", e.what());
    delete state;
    return SDL_APP_FAILURE;
  } catch (...) {
    VividLogger::app_critical("Unknown exception during SDL_AppInit");
    delete state;
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  // 验证应用状态
  if (!VividErrorHandler::check_condition(state != nullptr,
                                          "Invalid application state: null pointer")) {
    return SDL_APP_FAILURE;
  }

  if (!VividErrorHandler::check_condition(state->is_valid(),
                                          "Invalid application state: not properly initialized")) {
    return SDL_APP_FAILURE;
  }

  try {
    // 执行一帧迭代
    if (state->app->iterate()) {
      return SDL_APP_CONTINUE;
    } else {
      // 应用请求退出
      VividLogger::app_info("Application requested exit");
      return SDL_APP_SUCCESS;
    }
  } catch (const std::exception& e) {
    VividLogger::app_error("Exception during SDL_AppIterate: %s", e.what());
    return SDL_APP_FAILURE;
  } catch (...) {
    VividLogger::app_error("Unknown exception during SDL_AppIterate");
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  // 验证应用状态和事件指针
  if (!VividErrorHandler::check_condition(state != nullptr,
                                          "Invalid application state: null pointer")) {
    return SDL_APP_FAILURE;
  }

  if (!VividErrorHandler::check_condition(event != nullptr, "Invalid event: null pointer")) {
    return SDL_APP_FAILURE;
  }

  if (!VividErrorHandler::check_condition(state->is_valid(),
                                          "Invalid application state: not properly initialized")) {
    return SDL_APP_FAILURE;
  }

  try {
    // 处理特殊事件
    if (event->type == SDL_EVENT_QUIT) {
      VividLogger::app_info("Received SDL_EVENT_QUIT");
      state->app->exit();
      return SDL_APP_SUCCESS;
    }

    // 如果没有EventQueues，则创建一个
    auto event_queues = state->app->resources().get<EventQueues>();
    if (!event_queues) {
      event_queues = &state->app->resources().insert<EventQueues>();
    }

    event_queues->raw_sdl_events.push(*event);

    // 让应用处理事件
    if (state->app->handle_event()) {
      return SDL_APP_CONTINUE;
    } else {
      return SDL_APP_SUCCESS;
    }
  } catch (const std::exception& e) {
    VividLogger::app_error("Exception during SDL_AppEvent: %s", e.what());
    return SDL_APP_FAILURE;
  } catch (...) {
    VividLogger::app_error("Unknown exception during SDL_AppEvent");
    return SDL_APP_FAILURE;
  }
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* state = static_cast<SDL3AppState*>(appstate);

  if (state) {
    const char* result_str = (result == SDL_APP_SUCCESS   ? "SUCCESS"
                              : result == SDL_APP_FAILURE ? "FAILURE"
                                                          : "CONTINUE");
    VividLogger::app_info("SDL_AppQuit called with result: %s", result_str);

    try {
      if (state->app && state->initialized) {
        VividLogger::app_info("Shutting down application");
        state->app->shutdown();

        // 安全关闭SDL子系统
        SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
        SDL_Quit();

        VividLogger::app_info("SDL subsystems shut down successfully");
      }

      // 重置断言报告
      VividAssertManager::reset_assertion_report();

      VividLogger::app_info("Application cleanup completed");
    } catch (const std::exception& e) {
      VividLogger::app_error("Exception during SDL_AppQuit: %s", e.what());
    } catch (...) {
      VividLogger::app_error("Unknown exception during SDL_AppQuit");
    }

    delete state;
  } else {
    // 即使state为空，也要确保SDL正确关闭
    SDL_Quit();
  }
}

}  // extern "C"

SDL3AppBuilder create_sdl3_app() { return SDL3AppBuilder{}; }

// 应用SDL3元数据的辅助函数实现
// 完全按照SDL3官方规范设置所有支持的元数据属性
void apply_sdl3_metadata(const SDL3AppMetadata& metadata) {
  VividLogger::app_debug("Applying SDL3 metadata");

  // 清除之前的错误
  VividErrorHandler::clear_error();

  // 设置基本元数据（name, version, identifier）
  // 根据SDL3规范：name有默认值"SDL Application"，version和identifier无默认值
  if (metadata.has_basic_info()) {
    // 用户提供了完整的基本信息
    if (SDL_SetAppMetadata(metadata.name.c_str(), metadata.version.c_str(),
                           metadata.identifier.c_str())) {
      VividLogger::app_info("SDL3 App Info: %s v%s (%s)", metadata.name.c_str(),
                            metadata.version.c_str(), metadata.identifier.c_str());
    } else {
      VividLogger::app_warn("Failed to set basic app metadata: %s", VividErrorHandler::get_error());
    }
  } else {
    // 缺少必要信息，使用可用的信息和合理的默认值
    std::string final_name = metadata.name.empty() ? "SDL Application" : metadata.name;
    std::string final_version = metadata.version.empty() ? "1.0.0" : metadata.version;
    std::string final_identifier
        = metadata.identifier.empty() ? "com.example.sdlapp" : metadata.identifier;

    if (metadata.version.empty() || metadata.identifier.empty()) {
      VividLogger::app_warn("App metadata incomplete. Missing %s%s",
                            (metadata.version.empty() ? "version " : ""),
                            (metadata.identifier.empty() ? "identifier " : ""));
    }

    if (SDL_SetAppMetadata(final_name.c_str(), final_version.c_str(), final_identifier.c_str())) {
      VividLogger::app_info("SDL3 App Info (with defaults): %s v%s (%s)", final_name.c_str(),
                            final_version.c_str(), final_identifier.c_str());
    } else {
      VividLogger::app_error("Failed to set app metadata: %s", VividErrorHandler::get_error());
    }
  }

  // 设置扩展属性（按SDL3规范，仅在非空时设置）
  // SDL_PROP_APP_METADATA_CREATOR_STRING - 创建者信息
  if (!metadata.creator.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING,
                                   metadata.creator.c_str())) {
      VividLogger::app_debug("Set creator metadata: %s", metadata.creator.c_str());
    } else {
      VividLogger::app_warn("Failed to set creator metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_COPYRIGHT_STRING - 版权信息
  if (!metadata.copyright.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
                                   metadata.copyright.c_str())) {
      VividLogger::app_debug("Set copyright metadata: %s", metadata.copyright.c_str());
    } else {
      VividLogger::app_warn("Failed to set copyright metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_URL_STRING - 应用网址
  if (!metadata.url.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, metadata.url.c_str())) {
      VividLogger::app_debug("Set URL metadata: %s", metadata.url.c_str());
    } else {
      VividLogger::app_warn("Failed to set URL metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_TYPE_STRING - 应用类型
  // 根据SDL3规范，默认值为"application"，总是设置此属性
  if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, metadata.type.c_str())) {
    VividLogger::app_debug("Set application type: %s", metadata.type.c_str());
  } else {
    VividLogger::app_warn("Failed to set application type: %s", VividErrorHandler::get_error());
  }

  // 设置自定义属性（非SDL3官方规范）
  if (!metadata.custom_properties.empty()) {
    VividLogger::app_debug("Setting %zu custom metadata properties",
                           metadata.custom_properties.size());
    for (const auto& [key, value] : metadata.custom_properties) {
      if (SDL_SetAppMetadataProperty(key.c_str(), value.c_str())) {
        VividLogger::app_debug("Set custom property: %s = %s", key.c_str(), value.c_str());
      } else {
        VividLogger::app_warn("Failed to set custom property %s: %s", key.c_str(),
                              VividErrorHandler::get_error());
      }
    }
  }

  VividLogger::app_debug("SDL3 metadata application completed");
}
