# SDL3 应用迁移指南

## 概述

本指南展示如何将传统的 VIVID 应用迁移到使用 SDL3 callbacks 的跨平台应用。

## 传统方式 vs SDL3 Callbacks

### 传统方式 (仍然支持)

```cpp
#include "vivid/app/App.h"
#include "vivid/plugins/DefaultPlugin.h"

int main() {
    auto& app = App::new_app()
        .add_plugin<DefaultPlugin>()
        .add_startup_system(my_startup_system);

    app.run();  // 传统的主循环
    return 0;
}
```

### SDL3 Callbacks 方式 (推荐用于跨平台)

```cpp
#include "vivid/app/SDL3Main.h"  // 注意：只在一个源文件中包含
#include "vivid/plugins/DefaultPlugin.h"

// 方式1：使用宏（最简单）
VIVID_SDL3_MAIN(
    create_sdl3_app()
        .add_plugin<DefaultPlugin>()
        .add_startup_system(my_startup_system)
)

// 不需要 main() 函数！SDL3 会处理入口点
```

## 迁移步骤

### 1. 更新包含的头文件

**之前:**

```cpp
#include "vivid/app/App.h"
```

**之后:**

```cpp
#include "vivid/app/SDL3Main.h"  // 替换 App.h
```

### 2. 替换应用创建方式

**之前:**

```cpp
int main() {
    auto& app = App::new_app()
        .add_plugin<DefaultPlugin>();
    app.run();
    return 0;
}
```

**之后:**

```cpp
VIVID_SDL3_MAIN(
    create_sdl3_app()
        .add_plugin<DefaultPlugin>()
)
```

### 3. 系统迁移

系统代码无需更改，SDL3 callbacks 完全兼容现有的系统：

```cpp
// 这些系统在两种模式下都能正常工作
void my_startup_system(Resources& res, entt::registry& world) {
    // 启动逻辑
}

void my_update_system(entt::registry& world) {
    // 更新逻辑
}
```

### 4. 资源管理

资源管理 API 保持不变：

```cpp
VIVID_SDL3_MAIN(
    create_sdl3_app()
        .insert_resource<MyResource>(constructor_args)
        .add_plugin<DefaultPlugin>()
)
```

## 平台特定优势

### 传统模式

- ✅ 简单直接
- ✅ 完全控制主循环
- ❌ 在某些平台上需要特殊处理（iOS, Emscripten）
- ❌ 可能不符合平台最佳实践

### SDL3 Callbacks 模式

- ✅ 跨平台兼容（iOS, Android, Web, Desktop）
- ✅ 平台优化的帧率控制
- ✅ 更好的电池寿命（移动设备）
- ✅ 与平台窗口系统更好集成
- ❌ 稍微复杂的调试（回调驱动）

## 调试提示

### 传统模式调试

```cpp
void debug_system(Resources&, entt::registry& world) {
    // 可以设置断点，单步执行主循环
}
```

### SDL3 Callbacks 调试

```cpp
void debug_system(Resources&, entt::registry& world) {
    static int frame = 0;
    frame++;

    if (frame % 60 == 0) {
        std::cout << "Frame: " << frame << std::endl;
        // 在这里设置条件断点
    }
}
```

## 性能考虑

- SDL3 callbacks 通常提供更好的性能，因为平台可以优化帧率
- 在 Web 平台上，callbacks 是必须的
- 在移动平台上，callbacks 可以显著改善电池寿命

## 兼容性

- 现有的插件和系统 100% 兼容
- 可以在同一个项目中混合使用两种模式（不同的可执行文件）
- 资源和组件系统完全相同

## 选择建议

- **桌面应用，简单需求**: 使用传统模式
- **跨平台应用**: 使用 SDL3 callbacks
- **移动/Web 应用**: 必须使用 SDL3 callbacks
- **游戏项目**: 推荐 SDL3 callbacks
