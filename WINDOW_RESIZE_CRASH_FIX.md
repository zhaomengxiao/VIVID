# Window Resize Crash Fix

## Problem Description

The application crashed when resizing the window. This document records the root cause and the solution.

## Root Cause Analysis

### Crash Location

The crash occurred during window resize operations, specifically when the rendering pipeline tried to use invalidated GPU resources.

### Execution Flow (Before Fix)

When window size changes, the following sequence occurs:

```
Frame N (normal):
1. showImGuiDemoImpl()    → ImGui::NewFrame()
2. renderMeshImpl()        → Create render pass, draw meshes
3. renderImGuiImpl()       → Render UI to the pass
4. submitImpl()            → End pass, submit commands, present
✅ Success

Frame N+1 (window resized):
1. showImGuiDemoImpl()    → ImGui::NewFrame()
2. renderMeshImpl()        → Detect size change
                           → Call reconfigureSurface()
                           → ImGui::EndFrame()
                           → return early ⚠️
3. renderImGuiImpl()       → STILL EXECUTES (Flecs runs all systems)
                           → Tries to render to webgpuRes.renderPass
                           → ❌ renderPass points to previous frame's pass (already released)
4. submitImpl()            → STILL EXECUTES
                           → Tries to end nullptr renderPass
                           → ❌ CRASH: Access violation
```

### Key Issue

When `renderMeshImpl` detects a window size change:

1. It calls `reconfigureSurface()` which **releases and recreates depth resources**
2. It calls `ImGui::EndFrame()` and **returns early**
3. However, `webgpuRes.renderPass` still holds a **stale pointer** from previous frame
4. Subsequent systems (`renderImGuiImpl` and `submitImpl`) still execute and try to use this invalid pointer

## Solution

### Core Fix: Explicit Frame Skipping Protocol

When a frame needs to be skipped (window resize, minimization, etc.), we:

1. Set `webgpuRes.renderPass = nullptr` to signal "no rendering this frame"
2. Check this flag in all dependent systems before accessing render resources

### Code Changes

#### 1. renderMeshImpl() - Mark frame as skipped (Render System)

**Responsibility**: Detect errors and signal frame skip, NOT manage ImGui lifecycle.

```cpp
// When window is minimized
if (pixel_width <= 0 || pixel_height <= 0) {
  webgpuRes.renderPass = nullptr;  // ⭐ NEW: Mark render pass as invalid to signal frame skip
  return;  // UI system will handle ImGui::EndFrame()
}

// When window size changed
if (webgpuRes.configuredWidth != static_cast<uint32_t>(pixel_width)
    || webgpuRes.configuredHeight != static_cast<uint32_t>(pixel_height)) {
  reconfigureSurface(world, static_cast<uint32_t>(pixel_width),
                     static_cast<uint32_t>(pixel_height));
  webgpuRes.renderPass = nullptr;  // ⭐ NEW: Signal frame skip
  return;  // UI system will handle ImGui::EndFrame()
}

// When surface texture acquisition fails
if (surfaceTexture.status != Success) {
  webgpuRes.renderPass = nullptr;  // ⭐ NEW: Signal frame skip
  return;  // UI system will handle ImGui::EndFrame()
}
```

#### 2. renderImGuiImpl() - Unified ImGui lifecycle management (UI System)

**Responsibility**: Always close ImGui frame, either by rendering or explicit EndFrame.

```cpp
void UISystems::renderImGuiImpl(flecs::entity e, RENDER::WebGPUResources& webgpuRes) {
  // IMPORTANT: This function is responsible for closing the ImGui frame,
  // either by calling ImGui::Render() (which calls EndFrame internally)
  // or by explicitly calling ImGui::EndFrame() if rendering is skipped.

  // ⭐ NEW: Check if frame was skipped by render system
  if (webgpuRes.renderPass == nullptr) {
    ImGui::EndFrame();  // ⭐ Only place that explicitly calls EndFrame
    return;
  }

  // Normal rendering
  ImGui::Render();  // ⭐ Internally calls EndFrame
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), webgpuRes.renderPass);
}
```

#### 3. submitImpl() - Check before submitting

```cpp
void RenderSystems::submitImpl(flecs::iter& it) {
  auto world = it.world();
  auto& webgpuRes = world.get<WebGPUResources>();

  // ⭐ NEW: Skip if no render pass was created this frame
  if (webgpuRes.renderPass == nullptr) {
    return;  // Nothing to submit
  }

  // Normal submission
  wgpuRenderPassEncoderEnd(webgpuRes.renderPass);
  wgpuRenderPassEncoderRelease(webgpuRes.renderPass);
  // ... rest of submission logic
}
```

## Execution Flow (After Fix)

```
Frame N+1 (window resized):
1. showImGuiDemoImpl()    → ImGui::NewFrame()  [UI System opens frame]
2. renderMeshImpl()        → Detect size change
                           → Call reconfigureSurface()
                           → webgpuRes.renderPass = nullptr ⭐
                           → return early (NO ImGui::EndFrame)
3. renderImGuiImpl()       → Check renderPass == nullptr ⭐
                           → ImGui::EndFrame() ⭐  [UI System closes frame]
                           → return early
4. submitImpl()            → Check renderPass == nullptr ⭐
                           → return early (nothing to submit)
✅ Frame safely skipped

Frame N+2 (normal rendering resumes):
1. showImGuiDemoImpl()    → ImGui::NewFrame()  [UI System opens frame]
2. renderMeshImpl()        → Size matches, create new render pass
3. renderImGuiImpl()       → ImGui::Render() ⭐  [UI System closes frame via Render]
                           → Render to valid pass
4. submitImpl()            → Submit successfully
✅ Success with new surface size
```

## Key Principles

### 1. **Explicit State Signaling**

Use `nullptr` as a clear signal that "this frame has no valid render pass".

### 2. **Separation of Concerns**

- **Render System**: Detects errors, sets flags (`renderPass = nullptr`), does NOT touch ImGui lifecycle
- **UI System**: Owns ImGui lifecycle completely (NewFrame → Render/EndFrame)

### 3. **Defensive Checks**

Every system that depends on render resources must check validity before use.

### 4. **ECS System Order Dependency**

In Flecs (and most ECS), systems run in order regardless of early returns. Must handle this explicitly.

### 5. **ImGui Frame Lifecycle Management**

- `ImGui::NewFrame()` opened by UI system (showImGuiDemoImpl)
- `ImGui::EndFrame()` closed by UI system (renderImGuiImpl), either:
  - Via `ImGui::Render()` (normal path, calls EndFrame internally)
  - Via explicit `ImGui::EndFrame()` (skip path when renderPass == nullptr)
- **Critical**: Render system NEVER calls ImGui::EndFrame()

## Prevention Guidelines

When adding new rendering systems:

1. ✅ **Always check** `webgpuRes.renderPass` validity before use
2. ✅ **Always set** `renderPass = nullptr` when skipping a frame
3. ✅ **Never call** `ImGui::EndFrame()` in render systems - let UI system handle it
4. ✅ **Never assume** previous frame's resources are still valid
5. ✅ **Single Responsibility**: Each module manages its own concerns
   - Render module: GPU resources, rendering logic
   - UI module: ImGui lifecycle, UI rendering

## Related Files

- `lib/source/render/render_systems.cpp` - Main rendering systems
- `lib/source/ui/ui_system.cpp` - ImGui integration
- `lib/include/vivid/render/render_component.h` - WebGPUResources definition

## Refactoring History

### 2025-10-23 - Initial Fix

- Added `webgpuRes.renderPass = nullptr` to signal frame skip
- Added checks in UI and submit systems
- Initial fix included `ImGui::EndFrame()` in render system

### 2025-10-23 - Architecture Refactoring

- **Moved** `ImGui::EndFrame()` management to UI system only
- **Removed** all `ImGui::EndFrame()` calls from render system
- **Unified** ImGui lifecycle management under single responsibility principle
- Result: Cleaner separation of concerns, UI module owns ImGui completely

## Status

✅ Fixed and Verified
✅ Refactored for better architecture
