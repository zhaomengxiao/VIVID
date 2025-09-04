#include "vivid/log/log.h"

#include <cstdarg>

// =============================================================================
// VividLogger 静态成员定义和实现
// =============================================================================

VividLogConfig VividLogger::config_;
bool VividLogger::initialized_ = false;

void VividLogger::initialize(const VividLogConfig& config) {
  config_ = config;

  // 设置默认日志级别
  SDL_SetLogPriorities(static_cast<SDL_LogPriority>(config_.default_level));

  // 设置各分类的日志级别
  for (const auto& [category, level] : config_.category_levels) {
    SDL_SetLogPriority(static_cast<int>(category), static_cast<SDL_LogPriority>(level));
  }

  // 设置自定义日志输出函数（如果有）
  if (config_.custom_output) {
    SDL_SetLogOutputFunction(
        [](void* userdata, int category, SDL_LogPriority priority, const char* message) {
          auto* config = static_cast<const VividLogConfig*>(userdata);
          if (config && config->custom_output) {
            config->custom_output(static_cast<VividLogCategory>(category),
                                  static_cast<VividLogLevel>(priority), message);
          }
        },
        const_cast<VividLogConfig*>(&config_));
  }

  initialized_ = true;
  VividLogger::info("VIVID Logger initialized successfully");
}

void VividLogger::set_config(const VividLogConfig& config) {
  config_ = config;
  if (initialized_) {
    initialize(config_);
  }
}

const VividLogConfig& VividLogger::get_config() { return config_; }

void VividLogger::set_priority(VividLogCategory category, VividLogLevel level) {
  SDL_SetLogPriority(static_cast<int>(category), static_cast<SDL_LogPriority>(level));
  config_.set_category_level(category, level);
}

void VividLogger::reset_priorities() {
  SDL_ResetLogPriorities();
  config_.category_levels.clear();
}

void VividLogger::log(VividLogCategory category, VividLogLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(static_cast<int>(category), static_cast<SDL_LogPriority>(level), format, args);
  va_end(args);
}

// 便捷的日志输出函数（使用APPLICATION分类）
void VividLogger::trace(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, format, args);
  va_end(args);
}

void VividLogger::verbose(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE, format, args);
  va_end(args);
}

void VividLogger::debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, format, args);
  va_end(args);
}

void VividLogger::info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, args);
  va_end(args);
}

void VividLogger::warn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, format, args);
  va_end(args);
}

void VividLogger::error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, format, args);
  va_end(args);
}

void VividLogger::critical(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, format, args);
  va_end(args);
}

// 分类日志输出函数
void VividLogger::app_trace(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, format, args);
  va_end(args);
}

void VividLogger::app_debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, format, args);
  va_end(args);
}

void VividLogger::app_info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, args);
  va_end(args);
}

void VividLogger::app_warn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, format, args);
  va_end(args);
}

void VividLogger::app_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, format, args);
  va_end(args);
}

void VividLogger::app_critical(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, format, args);
  va_end(args);
}

// 系统日志
void VividLogger::system_info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_SYSTEM, SDL_LOG_PRIORITY_INFO, format, args);
  va_end(args);
}

void VividLogger::system_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_SYSTEM, SDL_LOG_PRIORITY_ERROR, format, args);
  va_end(args);
}

// 渲染日志
void VividLogger::render_debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_RENDER, SDL_LOG_PRIORITY_DEBUG, format, args);
  va_end(args);
}

void VividLogger::render_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_RENDER, SDL_LOG_PRIORITY_ERROR, format, args);
  va_end(args);
}

// GPU日志
void VividLogger::gpu_debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_DEBUG, format, args);
  va_end(args);
}

void VividLogger::gpu_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_ERROR, format, args);
  va_end(args);
}

// =============================================================================
// VividErrorHandler 实现
// =============================================================================

const char* VividErrorHandler::get_error() { return SDL_GetError(); }

bool VividErrorHandler::set_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SDL_SetErrorV(format, args);
  va_end(args);
  return false;  // 便于错误处理中直接返回
}

void VividErrorHandler::clear_error() { SDL_ClearError(); }

bool VividErrorHandler::has_error() {
  const char* error = SDL_GetError();
  return error && error[0] != '\0';
}

bool VividErrorHandler::check_sdl_result(int result, const char* operation) {
  if (result < 0) {
    const char* error = SDL_GetError();
    VividLogger::error("%s failed: %s", operation, error ? error : "Unknown error");
    return false;
  }
  return true;
}

bool VividErrorHandler::check_sdl_pointer(const void* ptr, const char* operation) {
  if (!ptr) {
    const char* error = SDL_GetError();
    VividLogger::error("%s returned null: %s", operation, error ? error : "Unknown error");
    return false;
  }
  return true;
}

// =============================================================================
// VividAssertManager 静态成员定义和实现
// =============================================================================

VividAssertConfig VividAssertManager::config_;
bool VividAssertManager::initialized_ = false;

void VividAssertManager::initialize(const VividAssertConfig& config) {
  config_ = config;

  // 设置自定义断言处理函数
  SDL_SetAssertionHandler(assert_handler, nullptr);

  initialized_ = true;
  VividLogger::info("VIVID Assert Manager initialized successfully");
}

void VividAssertManager::set_config(const VividAssertConfig& config) {
  config_ = config;
  if (initialized_) {
    initialize(config_);
  }
}

const VividAssertConfig& VividAssertManager::get_config() { return config_; }

const SDL_AssertData* VividAssertManager::get_assertion_report() {
  return SDL_GetAssertionReport();
}

void VividAssertManager::reset_assertion_report() { SDL_ResetAssertionReport(); }

SDL_AssertState VividAssertManager::assert_handler(const SDL_AssertData* data, void* userdata) {
  // 记录断言失败到日志系统
  if (config_.log_on_assert) {
    VividLogger::log(VividLogCategory::Assert, config_.assert_log_level,
                     "Assertion failed: %s at %s:%d in function %s", data->condition,
                     data->filename, data->linenum, data->function);
  }

  // 如果有自定义处理函数，调用它
  if (config_.custom_handler) {
    return config_.custom_handler(data);
  }

  // 默认行为：在调试模式下中断，在发布模式下忽略
#ifdef _DEBUG
  return SDL_ASSERTION_BREAK;
#else
  return SDL_ASSERTION_IGNORE;
#endif
}
