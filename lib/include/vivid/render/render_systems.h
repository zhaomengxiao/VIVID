#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>

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
#define VIVID_LOG_SUCCESS(msg, ...) VividLogger::app_info("✅ " msg, ##__VA_ARGS__);
#define VIVID_LOG_ERROR(msg) VividLogger::app_error("❌ " msg);

namespace vivid::render {

// WebGPU Resources (singleton/resource)
struct WebGPUContext {
  bool initialized_ = false;  // Flag to ensure one-time initialization
  WGPUInstance instance_ = nullptr;
  WGPUAdapter adapter_ = nullptr;
  bool adapter_request_ended_ = false;
  WGPUDevice device_ = nullptr;
  bool device_request_ended_ = false;
  WGPUQueue queue_ = nullptr;
  WGPURenderPipeline pipeline_ = nullptr;
  WGPUTextureFormat surface_format_ = WGPUTextureFormat_Undefined;
  WGPUSurfaceConfiguration surface_configuration_ = WGPU_SURFACE_CONFIGURATION_INIT;
  WGPUSurface surface_ = nullptr;
  uint32_t configured_width_ = 0;
  uint32_t configured_height_ = 0;
  // Depth resources
  WGPUTexture depth_texture_ = nullptr;
  WGPUTextureView depth_view_ = nullptr;
  WGPUTextureFormat depth_format_ = WGPUTextureFormat_Depth24Plus;
  // Render context
  WGPURenderPassEncoder render_pass_ = nullptr;
  WGPUCommandEncoder encoder_ = nullptr;
  WGPUTextureView target_view_ = nullptr;
  WGPUSurfaceTexture surface_texture_ = WGPU_SURFACE_TEXTURE_INIT;
};

// GPU资源组件 - WebGPU version
struct GpuMeshComponent {
  WGPUBuffer vertex_buffer_ = nullptr;
  WGPUBuffer index_buffer_ = nullptr;
  uint32_t index_count_ = 0;
  WGPUBuffer uniform_buffer_ = nullptr;
  WGPUBindGroup bind_group_ = nullptr;
  WGPUVertexBufferLayout vertex_buffer_layout_ = {};
  WGPUPipelineLayout layout_ = nullptr;
  WGPUBindGroupLayout bind_group_layout_ = nullptr;
  WGPURenderPipeline pipeline_ = nullptr;
};

// Forward declarations
struct ShutdownPhase {};  // Custom phase for cleanup systems

// Render Systems Module - manages WebGPU initialization, scene sync, and drawing
struct RenderSystems {
  explicit RenderSystems(flecs::world& world);

private:
  // Multiple singletons: NO entity parameter!
  static void InitWebGPUImpl(const vivid::window::WindowContext& window_context,
                             WebGPUContext& webgpu_res);
  static void SyncSceneImpl(flecs::entity e, const MeshComponent& mesh,
                            const MaterialComponent& material, WebGPUContext& webgpu_res);
  // PreparePhase systems
  static void PrepareSurfaceImpl(const flecs::iter& it);
  static void PrepareViewportResourcesImpl(const flecs::iter& it);
  // RenderPhase systems
  static void BeginMainRenderPassImpl(const flecs::iter& it);
  static void BeginViewportRenderPassImpl(const flecs::iter& it);
  static void RenderSceneImpl(const flecs::iter& it);
  static void EndViewportRenderPassImpl(const flecs::iter& it);
  // SubmitPhase systems
  static void SubmitMainRenderPassImpl(const flecs::iter& it);
  static void ReleaseWebGPUResourcesImpl(const flecs::iter& it);

  // Additional system implementations (not registered, but converted to Flecs format)
  static void CreateWebGPUInstanceImpl(const flecs::iter& it);
  static void RequestWebGPUAdapterSyncImpl(const flecs::iter& it);
  static void InspectWebGPUAdapterImpl(const flecs::iter& it);
  static void RequestWebGPUDeviceSyncImpl(const flecs::iter& it);
  static void InspectWebGPUDeviceImpl(const flecs::iter& it);
  static void TestCommandQueueImpl(const flecs::iter& it);
  static void CreatePipelineImpl(const flecs::iter& it);
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
    VIVID_LOG_MODULE_INFO("│   └── Executes: InitWebGPUImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: PreUpdate");
    VIVID_LOG_MODULE_INFO("├── 🔄 SyncScene");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: MeshComponent, MaterialComponent, WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   ├── Filters: without<GpuMeshComponent>");
    VIVID_LOG_MODULE_INFO("│   └── Executes: SyncSceneImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: PreparePhase");
    VIVID_LOG_MODULE_INFO("├── 🔄 PrepareSurface");
    VIVID_LOG_MODULE_INFO("│   └── Executes: PrepareSurfaceImpl()");
    VIVID_LOG_MODULE_INFO("└── 🔄 PrepareViewportResources");
    VIVID_LOG_MODULE_INFO("    └── Executes: PrepareViewportResourcesImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: RenderPhase");
    VIVID_LOG_MODULE_INFO("├── 🔄 BeginMainRenderPass (conditional: no viewport)");
    VIVID_LOG_MODULE_INFO("├── 🔄 BeginMainRenderPass (conditional: no viewport)");
    VIVID_LOG_MODULE_INFO("│   └── Executes: BeginMainRenderPassImpl()");
    VIVID_LOG_MODULE_INFO("├── 🔄 BeginViewportRenderPass");
    VIVID_LOG_MODULE_INFO("│   └── Executes: BeginViewportRenderPassImpl()");
    VIVID_LOG_MODULE_INFO("├── 🔄 RenderScene");
    VIVID_LOG_MODULE_INFO("│   └── Executes: RenderSceneImpl()");
    VIVID_LOG_MODULE_INFO("└── 🔄 EndViewportRenderPass");
    VIVID_LOG_MODULE_INFO("    └── Executes: EndViewportRenderPassImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: SubmitPhase");
    VIVID_LOG_MODULE_INFO("└── 🔄 SubmitMainRenderPass (conditional: has main render pass)");
    VIVID_LOG_MODULE_INFO("    └── Executes: SubmitMainRenderPassImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
    VIVID_LOG_MODULE_INFO("└── 🔄 ReleaseWebGPUResources");
    VIVID_LOG_MODULE_INFO("    └── Executes: ReleaseWebGPUResourcesImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("💾 RESOURCES & COMPONENTS:");
    VIVID_LOG_MODULE_INFO("├── 🔹 WebGPUContext (singleton)");
    VIVID_LOG_MODULE_INFO("└── 🔹 GpuMeshComponent");
  });

  // Register module
  world.module<RenderSystems>();

  // Import component modules
  world.import <RenderComponents>();
  VIVID_LOG_SYSTEM("Registering RenderSystems...");
  world.set<WebGPUContext>({});
  world.component<GpuMeshComponent>();

  // add custom phases
  VividLogger::app_info("📍 Creating custom pipeline phases...");
  flecs::entity const kExtractPhase
      = world.entity("ExtractPhase").add(flecs::Phase).depends_on(flecs::OnStore);
  flecs::entity const kPreparePhase
      = world.entity("PreparePhase").add(flecs::Phase).depends_on(kExtractPhase);
  flecs::entity const kQueuePhase
      = world.entity("QueuePhase").add(flecs::Phase).depends_on(kPreparePhase);
  flecs::entity const kSortPhase
      = world.entity("SortPhase").add(flecs::Phase).depends_on(kQueuePhase);
  flecs::entity const kRenderPhase
      = world.entity("RenderPhase").add(flecs::Phase).depends_on(kSortPhase);
  flecs::entity const kRenderUiPhase
      = world.entity("RenderUIPhase").add(flecs::Phase).depends_on(kRenderPhase);
  flecs::entity const kSubmitPhase
      = world.entity("SubmitPhase").add(flecs::Phase).depends_on(kRenderUiPhase);
  VIVID_LOG_SUCCESS("Custom pipeline phases created");

  // Initialization - deferred to PreUpdate to see OnStart changes (defer mechanism)
  // OnStart systems' changes are only visible after the OnStart phase completes
  VIVID_LOG_SYSTEM("Registering InitWebGPU system...");

  // Debug: Check if singletons exist (only in debug builds)
#ifndef NDEBUG
  if (world.has<vivid::window::WindowContext>()) {
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
  world.system<const vivid::window::WindowContext, WebGPUContext>("InitWebGPU")
      .term_at(0)
      .src<vivid::window::WindowContext>()
      .term_at(1)
      .term_at(1)
      .src<WebGPUContext>()
      .kind(flecs::OnStart)
      .each(InitWebGPUImpl);
  VIVID_LOG_SUCCESS("InitWebGPU system registered successfully");

  // Scene sync - runs every frame before update
  world.system<MeshComponent, MaterialComponent, WebGPUContext>("SyncScene")
      .without<GpuMeshComponent>()
      .term_at(2)
      .src<WebGPUContext>()
      .kind(flecs::PreUpdate)
      .each(SyncSceneImpl);

  // PreparePhase systems
  world.system("PrepareSurface").kind(kPreparePhase).run(PrepareSurfaceImpl);
  world.system("PrepareViewportResources").kind(kPreparePhase).run(PrepareViewportResourcesImpl);

  // RenderPhase systems
  world.system("BeginMainRenderPass").kind(kRenderPhase).run(BeginMainRenderPassImpl);
  world.system("BeginViewportRenderPass").kind(kRenderPhase).run(BeginViewportRenderPassImpl);
  world.system("RenderScene").kind(kRenderPhase).run(RenderSceneImpl);
  world.system("EndViewportRenderPass").kind(kRenderPhase).run(EndViewportRenderPassImpl);

  // SubmitPhase systems
  world.system("SubmitMainRenderPass").kind(kSubmitPhase).run(SubmitMainRenderPassImpl);

  // Cleanup - runs once at shutdown
  world.system("ReleaseWebGPUResources").kind<ShutdownPhase>().run(ReleaseWebGPUResourcesImpl);

  VIVID_LOG_SUCCESS("RenderSystems module registration completed!");
}

}  // namespace vivid::render
