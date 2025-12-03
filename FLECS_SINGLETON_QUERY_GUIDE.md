# Flecs 单例查询的正确使用方式

## 问题背景

在使用 Flecs ECS 时，遇到了单例组件查询的问题。当尝试按照混合查询（实体组件 + 单例）的模式来查询纯单例时，出现了运行时错误：

```
fatal: delegate.hpp: 328: assert(iter->entities != nullptr):
query does not return entities ($this variable is not populated) (INVALID_PARAMETER)
```

## 错误代码

```cpp
// ❌ 错误：对纯单例使用 .term_at().src<>()
world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
    .term_at(0)
    .src<VIVID::APP::EventQueues>()
    .kind(flecs::PreUpdate)
    .each(processImGuiEventImpl);
```

## 正确代码

```cpp
// ✅ 正确：纯单例查询不需要 .term_at().src<>()
world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
    .kind(flecs::PreUpdate)
    .each(processImGuiEventImpl);
```

## 核心原理

### 1. 纯单例查询

当系统**只查询单例组件**时，Flecs 会自动识别并正确处理：

```cpp
// 系统定义
world.system<SingletonComponent>("MySystem")
    .each(mySystemImpl);

// 实现函数
static void mySystemImpl(flecs::entity e, SingletonComponent& singleton) {
    // e 是单例实体本身
    // singleton 是单例组件的引用
}
```

**特点**：

- `.each()` 会执行**一次**，迭代单例实体
- `flecs::entity e` 参数是单例组件所在的特殊实体
- 不需要显式使用 `.term_at().src<>()`

### 2. 混合查询（实体组件 + 单例）

当系统**同时查询实体组件和单例**时，需要明确指定哪个是单例：

```cpp
// 系统定义
world.system<EntityComponent, SingletonComponent>("MySystem")
    .term_at(1)  // 指定第1个参数（索引从0开始，所以1是第二个参数）
    .src<SingletonComponent>()  // 声明它来自单例
    .each(mySystemImpl);

// 实现函数
static void mySystemImpl(flecs::entity e, EntityComponent& comp, SingletonComponent& singleton) {
    // e 是拥有 EntityComponent 的实体
    // comp 是实体的组件
    // singleton 是单例组件（每次迭代都是同一个）
}
```

**特点**：

- `.each()` 迭代所有拥有 `EntityComponent` 的实体
- `flecs::entity e` 参数是当前迭代的实体
- 单例组件在每次迭代中都是同一个实例

### 3. 为什么纯单例不能使用 `.term_at().src<>()`？

`.term_at(index).src<T>()` 的语义是：

> "第 `index` 个组件不是从当前迭代的实体获取，而是从单例 `T` 获取"

这个语义**隐含了一个前提**：有实体可以迭代。

当所有组件都通过 `.src<>()` 声明为单例时：

- 查询不再关联任何实体组件
- Flecs 无法构建实体迭代器
- `.each()` 的 `entity` 参数无法填充
- 导致断言失败：`iter->entities != nullptr`

## 实际案例对比

### 案例 1：InitImGui 系统（混合查询）

```cpp
// 查询：实体的 WindowGpuComponent + 单例 WebGPUResources
world.system<WINDOW::WindowGpuComponent, RENDER::WebGPUResources>("InitImGui")
    .term_at(1)  // 第1个参数是单例
    .src<RENDER::WebGPUResources>()
    .kind(flecs::OnStart)
    .each(initImGuiImpl);

// 实现
static void initImGuiImpl(flecs::entity e,
                         WINDOW::WindowGpuComponent& gpu_comp,
                         RENDER::WebGPUResources& webgpuRes) {
    // e 是拥有 WindowGpuComponent 的实体
    // 迭代所有拥有 WindowGpuComponent 的实体
}
```

**分析**：

- 有实体组件 `WindowGpuComponent` 可以迭代
- 需要 `.term_at(1).src<>()` 告诉 Flecs 第二个参数是单例
- ✅ 正确使用

### 案例 2：ProcessImGuiEvent 系统（纯单例查询）

```cpp
// 查询：只有单例 EventQueues
world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
    .kind(flecs::PreUpdate)
    .each(processImGuiEventImpl);  // 不需要 .term_at().src<>()

// 实现
static void processImGuiEventImpl(flecs::entity e,
                                  VIVID::APP::EventQueues& eventQueues) {
    // e 是 EventQueues 单例所在的实体
    // 只执行一次，访问单例
}
```

**分析**：

- 只有单例组件，无实体组件
- Flecs 自动识别 `EventQueues` 为单例
- 不需要 `.term_at().src<>()`（用了反而会出错）
- ✅ 正确使用

## 总结规则

### ✅ 正确用法

1. **纯单例查询**：

   ```cpp
   world.system<Singleton>().each(callback);
   ```

2. **混合查询（实体组件在前）**：

   ```cpp
   world.system<EntityComp, Singleton>()
       .term_at(1).src<Singleton>()
       .each(callback);
   ```

3. **混合查询（多个单例）**：
   ```cpp
   world.system<EntityComp, Singleton1, Singleton2>()
       .term_at(1).src<Singleton1>()
       .term_at(2).src<Singleton2>()
       .each(callback);
   ```

### ❌ 错误用法

1. **纯单例使用 `.term_at().src<>()`**：

   ```cpp
   // ❌ 错误！
   world.system<Singleton>()
       .term_at(0).src<Singleton>()  // 多余且会导致错误
       .each(callback);
   ```

2. **索引错误**：
   ```cpp
   // ❌ 错误！索引从0开始，第二个参数应该是 term_at(1)
   world.system<EntityComp, Singleton>()
       .term_at(2).src<Singleton>()  // 索引越界
       .each(callback);
   ```

## 底层原理（基于 Flecs 源码分析）

### 源码位置

- `flecs.h`: 核心 C API 定义
- `flecs/addons/cpp/delegate.hpp`: C++ 回调委托实现
- `flecs/addons/cpp/utils/signature.hpp`: 类型签名推断
- `flecs/addons/cpp/mixins/term/builder_i.hpp`: Term builder API

### 关键机制

#### 1. `$this` 默认源（来自 `flecs.h:779-781, 4485-4486`）

```c
typedef struct ecs_term_ref_t {
    ecs_entity_t id;  // If left to 0 and flags does not specify whether
                      // id is an entity or a variable the id will be
                      // initialized to #EcsThis.
    // ...
} ecs_term_ref_t;
```

**官方注释**：

> "If ecs_term_t::src is not populated, it will be automatically initialized to the $this source for the created query."

这意味着：

- **默认情况下**，每个 term 的 `src` 会自动设置为 `EcsThis`（$this 变量）
- `$this` 代表"当前迭代的实体"
- 查询会迭代拥有相应组件的实体

#### 2. `.src<T>()` 覆盖默认行为（来自 `builder_i.hpp:142-146`）

```cpp
template<typename T>
Base& src() {
    this->src(_::type<T>::id(this->world_v()));  // 设置 src.id 为 T 的类型 ID
    return *this;
}
```

当调用 `.term_at(i).src<T>()` 时：

- **显式设置** term 的 `src.id` 为 `T` 的 ID（通常是单例实体）
- **覆盖了** 默认的 `EcsThis` 行为
- 告诉查询："第 i 个组件不是从当前迭代的实体获取，而是从实体 T 获取"

#### 3. 回调类型与实体需求（来自 `delegate.hpp:318-332, 371-376`）

Flecs 支持多种回调签名，通过模板重载区分：

```cpp
// 签名 1: func(flecs::entity, Components...)
// 需要 iter->entities != nullptr
template <...>
static void invoke_callback(ecs_iter_t *iter, const Func& func, size_t i, ...) {
    ecs_assert(iter->entities != nullptr, ECS_INVALID_PARAMETER,   // 第 327 行
        "query does not return entities ($this variable is not populated)");
    func(flecs::entity(iter->world, iter->entities[i]), ...);  // 传递实体参数
}

// 签名 2: func(Components...)
// 不需要 iter->entities
template <...>
static void invoke_callback(ecs_iter_t *iter, const Func& func, size_t i, ...) {
    // count 处理逻辑（第 371-376 行）:
    size_t count = static_cast<size_t>(iter->count);
    if (count == 0 && !iter->table) {
        // If query has no This terms, count can be 0. Since each does not
        // have an entity parameter, just pass through components
        count = 1;  // 单例情况，执行一次
    }
    func(...);  // 不传递实体参数
}
```

**关键点**：

- 签名 1 (`func(entity, ...)`) **必须有** `iter->entities`，因为需要传递实体参数
- 签名 2 (`func(...)`) **不需要** `iter->entities`，可以处理纯单例查询

#### 4. 为什么纯单例 + `.src<>()` 会失败？

当系统这样定义时：

```cpp
world.system<EventQueues>("ProcessEvent")
    .term_at(0).src<EventQueues>()  // ❌ 显式标记为单例
    .each(func);  // func(entity e, EventQueues& q)
```

执行流程：

1. `.term_at(0).src<EventQueues>()` 设置 `term[0].src.id = EventQueues的ID`（不是 `EcsThis`）
2. 查询中**没有任何 term 使用 `$this` 源**（都被 `.src<>()` 覆盖了）
3. 查询不匹配任何实体，`iter->entities == nullptr`
4. `.each()` 尝试调用 `func(entity, ...)`
5. `invoke_callback` 中的断言触发：**`iter->entities != nullptr` 失败**

#### 5. 不使用 `.src<>()` 为什么可以工作？

当系统这样定义时：

```cpp
world.system<EventQueues>("ProcessEvent")
    .each(func);  // func(entity e, EventQueues& q)
```

执行流程：

1. `term[0].src` **未被显式设置**
2. Flecs **自动初始化** `term[0].src.id = EcsThis`（根据 flecs.h:779-781）
3. Flecs 检测到 `EventQueues` 是单例组件
4. 查询找到唯一的单例实体
5. `iter->entities[0]` 指向单例实体
6. `.each()` 成功调用 `func(entity, ...)` **一次**
7. `entity` 参数是单例组件所在的特殊实体

### 总结

| 场景                      | `.src<>()`  | `term.src.id`     | `iter->entities` | `.each()` 结果     |
| ------------------------- | ----------- | ----------------- | ---------------- | ------------------ |
| 纯单例（不用 `.src<>()`） | ❌ 未使用   | `EcsThis`（自动） | 单例实体         | ✅ 成功，执行 1 次 |
| 纯单例（用 `.src<>()`）   | ✅ 使用     | 单例 ID           | `nullptr`        | ❌ 断言失败        |
| 混合查询                  | ✅ 部分使用 | 混合              | 实体列表         | ✅ 成功，迭代实体  |

**核心要点**：

- `.src<T>()` 是为**混合查询**设计的，用于区分"实体组件"和"单例组件"
- 对于**纯单例查询**，不应使用 `.src<>()`，让 Flecs 自动处理
- Flecs 的自动推断逻辑会正确识别单例，并设置合适的迭代器

## 最佳实践建议

1. **优先使用自动推断**：

   - 对于纯单例查询，让 Flecs 自动识别
   - 代码更简洁，不易出错

2. **只在必要时使用 `.term_at().src<>()`**：

   - 仅在混合查询（实体组件 + 单例）时使用
   - 明确指定哪些是单例，哪些是实体组件

3. **保持一致的参数顺序**：

   - 建议：实体组件在前，单例在后
   - 例如：`system<EntityComp1, EntityComp2, Singleton1, Singleton2>`
   - 便于阅读和维护

4. **使用 `.run()` 作为备选方案**：

   - 如果觉得 `.each()` 语义不清晰，可以使用 `.run()`
   - 通过 `it.field<T>(index)` 显式访问组件
   - 更灵活，但代码稍显冗长

   ```cpp
   world.system<EventQueues>("ProcessEvent")
       .run([](flecs::iter& it) {
           auto* events = it.field<EventQueues>(0);
           // ...
       });
   ```

## API 设计问题与改进建议

### 当前设计的不一致性

Flecs 的单例查询 API 存在以下不一致性问题：

#### 问题 1：隐式 vs 显式行为不统一

```cpp
// 情况 A：纯单例 - 依赖隐式行为（不能显式）
world.system<Singleton>()
    // ❌ 不能用 .term_at(0).src<Singleton>()
    .each(callback);

// 情况 B：混合查询 - 必须显式声明
world.system<EntityComp, Singleton>()
    .term_at(1).src<Singleton>()  // ✅ 必须显式
    .each(callback);
```

**问题**：同样是单例，一种情况下不能显式声明，另一种必须显式声明，缺乏一致性。

#### 问题 2：错误提示不够清晰

当错误使用时，报错信息：

```
assert(iter->entities != nullptr): query does not return entities
($this variable is not populated)
```

**问题**：错误信息没有指出问题根源是"不应该对纯单例使用 `.src<>()`"，开发者很难理解。

#### 问题 3：没有编译时检查

编译器无法在编译时检测到这个错误用法，只能在运行时触发断言。

### 改进建议

#### 方案 1：提供专门的单例查询 API ⭐ 推荐

为纯单例查询提供独立的、语义更明确的 API：

```cpp
// 新 API 建议
world.singleton_system<Singleton>("MySingleton")
    .each([](flecs::entity e, Singleton& s) { ... });

// 或者更简洁的
world.singleton<Singleton>([](Singleton& s) {
    // 回调签名不包含 entity，更符合单例语义
});

// 对比现有 API（容易误用）
world.system<Singleton>()  // 看起来像普通系统
    .each(...);            // 但有特殊规则
```

**优点**：

- 语义明确，一看就知道是单例系统
- 避免与普通系统混淆
- 可以提供不同的默认行为

#### 方案 2：允许显式声明但自动处理

允许用户显式使用 `.src<>()`，但 Flecs 内部智能处理：

```cpp
// 当前行为：❌ 运行时错误
world.system<Singleton>()
    .term_at(0).src<Singleton>()  // 导致断言失败
    .each(callback);

// 改进后：✅ 自动识别并正确处理
world.system<Singleton>()
    .term_at(0).src<Singleton>()  // Flecs 检测到所有 terms 都是单例
    .each(callback);              // 自动提供 entity 参数（单例实体）
```

**实现思路**：

```cpp
// 在 delegate.hpp 的 invoke_callback 中
if (iter->entities == nullptr) {
    // 检查是否所有 terms 都是单例
    if (all_terms_are_singletons(iter)) {
        // 创建临时的 entities 数组，指向单例实体
        ecs_entity_t singleton_entity = get_singleton_entity(iter, 0);
        iter->entities = &singleton_entity;
        iter->count = 1;
    } else {
        // 原有的断言逻辑
        ecs_assert(false, ...);
    }
}
```

**优点**：

- API 更一致
- 向后兼容
- 用户可以选择显式或隐式

#### 方案 3：更好的编译时检查

使用 C++ 模板元编程进行编译时检查：

```cpp
template <typename... Components>
struct system_builder {
    // 编译时检查：如果只有一个组件，禁用 .src<>()
    template <typename T>
    auto src() -> std::enable_if_t<(sizeof...(Components) > 1), system_builder&> {
        // 只有多组件查询才能使用 .src<>()
        return *this;
    }

    // 单组件查询使用 .src<>() 会产生编译错误
};
```

**优点**：

- 在编译时就能发现问题
- 更友好的错误提示

**缺点**：

- 可能限制某些合法用例
- 增加模板复杂度

#### 方案 4：改进错误提示 ⭐ 最容易实现

在断言中提供更详细的错误信息：

```cpp
// 当前错误信息
ecs_assert(iter->entities != nullptr, ECS_INVALID_PARAMETER,
    "query does not return entities ($this variable is not populated)");

// 改进后的错误信息
ecs_assert(iter->entities != nullptr, ECS_INVALID_PARAMETER,
    "query does not return entities ($this variable is not populated). "
    "Possible causes:\n"
    "  1. All terms use .src<>(), making this a pure singleton query\n"
    "  2. For pure singleton queries, do NOT use .term_at().src<>()\n"
    "  3. Let Flecs auto-detect singletons instead");
```

**优点**：

- 改动最小
- 立即见效
- 帮助开发者快速定位问题

#### 方案 5：提供查询验证工具

添加辅助函数，让开发者可以验证查询配置：

```cpp
// 新增 API
world.system<Singleton>()
    .term_at(0).src<Singleton>()
    .validate()  // ✅ 抛出友好的错误或警告
    .each(callback);

// 或者在调试模式下自动验证
#ifdef FLECS_DEBUG
    // 自动检查并输出警告
    if (is_pure_singleton_query() && has_explicit_src()) {
        ecs_warn("Pure singleton query should not use .src<>(). "
                 "Flecs will auto-detect singletons.");
    }
#endif
```

### 推荐的短期和长期方案

#### 短期方案（立即可用）

1. **在代码中添加注释和文档**

   ```cpp
   // ✅ 正确：纯单例查询，让 Flecs 自动处理
   world.system<EventQueues>("ProcessEvent")
       .each(processImpl);

   // ✅ 正确：混合查询，显式指定单例
   world.system<WindowComp, WebGPURes>("InitGPU")
       .term_at(1).src<WebGPURes>()
       .each(initImpl);
   ```

2. **创建辅助工具函数**

   ```cpp
   // 封装常见模式
   template<typename Singleton, typename Func>
   void register_singleton_system(flecs::world& world,
                                  const char* name,
                                  Func&& func) {
       world.system<Singleton>(name)
           .each(std::forward<Func>(func));
   }

   // 使用
   register_singleton_system<EventQueues>(world, "ProcessEvent",
       [](flecs::entity e, EventQueues& q) { ... });
   ```

3. **添加静态分析规则**（如果使用 clang-tidy）
   ```yaml
   # .clang-tidy
   Checks: "-*,readability-*"
   CheckOptions:
     - key: readability-identifier-naming.PatternCheck
       value: "warn on: system<SingleType>().term_at(0).src"
   ```

#### 长期方案（建议向 Flecs 提交）

1. **提交 Issue/PR 到 Flecs 仓库**

   - 描述这个 API 不一致性问题
   - 提出改进建议（如方案 1、2、4）
   - 提供使用案例说明混淆点

2. **改进文档**

   - 在官方文档中明确说明这个区别
   - 添加"Common Pitfalls"章节
   - 提供清晰的对比示例

3. **社区讨论**
   - 在 Flecs Discord/Forums 发起讨论
   - 收集其他开发者的意见
   - 形成共识后推动改进

### 当前项目的最佳实践

在 Flecs 改进之前，我们可以在项目中采用以下实践：

```cpp
// 1. 创建明确的辅助宏
#define VIVID_SINGLETON_SYSTEM(World, Type, Name, Callback) \
    World.system<Type>(Name).each(Callback)

#define VIVID_MIXED_SYSTEM(World, EntityComp, SingletonComp, Name, Callback) \
    World.system<EntityComp, SingletonComp>(Name) \
        .term_at(1).src<SingletonComp>() \
        .each(Callback)

// 2. 使用示例
VIVID_SINGLETON_SYSTEM(world, EventQueues, "ProcessEvent",
    processImGuiEventImpl);

VIVID_MIXED_SYSTEM(world, WindowGpuComponent, WebGPUResources, "InitImGui",
    initImGuiImpl);

// 3. 添加代码审查规则
// - 所有 system<SingleType>() 都应该被审查
// - 确认是否需要 .src<>()
// - 添加注释说明为什么这样写
```

## 参考资源

- [Flecs 官方文档 - Systems](https://www.flecs.dev/flecs/md_docs_2Systems.html)
- [Flecs 官方文档 - Queries](https://www.flecs.dev/flecs/md_docs_2Queries.html)
- [Flecs GitHub - Query API](https://github.com/SanderMertens/flecs)
- [Flecs GitHub Issues](https://github.com/SanderMertens/flecs/issues) - 可以在这里提交改进建议

---

**创建日期**：2025-10-14  
**标签**：Flecs, ECS, Singleton, Query, System, API Design  
**难度**：中级  
**相关 Issue**：建议向 Flecs 提交改进建议
