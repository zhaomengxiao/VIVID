#include "vivid/app/SDL3App.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

namespace vivid::app {

// 前向声明用户定义的应用创建函数
extern SDL3AppBuilder CreateAppInstance();

// SDL3 callback functions implementation

extern "C" {

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  // 创建应用状态
  auto state = std::make_unique<SDL3AppState>();

  try {
    // 使用宏定义的函数创建应用
    auto builder = CreateAppInstance();
    auto bundle = builder.ReleaseAppBundle();
    state->app_ = std::move(bundle.app_);
    state->metadata_ = std::move(bundle.metadata_);
    state->log_config_ = std::move(bundle.log_config_);
    state->assert_config_ = std::move(bundle.assert_config_);

    // 初始化日志系统
    VividLogger::initialize(state->log_config_);
    VividLogger::app_info("SDL_AppInit called with %d arguments", argc);

    // 初始化断言系统
    VividAssertManager::initialize(state->assert_config_);

    // 清除任何之前的错误
    VividErrorHandler::clear_error();

    // 应用SDL3元数据
    ApplySdl3Metadata(state->metadata_);

    // 初始化SDL子系统
    if (!VividErrorHandler::check_sdl_result(
            static_cast<int>(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)), "SDL_Init")) {
      VividLogger::app_error("Failed to initialize SDL");
      return SDL_APP_FAILURE;
    }

    // 初始化应用
    VIVID_ASSERT(state->app_ != nullptr);
    if (state->app_ && state->app_->Initialize(argc, argv)) {
      state->initialized_ = true;
      *appstate = state.release();
      VividLogger::app_info("SDL3 application initialized successfully");
      return SDL_APP_CONTINUE;
    }
    VividLogger::app_error("Failed to initialize SDL3 application");
    return SDL_APP_FAILURE;
  } catch (const std::exception& e) {
    VividLogger::app_critical("Exception during SDL_AppInit: %s", e.what());
    return SDL_APP_FAILURE;
  } catch (...) {
    VividLogger::app_critical("Unknown exception during SDL_AppInit");
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

  if (!VividErrorHandler::check_condition(state->IsValid(),
                                          "Invalid application state: not properly initialized")) {
    return SDL_APP_FAILURE;
  }

  try {
    // 执行一帧迭代
    if (state->app_->Iterate()) {
      return SDL_APP_CONTINUE;
    }  // 应用请求退出
    VividLogger::app_info("Application requested exit");
    return SDL_APP_SUCCESS;

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

  if (!VividErrorHandler::check_condition(state->IsValid(),
                                          "Invalid application state: not properly initialized")) {
    return SDL_APP_FAILURE;
  }

  try {
    // 处理特殊事件
    if (event->type == SDL_EVENT_QUIT) {
      VividLogger::app_info("Received SDL_EVENT_QUIT");
      state->app_->Exit();
      return SDL_APP_SUCCESS;
    }

    // Let the application handle events
    if (state->app_->HandleEvent(event)) {
      return SDL_APP_CONTINUE;
    }
    return SDL_APP_SUCCESS;

  } catch (const std::exception& e) {
    VividLogger::app_error("Exception during SDL_AppEvent: %s", e.what());
    return SDL_APP_FAILURE;
  } catch (...) {
    VividLogger::app_error("Unknown exception during SDL_AppEvent");
    return SDL_APP_FAILURE;
  }
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto state = std::unique_ptr<SDL3AppState>(static_cast<SDL3AppState*>(appstate));

  if (state != nullptr) {
    const char* result_str = "UNKNOWN";
    if (result == SDL_APP_SUCCESS) {
      result_str = "SUCCESS";
    } else if (result == SDL_APP_FAILURE) {
      result_str = "FAILURE";
    } else {
      result_str = "CONTINUE";
    }
    VividLogger::app_info("SDL_AppQuit called with result: %s", result_str);

    try {
      if (state->app_ && state->initialized_) {
        VividLogger::app_info("Shutting down application");
        state->app_->Shutdown();

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
  } else {
    // 即使state为空，也要确保SDL正确关闭
    SDL_Quit();
  }
}

}  // extern "C"

SDL3AppBuilder CreateSdl3App() { return SDL3AppBuilder{}; }

// 应用SDL3元数据的辅助函数实现
// 完全按照SDL3官方规范设置所有支持的元数据属性
void ApplySdl3Metadata(const SDL3AppMetadata& metadata) {
  VividLogger::app_info("Applying SDL3 metadata");

  // 清除之前的错误
  VividErrorHandler::clear_error();

  // 设置基本元数据（name, version, identifier）
  // 根据SDL3规范：name有默认值"SDL Application"，version和identifier无默认值
  if (metadata.HasBasicInfo()) {
    // 用户提供了完整的基本信息
    if (SDL_SetAppMetadata(metadata.name_.c_str(), metadata.version_.c_str(),
                           metadata.identifier_.c_str())) {
      VividLogger::app_info("SDL3 App Info: %s v%s (%s)", metadata.name_.c_str(),
                            metadata.version_.c_str(), metadata.identifier_.c_str());
    } else {
      VividLogger::app_warn("Failed to set basic app metadata: %s", VividErrorHandler::get_error());
    }
  } else {
    // 缺少必要信息，使用可用的信息和合理的默认值
    std::string const kFinalName = metadata.name_.empty() ? "SDL Application" : metadata.name_;
    std::string const kFinalVersion = metadata.version_.empty() ? "1.0.0" : metadata.version_;
    std::string const kFinalIdentifier
        = metadata.identifier_.empty() ? "com.example.sdlapp" : metadata.identifier_;

    if (metadata.version_.empty() || metadata.identifier_.empty()) {
      VividLogger::app_warn("App metadata incomplete. Missing %s%s",
                            (metadata.version_.empty() ? "version " : ""),
                            (metadata.identifier_.empty() ? "identifier " : ""));
    }

    if (SDL_SetAppMetadata(kFinalName.c_str(), kFinalVersion.c_str(), kFinalIdentifier.c_str())) {
      VividLogger::app_info("SDL3 App Info (with defaults): %s v%s (%s)", kFinalName.c_str(),
                            kFinalVersion.c_str(), kFinalIdentifier.c_str());
    } else {
      VividLogger::app_error("Failed to set app metadata: %s", VividErrorHandler::get_error());
    }
  }

  // 设置扩展属性（按SDL3规范，仅在非空时设置）
  // SDL_PROP_APP_METADATA_CREATOR_STRING - 创建者信息
  if (!metadata.creator_.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING,
                                   metadata.creator_.c_str())) {
      VividLogger::app_info("Set creator metadata: %s", metadata.creator_.c_str());
    } else {
      VividLogger::app_warn("Failed to set creator metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_COPYRIGHT_STRING - 版权信息
  if (!metadata.copyright_.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
                                   metadata.copyright_.c_str())) {
      VividLogger::app_info("Set copyright metadata: %s", metadata.copyright_.c_str());
    } else {
      VividLogger::app_warn("Failed to set copyright metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_URL_STRING - 应用网址
  if (!metadata.url_.empty()) {
    if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, metadata.url_.c_str())) {
      VividLogger::app_info("Set URL metadata: %s", metadata.url_.c_str());
    } else {
      VividLogger::app_warn("Failed to set URL metadata: %s", VividErrorHandler::get_error());
    }
  }

  // SDL_PROP_APP_METADATA_TYPE_STRING - 应用类型
  // 根据SDL3规范，默认值为"application"，总是设置此属性
  if (SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, metadata.type_.c_str())) {
    VividLogger::app_info("Set application type: %s", metadata.type_.c_str());
  } else {
    VividLogger::app_warn("Failed to set application type: %s", VividErrorHandler::get_error());
  }

  // 设置自定义属性（非SDL3官方规范）
  if (!metadata.custom_properties_.empty()) {
    VividLogger::app_info("Setting %zu custom metadata properties",
                          metadata.custom_properties_.size());
    for (const auto& [key, value] : metadata.custom_properties_) {
      if (SDL_SetAppMetadataProperty(key.c_str(), value.c_str())) {
        VividLogger::app_info("Set custom property: %s = %s", key.c_str(), value.c_str());
      } else {
        VividLogger::app_warn("Failed to set custom property %s: %s", key.c_str(),
                              VividErrorHandler::get_error());
      }
    }
  }

  VividLogger::app_info("SDL3 metadata application completed");
}
}  // namespace vivid::app
