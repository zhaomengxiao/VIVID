# VIVID Engine 文档索引

本文档提供 VIVID 引擎所有技术文档的快速索引和概览。

---

## 📚 核心开发指南

### Flecs ECS 系统

| 文档                                                              | 适用人群   | 内容概述                                                    |
| ----------------------------------------------------------------- | ---------- | ----------------------------------------------------------- |
| **[Flecs 系统编写指南](./FLECS_SYSTEM_WRITING_GUIDE.md)** ⭐      | 所有开发者 | 标准系统编写模式、`.each()` vs `.run()`、实际案例、常见陷阱 |
| **[Flecs 单例查询深度解析](./FLECS_SINGLETON_QUERY_GUIDE.md)** 🔬 | 高级开发者 | 单例查询底层原理、源码分析、API 设计问题                    |

**推荐阅读顺序**：

1. 新手先阅读"系统编写指南"掌握基础模式
2. 遇到单例相关问题时查阅"单例查询深度解析"

---

## 🔧 框架使用指南

### 应用程序框架

| 文档                                               | 内容                                  |
| -------------------------------------------------- | ------------------------------------- |
| **[SDL3App 使用示例](./SDL3App_usage_example.md)** | SDL3 应用程序基础、窗口创建、事件处理 |

---

## 📖 迁移和历史文档

| 文档                                                               | 用途                                  |
| ------------------------------------------------------------------ | ------------------------------------- |
| **[EnTT 到 Flecs 迁移总结](./ENTT_TO_FLECS_MIGRATION_SUMMARY.md)** | 了解从 EnTT 迁移到 Flecs 的决策和过程 |
| **[Flecs 迁移总结](./FLECS_MIGRATION_SUMMARY.md)**                 | 详细的迁移步骤和技术细节              |

---

## 🏗️ 架构文档

### 系统架构概览

VIVID 引擎基于 Flecs ECS 架构，主要模块包括：

```
┌─────────────────────────────────────────┐
│            Application Layer             │
│         (Standalone/Editor)              │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Core Engine Modules              │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐│
│  │  UI  │  │Window│  │Render│  │Input ││
│  │System│  │System│  │System│  │System││
│  └──────┘  └──────┘  └──────┘  └──────┘│
│  ┌──────┐  ┌──────┐                    │
│  │Physics  │  Log  │                    │
│  │System│  │System│                    │
│  └──────┘  └──────┘                    │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│          Flecs ECS Framework             │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│      Third-Party Libraries               │
│   SDL3 | WebGPU | ImGui | PhysX | GLM   │
└─────────────────────────────────────────┘
```

### 各模块职责

#### App 模块

- **职责**: 应用程序生命周期管理
- **核心类**: `SDL3App`, `EventQueues`
- **相关文档**: [SDL3App 使用示例](./SDL3App_usage_example.md)

#### Window 模块

- **职责**: 窗口创建和管理
- **核心组件**: `WindowComponent`, `WindowGpuComponent`
- **核心系统**: `WindowInitialization`, `WindowUpdate`, `WindowCleanup`
- **相关文档**: [Flecs 系统编写指南 - Window 模组](./FLECS_SYSTEM_WRITING_GUIDE.md#window-模组)

#### Render 模块

- **职责**: WebGPU 渲染管线
- **核心组件**: `WebGPUResources`, `MeshComponent`, `MaterialComponent`, `GpuMeshComponent`
- **核心系统**: `InitWebGPU`, `SyncScene`, `Draw`, `ReleaseWebGPUResources`
- **相关文档**: [Flecs 系统编写指南 - Render 模组](./FLECS_SYSTEM_WRITING_GUIDE.md#render-模组)

#### UI 模块

- **职责**: ImGui 用户界面集成
- **核心系统**: `InitImGui`, `ProcessImGuiEvent`, `ShowImGuiDemo`, `ShutDownImGui`
- **相关文档**: [Flecs 系统编写指南 - UI 模组](./FLECS_SYSTEM_WRITING_GUIDE.md#ui-模组)

#### Input 模块

- **职责**: 用户输入处理（键盘、鼠标、手柄）
- **核心系统**: 输入事件系统

#### Physics 模块

- **职责**: 物理模拟（基于 PhysX）
- **核心系统**: 物理更新系统

#### Log 模块

- **职责**: 日志记录和调试输出
- **核心类**: `VividLogger`

---

## 🎯 快速查找

### 按任务类型查找

#### 想要编写新的 ECS 系统？

→ [Flecs 系统编写指南](./FLECS_SYSTEM_WRITING_GUIDE.md)

#### 遇到单例查询问题？

→ [Flecs 单例查询深度解析](./FLECS_SINGLETON_QUERY_GUIDE.md)

#### 想要创建窗口和处理事件？

→ [SDL3App 使用示例](./SDL3App_usage_example.md)

#### 想要了解从 EnTT 迁移的背景？

→ [EnTT 到 Flecs 迁移总结](./ENTT_TO_FLECS_MIGRATION_SUMMARY.md)

### 按技术栈查找

#### Flecs ECS

- [系统编写指南](./FLECS_SYSTEM_WRITING_GUIDE.md) - 标准编写模式
- [单例查询解析](./FLECS_SINGLETON_QUERY_GUIDE.md) - 深度技术分析
- [迁移总结](./FLECS_MIGRATION_SUMMARY.md) - 迁移记录

#### SDL3

- [SDL3App 使用示例](./SDL3App_usage_example.md) - 窗口和事件处理

#### WebGPU

- [Flecs 系统编写指南 - Render 模组](./FLECS_SYSTEM_WRITING_GUIDE.md#render-模组) - 渲染系统实现

#### ImGui

- [Flecs 系统编写指南 - UI 模组](./FLECS_SYSTEM_WRITING_GUIDE.md#ui-模组) - UI 系统集成

---

## 📝 编码规范和最佳实践

### Flecs 系统编写规范

参考 [Flecs 系统编写指南](./FLECS_SYSTEM_WRITING_GUIDE.md) 中的编码检查清单：

- [ ] 所有依赖在系统注册时声明
- [ ] 避免在实现中手动创建 query
- [ ] 纯单例查询不使用 `.term_at().src<>()`
- [ ] 混合查询正确使用 `.term_at(N).src<>()`
- [ ] 索引计算正确（从 0 开始）
- [ ] 必要的头文件已包含
- [ ] 代码风格与项目一致

### 代码格式化

```powershell
# 检查代码格式
.\scripts\check_code.ps1

# 自动格式化代码
.\scripts\format_code.ps1
```

### 注释规范

- ✅ 使用英文编写代码注释
- ✅ 使用文档注释说明公共 API
- ✅ 复杂逻辑添加必要的行内注释

---

## 🔗 外部资源

### 官方文档

- [Flecs 官方文档](https://www.flecs.dev/flecs/) - Flecs ECS 框架
- [SDL3 Wiki](https://wiki.libsdl.org/SDL3/FrontPage) - SDL3 多媒体库
- [WebGPU 规范](https://www.w3.org/TR/webgpu/) - WebGPU 标准
- [ImGui 文档](https://github.com/ocornut/imgui/wiki) - ImGui 即时模式 GUI
- [GLM 文档](https://github.com/g-truc/glm/blob/master/manual.md) - OpenGL 数学库
- [PhysX 文档](https://nvidia-omniverse.github.io/PhysX/physx/5.4.0/docs/index.html) - NVIDIA PhysX

### 教程和示例

- [Flecs Examples](https://github.com/SanderMertens/flecs/tree/master/examples) - 官方示例代码
- [WebGPU Samples](https://webgpu.github.io/webgpu-samples/) - WebGPU 示例集合
- [SDL3 Examples](https://github.com/libsdl-org/SDL/tree/main/examples) - SDL3 官方示例

---

## 📊 文档维护

### 贡献指南

当您添加新功能或修改现有模块时，请：

1. **更新相关文档**：确保文档与代码同步
2. **添加示例代码**：提供清晰的使用示例
3. **更新本索引**：在此索引中添加新文档链接

### 文档更新日志

| 日期       | 文档                           | 变更             |
| ---------- | ------------------------------ | ---------------- |
| 2025-10-14 | FLECS_SYSTEM_WRITING_GUIDE.md  | 创建系统编写指南 |
| 2025-10-14 | FLECS_SINGLETON_QUERY_GUIDE.md | 创建单例查询解析 |
| 2025-10-14 | DOCUMENTATION_INDEX.md         | 创建文档索引     |

---

## 📞 获取帮助

如果您在使用 VIVID 引擎时遇到问题：

1. **查看相关文档**：使用上面的快速查找定位文档
2. **检查示例代码**：参考实际案例理解用法
3. **查阅官方文档**：查看底层库的官方文档
4. **提交 Issue**：在项目仓库报告问题或建议

---

**最后更新**: 2025-10-14  
**维护者**: VIVID Engine Team
