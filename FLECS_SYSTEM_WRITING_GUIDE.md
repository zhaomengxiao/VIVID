# Flecs 系统编写指南

本文档说明在 VIVID 引擎中编写 Flecs ECS 系统的标准模式和最佳实践。

## 目录

- [单例查询快速参考](#单例查询快速参考-重要) ⭐ **新手必读**
- [核心原则](#核心原则)
- [系统编写模式](#系统编写模式)
  - [1. 纯实体组件查询](#1-纯实体组件查询)
  - [2. 混合查询（实体组件 + 单例）](#2-混合查询实体组件--单例)
  - [3. 纯单例查询](#3-纯单例查询)
    - [3.1 单个单例](#31-单个单例)
    - [3.2 多个单例](#32-多个单例--重要)
  - [4. 多查询系统](#4-多查询系统)
- [何时使用 .each() vs .run()](#何时使用-each-vs-run)
- [实际案例](#实际案例)
- [常见陷阱](#常见陷阱)
- [迁移指南](#迁移指南)

---

## 单例查询快速参考 ⭐ 重要

### 单例数量决定写法

| 单例数量  | `.term_at().src<>()` | `entity` 参数 | 说明                     |
| --------- | -------------------- | ------------- | ------------------------ |
| **1 个**  | ❌ 不使用            | ✅ 可选       | 让 Flecs 自动识别        |
| **2+ 个** | ✅ 必须全部声明      | ❌ 不能有     | 显式声明且无 entity 参数 |

### 完整示例对比

```cpp
// ============================================================================
// 场景 1：单个单例 (EventQueues)
// ============================================================================
world.system<EventQueues>("ProcessEvent")
    // ❌ 不要用 .term_at(0).src<EventQueues>()
    .each(processImpl);

static void processImpl(flecs::entity e, EventQueues& q) {
    // ✅ 可以有 entity 参数
}

// ============================================================================
// 场景 2：多个单例 (WindowContext + WebGPUContext)
// ============================================================================
world.system<WindowContext, WebGPUContext>("InitWebGPU")
    .term_at(0).src<WindowContext>()    // ✅ 必须显式声明
    .term_at(1).src<WebGPUContext>()    // ✅ 必须显式声明
    .each(initImpl);

static void initImpl(WindowContext& w, WebGPUContext& g) {
    // ❌ 不能有 entity 参数
}

// ============================================================================
// 场景 3：混合查询 (实体组件 + 单例)
// ============================================================================
world.system<MeshComponent, WebGPUContext>("SyncScene")
    .term_at(1).src<WebGPUContext>()    // ✅ 只为单例声明
    .each(syncImpl);

static void syncImpl(flecs::entity e, MeshComponent& mesh, WebGPUContext& gpu) {
    // ✅ 可以有 entity 参数，e 是 MeshComponent 所在的实体
}
```

---

## 核心原则

### ✅ 推荐做法

1. **依赖前置声明**：在系统注册时明确声明所有依赖的组件
2. **避免手动查询**：优先使用系统参数自动注入，而非在实现中手动创建 query
3. **使用 `.each()`**：对于单一查询类型的系统，使用 `.each()` 而非 `.run()`
4. **多单例规则**：⚠️ 记住 "2+ 单例 = 全部 `.src<>()` + 无 entity"
5. **保持一致**：整个项目使用统一的编写模式

### ❌ 避免做法

1. **隐藏依赖**：在实现中使用 `world.get<T>()` 访问未声明的组件
2. **嵌套查询**：在 `.run()` 中手动创建 query 并嵌套 lambda
3. **滥用 `.run()`**：对简单查询也使用 `.run()` 而非 `.each()`

---

## 系统编写模式

### 1. 纯实体组件查询

**适用场景**：系统只需要访问实体的组件，不需要访问单例资源。

#### 模式

```cpp
// ============================================================================
// 头文件：定义系统实现函数签名
// ============================================================================
struct MySystem {
  MySystem(flecs::world& world) {
    world.system<ComponentA, ComponentB>("MySystem")
        .kind(flecs::OnUpdate)
        .each(mySystemImpl);
  }

private:
  static void mySystemImpl(flecs::entity e, ComponentA& a, ComponentB& b);
};

// ============================================================================
// 源文件：实现系统逻辑
// ============================================================================
void MySystem::mySystemImpl(flecs::entity e, ComponentA& a, ComponentB& b) {
  // 直接使用组件参数
  a.value += b.delta;

  // 可以使用 entity 访问其他信息
  VividLogger::app_debug("Processing entity: %llu", e.id());
}
```

#### 实际案例：WindowUpdate

```cpp
// 头文件
struct WindowSystems {
  WindowSystems(flecs::world& world) {
    world.system<WindowComponent, WindowGpuComponent>("WindowUpdate")
        .kind(flecs::OnUpdate)
        .each(windowUpdateImpl);
  }

private:
  static void windowUpdateImpl(flecs::entity e, WindowComponent& window_comp,
                               WindowGpuComponent& gpu_comp);
};

// 源文件
void WindowSystems::windowUpdateImpl(flecs::entity entity,
                                     WindowComponent& window_comp,
                                     WindowGpuComponent& gpu_comp) {
  if (!gpu_comp.initialized || !gpu_comp.window_handle) {
    return;
  }

  // 只更新实际改变的属性
  if (window_comp.title != gpu_comp.cached_title) {
    SDL_SetWindowTitle(gpu_comp.window_handle, window_comp.title.c_str());
    gpu_comp.cached_title = window_comp.title;
  }
  // ...
}
```

---

### 2. 混合查询（实体组件 + 单例）

**适用场景**：系统需要访问实体组件，同时需要访问全局单例资源（如 WebGPU、配置等）。

#### 模式

```cpp
// ============================================================================
// 头文件
// ============================================================================
struct MySystem {
  MySystem(flecs::world& world) {
    // 关键：使用 .term_at(N).src<Singleton>() 声明单例
    world.system<EntityComp, Singleton>("MySystem")
        .term_at(1)                    // 第1个参数（索引从0开始）
        .src<Singleton>()              // 从单例获取
        .kind(flecs::OnUpdate)
        .each(mySystemImpl);
  }

private:
  static void mySystemImpl(flecs::entity e, EntityComp& comp, Singleton& singleton);
};

// ============================================================================
// 源文件
// ============================================================================
void MySystem::mySystemImpl(flecs::entity e, EntityComp& comp, Singleton& singleton) {
  // 直接使用实体组件和单例
  comp.process(singleton.config);
}
```

#### 参数索引说明

```cpp
// 索引从 0 开始
world.system<CompA, CompB, CompC, Singleton1, Singleton2>("System")
    //        ^0    ^1    ^2    ^3          ^4
    .term_at(3).src<Singleton1>()  // 第3个参数是单例
    .term_at(4).src<Singleton2>()  // 第4个参数是单例
    .each(callback);
```

#### 实际案例：InitWebGPU

```cpp
// 头文件
struct RenderSystems {
  RenderSystems(flecs::world& world) {
    world.system<WINDOW::WindowGpuComponent, WebGPUResources>("InitWebGPU")
        .term_at(1)                    // 第1个参数是单例
        .src<WebGPUResources>()
        .kind(flecs::OnStart)
        .each(initWebGPUImpl);
  }

private:
  static void initWebGPUImpl(flecs::entity e,
                             WINDOW::WindowGpuComponent& gpu_comp,
                             WebGPUResources& webgpuRes);
};

// 源文件
void RenderSystems::initWebGPUImpl(flecs::entity entity,
                                   WINDOW::WindowGpuComponent& gpu_comp,
                                   WebGPUResources& webgpuRes) {
  if (webgpuRes.initialized) return;

  // 使用窗口组件信息初始化 WebGPU
  if (gpu_comp.window_handle) {
    SDL_GetWindowSizeInPixels(gpu_comp.window_handle, &width, &height);
    webgpuRes.surface = createSurface(gpu_comp.window_handle);
  }

  webgpuRes.initialized = true;
}
```

#### 实际案例：SyncScene（多个实体组件 + 单例）

```cpp
// 头文件
world.system<MeshComponent, MaterialComponent, WebGPUResources>("SyncScene")
    .without<GpuMeshComponent>()       // 过滤条件
    .term_at(2)                        // 第2个参数是单例
    .src<WebGPUResources>()
    .kind(flecs::OnStart)
    .each(syncSceneImpl);

// 实现
void RenderSystems::syncSceneImpl(flecs::entity entity,
                                  MeshComponent& mesh,
                                  MaterialComponent& material,
                                  WebGPUResources& webgpuRes) {
  // 使用 CPU 数据创建 GPU 资源
  WGPUBuffer vertexBuffer = createBuffer(webgpuRes.device, mesh.m_Vertices);
  WGPUBuffer indexBuffer = createBuffer(webgpuRes.device, mesh.m_Indices);

  // 添加 GPU 组件
  entity.set<GpuMeshComponent>({vertexBuffer, indexBuffer, ...});
}
```

---

### 3. 纯单例查询

**适用场景**：系统只需要访问单例资源，不需要迭代实体。

#### 3.1 单个单例

⚠️ **重要**：单个单例查询**不要**使用 `.term_at().src<>()`，让 Flecs 自动识别！

```cpp
// ============================================================================
// 头文件
// ============================================================================
struct MySystem {
  MySystem(flecs::world& world) {
    // ✅ 正确：不使用 .term_at().src<>()
    world.system<Singleton>("MySystem")
        .kind(flecs::PreUpdate)
        .each(mySystemImpl);
  }

private:
  static void mySystemImpl(flecs::entity e, Singleton& singleton);
};

// ============================================================================
// 源文件
// ============================================================================
void MySystem::mySystemImpl(flecs::entity e, Singleton& singleton) {
  // e 是单例组件所在的实体
  // 此函数只执行一次
  singleton.update();
}
```

#### 为什么不用 `.term_at().src<>()`？

```cpp
// ❌ 错误：会导致运行时断言失败
world.system<Singleton>("MySystem")
    .term_at(0).src<Singleton>()  // 不要这样做！
    .each(callback);

// 错误原因：
// - .term_at().src<>() 覆盖了默认的 $this 源
// - 查询不再匹配任何实体
// - iter->entities == nullptr
// - .each() 需要 entity 参数，触发断言失败
```

详见：[`FLECS_SINGLETON_QUERY_GUIDE.md`](./FLECS_SINGLETON_QUERY_GUIDE.md)

#### 3.2 多个单例 ⭐ 重要

⚠️ **关键发现**：当系统查询**多个单例**时，规则完全不同！

##### 模式

```cpp
// ============================================================================
// 头文件
// ============================================================================
struct MySystem {
  MySystem(flecs::world& world) {
    // ✅ 正确：必须为每个单例使用 .term_at().src<>()
    // ✅ 关键：实现函数 NO entity 参数！
    world.system<Singleton1, Singleton2>("MySystem")
        .term_at(0).src<Singleton1>()  // 第0个参数是单例
        .term_at(1).src<Singleton2>()  // 第1个参数是单例
        .kind(flecs::PreUpdate)
        .each(mySystemImpl);
  }

private:
  // ⚠️ 注意：NO flecs::entity 参数！
  static void mySystemImpl(Singleton1& s1, Singleton2& s2);
};

// ============================================================================
// 源文件
// ============================================================================
void MySystem::mySystemImpl(Singleton1& s1, Singleton2& s2) {
  // 直接使用两个单例，没有 entity 参数
  s1.update(s2);
}
```

##### 为什么必须这样做？

```cpp
// ❌ 错误 1：不使用 .term_at().src<>()
world.system<Singleton1, Singleton2>("MySystem")
    .each([](flecs::entity e, Singleton1& s1, Singleton2& s2) { ... });
// 结果：系统不会执行！Flecs 找不到同时拥有两个单例的实体

// ❌ 错误 2：使用 .term_at().src<>() 但有 entity 参数
world.system<Singleton1, Singleton2>("MySystem")
    .term_at(0).src<Singleton1>()
    .term_at(1).src<Singleton2>()
    .each([](flecs::entity e, Singleton1& s1, Singleton2& s2) { ... });
// 结果：系统不会执行！有 entity 参数会导致查询匹配失败

// ✅ 正确：使用 .term_at().src<>() 且 NO entity 参数
world.system<Singleton1, Singleton2>("MySystem")
    .term_at(0).src<Singleton1>()
    .term_at(1).src<Singleton2>()
    .each([](Singleton1& s1, Singleton2& s2) { ... });
// 结果：系统正确执行一次
```

#### 实际案例：InitWebGPU 和 InitImGui

```cpp
// 头文件
struct RenderSystems {
  RenderSystems(flecs::world& world) {
    // 两个单例：WindowContext 和 WebGPUContext
    world.system<WINDOW::WindowContext, WebGPUContext>("InitWebGPU")
        .term_at(0).src<WINDOW::WindowContext>()  // 显式声明单例
        .term_at(1).src<WebGPUContext>()          // 显式声明单例
        .kind(flecs::OnStart)
        .each(initWebGPUImpl);
  }

private:
  // NO entity 参数！
  static void initWebGPUImpl(WINDOW::WindowContext& windowContext,
                             WebGPUContext& webgpuRes);
};

// 源文件
void RenderSystems::initWebGPUImpl(WINDOW::WindowContext& windowContext,
                                   WebGPUContext& webgpuRes) {
  if (webgpuRes.initialized) return;

  // 使用两个单例初始化 WebGPU
  if (windowContext.window_handle) {
    webgpuRes.surface = createSurface(windowContext.window_handle);
  }

  webgpuRes.initialized = true;
}
```

#### 实际案例：ProcessImGuiEvent（单个单例）

```cpp
// 头文件
struct UISystems {
  UISystems(flecs::world& world) {
    // ✅ 纯单例，不用 .term_at().src<>()
    world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
        .kind(flecs::PreUpdate)
        .each(processImGuiEventImpl);
  }

private:
  static void processImGuiEventImpl(flecs::entity e,
                                    VIVID::APP::EventQueues& eventQueues);
};

// 源文件
void UISystems::processImGuiEventImpl(flecs::entity e,
                                      VIVID::APP::EventQueues& eventQueues) {
  if (!eventQueues.raw_sdl_events.empty()) {
    ImGui_ImplSDL3_ProcessEvent(&eventQueues.raw_sdl_events.front());
    eventQueues.raw_sdl_events.pop();
  }
}
```

---

### 4. 多查询系统

**适用场景**：系统需要多个不同类型的查询（如渲染系统需要查询窗口、相机、光源、网格等）。

#### 模式

对于这种复杂情况，保持使用 `.run()` 方式：

```cpp
// ============================================================================
// 头文件
// ============================================================================
struct MySystem {
  MySystem(flecs::world& world) {
    world.system("ComplexSystem")
        .kind(flecs::OnUpdate)
        .run(complexSystemImpl);
  }

private:
  static void complexSystemImpl(flecs::iter& it);
};

// ============================================================================
// 源文件
// ============================================================================
void MySystem::complexSystemImpl(flecs::iter& it) {
  auto world = it.world();

  // 查询类型 A
  world.each<TypeA>([&](flecs::entity e, TypeA& a) {
    // ...
  });

  // 查询类型 B
  world.each<TypeB, TypeC>([&](flecs::entity e, TypeB& b, TypeC& c) {
    // ...
  });

  // 访问单例
  auto& singleton = world.get<Singleton>();
  // ...
}
```

#### 实际案例：Draw 系统

```cpp
void RenderSystems::drawImpl(flecs::iter& it) {
  auto world = it.world();
  auto& webgpuRes = world.get<WebGPUResources>();

  // 查询窗口大小
  int width = 0, height = 0;
  world.each<WindowGpuComponent>([&](flecs::entity e, WindowGpuComponent& gpu) {
    SDL_GetWindowSizeInPixels(gpu.window_handle, &width, &height);
  });

  // 查询相机
  glm::mat4 viewMatrix, projectionMatrix;
  world.each<TransformComponent, CameraComponent>([&](flecs::entity e,
                                                      TransformComponent& transform,
                                                      CameraComponent& camera) {
    viewMatrix = buildViewMatrix(transform);
    projectionMatrix = camera.ProjectionMatrix;
  });

  // 查询光源
  glm::vec3 lightPos;
  world.each<TransformComponent, LightComponent>([&](flecs::entity e,
                                                     TransformComponent& transform,
                                                     LightComponent& light) {
    lightPos = transform.Position;
  });

  // 绘制所有网格
  world.each<GpuMeshComponent, TransformComponent>([&](flecs::entity e,
                                                       GpuMeshComponent& gpu,
                                                       TransformComponent& transform) {
    drawMesh(gpu, transform, viewMatrix, projectionMatrix, lightPos);
  });
}
```

---

## 何时使用 .each() vs .run()

### 使用 `.each()` 的场景 ✅

| 场景             | 示例                                                                    |
| ---------------- | ----------------------------------------------------------------------- |
| 单一实体类型查询 | `system<CompA, CompB>().each(...)`                                      |
| 混合查询         | `system<EntityComp, Singleton>().term_at(1).src<Singleton>().each(...)` |
| 纯单例查询       | `system<Singleton>().each(...)`                                         |

**优点**：

- ✅ 依赖关系明确
- ✅ 性能更好（query 预编译缓存）
- ✅ 代码更简洁
- ✅ 类型安全

### 使用 `.run()` 的场景 ⏸️

| 场景         | 示例                                  |
| ------------ | ------------------------------------- |
| 多个不同查询 | 渲染系统（窗口 + 相机 + 光源 + 网格） |
| 清理系统     | 需要全局操作 + 迭代清理               |
| 复杂初始化   | 包含大量非 ECS 逻辑                   |

**特点**：

- 灵活性高，可以执行任意逻辑
- 需要手动管理查询
- 代码相对复杂

---

## 实际案例

### UI 模组

```cpp
struct UISystems {
  UISystems(flecs::world& world) {
    world.module<UISystems>();
    world.import<UIComponents>();

    // 1. 混合查询：WindowGpuComponent + WebGPUResources
    world.system<WINDOW::WindowGpuComponent, RENDER::WebGPUResources>("InitImGui")
        .term_at(1).src<RENDER::WebGPUResources>()
        .kind(flecs::OnStart)
        .each(initImGuiImpl);

    // 2. 纯单例查询：EventQueues
    world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
        .kind(flecs::PreUpdate)
        .each(processImGuiEventImpl);

    // 3. 无依赖系统
    world.system("ShowImGuiDemo")
        .kind(flecs::PreUpdate)
        .run(showImGuiDemoImpl);
  }

private:
  static void initImGuiImpl(flecs::entity e,
                           WINDOW::WindowGpuComponent& gpu_comp,
                           RENDER::WebGPUResources& webgpuRes);
  static void processImGuiEventImpl(flecs::entity e,
                                    VIVID::APP::EventQueues& eventQueues);
  static void showImGuiDemoImpl(flecs::iter& it);
};
```

### Window 模组

```cpp
struct WindowSystems {
  WindowSystems(flecs::world& world) {
    world.module<WindowSystems>();
    world.import<WindowComponents>();

    // 1. 实体组件查询
    world.system<WindowComponent, WindowGpuComponent>("WindowInitialization")
        .kind(flecs::OnStart)
        .each(windowInitializationImpl);

    // 2. 实体组件查询（每帧）
    world.system<WindowComponent, WindowGpuComponent>("WindowUpdate")
        .kind(flecs::OnUpdate)
        .each(windowUpdateImpl);

    // 3. 清理系统（保持 .run()）
    world.system("WindowCleanup")
        .kind<ShutdownPhase>()
        .run(windowCleanupImpl);
  }

private:
  static void windowInitializationImpl(flecs::entity e,
                                       WindowComponent& window_comp,
                                       WindowGpuComponent& gpu_comp);
  static void windowUpdateImpl(flecs::entity e,
                               WindowComponent& window_comp,
                               WindowGpuComponent& gpu_comp);
  static void windowCleanupImpl(flecs::iter& it);
};
```

### Render 模组

```cpp
struct RenderSystems {
  RenderSystems(flecs::world& world) {
    world.module<RenderSystems>();
    world.import<RenderComponents>();

    // 1. 混合查询：WindowGpuComponent + WebGPUResources
    world.system<WINDOW::WindowGpuComponent, WebGPUResources>("InitWebGPU")
        .term_at(1).src<WebGPUResources>()
        .kind(flecs::OnStart)
        .each(initWebGPUImpl);

    // 2. 混合查询：MeshComponent + MaterialComponent + WebGPUResources
    world.system<MeshComponent, MaterialComponent, WebGPUResources>("SyncScene")
        .without<GpuMeshComponent>()
        .term_at(2).src<WebGPUResources>()
        .kind(flecs::OnStart)
        .each(syncSceneImpl);

    // 3. 多查询系统（保持 .run()）
    world.system("Draw")
        .kind(flecs::OnUpdate)
        .run(drawImpl);

    // 4. 清理系统（保持 .run()）
    world.system("ReleaseWebGPUResources")
        .kind<ShutdownPhase>()
        .run(releaseWebGPUResourcesImpl);
  }

private:
  static void initWebGPUImpl(flecs::entity e,
                            WINDOW::WindowGpuComponent& gpu_comp,
                            WebGPUResources& webgpuRes);
  static void syncSceneImpl(flecs::entity e,
                           MeshComponent& mesh,
                           MaterialComponent& material,
                           WebGPUResources& webgpuRes);
  static void drawImpl(flecs::iter& it);
  static void releaseWebGPUResourcesImpl(flecs::iter& it);
};
```

---

## 常见陷阱

### ❌ 陷阱 1：纯单例使用 `.term_at().src<>()`

```cpp
// ❌ 错误
world.system<EventQueues>("Process")
    .term_at(0).src<EventQueues>()  // 导致运行时错误！
    .each(callback);

// ✅ 正确
world.system<EventQueues>("Process")
    .each(callback);  // Flecs 自动识别单例
```

**错误现象**：

```
assert(iter->entities != nullptr): query does not return entities
($this variable is not populated)
```

### ❌ 陷阱 2：索引错误

```cpp
// ❌ 错误：索引从 0 开始，不是 1
world.system<EntityComp, Singleton>("System")
    .term_at(2).src<Singleton>()  // 越界！
    .each(callback);

// ✅ 正确
world.system<EntityComp, Singleton>("System")
    .term_at(1).src<Singleton>()  // 第1个参数（索引从0开始）
    .each(callback);
```

### ❌ 陷阱 3：嵌套手动查询

```cpp
// ❌ 避免：在 .run() 中手动创建 query
void MySystem::impl(flecs::iter& it) {
  auto world = it.world();
  auto query = world.query<ComponentA, ComponentB>();  // 每次都创建！
  query.each([&](flecs::entity e, ComponentA& a, ComponentB& b) {
    // ...
  });
}

// ✅ 正确：使用 .each() 让 Flecs 管理 query
world.system<ComponentA, ComponentB>("MySystem")
    .each(impl);

void MySystem::impl(flecs::entity e, ComponentA& a, ComponentB& b) {
  // ...
}
```

### ❌ 陷阱 4：忘记包含必要的头文件

```cpp
// 在头文件中使用 WINDOW::WindowGpuComponent
static void initWebGPUImpl(flecs::entity e,
                           WINDOW::WindowGpuComponent& gpu_comp,  // ❌ 错误类型
                           WebGPUResources& webgpuRes);

// ✅ 正确：在头文件顶部添加
#include "vivid/window/window_component.h"
```

---

## 迁移指南

### 从旧模式迁移到新模式

#### Step 1：识别系统类型

```cpp
// 旧代码
void MySystem::impl(flecs::iter& it) {
  auto world = it.world();
  auto& singleton = world.get<Singleton>();  // 访问单例

  auto query = world.query<CompA, CompB>();   // 创建查询
  query.each([&](flecs::entity e, CompA& a, CompB& b) {
    // 使用单例和组件
  });
}
```

**分析**：这是一个混合查询（实体组件 + 单例）

#### Step 2：更新头文件

```cpp
// 修改前
static void impl(flecs::iter& it);

// 修改后
static void impl(flecs::entity e, CompA& a, CompB& b, Singleton& singleton);
```

#### Step 3：更新系统注册

```cpp
// 修改前
world.system("MySystem").kind(flecs::OnUpdate).run(impl);

// 修改后
world.system<CompA, CompB, Singleton>("MySystem")
    .term_at(2).src<Singleton>()
    .kind(flecs::OnUpdate)
    .each(impl);
```

#### Step 4：更新实现

```cpp
// 修改前
void MySystem::impl(flecs::iter& it) {
  auto world = it.world();
  auto& singleton = world.get<Singleton>();

  auto query = world.query<CompA, CompB>();
  query.each([&](flecs::entity e, CompA& a, CompB& b) {
    // 逻辑代码
    a.process(b, singleton);
  });
}

// 修改后
void MySystem::impl(flecs::entity e, CompA& a, CompB& b, Singleton& singleton) {
  // 直接使用参数
  a.process(b, singleton);
}
```

---

## 总结

### 快速决策表

| 查询类型 | 模式                                            | `.term_at().src<>()`? |
| -------- | ----------------------------------------------- | --------------------- |
| 实体组件 | `.system<A, B>().each(...)`                     | ❌ 不需要             |
| 纯单例   | `.system<S>().each(...)`                        | ❌ **不要用**！       |
| 混合查询 | `.system<A, S>().term_at(1).src<S>().each(...)` | ✅ 需要               |
| 多查询   | `.system().run(...)`                            | N/A                   |

### 编码检查清单

- [ ] 所有依赖在系统注册时声明？
- [ ] 避免在实现中手动创建 query？
- [ ] 纯单例查询没有使用 `.term_at().src<>()`？
- [ ] 混合查询正确使用了 `.term_at(N).src<>()`？
- [ ] 索引计算正确（从 0 开始）？
- [ ] 必要的头文件已包含？
- [ ] 代码风格与项目其他部分一致？

### 参考文档

- [Flecs 单例查询指南](./FLECS_SINGLETON_QUERY_GUIDE.md) - 深入理解单例查询
- [Flecs 官方文档 - Systems](https://www.flecs.dev/flecs/md_docs_2Systems.html)
- [Flecs 官方文档 - Queries](https://www.flecs.dev/flecs/md_docs_2Queries.html)

---

**创建日期**：2025-10-14  
**版本**：1.0  
**维护者**：VIVID Engine Team
