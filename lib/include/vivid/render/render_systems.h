#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <string>

#include "render_component.h"
#include "vivid/log/log.h"
#include "vivid/window/window_component.h"

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

// 渲染上下文 - 存储渲染过程中的共享数据
struct RenderContext {
  // 表面信息
  uint32_t surfaceWidth = 0;
  uint32_t surfaceHeight = 0;
  WGPUTextureView targetView = nullptr;
  WGPURenderPassEncoder renderPass = nullptr;
  WGPUCommandEncoder encoder = nullptr;
  WGPUSurfaceTexture surfaceTexture = {};

  // 场景信息
  glm::mat4 viewMatrix = glm::mat4(1.0f);
  glm::mat4 projectionMatrix = glm::mat4(1.0f);
  glm::vec3 viewPos = glm::vec3(0.0f);
  glm::vec3 lightPos = glm::vec3(5.0f, 5.0f, 5.0f);
  glm::vec3 lightColor = glm::vec3(1.0f);
  glm::vec3 ambientColor = glm::vec3(0.2f);
  float constant = 1.0f, linear = 0.09f, quadratic = 0.032f;

  // 渲染状态
  bool surfaceReady = false;
  bool sceneReady = false;
  bool meshesReady = false;
  bool uiReady = false;
};

// Forward declarations
struct ShutdownPhase {};  // Custom phase for cleanup systems

// Render Systems Module - manages WebGPU initialization, scene sync, and drawing
struct RenderSystems {
  RenderSystems(flecs::world& world);

private:
  // New modular system implementations
  static void surfaceManagementImpl(flecs::entity e, VIVID::WINDOW::WindowGpuComponent& gpu_comp,
                                    WebGPUResources& webgpuRes, RenderContext& renderCtx);
  static void sceneCollectionImpl(flecs::entity e, RenderContext& renderCtx);
  static void meshRenderImpl(flecs::entity e, GpuMeshComponent& gpuMesh,
                             TransformComponent& transform, MaterialComponent& material,
                             RenderContext& renderCtx);
  static void uiRenderImpl(flecs::entity e, RenderContext& renderCtx);
  static void commandSubmissionImpl(flecs::entity e, WebGPUResources& webgpuRes,
                                    RenderContext& renderCtx);

  // Legacy system implementations (kept for compatibility)
  static void initWebGPUImpl(flecs::entity e, VIVID::WINDOW::WindowGpuComponent& gpu_comp,
                             WebGPUResources& webgpuRes);
  static void syncSceneImpl(flecs::entity e, MeshComponent& mesh, MaterialComponent& material,
                            WebGPUResources& webgpuRes);
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
  world.set<RenderContext>({});
  world.component<GpuMeshComponent>();

  // Initialization - deferred to PreUpdate to see OnStart changes (defer mechanism)
  // OnStart systems' changes are only visible after the OnStart phase completes
  world.system<VIVID::WINDOW::WindowGpuComponent, WebGPUResources>("InitWebGPU")
      .term_at(1)
      .src<WebGPUResources>()
      .kind(flecs::OnStart)
      .each(initWebGPUImpl);

  // Scene sync - runs every frame before update
  world.system<MeshComponent, MaterialComponent, WebGPUResources>("SyncScene")
      .without<GpuMeshComponent>()
      .term_at(2)
      .src<WebGPUResources>()
      .kind(flecs::PreUpdate)
      .each(syncSceneImpl);

  // world
  //     .system<VIVID::WINDOW::WindowGpuComponent, WebGPUResources, RenderContext>(
  //         "SurfaceManagement")
  //     .term_at(1)
  //     .src<WebGPUResources>()
  //     .term_at(2)
  //     .src<RenderContext>()
  //     .kind(flecs::OnUpdate)
  //     .each(surfaceManagementImpl);

  // world.system<RenderContext>("SceneCollection").kind(flecs::OnUpdate).each(sceneCollectionImpl);

  // world.system<GpuMeshComponent, TransformComponent, MaterialComponent,
  // RenderContext>("MeshRender")
  //     .term_at(3)
  //     .src<RenderContext>()
  //     .kind(flecs::OnUpdate)
  //     .each(meshRenderImpl);

  // world.system<RenderContext>("UIRender").kind(flecs::OnUpdate).each(uiRenderImpl);

  // world.system<WebGPUResources, RenderContext>("CommandSubmission")
  //     .kind(flecs::OnUpdate)
  //     .each(commandSubmissionImpl);

  // Drawing - runs every frame (temporarily disabled during refactoring)
  world.system("Draw").kind(flecs::OnUpdate).run(drawImpl);

  // Cleanup - runs once at shutdown
  world.system("ReleaseWebGPUResources").kind<ShutdownPhase>().run(releaseWebGPUResourcesImpl);

  VividLogger::app_info("RenderSystems registered successfully");
}

}  // namespace VIVID::RENDER