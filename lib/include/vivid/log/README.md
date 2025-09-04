# VIVID 日志系统使用指南

基于 SDL3 官方 API 开发的 VIVID 日志系统，提供完整的日志、错误处理和断言功能。

## 快速开始

```cpp
#include "vivid/log/log.h"

// 初始化日志系统
VividLogger::initialize();

// 使用日志
VividLogger::info("Application started");
VividLogger::error("Something went wrong: %s", error_msg);

// 错误处理
if (!VividErrorHandler::check_sdl_result(result, "SDL_CreateWindow")) {
    return false; // 错误已自动记录
}

// 断言
VIVID_ASSERT(window != nullptr);
```

## 主要特性

### ✅ 完全基于 SDL3 官方 API

- 使用 `SDL_LogMessageV` 作为核心日志函数
- 遵循 SDL3 日志分类和优先级系统
- 支持 SDL3 的所有日志级别和分类

### ✅ 简洁的接口设计

- 便捷的日志函数：`VividLogger::info()`, `VividLogger::error()`等
- 分类日志函数：`VividLogger::app_info()`, `VividLogger::render_debug()`等
- 错误检查辅助函数：`VividErrorHandler::check_sdl_result()`
- 断言宏：`VIVID_ASSERT()`, `VIVID_ASSERT_RELEASE()`

### ✅ 高度可配置

- 支持自定义日志输出函数
- 可配置各分类的日志级别
- 可配置断言处理行为

## 详细 API 文档

### 日志系统 (VividLogger)

#### 初始化

```cpp
// 使用默认配置初始化
VividLogger::initialize();

// 使用自定义配置初始化
VividLogConfig config;
config.default_level = VividLogLevel::Debug;
config.set_category_level(VividLogCategory::Render, VividLogLevel::Verbose);
VividLogger::initialize(config);
```

#### 基本日志函数

```cpp
VividLogger::trace("Detailed trace: %d", value);
VividLogger::verbose("Verbose information");
VividLogger::debug("Debug message");
VividLogger::info("Information message");
VividLogger::warn("Warning message");
VividLogger::error("Error message");
VividLogger::critical("Critical error");
```

#### 分类日志函数

```cpp
// 应用程序日志
VividLogger::app_info("Application started");
VividLogger::app_error("Application error");
VividLogger::app_critical("Critical application error");

// 系统日志
VividLogger::system_info("System information");
VividLogger::system_error("System error");

// 渲染日志
VividLogger::render_debug("Render debug info");
VividLogger::render_error("Render error");

// GPU日志
VividLogger::gpu_debug("GPU debug info");
VividLogger::gpu_error("GPU error");
```

#### 通用日志函数

```cpp
VividLogger::log(VividLogCategory::Audio, VividLogLevel::Warn,
                "Audio warning: %s", message);
```

### 错误处理系统 (VividErrorHandler)

#### 基本错误处理

```cpp
// 获取最后的SDL错误
const char* error = VividErrorHandler::get_error();

// 设置自定义错误
VividErrorHandler::set_error("Custom error: %s", description);

// 清除错误
VividErrorHandler::clear_error();

// 检查是否有错误
if (VividErrorHandler::has_error()) {
    // 处理错误
}
```

#### 便捷检查函数

```cpp
// 检查条件
if (!VividErrorHandler::check_condition(ptr != nullptr, "Pointer is null")) {
    return false; // 错误已设置和记录
}

// 检查SDL函数返回值
if (!VividErrorHandler::check_sdl_result(SDL_Init(SDL_INIT_VIDEO), "SDL_Init")) {
    return false;
}

// 检查SDL指针返回值
SDL_Window* window = SDL_CreateWindow("Title", 800, 600, 0);
if (!VividErrorHandler::check_sdl_pointer(window, "SDL_CreateWindow")) {
    return false;
}
```

### 断言系统 (VividAssertManager)

#### 断言宏

```cpp
// 标准断言（仅在调试模式有效）
VIVID_ASSERT(condition);

// 发布版本断言（在发布版本中也有效）
VIVID_ASSERT_RELEASE(critical_condition);

// 偏执断言（仅在高断言级别时有效）
VIVID_ASSERT_PARANOID(expensive_check());
```

#### 断言管理

```cpp
// 初始化断言系统
VividAssertConfig config;
config.log_on_assert = true;
config.assert_log_level = VividLogLevel::Critical;
VividAssertManager::initialize(config);

// 获取断言报告
const SDL_AssertData* report = VividAssertManager::get_assertion_report();

// 重置断言报告
VividAssertManager::reset_assertion_report();
```

## 与 SDL3App 集成

VIVID 日志系统已完全集成到 SDL3App 中，通过类型别名提供向后兼容性：

```cpp
#include "vivid/app/SDL3App.h"

// 这些别名指向VIVID日志系统
using SDL3Logger = VividLogger;
using SDL3ErrorHandler = VividErrorHandler;
using SDL3AssertManager = VividAssertManager;

VIVID_SDL3_MAIN(
    .set_app_info("My Game", "1.0.0", "com.example.mygame")
    .set_default_log_level(VividLogLevel::Info)
    .add_plugin<DefaultPlugin>()
)
```

## 环境变量配置

可以通过 SDL3 环境变量配置日志行为：

```bash
# 设置日志级别
export SDL_LOGGING="app=debug,render=verbose,*=warn"
```

## 最佳实践

1. **合理设置日志级别**：开发时使用 Debug/Verbose，发布时使用 Info/Warn
2. **使用分类日志**：不同模块使用对应的日志分类
3. **错误检查**：对所有 SDL 函数调用使用错误检查辅助函数
4. **断言使用**：用于检查程序假设和不变量
5. **模块化使用**：可以独立使用 VIVID 日志系统，无需 SDL3App

## 性能考虑

- 日志调用在禁用时开销极小
- 断言在发布版本中会被完全移除（除非使用 RELEASE 版本）
- 错误检查函数经过优化，性能影响最小
- 基于 SDL3 的原生 API，性能和兼容性最佳
