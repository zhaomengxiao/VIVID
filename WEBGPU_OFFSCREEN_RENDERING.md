# WebGPU 离屏渲染详解：从 Surface 到 TextureView

## 引言

在现代图形渲染中，离屏渲染（Offscreen Rendering）是一个非常重要的技术。它允许我们将场景渲染到纹理而不是直接渲染到窗口，这在编辑器视口、后处理效果、渲染到纹理等技术中都有广泛应用。

本文将深入探讨 WebGPU 中离屏渲染的实现原理，重点解释 **Surface**、**Texture** 和 **TextureView** 这三个核心概念及其关系。

## WebGPU 核心概念

### 1. Surface（表面）

**Surface** 是 WebGPU 连接窗口系统的抽象层，负责将渲染结果呈现到屏幕上。

#### 特点：

- **平台抽象**：封装不同平台的窗口系统（Windows HWND、Linux X11、macOS Metal Layer 等）
- **交换链管理**：内部管理交换链（swap chain），自动处理多缓冲
- **生命周期管理**：由 Surface 管理用于呈现的纹理，应用程序不能直接创建这些纹理

#### 创建 Surface：

```cpp
// 从 SDL 窗口句柄创建 Surface
webgpuRes.surface = ImGui_ImplSDL3_CreateWGPUSurface(
    webgpuRes.instance,
    windowContext.window_handle
);
```

#### 配置 Surface：

```cpp
WGPUSurfaceConfiguration config = {};
config.width = width;
config.height = height;
config.format = webgpuRes.surfaceFormat;
config.usage = WGPUTextureUsage_RenderAttachment;
config.device = webgpuRes.device;
config.presentMode = WGPUPresentMode_Fifo;
wgpuSurfaceConfigure(webgpuRes.surface, &config);
```

### 2. Texture（纹理）

**Texture** 是 GPU 内存中的图像数据，是存储像素信息的实际资源。

#### 特点：

- **存储实体**：实际的 GPU 内存资源
- **不可直接使用**：不能直接在渲染管线中使用，必须通过 TextureView 访问
- **用途多样**：可用于渲染目标（RenderAttachment）、纹理采样（TextureBinding）、复制操作等

#### 创建方式：

##### 方式 1：从 Surface 获取（窗口渲染）

```cpp
// 每帧从 Surface 获取当前可用的 Texture
WGPUSurfaceTexture surfaceTexture = {};
wgpuSurfaceGetCurrentTexture(webgpuRes.surface, &surfaceTexture);

// surfaceTexture.texture 是由 Surface 管理的 Texture
WGPUTexture texture = surfaceTexture.texture;
```

**注意**：Surface 管理的 Texture 只能用于 `RenderAttachment`，不能用于其他用途。

##### 方式 2：直接创建（离屏渲染）

```cpp
// 直接创建 Texture，完全控制其属性
WGPUTextureDescriptor renderTexDesc = {};
renderTexDesc.label = "Viewport render texture";
renderTexDesc.size = {width, height, 1};
renderTexDesc.dimension = WGPUTextureDimension_2D;
renderTexDesc.format = webgpuRes.surfaceFormat;
renderTexDesc.mipLevelCount = 1;
renderTexDesc.sampleCount = 1;
// 可以同时用于渲染和采样
renderTexDesc.usage = WGPUTextureUsage_RenderAttachment
                    | WGPUTextureUsage_TextureBinding;

WGPUTexture texture = wgpuDeviceCreateTexture(
    webgpuRes.device,
    &renderTexDesc
);
```

**优势**：可以自由指定 Texture 的用途，例如同时用于渲染和作为纹理采样。

### 3. TextureView（纹理视图）

**TextureView** 是 Texture 的访问视图，定义了如何访问和解释 Texture 的数据。

#### 为什么需要 TextureView？

- **访问控制**：指定访问 Texture 的哪一部分（mip 级别、数组层、aspect 等）
- **格式转换**：可以以不同格式读取（如果 Texture 支持）
- **维度视图**：可以从 3D Texture 创建 2D 视图，或从数组纹理创建单个层视图

#### TextureViewDescriptor 的关键字段：

```cpp
WGPUTextureViewDescriptor viewDesc = {};
viewDesc.format = textureFormat;              // 视图格式
viewDesc.dimension = WGPUTextureViewDimension_2D;  // 视图维度
viewDesc.baseMipLevel = 0;                    // 起始 mip 级别
viewDesc.mipLevelCount = 1;                    // mip 级别数量
viewDesc.baseArrayLayer = 0;                   // 起始数组层
viewDesc.arrayLayerCount = 1;                  // 数组层数量
viewDesc.aspect = WGPUTextureAspect_All;       // 访问的 aspect（颜色/深度/模板）
viewDesc.usage = WGPUTextureUsage_RenderAttachment;  // 视图用途

WGPUTextureView view = wgpuTextureCreateView(texture, &viewDesc);
```

## 离屏渲染 vs 窗口渲染

### 窗口渲染（Surface-based Rendering）

**流程**：

```
窗口句柄 → Surface → 每帧获取 Texture → 创建 TextureView → RenderPass → 呈现到窗口
```

**特点**：

1. **Texture 由 Surface 管理**：每帧调用 `wgpuSurfaceGetCurrentTexture()` 获取
2. **用途受限**：只能用于 `RenderAttachment`，不能用于采样
3. **自动呈现**：渲染完成后自动呈现到窗口
4. **格式限制**：受 Surface capabilities 限制

**代码示例**：

```cpp
// 1. 从 Surface 获取当前帧的 Texture
WGPUSurfaceTexture surfaceTexture = {};
wgpuSurfaceGetCurrentTexture(webgpuRes.surface, &surfaceTexture);

// 2. 创建 TextureView
WGPUTextureViewDescriptor viewDesc = {};
viewDesc.format = wgpuTextureGetFormat(surfaceTexture.texture);
viewDesc.dimension = WGPUTextureViewDimension_2D;
viewDesc.aspect = WGPUTextureAspect_All;
viewDesc.usage = WGPUTextureUsage_RenderAttachment;
WGPUTextureView targetView = wgpuTextureCreateView(
    surfaceTexture.texture,
    &viewDesc
);

// 3. 用于 RenderPass
WGPURenderPassColorAttachment colorAttachment = {};
colorAttachment.view = targetView;
colorAttachment.loadOp = WGPULoadOp_Clear;
colorAttachment.storeOp = WGPUStoreOp_Store;
// ... 配置 render pass
```

### 离屏渲染（Offscreen Rendering）

**流程**：

```
直接创建 Texture → 创建 TextureView → RenderPass → 使用 Texture（如显示在 ImGui）
```

**特点**：

1. **完全控制**：直接创建和管理 Texture 的生命周期
2. **用途灵活**：可以同时用于渲染和采样
3. **手动处理**：渲染完成后需要手动处理结果（如作为 ImGui 纹理显示）
4. **格式自由**：可以自由选择格式、大小等属性

**代码示例**：

```cpp
// 1. 创建离屏 Texture
WGPUTextureDescriptor renderTexDesc = {};
renderTexDesc.size = {width, height, 1};
renderTexDesc.dimension = WGPUTextureDimension_2D;
renderTexDesc.format = webgpuRes.surfaceFormat;
renderTexDesc.usage = WGPUTextureUsage_RenderAttachment
                    | WGPUTextureUsage_TextureBinding;  // 可以用于采样
WGPUTexture renderTexture = wgpuDeviceCreateTexture(
    webgpuRes.device,
    &renderTexDesc
);

// 2. 创建 TextureView
WGPUTextureViewDescriptor renderViewDesc = {};
renderViewDesc.format = webgpuRes.surfaceFormat;
renderViewDesc.dimension = WGPUTextureViewDimension_2D;
renderViewDesc.aspect = WGPUTextureAspect_All;
WGPUTextureView renderTextureView = wgpuTextureCreateView(
    renderTexture,
    &renderViewDesc
);

// 3. 用于 RenderPass
WGPURenderPassColorAttachment colorAttachment = {};
colorAttachment.view = renderTextureView;
colorAttachment.loadOp = WGPULoadOp_Clear;
colorAttachment.storeOp = WGPUStoreOp_Store;
// ... 配置 render pass

// 4. 渲染完成后，可以作为 ImGui 纹理显示
ImGui::Image(
    reinterpret_cast<ImTextureID>(renderTextureView),
    ImVec2(width, height)
);
```

## 核心关系图

```
┌─────────────────────────────────────────────────────────┐
│                    Window (SDL)                         │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  Surface (平台抽象层，管理交换链)                          │
│  - 每帧调用 wgpuSurfaceGetCurrentTexture()               │
└────────────────────┬────────────────────────────────────┘
                     │ 返回
                     ▼
┌─────────────────────────────────────────────────────────┐
│  Texture (GPU 内存中的图像数据)                          │
│  - 由 Surface 管理 或 直接创建                           │
└────────────────────┬────────────────────────────────────┘
                     │ 创建 View
                     ▼
┌─────────────────────────────────────────────────────────┐
│  TextureView (Texture 的访问视图)                        │
│  - 指定如何访问 Texture 的哪一部分                       │
│  - 用于 RenderPass 的 attachment                        │
└────────────────────┬────────────────────────────────────┘
                     │ 作为 RenderPass 的 attachment
                     ▼
┌─────────────────────────────────────────────────────────┐
│  RenderPass (渲染通道)                                   │
│  - colorAttachment.view = TextureView                   │
└─────────────────────────────────────────────────────────┘
```

## 关键区别总结

| 特性             | Surface 渲染（窗口）                              | 离屏渲染                                          |
| ---------------- | ------------------------------------------------- | ------------------------------------------------- |
| **Texture 来源** | 从 Surface 获取（`wgpuSurfaceGetCurrentTexture`） | 直接创建（`wgpuDeviceCreateTexture`）             |
| **Texture 管理** | 由 Surface 管理，每帧获取                         | 自己管理生命周期                                  |
| **用途限制**     | 主要用于 `RenderAttachment`                       | 可同时用于 `RenderAttachment` 和 `TextureBinding` |
| **呈现方式**     | 自动呈现到窗口                                    | 需要手动处理（如作为 ImGui 纹理）                 |
| **灵活性**       | 受 Surface capabilities 限制                      | 完全灵活                                          |
| **适用场景**     | 主窗口渲染                                        | 编辑器视口、后处理、渲染到纹理                    |

## 实际应用场景

### 1. 编辑器视口渲染

在编辑器中将 3D 场景渲染到视口窗口中：

```cpp
// 创建视口离屏纹理
WGPUTexture viewportTexture = createOffscreenTexture(width, height);
WGPUTextureView viewportView = createTextureView(viewportTexture);

// 渲染场景到视口纹理
renderSceneToViewport(viewportView);

// 在 ImGui 窗口中显示
ImGui::Image(reinterpret_cast<ImTextureID>(viewportView), ImVec2(width, height));
```

### 2. 后处理效果

将场景先渲染到离屏纹理，然后进行后处理：

```cpp
// 第一遍：渲染场景到离屏纹理
renderSceneToTexture(sceneTexture);

// 第二遍：对离屏纹理进行后处理
applyPostProcessing(sceneTexture, finalTexture);
```

### 3. 渲染到纹理（Render to Texture）

将渲染结果保存为纹理，用于后续处理或显示：

```cpp
// 渲染到纹理
WGPUTexture renderTarget = createRenderTargetTexture();
renderToTexture(renderTarget);

// 可以用于：
// - 作为其他渲染的输入
// - 保存到文件
// - 显示在 UI 中
```

## 最佳实践

### 1. 资源管理

- **及时释放**：使用完 Texture 和 TextureView 后及时释放
- **延迟释放**：对于可能需要跨帧使用的资源，使用延迟释放机制

```cpp
// 延迟释放示例
void queueDelayedRelease(WGPUTexture texture, WGPUTextureView view) {
    // 延迟 3 帧后释放，确保 GPU 不再使用
    delayedReleaseQueue.push({texture, view, currentFrame + 3});
}
```

### 2. 错误处理

- **检查 Surface 状态**：获取 Surface Texture 时检查状态

```cpp
WGPUSurfaceTexture surfaceTexture = {};
wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
    && surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
    // 处理错误或重新配置 Surface
    reconfigureSurface(surface, width, height);
}
```

### 3. 性能优化

- **重用资源**：在尺寸不变时重用 Texture 和 TextureView
- **批量创建**：一次性创建所需的所有资源
- **合理设置用途**：根据实际需求设置 Texture 的 usage flags

## 总结

WebGPU 中的离屏渲染提供了强大的灵活性，允许我们将渲染结果作为纹理使用，而不仅仅局限于显示在窗口中。

**核心要点**：

1. **Surface** 是连接窗口系统的抽象层，管理用于窗口呈现的纹理
2. **Texture** 是 GPU 内存中的图像数据，可以来自 Surface 或直接创建
3. **TextureView** 是 Texture 的访问视图，是连接 Texture 和渲染管线的桥梁
4. **离屏渲染**通过直接创建 Texture 实现，提供了更大的控制权和灵活性
5. 无论 Texture 来自哪里，都必须创建 TextureView 才能用于渲染

理解这三个概念的关系，对于掌握 WebGPU 渲染流程至关重要。离屏渲染为编辑器、后处理、渲染到纹理等高级渲染技术提供了基础。

---

**参考资源**：

- [WebGPU Specification](https://www.w3.org/TR/webgpu/)
- [Dawn WebGPU Implementation](https://dawn.googlesource.com/dawn)
- [WebGPU Samples](https://webgpu.github.io/webgpu-samples/)
