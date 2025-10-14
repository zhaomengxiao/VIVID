#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <string>

#include "render_component.h"
#include "vivid/log/log.h"

namespace VIVID::RENDER {

// WebGPU Resources (singleton/resource)
struct WebGPUResources {
  bool initialized = false;  // Flag to ensure one-time initialization
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  bool adapterRequestEnded = false;
  WGPUDevice device = nullptr;
  bool deviceRequestEnded = false;
  WGPUQueue queue = nullptr;
  WGPURenderPipeline pipeline = nullptr;
  WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;
  WGPUSurfaceConfiguration surfaceConfiguration = {};
  WGPUSurface surface = nullptr;
  uint32_t configuredWidth = 0;
  uint32_t configuredHeight = 0;
  // Depth resources
  WGPUTexture depthTexture = nullptr;
  WGPUTextureView depthView = nullptr;
  WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;
};

// GPU资源组件 - WebGPU version
struct GpuMeshComponent {
  WGPUBuffer vertexBuffer = nullptr;
  WGPUBuffer indexBuffer = nullptr;
  uint32_t indexCount = 0;
  WGPUBuffer uniformBuffer = nullptr;
  WGPUBindGroup bindGroup = nullptr;
  WGPUVertexBufferLayout vertexBufferLayout = {};
  WGPUPipelineLayout layout = nullptr;
  WGPUBindGroupLayout bindGroupLayout = nullptr;
  WGPURenderPipeline pipeline = nullptr;
};

// Forward declarations
struct ShutdownPhase {};  // Custom phase for cleanup systems

// Render Systems Module - manages WebGPU initialization, scene sync, and drawing
struct RenderSystems {
  RenderSystems(flecs::world& world);

private:
  // Actually used system implementations (registered in constructor)
  static void initWebGPUImpl(flecs::iter& it);
  static void syncSceneImpl(flecs::iter& it);
  static void drawImpl(flecs::iter& it);
  static void releaseWebGPUResourcesImpl(flecs::iter& it);

  // Additional system implementations (not registered, but converted to Flecs format)
  static void createWebGPUInstanceImpl(flecs::iter& it);
  static void requestWebGPUAdapterSyncImpl(flecs::iter& it);
  static void inspectWebGPUAdapterImpl(flecs::iter& it);
  static void requestWebGPUDeviceSyncImpl(flecs::iter& it);
  static void inspectWebGPUDeviceImpl(flecs::iter& it);
  static void testCommandQueueImpl(flecs::iter& it);
  static void createPipelineImpl(flecs::iter& it);

  // Helper functions
  // static void reconfigureSurface(flecs::world world, uint32_t width, uint32_t height);
  // static WGPUAdapter getAdapter(wgpu::Instance& instance);
  // static WGPUDevice getDevice(wgpu::Instance& instance, wgpu::Adapter& adapter);
};

inline RenderSystems::RenderSystems(flecs::world& world) {
  // Register module
  world.module<RenderSystems>();

  // Import component modules
  world.import <RenderComponents>();
  // world.import <VIVID::WINDOW::WindowComponents>();  // Import window components for querying

  VividLogger::app_info("Registering RenderSystems...");
  world.set<WebGPUResources>({});
  world.component<GpuMeshComponent>();

  // Initialization - deferred to PreUpdate to see OnStart changes (defer mechanism)
  // OnStart systems' changes are only visible after the OnStart phase completes
  world.system("InitWebGPU").kind(flecs::PreUpdate).run(initWebGPUImpl);

  // Scene sync - runs every frame before update
  world.system("SyncScene").kind(flecs::OnStart).run(syncSceneImpl);

  // Drawing - runs every frame
  world.system("Draw").kind(flecs::OnUpdate).run(drawImpl);

  // Cleanup - runs once at shutdown
  world.system("ReleaseWebGPUResources").kind<ShutdownPhase>().run(releaseWebGPUResourcesImpl);

  VividLogger::app_info("RenderSystems registered successfully");
}

}  // namespace VIVID::RENDER