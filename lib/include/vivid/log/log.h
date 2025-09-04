#pragma once

// VIVID 日志系统 - 基于SDL3官方API
//
// 使用方法：
// 1. 包含此头文件：#include "vivid/log/log.h"
// 2. 初始化日志系统：VividLogger::initialize()
// 3. 使用日志函数：VividLogger::info("message")

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

// =============================================================================
// VIVID 日志系统封装
// =============================================================================

// VIVID 日志级别枚举（对应SDL_LogPriority）
enum class VividLogLevel {
  Trace = SDL_LOG_PRIORITY_TRACE,       // 跟踪信息，最详细的日志
  Verbose = SDL_LOG_PRIORITY_VERBOSE,   // 详细信息
  Debug = SDL_LOG_PRIORITY_DEBUG,       // 调试信息
  Info = SDL_LOG_PRIORITY_INFO,         // 一般信息
  Warn = SDL_LOG_PRIORITY_WARN,         // 警告信息
  Error = SDL_LOG_PRIORITY_ERROR,       // 错误信息
  Critical = SDL_LOG_PRIORITY_CRITICAL  // 严重错误
};

// VIVID 日志分类枚举（对应SDL_LogCategory）
enum class VividLogCategory {
  Application = SDL_LOG_CATEGORY_APPLICATION,  // 应用程序日志
  Error = SDL_LOG_CATEGORY_ERROR,              // 错误日志
  Assert = SDL_LOG_CATEGORY_ASSERT,            // 断言日志
  System = SDL_LOG_CATEGORY_SYSTEM,            // 系统日志
  Audio = SDL_LOG_CATEGORY_AUDIO,              // 音频日志
  Video = SDL_LOG_CATEGORY_VIDEO,              // 视频日志
  Render = SDL_LOG_CATEGORY_RENDER,            // 渲染日志
  Input = SDL_LOG_CATEGORY_INPUT,              // 输入日志
  Test = SDL_LOG_CATEGORY_TEST,                // 测试日志
  GPU = SDL_LOG_CATEGORY_GPU,                  // GPU日志
  Custom = SDL_LOG_CATEGORY_CUSTOM             // 自定义日志分类起始点
};

// VIVID 日志配置结构
struct VividLogConfig {
  // 默认日志级别（对应所有未指定的分类）
  VividLogLevel default_level = VividLogLevel::Error;

  // 各分类的日志级别设置
  std::unordered_map<VividLogCategory, VividLogLevel> category_levels;

  // 自定义日志输出函数
  std::function<void(VividLogCategory, VividLogLevel, const char*)> custom_output;

  // 是否启用日志前缀（时间戳、分类等）
  bool enable_prefix = true;

  // 设置特定分类的日志级别
  void set_category_level(VividLogCategory category, VividLogLevel level) {
    category_levels[category] = level;
  }

  // 获取特定分类的日志级别
  VividLogLevel get_category_level(VividLogCategory category) const {
    auto it = category_levels.find(category);
    return it != category_levels.end() ? it->second : default_level;
  }
};

// VIVID 日志管理器类
class VividLogger {
private:
  static VividLogConfig config_;
  static bool initialized_;

public:
  // 初始化日志系统
  static void initialize(const VividLogConfig& config = VividLogConfig{});

  // 设置日志配置
  static void set_config(const VividLogConfig& config);

  // 获取当前配置
  static const VividLogConfig& get_config();

  // 设置特定分类的日志级别
  static void set_priority(VividLogCategory category, VividLogLevel level);

  // 重置所有日志级别为默认值
  static void reset_priorities();

  // 通用日志输出函数
  static void log(VividLogCategory category, VividLogLevel level, const char* format, ...);

  // 便捷的日志输出函数（使用APPLICATION分类）
  static void trace(const char* format, ...);
  static void verbose(const char* format, ...);
  static void debug(const char* format, ...);
  static void info(const char* format, ...);
  static void warn(const char* format, ...);
  static void error(const char* format, ...);
  static void critical(const char* format, ...);

  // 分类日志输出函数
  static void app_trace(const char* format, ...);
  static void app_debug(const char* format, ...);
  static void app_info(const char* format, ...);
  static void app_warn(const char* format, ...);
  static void app_error(const char* format, ...);
  static void app_critical(const char* format, ...);

  // 系统日志
  static void system_info(const char* format, ...);
  static void system_error(const char* format, ...);

  // 渲染日志
  static void render_debug(const char* format, ...);
  static void render_error(const char* format, ...);

  // GPU日志
  static void gpu_debug(const char* format, ...);
  static void gpu_error(const char* format, ...);
};

// =============================================================================
// VIVID 错误处理系统封装
// =============================================================================

// VIVID 错误处理器类
class VividErrorHandler {
public:
  // 获取最后的SDL错误信息
  static const char* get_error();

  // 设置SDL错误信息
  static bool set_error(const char* format, ...);

  // 清除SDL错误信息
  static void clear_error();

  // 检查是否有错误
  static bool has_error();

  // 错误处理辅助函数：如果condition为false，设置错误并返回false
  template <typename... Args>
  static bool check_condition(bool condition, const char* format, Args... args) {
    if (!condition) {
      set_error(format, args...);
      return false;
    }
    return true;
  }

  // 错误处理辅助函数：检查SDL函数返回值，如果失败则记录错误
  static bool check_sdl_result(int result, const char* operation = "SDL operation");

  // 错误处理辅助函数：检查SDL指针返回值，如果为空则记录错误
  static bool check_sdl_pointer(const void* ptr, const char* operation = "SDL operation");
};

// =============================================================================
// VIVID 断言系统封装
// =============================================================================

// VIVID 断言配置
struct VividAssertConfig {
  // 断言处理函数类型
  using AssertHandler = std::function<SDL_AssertState(const SDL_AssertData*)>;

  // 自定义断言处理函数
  AssertHandler custom_handler;

  // 是否在断言失败时记录日志
  bool log_on_assert = true;

  // 断言失败时的日志级别
  VividLogLevel assert_log_level = VividLogLevel::Error;
};

// VIVID 断言管理器类
class VividAssertManager {
private:
  static VividAssertConfig config_;
  static bool initialized_;

public:
  // 初始化断言系统
  static void initialize(const VividAssertConfig& config = VividAssertConfig{});

  // 设置断言配置
  static void set_config(const VividAssertConfig& config);

  // 获取当前配置
  static const VividAssertConfig& get_config();

  // 获取断言报告
  static const SDL_AssertData* get_assertion_report();

  // 重置断言报告
  static void reset_assertion_report();

  // 内部断言处理函数
  static SDL_AssertState assert_handler(const SDL_AssertData* data, void* userdata);
};

// 便捷的断言宏定义
#define VIVID_ASSERT(condition) SDL_assert(condition)
#define VIVID_ASSERT_RELEASE(condition) SDL_assert_release(condition)
#define VIVID_ASSERT_PARANOID(condition) SDL_assert_paranoid(condition)
