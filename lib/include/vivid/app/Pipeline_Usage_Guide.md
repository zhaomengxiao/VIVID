# Flecs Pipeline 使用指南

## 核心概念

### 1. Entity、Pipeline 和 Phase 的关系

在 Flecs 中，**一切皆为 Entity**：

```cpp
// Pipeline 是一个带有 Pipeline 标签的 entity
flecs::entity pipeline = world.entity("MyPipeline")
    .add(flecs::pipeline::Pipeline);

// Phase 是一个带有 Phase 标签的 entity
flecs::entity phase = world.entity<UpdatePhase>()
    .add(flecs::Phase)
    .depends_on<PreUpdatePhase>();  // 关系：DependsOn

// System 也是 entity，通过 kind() 关联到 Phase
world.system<Transform>("MoveSystem")
    .kind<UpdatePhase>()  // 建立关系：System -> Phase
    .each([](Transform& t) { });
```

### 2. 关系图示

```
World
  ├─ MainPipeline (entity with Pipeline tag)
  │   ├─ StartupPhase (entity with Phase tag)
  │   │   └─ InitSystem (system entity, kind -> StartupPhase)
  │   ├─ UpdatePhase (entity with Phase tag, depends_on -> StartupPhase)
  │   │   └─ MoveSystem (system entity, kind -> UpdatePhase)
  │   └─ RenderPhase (entity with Phase tag, depends_on -> UpdatePhase)
  │       └─ DrawSystem (system entity, kind -> RenderPhase)
  │
  └─ ShutdownPipeline (entity with Pipeline tag)
      └─ ShutdownPhase (entity with Phase tag)
          └─ CleanupSystem (system entity, kind -> ShutdownPhase)
```

## VIVID 的双 Pipeline 架构

### 设计理念

VIVID 使用两个独立的 pipeline：

1. **MainPipeline**: 每帧执行的常规系统

   - StartupPhase (仅启动时执行)
   - EventPhase
   - PreUpdatePhase
   - UpdatePhase
   - PostUpdatePhase
   - RenderPhase
   - CleanupPhase

2. **ShutdownPipeline**: 应用关闭时执行的清理系统
   - ShutdownPhase (仅关闭时执行一次)

### 为什么不使用 `flecs::OnStore`？

```cpp
// ❌ 错误方式：ShutdownPhase 依赖 OnStore
world.entity<ShutdownPhase>()
    .add(flecs::Phase)
    .depends_on(flecs::OnStore);  // OnStore 每帧都会调用！

// ✅ 正确方式：独立的 ShutdownPipeline
auto shutdown_pipeline = world.entity("ShutdownPipeline")
    .add(flecs::pipeline::Pipeline);

world.entity<ShutdownPhase>()
    .add(flecs::Phase);  // 不依赖任何默认阶段

// 只在应用关闭时切换并执行
world.set_pipeline(shutdown_pipeline);
world.progress(0);  // 执行一次 ShutdownPhase
```

## API 使用示例

### 1. 基本使用

```cpp
#include "vivid/app/App_FlecsPipeline.h"

int main() {
    App::new_app()
        // 常规系统自动在 MainPipeline 中运行
        .import_module<RenderModule>()
        .import_systems<PhysicsBundle>()
        .insert_resource<Config>()
        .run();  // 自动处理 pipeline 切换
}
```

### 2. 添加 Shutdown 系统

```cpp
struct CleanupModule {
    CleanupModule(flecs::world& world) {
        world.module<CleanupModule>();

        // 这个系统只在应用关闭时执行一次
        world.system("CleanupResources")
            .kind<ShutdownPhase>()  // 注册到 ShutdownPhase
            .iter([](flecs::iter& it) {
                std::cout << "Cleaning up resources..." << std::endl;
                // 释放资源、保存状态等
            });
    }
};

// 使用
App::new_app()
    .import_module<CleanupModule>()
    .run();  // shutdown时自动调用 CleanupResources
```

### 3. 手动控制 Pipeline 切换

```cpp
App& app = App::new_app();

// 初始化
app.initialize(0, nullptr);

// 游戏循环
while (app.is_running()) {
    app.iterate();  // 使用 MainPipeline
}

// 手动触发 Shutdown
app.use_shutdown_pipeline();
app.world().progress(0);  // 执行 ShutdownPhase

// 恢复主 pipeline（可选）
app.use_main_pipeline();
```

### 4. 查询当前 Pipeline

```cpp
App& app = App::new_app();

// 获取当前 pipeline
flecs::entity current = app.get_pipeline();
std::cout << "Current pipeline: " << current.name() << std::endl;

// 检查是否为特定 pipeline
if (current == app.world().entity("MainPipeline")) {
    std::cout << "Using main pipeline" << std::endl;
}
```

## 完整示例：游戏引擎

```cpp
// 定义游戏模块
struct GameModule {
    GameModule(flecs::world& world) {
        world.module<GameModule>();

        // 启动系统
        world.system("LoadAssets")
            .kind<StartupPhase>()
            .iter([](flecs::iter& it) {
                std::cout << "Loading assets..." << std::endl;
            });

        // 更新系统
        world.system<Transform, Velocity>("Movement")
            .kind<UpdatePhase>()
            .each([](Transform& t, Velocity& v) {
                t.x += v.x;
                t.y += v.y;
            });

        // 渲染系统
        world.system<Transform, Sprite>("Render")
            .kind<RenderPhase>()
            .each([](Transform& t, Sprite& s) {
                // 渲染逻辑
            });

        // 关闭系统
        world.system("SaveProgress")
            .kind<ShutdownPhase>()
            .iter([](flecs::iter& it) {
                std::cout << "Saving game progress..." << std::endl;
                // 保存游戏状态
            });
    }
};

// 主函数
int main() {
    App::new_app()
        .import_module<GameModule>()
        .insert_resource<GameConfig>()
        .run();

    // run() 内部执行流程：
    // 1. set_pipeline(main_pipeline)
    // 2. progress(0) -> 执行 StartupPhase (LoadAssets)
    // 3. while(running) { progress() } -> 执行 Update + Render
    // 4. set_pipeline(shutdown_pipeline)
    // 5. progress(0) -> 执行 ShutdownPhase (SaveProgress)

    return 0;
}
```

## 技术细节

### Pipeline 切换的成本

```cpp
// Pipeline 切换是轻量级操作，只是改变 world 内部的一个 entity 引用
world.set_pipeline(pipeline);  // O(1) 操作
```

### Phase 依赖解析

```cpp
// Flecs 自动根据 depends_on 关系构建执行顺序
world.entity<PhaseA>().add(flecs::Phase);
world.entity<PhaseB>().add(flecs::Phase).depends_on<PhaseA>();
world.entity<PhaseC>().add(flecs::Phase).depends_on<PhaseB>();

// 执行顺序: PhaseA -> PhaseB -> PhaseC
```

### 跨 Pipeline 的 Phase

```cpp
// ❌ 错误：Phase 不能同时属于多个 pipeline
// ShutdownPhase 不应该 depends_on 任何 MainPipeline 中的 Phase

// ✅ 正确：每个 pipeline 有独立的 Phase 层次结构
MainPipeline:
  StartupPhase -> UpdatePhase -> RenderPhase

ShutdownPipeline:
  ShutdownPhase (独立，无依赖)
```

## 常见问题

### Q: 为什么不直接在 CleanupPhase 中处理关闭逻辑？

A: CleanupPhase 每帧都会执行，用于帧间清理（如清空临时缓冲区）。ShutdownPhase 只在应用关闭时执行一次，用于最终资源释放。

### Q: ShutdownPhase 中的系统会自动清理 entity 吗？

A: 不会。Flecs 在 `world` 析构时会自动清理所有 entity 和组件。ShutdownPhase 用于自定义清理逻辑（如关闭文件、断开网络连接等）。

### Q: 可以创建更多自定义 pipeline 吗？

A: 可以！例如：

```cpp
// 创建编辑器专用 pipeline
auto editor_pipeline = world.entity("EditorPipeline")
    .add(flecs::pipeline::Pipeline);

world.entity<EditorUIPhase>().add(flecs::Phase);
world.entity<EditorToolsPhase>().add(flecs::Phase).depends_on<EditorUIPhase>();

// 在编辑器模式下切换
world.set_pipeline(editor_pipeline);
```

## 总结

- ✅ Pipeline、Phase、System 都是 entity
- ✅ 通过关系（depends_on, kind）构建执行图
- ✅ MainPipeline 用于每帧更新
- ✅ ShutdownPipeline 用于应用关闭
- ✅ 使用 `world.set_pipeline()` 切换
- ✅ `world.get_pipeline()` 获取当前 pipeline


