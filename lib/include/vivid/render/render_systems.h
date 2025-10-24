#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <string>

#include "render_component.h"
#include "vivid/log/log.h"
#include "vivid/window/window_component.h"

// Simplified logging macros for modules
#ifdef NDEBUG
#  define VIVID_LOG_MODULE_HEADER(name, icon, content) \
    VividLogger::app_info("🔧 " name " module registration...");
#  define VIVID_LOG_MODULE_INFO(msg) VividLogger::app_info("  " msg);
#else
#  define VIVID_LOG_MODULE_HEADER(name, icon, content)                                       \
    VividLogger::app_info(                                                                   \
        "╔══════════════════════════════════════════════════════════════════════════════╗"); \
    VividLogger::app_info("║                          " icon " " name                        \
                          " MODULE                           ║");                            \
    VividLogger::app_info(                                                                   \
        "║                                                                              ║"); \
    content VividLogger::app_info(                                                           \
        "╚══════════════════════════════════════════════════════════════════════════════╝");
#  define VIVID_LOG_MODULE_INFO(msg) VividLogger::app_info("║  " msg);
#endif

#define VIVID_LOG_SYSTEM(msg) VividLogger::app_info("🔧 " msg);
#define VIVID_LOG_SUCCESS(msg) VividLogger::app_info("✅ " msg);
#define VIVID_LOG_ERROR(msg) VividLogger::app_error("❌ " msg);

namespace VIVID::RENDER {

// WebGPU Resources (singleton/resource)
struct WebGPUContext {
  bool initialized = false;  // Flag to ensure one-time initialization
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  bool adapterRequestEnded = false;
  WGPUDevice device = nullptr;
  bool deviceRequestEnded = false;
  WGPUQueue queue = nullptr;
  WGPURenderPipeline pipeline = nullptr;
  WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;
  WGPUSurfaceConfiguration surfaceConfiguration = WGPU_SURFACE_CONFIGURATION_INIT;
  WGPUSurface surface = nullptr;
  uint32_t configuredWidth = 0;
  uint32_t configuredHeight = 0;
  // Depth resources
  WGPUTexture depthTexture = nullptr;
  WGPUTextureView depthView = nullptr;
  WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;
  // Render context
  WGPURenderPassEncoder renderPass = nullptr;
  WGPUCommandEncoder encoder = nullptr;
  WGPUTextureView targetView = nullptr;
  WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
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
  // Multiple singletons: NO entity parameter!
  static void initWebGPUImpl(const VIVID::WINDOW::WindowContext& windowContext,
                             WebGPUContext& webgpuRes);
  static void syncSceneImpl(flecs::entity e, MeshComponent& mesh, MaterialComponent& material,
                            WebGPUContext& webgpuRes);
  static void renderMeshImpl(flecs::iter& it);
  static void submitImpl(flecs::iter& it);
  static void releaseWebGPUResourcesImpl(flecs::iter& it);

  // Additional system implementations (not registered, but converted to Flecs format)
  static void createWebGPUInstanceImpl(flecs::iter& it);
  static void requestWebGPUAdapterSyncImpl(flecs::iter& it);
  static void inspectWebGPUAdapterImpl(flecs::iter& it);
  static void requestWebGPUDeviceSyncImpl(flecs::iter& it);
  static void inspectWebGPUDeviceImpl(flecs::iter& it);
  static void testCommandQueueImpl(flecs::iter& it);
  static void createPipelineImpl(flecs::iter& it);
};

inline RenderSystems::RenderSystems(flecs::world& world) {
  // Display module overview only in Debug mode to reduce verbosity
  VIVID_LOG_MODULE_HEADER("RENDER SYSTEMS", "🎨", {
    VIVID_LOG_MODULE_INFO("📦 Module: RenderSystems");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📋 DEPENDENCIES:");
    VIVID_LOG_MODULE_INFO("├── 📦 RenderComponents (imported)");
    VIVID_LOG_MODULE_INFO("└── 📦 WindowComponents (for WindowContext access)");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("🎯 CUSTOM PIPELINE PHASES:");
    VIVID_LOG_MODULE_INFO("├── 📍 ExtractPhase  ← depends_on(OnStore)");
    VIVID_LOG_MODULE_INFO("├── 📍 PreparePhase  ← depends_on(ExtractPhase)");
    VIVID_LOG_MODULE_INFO("├── 📍 QueuePhase    ← depends_on(PreparePhase)");
    VIVID_LOG_MODULE_INFO("├── 📍 SortPhase     ← depends_on(QueuePhase)");
    VIVID_LOG_MODULE_INFO("├── 📍 RenderPhase   ← depends_on(SortPhase)");
    VIVID_LOG_MODULE_INFO("├── 📍 RenderUIPhase ← depends_on(RenderPhase)");
    VIVID_LOG_MODULE_INFO("└── 📍 SubmitPhase   ← depends_on(RenderUIPhase)");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("🏗️  SYSTEMS REGISTRATION:");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: OnStart");
    VIVID_LOG_MODULE_INFO("├── 🔄 InitWebGPU");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WindowContext, WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: initWebGPUImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: PreUpdate");
    VIVID_LOG_MODULE_INFO("├── 🔄 SyncScene");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: MeshComponent, MaterialComponent, WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   ├── Filters: without<GpuMeshComponent>");
    VIVID_LOG_MODULE_INFO("│   └── Executes: syncSceneImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: RenderPhase");
    VIVID_LOG_MODULE_INFO("├── 🔄 RenderMesh");
    VIVID_LOG_MODULE_INFO("│   └── Executes: renderMeshImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: SubmitPhase");
    VIVID_LOG_MODULE_INFO("├── 🔄 Submit");
    VIVID_LOG_MODULE_INFO("│   └── Executes: submitImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
    VIVID_LOG_MODULE_INFO("└── 🔄 ReleaseWebGPUResources");
    VIVID_LOG_MODULE_INFO("    └── Executes: releaseWebGPUResourcesImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("💾 RESOURCES & COMPONENTS:");
    VIVID_LOG_MODULE_INFO("├── 🔹 WebGPUContext (singleton)");
    VIVID_LOG_MODULE_INFO("└── 🔹 GpuMeshComponent");
  });

  // Register module
  world.module<RenderSystems>();

  // Import component modules
  world.import <RenderComponents>();
  // world.import <VIVID::WINDOW::WindowComponents>();  // Import window components for querying
  VIVID_LOG_SYSTEM("Registering RenderSystems...");
  world.set<WebGPUContext>({});
  world.component<GpuMeshComponent>();

  // add custom phases
  VividLogger::app_info("📍 Creating custom pipeline phases...");
  flecs::entity ExtractPhase
      = world.entity("ExtractPhase").add(flecs::Phase).depends_on(flecs::OnStore);
  flecs::entity PreparePhase
      = world.entity("PreparePhase").add(flecs::Phase).depends_on(ExtractPhase);
  flecs::entity QueuePhase = world.entity("QueuePhase").add(flecs::Phase).depends_on(PreparePhase);
  flecs::entity SortPhase = world.entity("SortPhase").add(flecs::Phase).depends_on(QueuePhase);
  flecs::entity RenderPhase = world.entity("RenderPhase").add(flecs::Phase).depends_on(SortPhase);
  flecs::entity RenderUIPhase
      = world.entity("RenderUIPhase").add(flecs::Phase).depends_on(RenderPhase);
  flecs::entity SubmitPhase
      = world.entity("SubmitPhase").add(flecs::Phase).depends_on(RenderUIPhase);
  VIVID_LOG_SUCCESS("Custom pipeline phases created");

  // Initialization - deferred to PreUpdate to see OnStart changes (defer mechanism)
  // OnStart systems' changes are only visible after the OnStart phase completes
  VIVID_LOG_SYSTEM("Registering InitWebGPU system...");

  // Debug: Check if singletons exist (only in debug builds)
#ifndef NDEBUG
  if (world.has<WINDOW::WindowContext>()) {
    VividLogger::app_info("✅ WindowContext singleton exists for InitWebGPU");
  } else {
    VividLogger::app_error("❌ WindowContext singleton NOT found for InitWebGPU!");
  }

  if (world.has<WebGPUContext>()) {
    VividLogger::app_info("✅ WebGPUContext singleton exists for InitWebGPU");
  } else {
    VividLogger::app_error("❌ WebGPUContext singleton NOT found for InitWebGPU!");
  }
#endif

  // Multiple singletons: must use .term_at().src<>() for each AND no entity param
  world.system<const WINDOW::WindowContext, WebGPUContext>("InitWebGPU")
      .term_at(0)
      .src<WINDOW::WindowContext>()
      .term_at(1)
      .src<WebGPUContext>()
      .kind(flecs::OnStart)
      .each(initWebGPUImpl);
  VIVID_LOG_SUCCESS("InitWebGPU system registered successfully");

  // Scene sync - runs every frame before update
  world.system<MeshComponent, MaterialComponent, WebGPUContext>("SyncScene")
      .without<GpuMeshComponent>()
      .term_at(2)
      .src<WebGPUContext>()
      .kind(flecs::PreUpdate)
      .each(syncSceneImpl);

  // world
  //     .system<VIVID::WINDOW::WindowGpuComponent, WebGPUContext, RenderContext>(
  //         "SurfaceManagement")
  //     .term_at(1)
  //     .src<WebGPUContext>()
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

  // world.system<WebGPUContext, RenderContext>("CommandSubmission")
  //     .kind(flecs::OnUpdate)
  //     .each(commandSubmissionImpl);

  world.system("RenderMesh").kind(RenderPhase).run(renderMeshImpl);
  world.system("Submit").kind(SubmitPhase).run(submitImpl);

  // Cleanup - runs once at shutdown
  world.system("ReleaseWebGPUResources").kind<ShutdownPhase>().run(releaseWebGPUResourcesImpl);

  VIVID_LOG_SUCCESS("RenderSystems module registration completed!");
}

}  // namespace VIVID::RENDER