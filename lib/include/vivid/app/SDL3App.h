#pragma once

// SDL3 应用集成头文件
//
// 使用方法：
// 1. 包含此头文件：
//    #include "vivid/app/SDL3App.h"
// 2. 使用 VIVID_SDL3_MAIN 宏创建应用（无需其他设置）

#include <SDL3/SDL.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "App.h"
#include "vivid/log/log.h"

namespace vivid::app {

// 为了向后兼容，创建别名
using SDL3LogLevel = VividLogLevel;
using SDL3LogCategory = VividLogCategory;
using SDL3LogConfig = VividLogConfig;
using SDL3Logger = VividLogger;
using SDL3ErrorHandler = VividErrorHandler;
using SDL3AssertConfig = VividAssertConfig;
using SDL3AssertManager = VividAssertManager;

// =============================================================================
// SDL3 元数据属性枚举
// 对应SDL3官方支持的所有元数据属性
enum class SDL3MetadataProperty : std::uint8_t {
  // SDL_PROP_APP_METADATA_NAME_STRING
  // 应用程序的易读名称，如"My Game 2: Bad Guy's Revenge!"
  // 会显示在操作系统中将应用程序名称与窗口标题分开显示的位置（如音量控制小程序等）
  // 默认值："SDL Application"
  kName,

  // SDL_PROP_APP_METADATA_VERSION_STRING
  // 正在运行的应用程序版本，格式没有规定
  // 可以是"1.0.3beta2"、"April 22nd, 2024"或git哈希值等
  // 无默认值
  kVersion,

  // SDL_PROP_APP_METADATA_IDENTIFIER_STRING
  // 标识此应用程序的唯一字符串，必须采用反向域名格式，如"com.example.mygame2"
  // 用于桌面合成器识别和分组窗口，匹配应用程序与桌面设置和图标
  // 如果打包到Flatpak等容器中，应用ID应与容器名称匹配
  // 无默认值
  kIdentifier,

  // SDL_PROP_APP_METADATA_CREATOR_STRING
  // 此应用的创建者/开发者/制造者的易读名称，如"MojoWorkshop, LLC"
  // 无默认值
  kCreator,

  // SDL_PROP_APP_METADATA_COPYRIGHT_STRING
  // 人类可读的版权声明，如"Copyright (c) 2024 MojoWorkshop, LLC"
  // 请控制在一行内，不要粘贴完整的软件许可证
  // 无默认值
  kCopyright,

  // SDL_PROP_APP_METADATA_URL_STRING
  // 指向网络上应用程序的URL，可能是产品页面、店面或GitHub仓库
  // 供用户获取更多信息
  // 无默认值
  kUrl,

  // SDL_PROP_APP_METADATA_TYPE_STRING
  // 应用程序类型：
  // "game" - 视频游戏
  // "mediaplayer" - 媒体播放器
  // "application" - 通用应用程序（如果没有其他适用类型）
  // 未来的SDL版本可能会添加新类型
  // 默认值："application"
  kType
};

// SDL3 应用类型常量（符合官方规范）
namespace sdl3_app_type {
constexpr const char* kGame = "game";                // 视频游戏
constexpr const char* kMediaPlayer = "mediaplayer";  // 媒体播放器
constexpr const char* kApplication = "application";  // 通用应用程序（默认）
}  // namespace sdl3_app_type

// SDL3 应用元数据结构
// 完全对应SDL3官方支持的元数据属性
struct SDL3AppMetadata {
  // SDL_PROP_APP_METADATA_NAME_STRING - 应用程序易读名称
  // 默认值："SDL Application"
  std::string name_ = "SDL Application";

  // SDL_PROP_APP_METADATA_VERSION_STRING - 应用程序版本
  // 无默认值，格式不限（可以是"1.0.3beta2"、"April 22nd, 2024"、git哈希等）
  std::string version_;

  // SDL_PROP_APP_METADATA_IDENTIFIER_STRING - 应用程序唯一标识符
  // 无默认值，必须是反向域名格式（如"com.example.mygame2"）
  std::string identifier_;

  // SDL_PROP_APP_METADATA_CREATOR_STRING - 创建者/开发者/制造者名称
  // 无默认值（如"MojoWorkshop, LLC"）
  std::string creator_;

  // SDL_PROP_APP_METADATA_COPYRIGHT_STRING - 版权声明
  // 无默认值，应控制在一行内（如"Copyright (c) 2024 MojoWorkshop, LLC"）
  std::string copyright_;

  // SDL_PROP_APP_METADATA_URL_STRING - 应用程序网址
  // 无默认值，可以是产品页面、店面或GitHub仓库等
  std::string url_;

  // SDL_PROP_APP_METADATA_TYPE_STRING - 应用程序类型
  // 默认值："application"，可选值："game"、"mediaplayer"、"application"
  std::string type_ = "application";

  // 自定义元数据属性（不在SDL3官方规范内）
  std::unordered_map<std::string, std::string> custom_properties_;

  // 检查是否设置了基本元数据（name, version, identifier）
  // 根据SDL3规范，name有默认值，但version和identifier无默认值
  bool HasBasicInfo() const { return !name_.empty() && !version_.empty() && !identifier_.empty(); }

  // 检查是否为游戏类型应用
  bool IsGame() const { return type_ == "game"; }

  // 检查是否为媒体播放器类型应用
  bool IsMediaPlayer() const { return type_ == "mediaplayer"; }

  // 检查是否为通用应用类型
  bool IsApplication() const { return type_ == "application"; }
};

// SDL3 应用状态管理结构
struct SDL3AppState {
  std::unique_ptr<App> app_;
  SDL3AppMetadata metadata_;
  SDL3LogConfig log_config_;
  SDL3AssertConfig assert_config_;
  bool initialized_ = false;

  SDL3AppState() = default;
  ~SDL3AppState() = default;

  // 禁用拷贝和移动，确保状态唯一性
  SDL3AppState(const SDL3AppState&) = delete;
  SDL3AppState& operator=(const SDL3AppState&) = delete;
  SDL3AppState(SDL3AppState&&) = delete;
  SDL3AppState& operator=(SDL3AppState&&) = delete;

  // 状态验证
  bool IsValid() const { return app_ && initialized_; }
};

// SDL3 应用构建器类，保持链式调用风格
class SDL3AppBuilder {
private:
  std::unique_ptr<App> app_;
  SDL3AppMetadata metadata_;
  SDL3LogConfig log_config_;
  SDL3AssertConfig assert_config_;

public:
  SDL3AppBuilder() : app_(std::make_unique<App>()) {}
  ~SDL3AppBuilder() = default;

  // 禁用拷贝，启用移动
  SDL3AppBuilder(const SDL3AppBuilder&) = delete;
  SDL3AppBuilder& operator=(const SDL3AppBuilder&) = delete;
  SDL3AppBuilder(SDL3AppBuilder&&) noexcept = default;
  SDL3AppBuilder& operator=(SDL3AppBuilder&&) noexcept = default;

  // 添加模组
  template <typename T, typename... Args> SDL3AppBuilder& import_module(Args&&... args) {
    app_->ImportModule<T>(std::forward<Args>(args)...);
    return *this;
  }

  // 插入资源
  template <typename T, typename... Args> SDL3AppBuilder& insert_resource(Args&&... args) {
    app_->InsertResource<T>(std::forward<Args>(args)...);
    return *this;
  }

  // =============================================================================
  // SDL3 应用元数据设置方法
  // =============================================================================

  // 设置应用基本信息（推荐使用此方法设置核心信息）
  SDL3AppBuilder& set_app_info(const std::string& name, const std::string& version,
                               const std::string& identifier) {
    metadata_.name_ = name;
    metadata_.version_ = version;
    metadata_.identifier_ = identifier;
    return *this;
  }

  // 统一的元数据设置方法
  SDL3AppBuilder& set_metadata(SDL3MetadataProperty property, const std::string& value) {
    switch (property) {
      case SDL3MetadataProperty::kName:
        metadata_.name_ = value;
        break;
      case SDL3MetadataProperty::kVersion:
        metadata_.version_ = value;
        break;
      case SDL3MetadataProperty::kIdentifier:
        metadata_.identifier_ = value;
        break;
      case SDL3MetadataProperty::kCreator:
        metadata_.creator_ = value;
        break;
      case SDL3MetadataProperty::kCopyright:
        metadata_.copyright_ = value;
        break;
      case SDL3MetadataProperty::kUrl:
        metadata_.url_ = value;
        break;
      case SDL3MetadataProperty::kType:
        metadata_.type_ = value;
        break;
    }
    return *this;
  }

  // 设置自定义元数据属性
  SDL3AppBuilder& set_custom_metadata(const std::string& name, const std::string& value) {
    metadata_.custom_properties_[name] = value;
    return *this;
  }

  // 设置开启REST服务器
  SDL3AppBuilder& enable_rest_server() {
    app_->EnableRestServer();
    return *this;
  }

  // 设置开启flecs::stats
  SDL3AppBuilder& enable_stats() {
    app_->EnableStats();
    return *this;
  }

  // =============================================================================
  // SDL3 日志和错误处理配置方法
  // =============================================================================

  // 设置日志配置
  SDL3AppBuilder& set_log_config(const SDL3LogConfig& config) {
    log_config_ = config;
    return *this;
  }

  // 设置默认日志级别
  SDL3AppBuilder& set_default_log_level(SDL3LogLevel level) {
    log_config_.default_level = level;
    return *this;
  }

  // 设置特定分类的日志级别
  SDL3AppBuilder& set_log_level(SDL3LogCategory category, SDL3LogLevel level) {
    log_config_.set_category_level(category, level);
    return *this;
  }

  // 启用/禁用日志前缀
  SDL3AppBuilder& enable_log_prefix(bool enable = true) {
    log_config_.enable_prefix = enable;
    return *this;
  }

  // 设置自定义日志输出函数
  SDL3AppBuilder& set_custom_log_output(
      std::function<void(SDL3LogCategory, SDL3LogLevel, const char*)> output) {
    log_config_.custom_output = std::move(output);
    return *this;
  }

  // 设置断言配置
  SDL3AppBuilder& set_assert_config(const SDL3AssertConfig& config) {
    assert_config_ = config;
    return *this;
  }

  // 启用/禁用断言时日志记录
  SDL3AppBuilder& enable_assert_logging(bool enable = true) {
    assert_config_.log_on_assert = enable;
    return *this;
  }

  // 设置断言日志级别
  SDL3AppBuilder& set_assert_log_level(SDL3LogLevel level) {
    assert_config_.assert_log_level = level;
    return *this;
  }

  // 设置自定义断言处理函数
  SDL3AppBuilder& set_custom_assert_handler(SDL3AssertConfig::AssertHandler handler) {
    assert_config_.custom_handler = std::move(handler);
    return *this;
  }

  // 注意：SDL3应用通过回调系统自动运行，不需要显式的run方法

  // 获取内部App实例、元数据和配置（用于SDL3回调）
  struct SDL3AppBundle {
    std::unique_ptr<App> app_;
    SDL3AppMetadata metadata_;
    SDL3LogConfig log_config_;
    SDL3AssertConfig assert_config_;
  };

  SDL3AppBundle ReleaseAppBundle() {
    return SDL3AppBundle{std::move(app_), std::move(metadata_), std::move(log_config_),
                         std::move(assert_config_)};
  }

  // 保持向后兼容性
  std::pair<std::unique_ptr<App>, SDL3AppMetadata> release_app_with_metadata() {
    return std::make_pair(std::move(app_), std::move(metadata_));
  }

  // 保持向后兼容性
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
SDL3AppBuilder CreateSdl3App();

// 应用SDL3元数据的辅助函数
void ApplySdl3Metadata(const SDL3AppMetadata& metadata);

// =============================================================================
// SDL3 主入口宏
// =============================================================================

// 便捷函数：接受SDL3AppBuilder&参数，避免template关键字问题
template <typename ConfigFunc> SDL3AppBuilder CreateAppInstance(ConfigFunc&& config) {
  try {
    auto builder = CreateSdl3App();
    std::forward<ConfigFunc>(config)(builder);
    return std::move(builder);
  } catch (const std::exception& e) {
    VividLogger::app_error("Failed to create SDL3 app: %s", e.what());
    throw;
  }
}

}  // namespace vivid::app