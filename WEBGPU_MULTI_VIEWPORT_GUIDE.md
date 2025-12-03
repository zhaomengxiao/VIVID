# WebGPU 多视口渲染实践指南：避免 Uniform Buffer 覆盖陷阱

## 📋 目录

- [问题背景](#问题背景)
- [WebGPU 多视口渲染的挑战](#webgpu-多视口渲染的挑战)
- [Uniform Buffer 覆盖问题详解](#uniform-buffer-覆盖问题详解)
- [解决方案：原子化渲染流程](#解决方案原子化渲染流程)
- [最佳实践](#最佳实践)
- [代码示例](#代码示例)
- [性能考虑](#性能考虑)
- [总结](#总结)

---

## 问题背景

在实现多视口（Multi-Viewport）渲染系统时，我们遇到了一个看似诡异的问题：

- **症状 1**：两个视口显示相同的视角，即使它们的相机位置和朝向完全不同
- **症状 2**：调整一个视口的大小时，另一个视口中的物体会发生变形
- **症状 3**：日志显示每个视口的相机参数都是正确的，但渲染结果却不对

经过深入调查，我们发现问题的根源在于 **WebGPU 的 Uniform Buffer 更新机制**。

---

## WebGPU 多视口渲染的挑战

### 传统渲染流程的假设

在传统的单视口渲染流程中，我们通常遵循以下模式：

```
1. 创建 Command Encoder
2. 开始 Render Pass
3. 更新 Uniform Buffer（相机参数、投影矩阵等）
4. 记录 Draw 命令
5. 结束 Render Pass
6. 提交 Command Buffer
```

这个流程在单视口场景下工作良好，因为：

- Uniform Buffer 的更新和 Draw 命令是顺序执行的
- GPU 执行时，Uniform Buffer 已经包含了正确的数据

### 多视口渲染的陷阱

当我们尝试支持多个视口时，很自然地会想到这样的流程：

```cpp
// ❌ 错误的做法：批量处理所有视口
for (auto& viewport : viewports) {
    // 1. 为每个视口创建独立的 encoder 和 render pass
    auto encoder = createEncoder();
    auto renderPass = beginRenderPass(encoder, viewport);

    // 2. 更新 uniform buffer（使用当前视口的相机参数）
    updateUniformBuffer(viewport.camera);

    // 3. 记录 draw 命令
    drawScene(renderPass);

    // 4. 保存 render pass，稍后一起提交
    saveRenderPass(renderPass);
}

// 5. 最后统一提交所有 command buffer
for (auto& renderPass : savedRenderPasses) {
    endRenderPass(renderPass);
    submit(renderPass.encoder);
}
```

**这个流程看起来合理，但实际上存在严重问题！**

---

## Uniform Buffer 覆盖问题详解

### 核心问题：`wgpuQueueWriteBuffer` 是立即执行的

WebGPU 的 `wgpuQueueWriteBuffer()` 函数有一个关键特性：

> **`wgpuQueueWriteBuffer` 是立即执行的，不受 Command Encoder 控制！**

这意味着：

```cpp
// 视口1：写入 MainCamera 的参数
wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &mainCameraUniforms, sizeof(uniforms));
// ⚠️ 这个写入是立即生效的！

// 视口2：写入 SideCamera 的参数
wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &sideCameraUniforms, sizeof(uniforms));
// ⚠️ 这个写入覆盖了上面的数据，也是立即生效的！

// 当 GPU 执行时：
// - 视口1 的 draw 命令使用 uniformBuffer（此时包含 SideCamera 的数据！）
// - 视口2 的 draw 命令使用 uniformBuffer（包含 SideCamera 的数据）
// 结果：两个视口都显示 SideCamera 的视角！
```

### 问题时序图

```
时间线：
┌─────────────────────────────────────────────────────────┐
│ CPU 端执行（立即生效）                                      │
├─────────────────────────────────────────────────────────┤
│ 1. 创建 Viewport1 的 encoder 和 renderPass              │
│ 2. wgpuQueueWriteBuffer(viewport1.uniforms) ← 立即写入  │
│ 3. 记录 Viewport1 的 draw 命令                           │
│                                                          │
│ 4. 创建 Viewport2 的 encoder 和 renderPass              │
│ 5. wgpuQueueWriteBuffer(viewport2.uniforms) ← 覆盖！     │
│ 6. 记录 Viewport2 的 draw 命令                           │
│                                                          │
│ 7. 提交所有 command buffer                               │
└─────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────┐
│ GPU 端执行（异步）                                         │
├─────────────────────────────────────────────────────────┤
│ Viewport1 的 draw 命令执行                                │
│   → 读取 uniformBuffer（此时是 Viewport2 的数据！）      │
│   → 错误：显示 Viewport2 的视角                           │
│                                                          │
│ Viewport2 的 draw 命令执行                                │
│   → 读取 uniformBuffer（Viewport2 的数据）              │
│   → 正确：显示 Viewport2 的视角                           │
└─────────────────────────────────────────────────────────┘
```

### 为什么投影矩阵也会受影响？

投影矩阵的计算依赖于视口的宽高比：

```cpp
float aspect = static_cast<float>(width) / static_cast<float>(height);
projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
```

如果两个视口共享同一个 uniform buffer，后计算的投影矩阵会覆盖前面的，导致：

- 调整 Viewport1 的大小时，Viewport2 中的物体会变形（使用了错误的宽高比）
- 两个视口可能显示相同的透视效果

---

## 解决方案：原子化渲染流程

### 核心思想

**每个视口必须完整地渲染并立即提交，然后再处理下一个视口。**

这样可以确保：

1. Uniform Buffer 的更新和对应的 Draw 命令在同一个提交周期内
2. GPU 执行时，Uniform Buffer 包含的是正确的数据
3. 不同视口之间不会相互干扰

### 正确的流程

```cpp
// ✅ 正确的做法：原子化处理每个视口
for (auto& viewport : viewports) {
    // 1. 创建 encoder 和 render pass
    auto encoder = createEncoder();
    auto renderPass = beginRenderPass(encoder, viewport);

    // 2. 更新 uniform buffer（使用当前视口的相机参数）
    updateUniformBuffer(viewport.camera);

    // 3. 记录 draw 命令
    drawScene(renderPass);

    // 4. 立即结束 render pass
    endRenderPass(renderPass);

    // 5. 立即提交 command buffer
    submit(encoder);
    // ⚠️ 关键：必须等待这个视口完全提交后，再处理下一个！
}
```

### 实现代码

```cpp
void RenderSystems::beginViewportRenderPassImpl(const flecs::iter& it) {
    auto world = it.world();
    const auto& webgpuRes = world.get<WebGPUContext>();

    auto viewportQuery = world.query<CameraComponent, ViewportComponent, TransformComponent>();

    viewportQuery.each([&](flecs::entity entity,
                           const CameraComponent& camera,
                           ViewportComponent& viewport,
                           const TransformComponent& transform) {
        // 1. 创建独立的 command encoder
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        // 2. 创建 render pass
        WGPURenderPassEncoder renderPass =
            wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        // 3. 计算当前视口的相机参数
        SceneRenderContext sceneCtx = calculateViewportCamera(entity, viewport);

        // 4. 渲染场景（内部会调用 wgpuQueueWriteBuffer）
        RenderTarget target = {renderPass, viewport.width, viewport.height};
        renderSceneToTarget(world, target, sceneCtx, webgpuRes);

        // 5. 立即结束 render pass
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        // 6. 立即完成并提交 command buffer
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuCommandEncoderRelease(encoder);
        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);

        // ✅ 现在可以安全地处理下一个视口了
    });
}
```

### 关键点

1. **原子性**：每个视口的渲染和提交是一个不可分割的操作
2. **顺序性**：必须等待一个视口完全提交后，再处理下一个
3. **独立性**：每个视口使用独立的 encoder 和 render pass

---

## 最佳实践

### 1. 每个视口使用独立的资源

```cpp
// ✅ 推荐：每个视口有独立的 encoder 和 render pass
for (auto& viewport : viewports) {
    auto encoder = createEncoder();  // 独立创建
    auto renderPass = beginRenderPass(encoder, viewport);  // 独立创建
    // ... 渲染并立即提交
}
```

### 2. 避免批量提交

```cpp
// ❌ 避免：批量创建所有 render pass，最后统一提交
std::vector<RenderPass> passes;
for (auto& viewport : viewports) {
    passes.push_back(createRenderPass(viewport));
}
// 统一提交会导致 uniform buffer 覆盖问题
```

### 3. Uniform Buffer 管理策略

如果你需要为每个实体使用独立的 uniform buffer（更高级的方案）：

```cpp
// 方案A：每个实体 + 每个视口 = 独立的 uniform buffer
// 优点：完全避免覆盖问题
// 缺点：内存开销大

// 方案B：原子化渲染（我们采用的方案）
// 优点：实现简单，内存开销小
// 缺点：需要顺序处理视口
```

### 4. 投影矩阵计算

```cpp
// ✅ 正确：每个视口根据自己的尺寸计算投影矩阵
float aspect = static_cast<float>(viewport.width) /
               static_cast<float>(viewport.height);
projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

// ❌ 错误：使用全局的宽高比
float aspect = static_cast<float>(mainWindow.width) /
               static_cast<float>(mainWindow.height);
```

### 5. 调试技巧

添加详细的日志来验证每个视口的参数：

```cpp
VividLogger::app_debug(
    "Viewport '%s' - Size: %ux%u, ViewPos: (%.2f, %.2f, %.2f), "
    "Front: (%.3f, %.3f, %.3f), Aspect: %.3f",
    tag.Tag.c_str(), width, height,
    sceneCtx.viewPos.x, sceneCtx.viewPos.y, sceneCtx.viewPos.z,
    controller.Front.x, controller.Front.y, controller.Front.z,
    static_cast<float>(width) / static_cast<float>(height));
```

---

## 代码示例

### 完整的视口渲染实现

```cpp
// BeginViewportRenderPassSystem: 原子化渲染每个视口
void RenderSystems::beginViewportRenderPassImpl(const flecs::iter& it) {
    auto world = it.world();
    const auto& webgpuRes = world.get<WebGPUContext>();

    // 验证 WebGPU 状态
    if (!webgpuRes.device || !webgpuRes.queue || !webgpuRes.initialized) {
        return;
    }

    // 查询所有视口
    auto viewportQuery = world.query<CameraComponent, ViewportComponent, TransformComponent>();

    viewportQuery.each([&](flecs::entity entity,
                           const CameraComponent& camera,
                           ViewportComponent& viewport,
                           const TransformComponent& transform) {
        uint32_t width = static_cast<uint32_t>(viewport.Width);
        uint32_t height = static_cast<uint32_t>(viewport.Height);

        if (width == 0 || height == 0 ||
            !viewport.renderTextureView || !viewport.depthView) {
            return;
        }

        // 1. 创建独立的 command encoder
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = toWgpuStringView("Viewport command encoder");
        WGPUCommandEncoder encoder =
            wgpuDeviceCreateCommandEncoder(webgpuRes.device, &encoderDesc);

        // 2. 创建 render pass
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = viewport.renderTextureView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = WGPUColor{0.1, 0.1, 0.1, 1.0};

        WGPURenderPassDepthStencilAttachment depthAttachment = {};
        depthAttachment.view = viewport.depthView;
        depthAttachment.depthClearValue = 1.0f;
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Store;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthAttachment;

        WGPURenderPassEncoder renderPass =
            wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        // 3. 计算当前视口的相机参数
        SceneRenderContext sceneCtx = querySceneContext(world, width, height);
        sceneCtx.viewPos = transform.Position;

        if (entity.has<CameraControllerComponent>()) {
            const auto& controller = entity.get<CameraControllerComponent>();
            glm::vec3 target = transform.Position + controller.Front;
            sceneCtx.viewMatrix = glm::lookAt(transform.Position, target, controller.Up);
        } else {
            sceneCtx.viewMatrix = glm::lookAt(
                transform.Position,
                transform.Position + glm::vec3(0, 0, -1),
                glm::vec3(0, 1, 0));
        }

        // 根据视口尺寸计算投影矩阵
        if (camera.ProjectionMatrix != glm::mat4(1.0f)) {
            sceneCtx.projectionMatrix = camera.ProjectionMatrix;
        } else if (height > 0) {
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            sceneCtx.projectionMatrix =
                glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        }

        // 4. 渲染场景（会更新 uniform buffer 并记录 draw 命令）
        RenderTarget target = {renderPass, width, height};
        renderSceneToTarget(world, target, sceneCtx, webgpuRes);

        // 5. 立即结束 render pass
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        // 6. 立即完成并提交 command buffer
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = toWgpuStringView("Viewport command buffer");
        WGPUCommandBuffer command =
            wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuCommandEncoderRelease(encoder);

        if (command != nullptr && webgpuRes.queue != nullptr) {
            wgpuQueueSubmit(webgpuRes.queue, 1, &command);
            wgpuCommandBufferRelease(command);
        }

        // ✅ 现在可以安全地处理下一个视口
    });
}
```

---

## 性能考虑

### 原子化渲染的性能影响

**优点**：

- ✅ 实现简单，逻辑清晰
- ✅ 避免 uniform buffer 覆盖问题
- ✅ 内存开销小（共享 uniform buffer）

**缺点**：

- ⚠️ 需要顺序处理视口（不能并行）
- ⚠️ 每个视口都需要独立的提交操作

### 优化建议

如果性能成为瓶颈，可以考虑以下优化：

1. **使用多个 Uniform Buffer**：

   ```cpp
   // 为每个视口预分配独立的 uniform buffer
   std::vector<WGPUBuffer> viewportUniformBuffers;
   ```

2. **批量提交优化**：

   ```cpp
   // 如果 GPU 支持，可以尝试批量提交
   // 但需要确保 uniform buffer 更新和 draw 命令的配对正确
   ```

3. **异步渲染**：
   ```cpp
   // 使用多个 command queue 并行渲染不同视口
   // 但需要更复杂的同步机制
   ```

---

## 总结

### 关键要点

1. **`wgpuQueueWriteBuffer` 是立即执行的**，不受 command encoder 控制
2. **多视口渲染必须采用原子化流程**：每个视口完整渲染并立即提交
3. **每个视口需要独立的 encoder 和 render pass**
4. **投影矩阵必须根据每个视口的实际尺寸计算**

### 检查清单

在实现多视口渲染时，确保：

- [ ] 每个视口使用独立的 command encoder
- [ ] 每个视口使用独立的 render pass
- [ ] Uniform buffer 更新后立即提交对应的 command buffer
- [ ] 投影矩阵根据每个视口的宽高比计算
- [ ] 添加调试日志验证每个视口的参数
- [ ] 测试调整视口大小时不会影响其他视口

### 相关资源

- [WebGPU Specification - Queue Operations](https://www.w3.org/TR/webgpu/#gpu-queue)
- [WebGPU Specification - Command Buffers](https://www.w3.org/TR/webgpu/#command-buffers)
- [VIVID Engine - Multi-Viewport Implementation](https://github.com/vivid-engine/vivid)

---

**作者**: VIVID Engine Team  
**日期**: 2024  
**版本**: 1.0

---

## 附录：常见问题

### Q: 为什么不能使用 `wgpuCommandEncoderCopyBufferToBuffer` 来延迟 uniform 更新？

**A**: `wgpuCommandEncoderCopyBufferToBuffer` 确实可以延迟操作，但它需要源 buffer 和目标 buffer。如果使用这种方法，你需要：

1. 为每个视口创建独立的 uniform buffer
2. 使用 staging buffer 作为源
3. 在 command encoder 中记录 copy 命令

这增加了复杂性，而原子化渲染方案更简单直接。

### Q: 这个方案会影响性能吗？

**A**: 对于少量视口（2-4 个），性能影响可以忽略。如果视口数量很多（10+），可以考虑使用独立的 uniform buffer 方案。

### Q: 是否可以使用 compute shader 来更新 uniform？

**A**: 理论上可以，但会增加不必要的复杂性。原子化渲染方案已经足够简单和高效。

---

_本文档基于 VIVID Engine 的实际开发经验编写，如有问题或建议，欢迎提交 Issue 或 PR。_
