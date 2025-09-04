# SDL3App 日志和错误处理使用指南

基于 SDL3 官方文档优化后的 SDL3App 现在提供了完整的日志、错误处理和断言系统。

## 功能特性

### 1. SDL3 日志系统

- 完全集成 SDL3 的日志 API
- 支持多种日志级别和分类
- 可配置的日志输出
- 线程安全的日志记录

### 2. SDL3 错误处理

- 集成 SDL3 错误处理 API
- 便捷的错误检查函数
- 自动错误日志记录

### 3. SDL3 断言系统

- 集成 SDL3 断言 API
- 可配置的断言处理
- 断言失败时自动日志记录

## 基本使用方法

### 1. 简单应用示例

```cpp
#include "vivid/app/SDL3App.h"

VIVID_SDL3_MAIN(
    .set_app_info("My Game", "1.0.0", "com.example.mygame")
    .set_default_log_level(SDL3LogLevel::Info)
    .add_plugin<DefaultPlugin>()
    .add_startup_system(my_startup_system)
)

void my_startup_system() {
    // 使用日志系统
    SDL3Logger::app_info("Game started successfully!");
    SDL3Logger::app_debug("Debug information");
    SDL3Logger::render_debug("Renderer initialized");

    // 使用断言
    VIVID_ASSERT(some_condition);

    // 使用错误处理
    if (!SDL3ErrorHandler::check_condition(some_condition, "Something went wrong")) {
        return; // 错误已记录到日志
    }
}
```

### 2. 高级配置示例

```cpp
#include "vivid/app/SDL3App.h"

VIVID_SDL3_MAIN(
    .set_app_info("Advanced Game", "2.0.0", "com.example.advancedgame")
    // 配置日志系统
    .set_default_log_level(SDL3LogLevel::Debug)
    .set_log_level(SDL3LogCategory::Render, SDL3LogLevel::Verbose)
    .set_log_level(SDL3LogCategory::Audio, SDL3LogLevel::Warn)
    .enable_log_prefix(true)

    // 配置断言系统
    .enable_assert_logging(true)
    .set_assert_log_level(SDL3LogLevel::Critical)

    // 自定义日志输出函数
    .set_custom_log_output([](SDL3LogCategory category, SDL3LogLevel level, const char* message) {
        // 自定义日志处理逻辑
        printf("[CUSTOM] Category:%d Level:%d Message:%s\n",
               static_cast<int>(category), static_cast<int>(level), message);
    })

    // 自定义断言处理函数
    .set_custom_assert_handler([](const SDL_AssertData* data) -> SDL_AssertState {
        // 自定义断言处理逻辑
        printf("Custom assertion failed: %s\n", data->condition);
        return SDL_ASSERTION_IGNORE; // 或其他处理方式
    })

    .add_plugin<DefaultPlugin>()
    .add_startup_system(advanced_startup_system)
)
```

## 日志系统详细使用

### 1. 基本日志函数

```cpp
// 通用日志级别（应用程序分类）
SDL3Logger::trace("Detailed trace information: %d", value);
SDL3Logger::verbose("Verbose information");
SDL3Logger::debug("Debug message");
SDL3Logger::info("Information message");
SDL3Logger::warn("Warning message");
SDL3Logger::error("Error message");
SDL3Logger::critical("Critical error");
```

### 2. 分类日志函数

```cpp
// 应用程序日志
SDL3Logger::app_info("Application started");
SDL3Logger::app_error("Application error occurred");

// 系统日志
SDL3Logger::system_info("System information");
SDL3Logger::system_error("System error");

// 渲染日志
SDL3Logger::render_debug("Render debug info");
SDL3Logger::render_error("Render error");

// 通用分类日志
SDL3Logger::log(SDL3LogCategory::Audio, SDL3LogLevel::Warn, "Audio warning: %s", message);
```

### 3. 动态配置日志

```cpp
// 运行时更改日志级别
SDL3Logger::set_priority(SDL3LogCategory::Render, SDL3LogLevel::Debug);

// 重置所有日志级别
SDL3Logger::reset_priorities();

// 获取当前配置
const SDL3LogConfig& config = SDL3Logger::get_config();
```

## 错误处理系统详细使用

### 1. 基本错误处理

```cpp
// 获取最后的SDL错误
const char* error = SDL3ErrorHandler::get_error();
if (error && error[0] != '\0') {
    SDL3Logger::error("SDL Error: %s", error);
}

// 设置自定义错误
SDL3ErrorHandler::set_error("Custom error: %s", error_description);

// 清除错误
SDL3ErrorHandler::clear_error();

// 检查是否有错误
if (SDL3ErrorHandler::has_error()) {
    // 处理错误
}
```

### 2. 便捷错误检查函数

```cpp
// 检查条件，失败时设置错误
if (!SDL3ErrorHandler::check_condition(ptr != nullptr, "Pointer is null")) {
    return false; // 错误已设置和记录
}

// 检查SDL函数返回值
if (!SDL3ErrorHandler::check_sdl_result(SDL_Init(SDL_INIT_VIDEO), "SDL_Init")) {
    return false; // 错误已记录到日志
}

// 检查SDL指针返回值
SDL_Window* window = SDL_CreateWindow("Title", 800, 600, 0);
if (!SDL3ErrorHandler::check_sdl_pointer(window, "SDL_CreateWindow")) {
    return false; // 错误已记录到日志
}
```

## 断言系统详细使用

### 1. 基本断言宏

```cpp
// 标准断言（仅在调试模式有效）
VIVID_ASSERT(condition);

// 发布版本断言（在发布版本中也有效）
VIVID_ASSERT_RELEASE(critical_condition);

// 偏执断言（仅在SDL_ASSERT_LEVEL >= 3时有效）
VIVID_ASSERT_PARANOID(expensive_check());
```

### 2. 断言配置

```cpp
// 获取断言报告
const SDL_AssertData* report = SDL3AssertManager::get_assertion_report();
while (report) {
    SDL3Logger::info("Assertion: %s at %s:%d",
                    report->condition, report->filename, report->linenum);
    report = report->next;
}

// 重置断言报告
SDL3AssertManager::reset_assertion_report();
```

## 环境变量配置

可以通过环境变量配置日志行为：

```bash
# 设置日志级别
export SDL_LOGGING="app=debug,render=verbose,*=warn"

# 设置断言行为
export SDL_ASSERT="abort"  # 或 "break", "ignore", "always_ignore"
```

## 最佳实践

1. **合理设置日志级别**：开发时使用 Debug/Verbose，发布时使用 Info/Warn
2. **使用分类日志**：不同模块使用对应的日志分类
3. **错误检查**：对所有 SDL 函数调用使用错误检查
4. **断言使用**：用于检查程序假设和不变量
5. **自定义处理**：根据需要实现自定义日志输出和断言处理

## 性能考虑

- 日志调用在禁用时开销极小
- 断言在发布版本中会被完全移除（除非使用 RELEASE 版本）
- 错误检查函数经过优化，性能影响最小

这个优化版本完全基于 SDL3 官方文档，提供了专业级的日志、错误处理和调试支持。
