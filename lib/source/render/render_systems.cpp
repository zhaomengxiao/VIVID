#include "vivid/render/render_systems.h"

#include <SDL3/SDL.h>
// #define __EMSCRIPTEN__

#include <array>
#include <deque>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "vivid/log/log.h"
#include "vivid/render/render_component.h"
#include "vivid/window/window_component.h"
// ImGui rendering backend
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>

// Camera controller helpers for view matrix
#include "vivid/input/camera_controller.h"
// glm helpers for matrix ops
#include <glm/gtc/matrix_transform.hpp>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#endif
#include <webgpu/webgpu_cpp.h>

// Removed using directive - use explicit namespace qualifications instead

// Utility functions
namespace {
std::string_view ToStdStringView(WGPUStringView wgpu_string_view) {
  if (wgpu_string_view.data == nullptr) {
    return std::string_view();
  }
  if (wgpu_string_view.length == WGPU_STRLEN) {
    return std::string_view(wgpu_string_view.data);
  }
  return std::string_view(wgpu_string_view.data, wgpu_string_view.length);
}

WGPUStringView ToWgpuStringView(std::string_view std_string_view) {
  return {std_string_view.data(), std_string_view.size()};
}

WGPUStringView ToWgpuStringView(const char* c_string) { return {c_string, WGPU_STRLEN}; }

void SleepForMilliseconds(unsigned int milliseconds) {
#ifdef __EMSCRIPTEN__
  emscripten_sleep(milliseconds);
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}
}  // namespace

// ============================================================================
// Helper Structures for Render Refactoring
// ============================================================================

// Helper structure for render target configuration
struct RenderTarget {
  WGPURenderPassEncoder render_pass_;
  uint32_t width_;
  uint32_t height_;
};

// Helper structure for scene rendering context
struct SceneRenderContext {
  glm::mat4 view_matrix_;
  glm::mat4 projection_matrix_;
  glm::vec3 view_pos_;
  glm::vec3 light_pos_;
  vivid::render::Color3f light_color_;
  vivid::render::Color3f ambient_color_;
  float constant_;
  float linear_;
  float quadratic_;
};

// Structure to hold resources pending release
// We delay release by a few frames to ensure they're no longer in use
struct PendingRelease {
  WGPUTexture texture_ = nullptr;
  WGPUTextureView texture_view_ = nullptr;
  uint32_t frames_remaining_ = 3;  // Wait 3 frames before releasing
};

// Global pending releases for viewport textures
namespace {
std::deque<PendingRelease> pending_releases;

// Temporary storage for viewport render pass information
// Maps viewport entity ID to its render pass context
struct ViewportRenderContext {
  WGPUCommandEncoder encoder_ = nullptr;
  WGPURenderPassEncoder render_pass_ = nullptr;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  SceneRenderContext scene_ctx_{};
};

// Global map to store viewport render contexts (cleared each frame)
std::unordered_map<flecs::entity_t, ViewportRenderContext> viewport_render_contexts;

// Helper to queue resources for delayed release (prevents crashes when GPU still using them)
void QueueDelayedRelease(WGPUTexture texture, WGPUTextureView texture_view) {
  if (texture_view != nullptr) {
    pending_releases.push_back({nullptr, texture_view, 3});
  }
  if (texture != nullptr) {
    pending_releases.push_back({texture, nullptr, 3});
  }
}

// Process pending resource releases (called once per frame)
void ProcessPendingReleases() {
  for (auto it = pending_releases.begin(); it != pending_releases.end();) {
    if (--it->frames_remaining_ <= 0) {
      if (it->texture_view_ != nullptr) {
        wgpuTextureViewRelease(it->texture_view_);
      }
      if (it->texture_ != nullptr) {
        wgpuTextureRelease(it->texture_);
      }
      it = pending_releases.erase(it);
    } else {
      ++it;
    }
  }
}

/**
 * Fetch data from a GPU buffer back to the CPU.
 * This function blocks until the data is available on CPU, then calls the
 * `processBufferData` callback, and finally unmap the buffer.
 */
void FetchBufferDataSync(WGPUInstance instance, WGPUBuffer buffer, size_t buffer_size,
                         const std::function<void(const void*)>& process_buffer_data) {
  // Read the data back from buffer B
  // Context passed to `on_buffer_mapped` through the userdata pointer:
  struct OnBufferMappedContext {
    bool operation_ended_ = false;        // Turned true as soon as the callback is invoked
    bool mapping_is_successful_ = false;  // Turned true only if mapping succeeded
  };

  // This function has the type WGPUBufferMapCallback as defined in webgpu.h
  auto on_buffer_mapped = [](WGPUMapAsyncStatus status, struct WGPUStringView message,
                             void* userdata1, void* /* userdata2 */
                          ) {
    auto& context = *reinterpret_cast<OnBufferMappedContext*>(userdata1);
    context.operation_ended_ = true;
    if (status == WGPUMapAsyncStatus_Success) {
      context.mapping_is_successful_ = true;
    } else {
      std::cout << "Could not map buffer B! Status: " << status
                << ", message: " << ToStdStringView(message) << "\n";
    }
  };

  // We create an instance of the context shared with `on_buffer_mapped`
  OnBufferMappedContext context;

  // And we build the callback info:
  WGPUBufferMapCallbackInfo buffer_map_callback_info = {};
  buffer_map_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
  buffer_map_callback_info.callback = on_buffer_mapped;
  buffer_map_callback_info.userdata1 = &context;

  // And finally we launch the asynchronous operation
  wgpuBufferMapAsync(buffer, WGPUMapMode_Read,
                     0,  // offset
                     buffer_size, buffer_map_callback_info);

  // Process events until the map operation ended
  wgpuInstanceProcessEvents(instance);
  while (!context.operation_ended_) {
    SleepForMilliseconds(200);
    wgpuInstanceProcessEvents(instance);
  }

  if (context.mapping_is_successful_) {
    const void* buffer_data = wgpuBufferGetConstMappedRange(buffer, 0, buffer_size);
    process_buffer_data(buffer_data);
  }
}
}  // namespace

// Components

namespace vivid::render {

// Internal structs
// Uniforms for Blinn-Phong shading. Layout is 16-byte aligned for WGSL std140-like rules.
struct BPUniforms {
  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 projection_;
  glm::mat4 normal_matrix_;             // store as mat4 for alignment; use upper-left 3x3 in shader
  std::array<float, 4> view_pos_;       // xyz + pad
  std::array<float, 4> light_pos_;      // xyz + pad
  std::array<float, 4> object_color_;   // rgb + pad
  std::array<float, 4> light_color_;    // rgb + pad
  std::array<float, 4> ambient_color_;  // rgb + pad
  std::array<float, 4> specular_color_;  // rgb + pad
  std::array<float, 4> params_;          // constant, linear, quadratic, shininess
};

// ============================================================================
// Helper Functions
// ============================================================================

// Query and build scene rendering context (camera + light)
// This function extracts common scene data needed for rendering
static SceneRenderContext querySceneContext(flecs::world& world, uint32_t viewport_width,
                                            uint32_t viewport_height) {
  SceneRenderContext ctx{};
  ctx.view_matrix_ = glm::mat4(1.0f);
  ctx.projection_matrix_ = glm::mat4(1.0f);
  ctx.view_pos_ = glm::vec3(0.0f);

  // Query camera (first camera found becomes active)
  flecs::entity main_camera_entity;
  {
    auto camera_query = world.query<TransformComponent, CameraComponent>();
    camera_query.each([&](flecs::entity entity, const TransformComponent& /*transform*/,
                          const CameraComponent& /*camera*/) {
      if (!main_camera_entity.is_valid()) {
        main_camera_entity = entity;
      }
    });
  }

  if (main_camera_entity.is_valid()) {
    const auto& main_camera_transform = main_camera_entity.get<TransformComponent>();
    const auto& main_camera_component = main_camera_entity.get<CameraComponent>();

    ctx.view_pos_ = main_camera_transform.position_;

    // Build view matrix (check for camera controller component)
    if (main_camera_entity.has<CameraControllerComponent>()) {
      const auto& controller = main_camera_entity.get<CameraControllerComponent>();
      glm::vec3 target = main_camera_transform.position_ + controller.front_;
      ctx.view_matrix_ = glm::lookAt(main_camera_transform.position_, target, controller.up_);

    } else {
      ctx.view_matrix_
          = glm::lookAt(main_camera_transform.position_,
                        main_camera_transform.position_ + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    }

    // Build projection matrix (use provided or compute from aspect ratio)
    ctx.projection_matrix_ = main_camera_component.projection_matrix_;
    if (ctx.projection_matrix_ == glm::mat4(1.0F) && viewport_height > 0) {
      float const kAspect
          = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);
      ctx.projection_matrix_ = glm::perspective(glm::radians(45.0F), kAspect, 0.1F, 100.0F);
    }
  }

  // Query light (first light found becomes active)
  ctx.light_pos_ = glm::vec3(5.0F, 5.0F, 5.0F);
  ctx.light_color_ = Color3f(1.0F, 1.0F, 1.0F);
  ctx.ambient_color_ = Color3f(0.2F, 0.2F, 0.2F);
  ctx.constant_ = 1.0F;
  ctx.linear_ = 0.09F;
  ctx.quadratic_ = 0.032F;

  flecs::entity light_entity;
  {
    auto light_query = world.query<TransformComponent, LightComponent>();
    light_query.each([&](flecs::entity entity, const TransformComponent& /*light_transform*/,
                         const LightComponent& /*light_component*/) {
      if (!light_entity.is_valid()) {
        light_entity = entity;
      }
    });
  }

  if (light_entity.is_valid()) {
    const auto& light_transform = light_entity.get<TransformComponent>();
    const auto& light_component = light_entity.get<LightComponent>();

    ctx.light_pos_ = light_transform.position_;
    ctx.light_color_ = light_component.light_color_;
    ctx.ambient_color_ = light_component.ambient_color_;
    ctx.constant_ = light_component.constant_;
    ctx.linear_ = light_component.linear_;
    ctx.quadratic_ = light_component.quadratic_;
  }

  return ctx;
}

// Render all meshes to the specified render target
// This function is shared between main window rendering and viewport offscreen rendering
static void renderSceneToTarget(flecs::world& world, const RenderTarget& target,
                                const SceneRenderContext& scene_ctx,
                                const WebGPUContext& webgpu_res) {
  // Iterate over all GPU meshes and draw
  auto draw_query = world.query<GpuMeshComponent, TransformComponent, MaterialComponent>();
  draw_query.each([&](flecs::entity entity, const GpuMeshComponent& gpu,
                      const TransformComponent& transform, const MaterialComponent& material) {
    // Validate all GPU resources are valid before using them
    if (gpu.pipeline_ == nullptr || gpu.vertex_buffer_ == nullptr || gpu.index_buffer_ == nullptr
        || gpu.index_count_ == 0 || gpu.uniform_buffer_ == nullptr || gpu.bind_group_ == nullptr) {
      return;  // Skip entities with incomplete GPU resources
    }

    // Prepare per-entity uniforms
    BPUniforms uniforms = {};
    const glm::mat4 model = transform.GetTransform();
    const glm::mat4 normalMat = glm::transpose(glm::inverse(model));
    uniforms.model_ = model;
    uniforms.view_ = scene_ctx.view_matrix_;
    uniforms.projection_ = scene_ctx.projection_matrix_;
    uniforms.normal_matrix_ = normalMat;
    uniforms.view_pos_
        = {scene_ctx.view_pos_.x, scene_ctx.view_pos_.y, scene_ctx.view_pos_.z, 0.0F};
    uniforms.light_pos_
        = {scene_ctx.light_pos_.x, scene_ctx.light_pos_.y, scene_ctx.light_pos_.z, 0.0F};
    uniforms.object_color_
        = {material.object_color_.r_, material.object_color_.g_, material.object_color_.b_, 0.0F};
    uniforms.light_color_
        = {scene_ctx.light_color_.r_, scene_ctx.light_color_.g_, scene_ctx.light_color_.b_, 0.0F};
    uniforms.ambient_color_ = {scene_ctx.ambient_color_.r_, scene_ctx.ambient_color_.g_,
                               scene_ctx.ambient_color_.b_, 0.0F};
    uniforms.specular_color_ = {material.specular_color_.r_, material.specular_color_.g_,
                                material.specular_color_.b_, 0.0F};
    uniforms.params_
        = {scene_ctx.constant_, scene_ctx.linear_, scene_ctx.quadratic_, material.shininess_};

    // Update per-entity uniform buffer content
    wgpuQueueWriteBuffer(webgpu_res.queue_, gpu.uniform_buffer_, 0, &uniforms, sizeof(uniforms));

    // Bind pipeline and buffers, then draw
    wgpuRenderPassEncoderSetPipeline(target.render_pass_, gpu.pipeline_);
    wgpuRenderPassEncoderSetVertexBuffer(target.render_pass_, 0, gpu.vertex_buffer_, 0,
                                         WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetIndexBuffer(target.render_pass_, gpu.index_buffer_,
                                        WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetBindGroup(target.render_pass_, 0, gpu.bind_group_, 0, nullptr);
    wgpuRenderPassEncoderDrawIndexed(target.render_pass_, gpu.index_count_, 1, 0, 0, 0);
  });
}

// ============================================================================
// Viewport Offscreen Rendering Helpers
// ============================================================================

// Helper to restore old resources on creation failure
static void restoreViewportResources(ViewportComponent& viewport, WGPUTexture old_render,
                                     WGPUTextureView old_render_view, WGPUTexture old_depth,
                                     WGPUTextureView old_depth_view) {
  viewport.render_texture_ = old_render;
  viewport.render_texture_view_ = old_render_view;
  viewport.depth_texture_ = old_depth;
  viewport.depth_view_ = old_depth_view;
}

// Helper function to create or update offscreen render target for viewport
static void ensureViewportResources(ViewportComponent& viewport, const WebGPUContext& webgpu_res,
                                    uint32_t width, uint32_t height) {
  if (!webgpu_res.device_ || !webgpu_res.queue_
      || webgpu_res.surface_format_ == WGPUTextureFormat_Undefined) {
    return;
  }

  // Check if we need to recreate resources
  if (!viewport.initialized_ || viewport.configured_width_ != width
      || viewport.configured_height_ != height) {
    // Save old resources for delayed release
    WGPUTexture old_render_texture = viewport.render_texture_;
    WGPUTextureView old_render_texture_view = viewport.render_texture_view_;
    WGPUTexture old_depth_texture = viewport.depth_texture_;
    WGPUTextureView old_depth_view = viewport.depth_view_;

    // Clear references before creating new resources
    viewport.render_texture_ = nullptr;
    viewport.render_texture_view_ = nullptr;
    viewport.depth_texture_ = nullptr;
    viewport.depth_view_ = nullptr;

    // Create render texture_
    WGPUTextureDescriptor render_tex_desc = {};
    render_tex_desc.nextInChain = nullptr;
    render_tex_desc.label = ToWgpuStringView("Viewport render texture_");
    render_tex_desc.size = {width, height, 1};
    render_tex_desc.dimension = WGPUTextureDimension_2D;
    render_tex_desc.format = webgpu_res.surface_format_;
    render_tex_desc.mipLevelCount = 1;
    render_tex_desc.sampleCount = 1;
    render_tex_desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;

    viewport.render_texture_ = wgpuDeviceCreateTexture(webgpu_res.device_, &render_tex_desc);
    if (viewport.render_texture_ == nullptr) {
      VividLogger::app_error("Failed to create viewport render texture_");
      restoreViewportResources(viewport, old_render_texture, old_render_texture_view,
                               old_depth_texture, old_depth_view);
      return;
    }

    // Create render texture_ view
    WGPUTextureViewDescriptor render_view_desc = {};
    render_view_desc.nextInChain = nullptr;
    render_view_desc.label = ToWgpuStringView("Viewport render texture_ view");
    render_view_desc.format = webgpu_res.surface_format_;
    render_view_desc.dimension = WGPUTextureViewDimension_2D;
    render_view_desc.baseMipLevel = 0;
    render_view_desc.mipLevelCount = 1;
    render_view_desc.baseArrayLayer = 0;
    render_view_desc.arrayLayerCount = 1;
    render_view_desc.aspect = WGPUTextureAspect_All;
    viewport.render_texture_view_
        = wgpuTextureCreateView(viewport.render_texture_, &render_view_desc);
    if (viewport.render_texture_view_ == nullptr) {
      VividLogger::app_error("Failed to create viewport render texture_ view");
      wgpuTextureRelease(viewport.render_texture_);
      restoreViewportResources(viewport, old_render_texture, old_render_texture_view,
                               old_depth_texture, old_depth_view);
      return;
    }

    // Create depth texture_
    WGPUTextureDescriptor depth_desc = {};
    depth_desc.nextInChain = nullptr;
    depth_desc.label = ToWgpuStringView("Viewport depth texture_");
    depth_desc.size = {width, height, 1};
    depth_desc.dimension = WGPUTextureDimension_2D;
    depth_desc.format = webgpu_res.depth_format_;
    depth_desc.mipLevelCount = 1;
    depth_desc.sampleCount = 1;
    depth_desc.usage = WGPUTextureUsage_RenderAttachment;
    viewport.depth_texture_ = wgpuDeviceCreateTexture(webgpu_res.device_, &depth_desc);
    if (viewport.depth_texture_ == nullptr) {
      VividLogger::app_error("Failed to create viewport depth texture_");
      wgpuTextureViewRelease(viewport.render_texture_view_);
      wgpuTextureRelease(viewport.render_texture_);
      restoreViewportResources(viewport, old_render_texture, old_render_texture_view,
                               old_depth_texture, old_depth_view);
      return;
    }

    // Create depth view
    WGPUTextureViewDescriptor depth_view_desc = {};
    depth_view_desc.nextInChain = nullptr;
    depth_view_desc.label = ToWgpuStringView("Viewport depth texture_ view");
    depth_view_desc.format = webgpu_res.depth_format_;
    depth_view_desc.dimension = WGPUTextureViewDimension_2D;
    depth_view_desc.baseMipLevel = 0;
    depth_view_desc.mipLevelCount = 1;
    depth_view_desc.baseArrayLayer = 0;
    depth_view_desc.arrayLayerCount = 1;
    depth_view_desc.aspect = WGPUTextureAspect_DepthOnly;
    viewport.depth_view_ = wgpuTextureCreateView(viewport.depth_texture_, &depth_view_desc);
    if (viewport.depth_view_ == nullptr) {
      VividLogger::app_error("Failed to create viewport depth texture_ view");
      wgpuTextureRelease(viewport.depth_texture_);
      wgpuTextureViewRelease(viewport.render_texture_view_);
      wgpuTextureRelease(viewport.render_texture_);
      restoreViewportResources(viewport, old_render_texture, old_render_texture_view,
                               old_depth_texture, old_depth_view);
      return;
    }

    viewport.texture_id_ = reinterpret_cast<uintptr_t>(viewport.render_texture_view_);
    viewport.configured_width_ = width;
    viewport.configured_height_ = height;
    viewport.initialized_ = true;

    // Queue old resources for delayed release (after 3 frames)
    QueueDelayedRelease(old_render_texture, old_render_texture_view);
    QueueDelayedRelease(old_depth_texture, old_depth_view);
  } else if ((viewport.render_texture_view_ == nullptr) || (viewport.depth_view_ == nullptr)) {
    // Resources lost, mark for recreation
    viewport.initialized_ = false;
  }
}

static void reconfigureSurface(WebGPUContext& webgpu_res, uint32_t width, uint32_t height) {
  if ((webgpu_res.surface_ == nullptr) || (webgpu_res.device_ == nullptr)) {
    VividLogger::render_error("Surface or device is null");
    return;
  }

  if (width == 0 || height == 0) {
    VividLogger::render_error("Invalid dimensions: %ux%u, skipping reconfigure", width, height);
    return;
  }

  WGPUSurfaceConfiguration config = {};
  config.nextInChain = nullptr;
  config.width = width;
  config.height = height;
  config.format = webgpu_res.surface_format_;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.device = webgpu_res.device_;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;
  wgpuSurfaceConfigure(webgpu_res.surface_, &config);

  webgpu_res.configured_width_ = width;
  webgpu_res.configured_height_ = height;

  // (Re)create depth resources matching the surface size
  if (webgpu_res.depth_view_ != nullptr) {
    wgpuTextureViewRelease(webgpu_res.depth_view_);
    webgpu_res.depth_view_ = nullptr;
  }
  if (webgpu_res.depth_texture_ != nullptr) {
    wgpuTextureRelease(webgpu_res.depth_texture_);
    webgpu_res.depth_texture_ = nullptr;
  }

  WGPUTextureDescriptor depth_desc = {};
  depth_desc.nextInChain = nullptr;
  depth_desc.label = ToWgpuStringView("Depth texture_");
  depth_desc.usage = WGPUTextureUsage_RenderAttachment;
  depth_desc.dimension = WGPUTextureDimension_2D;
  depth_desc.size.width = width;
  depth_desc.size.height = height;
  depth_desc.size.depthOrArrayLayers = 1;
  depth_desc.format = webgpu_res.depth_format_;
  depth_desc.mipLevelCount = 1;
  depth_desc.sampleCount = 1;
  webgpu_res.depth_texture_ = wgpuDeviceCreateTexture(webgpu_res.device_, &depth_desc);

  WGPUTextureViewDescriptor depth_view_desc = {};
  depth_view_desc.nextInChain = nullptr;
  depth_view_desc.label = ToWgpuStringView("Depth texture_ view");
  depth_view_desc.format = webgpu_res.depth_format_;
  depth_view_desc.dimension = WGPUTextureViewDimension_2D;
  depth_view_desc.baseMipLevel = 0;
  depth_view_desc.mipLevelCount = 1;
  depth_view_desc.baseArrayLayer = 0;
  depth_view_desc.arrayLayerCount = 1;
  depth_view_desc.aspect = WGPUTextureAspect_All;
  webgpu_res.depth_view_ = wgpuTextureCreateView(webgpu_res.depth_texture_, &depth_view_desc);
}

static WGPUAdapter getAdapter(wgpu::Instance& instance) {
  wgpu::Adapter acquiredAdapter;
  wgpu::RequestAdapterOptions const adapterOptions;

  auto onRequestAdapter
      = [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
          if (status != wgpu::RequestAdapterStatus::Success) {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
          }
          acquiredAdapter = std::move(adapter);  // FIXME-WGPU: no need to use std::move?
        };

  // Synchronously (wait until) acquire Adapter
  wgpu::Future wait_adapter_func{
      instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter)};
  wgpu::WaitStatus wait_status_adapter = instance.WaitAny(wait_adapter_func, UINT64_MAX);
  VIVID_ASSERT(acquiredAdapter != nullptr && wait_status_adapter == wgpu::WaitStatus::Success
               && "Error on Adapter request");
#ifndef NDEBUG
  ImGui_ImplWGPU_PrintAdapterInfo_Helper(acquiredAdapter.Get());
#endif
  return acquiredAdapter.MoveToCHandle();
}

static WGPUDevice getDevice(wgpu::Instance& instance, wgpu::Adapter& adapter) {
  // Set device callback functions
  wgpu::DeviceDescriptor deviceDesc;
  deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                   ImGui_ImplWGPU_DAWN_DeviceLostCallback_Helper);
  deviceDesc.SetUncapturedErrorCallback(ImGui_ImplWGPU_DAWN_ErrorCallback_Helper);

  wgpu::Device acquiredDevice;
  auto onRequestDevice
      = [&](wgpu::RequestDeviceStatus status, wgpu::Device localDevice, wgpu::StringView message) {
          if (status != wgpu::RequestDeviceStatus::Success) {
            printf("Failed to get an device: %s\n", message.data);
            return;
          }
          acquiredDevice = std::move(localDevice);
        };

  // Synchronously (wait until) get Device
  wgpu::Future wait_device_func{
      adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice)};
  wgpu::WaitStatus wait_status_device = instance.WaitAny(wait_device_func, UINT64_MAX);
  IM_ASSERT(acquiredDevice != nullptr && waitStatusDevice == wgpu::WaitStatus::Success
            && "Error on Device request");
  return acquiredDevice.MoveToCHandle();
}

// ============================================================================
// System Implementations (not registered, converted to Flecs format)
// ============================================================================

void RenderSystems::CreateWebGPUInstanceImpl(const flecs::iter& it) {
  auto world = it.world();
  // We create a descriptor

  // The descriptor is a kind of way to pack many function arguments together, because some
  // descriptors really have a lot of fields. It can also be used to write utility functions that
  // take care of populating the arguments, to ease the program’s architecture.
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // We create the instance using this descriptor
  WGPUInstance instance = wgpuCreateInstance(&desc);

  // We can check whether there is actually an instance created
  if (!instance) {
    VividLogger::app_error("Could not initialize WebGPU!");
    return;
  }

  auto& webgpu_res = world.ensure<WebGPUContext>();
  webgpu_res.instance_ = instance;

  // Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  VividLogger::app_info("WGPU instance: %p", instance);
}

void RenderSystems::RequestWebGPUAdapterSyncImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Requesting WebGPU adapter...");

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpu_res = world.get_mut<WebGPUContext>();

  // A simple structure holding the local information shared with the
  // onAdapterRequestEnded callback.
  struct UserData {
    WGPUAdapter adapter_ = nullptr;
    bool request_ended_ = false;
  };
  UserData user_data;

  // Callback called by wgpuInstanceRequestAdapter when the request returns
  // This is a C++ lambda function, but could be any function defined in the
  // global scope. It must be non-capturing (the brackets [] are empty) so
  // that it behaves like a regular C function pointer, which is what
  // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
  // is to convey what we want to capture through the pUserData pointer,
  // provided as the last argument of wgpuInstanceRequestAdapter and received
  // by the callback as its last argument.
  WGPURequestAdapterCallback onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                                        WGPUAdapter adapter, WGPUStringView message,
                                                        void* userdata1, void* /*userdata2*/) {
    UserData& user_data = *reinterpret_cast<UserData*>(userdata1);
    if (status == WGPURequestAdapterStatus_Success) {
      user_data.adapter_ = adapter;
    } else {
      VividLogger::app_error("Could not get WebGPU adapter: %s", ToStdStringView(message).data());
    }
    user_data.request_ended_ = true;
  };

  WGPURequestAdapterCallbackInfo const kCallbackInfo = {
      nullptr, WGPUCallbackMode_AllowProcessEvents, onAdapterRequestEnded, (void*)&user_data,
      nullptr,
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(webgpu_res.instance_ /* equivalent of navigator.g_pu */, &adapterOpts,
                             kCallbackInfo);

  // We wait until userData.requestEnded gets true

  // Hand the execution to the WebGPU instance so that it can check for
  // pending async operations, in which case it invokes our callbacks.
  // NB: We test once before the loop not to wait for 200ms in case it is
  // already ready
  wgpuInstanceProcessEvents(webgpu_res.instance_);

  while (!user_data.request_ended_) {
    // Waiting for 200 ms to avoid asking too often to process events
    SleepForMilliseconds(200);

    wgpuInstanceProcessEvents(webgpu_res.instance_);
  }

  VIVID_ASSERT(user_data.request_ended_);

  auto& webgpu_res_mut = world.get_mut<WebGPUContext>();
  webgpu_res_mut.adapter_ = user_data.adapter_;
  webgpu_res_mut.adapter_request_ended_ = user_data.request_ended_;

  VividLogger::app_info("Got WebGPU adapter");
}

void RenderSystems::InspectWebGPUAdapterImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Inspecting WebGPU adapter...");

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  const auto& webgpu_res = world.get<WebGPUContext>();

#ifndef __EMSCRIPTEN__
  WGPULimits supported_limits = {};
  supported_limits.nextInChain = nullptr;

#  ifdef WEBGPU_BACKEND_DAWN
  bool success = wgpuAdapterGetLimits(webgpuRes.adapter_, &supportedLimits) == WGPUStatus_Success;
#  else
  bool const kSuccess = wgpuAdapterGetLimits(webgpu_res.adapter_, &supported_limits) != 0;
#  endif

  if (kSuccess) {
    VividLogger::app_info("Adapter limits:");
    VividLogger::app_info(" - maxTextureDimension1D: %u", supported_limits.maxTextureDimension1D);
    VividLogger::app_info(" - maxTextureDimension2D: %u", supported_limits.maxTextureDimension2D);
    VividLogger::app_info(" - maxTextureDimension3D: %u", supported_limits.maxTextureDimension3D);
    VividLogger::app_info(" - maxTextureArrayLayers: %u", supported_limits.maxTextureArrayLayers);
  }
#endif  // NOT __EMSCRIPTEN__

  // Features

  WGPUSupportedFeatures supported_features = {};

  // Call the function a first time with a null return address, just to get
  // the entry count.
  wgpuAdapterGetFeatures(webgpu_res.adapter_, &supported_features);

  std::cout << "Adapter features:" << '\n';
  std::cout << std::hex;  // Write integers as hexadecimal to ease comparison with webgpu.h literals
  for (size_t i = 0; i < supported_features.featureCount; ++i) {
    std::cout << " - 0x" << supported_features.features[i] << '\n';
  }
  std::cout << std::dec;  // Restore decimal numbers

  // Free the memory that had potentially been allocated by wgpuAdapterGetFeatures()
  wgpuSupportedFeaturesFreeMembers(supported_features);
  // One shall no longer use features beyond this line.

  // Properties
  WGPUAdapterInfo properties;
  properties.nextInChain = nullptr;
  wgpuAdapterGetInfo(webgpu_res.adapter_, &properties);
  VividLogger::app_info("Adapter properties:");
  VividLogger::app_info(" - vendorID: %u", properties.vendorID);
  VividLogger::app_info(" - vendorName: %s", ToStdStringView(properties.vendor).data());
  VividLogger::app_info(" - architecture: %s", ToStdStringView(properties.architecture).data());
  VividLogger::app_info(" - deviceID: %u", properties.deviceID);
  VividLogger::app_info(" - name: %s", ToStdStringView(properties.device).data());
  VividLogger::app_info(" - driverDescription: %s", ToStdStringView(properties.description).data());
  VividLogger::app_info(" - adapterType: 0x%X", properties.adapterType);
  VividLogger::app_info(" - backendType: 0x%X", properties.backendType);
  wgpuAdapterInfoFreeMembers(properties);
}

void RenderSystems::RequestWebGPUDeviceSyncImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Requesting WebGPU device...");
  WGPUDeviceDescriptor device_desc = {};
  device_desc.nextInChain = nullptr;
  // Any name works here, that's your call
  device_desc.label = ToWgpuStringView("My Device");
  device_desc.requiredFeatureCount = 0;
  device_desc.requiredFeatures = nullptr;
  device_desc.requiredLimits = nullptr;
  device_desc.defaultQueue.label = ToWgpuStringView("The Default Queue");

  auto onDeviceLost = [](WGPUDevice const* device, WGPUDeviceLostReason reason,
                         struct WGPUStringView message, void* /* userdata1 */, void* /* userdata2 */
                      ) {
    // All we do is display a message when the device is lost
    std::cout << "Device " << device << " was lost: reason " << reason << " ("
              << ToStdStringView(message) << ")" << std::endl;
  };

  device_desc.deviceLostCallbackInfo.callback = onDeviceLost;
  device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

  auto on_device_error
      = [](WGPUDevice const* device, WGPUErrorType type, struct WGPUStringView message,
           void* /* userdata1 */, void* /* userdata2 */
        ) {
          std::cout << "Uncaptured error in device " << device << ": type " << type << " ("
                    << ToStdStringView(message) << ")" << std::endl;
        };

  device_desc.uncapturedErrorCallbackInfo.callback = on_device_error;

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpu_res = world.get_mut<WebGPUContext>();

  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  // The callback
  auto on_device_request_ended = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                    WGPUStringView message, void* userdata1, void* /* userdata2 */
                                 ) {
    UserData& userData = *reinterpret_cast<UserData*>(userdata1);
    if (status == WGPURequestDeviceStatus_Success) {
      userData.device = device;
    } else {
      std::cerr << "Error while requesting device: " << ToStdStringView(message) << std::endl;
    }
    userData.requestEnded = true;
  };

  // Build the callback info
  WGPURequestDeviceCallbackInfo callbackInfo = {/* nextInChain = */ nullptr,
                                                /* mode = */ WGPUCallbackMode_AllowProcessEvents,
                                                /* callback = */ on_device_request_ended,
                                                /* userdata1 = */ &userData,
                                                /* userdata2 = */ nullptr};

  // Call to the WebGPU request adapter procedure
  wgpuAdapterRequestDevice(webgpu_res.adapter_, &device_desc, callbackInfo);

  // Hand the execution to the WebGPU instance until the request ended
  wgpuInstanceProcessEvents(webgpu_res.instance_);
  while (!userData.requestEnded) {
    SleepForMilliseconds(200);
    wgpuInstanceProcessEvents(webgpu_res.instance_);
  }

  VIVID_ASSERT(userData.requestEnded);

  auto& webgpu_res_mut = world.get_mut<WebGPUContext>();
  webgpu_res_mut.device_ = userData.device;
  webgpu_res_mut.device_request_ended_ = userData.requestEnded;
  // Set default queue for later write/submit operations
  webgpu_res_mut.queue_ = wgpuDeviceGetQueue(webgpu_res_mut.device_);

  VividLogger::app_info("Got WebGPU device");
}

void RenderSystems::InspectWebGPUDeviceImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Inspecting WebGPU device...");

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpu_res = world.get<WebGPUContext>();

  WGPUSupportedFeatures features = {};
  wgpuDeviceGetFeatures(webgpu_res.device_, &features);
  std::cout << "Device features:" << '\n';
  std::cout << std::hex;
  for (size_t i = 0; i < features.featureCount; ++i) {
    std::cout << " - 0x" << features.features[i] << '\n';
  }
  std::cout << std::dec;
  wgpuSupportedFeaturesFreeMembers(features);

  WGPULimits limits = {};
  bool success = wgpuDeviceGetLimits(webgpu_res.device_, &limits) == WGPUStatus_Success;

  if (success) {
    std::cout << "Device limits:" << '\n';
    std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << '\n';
    std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << '\n';
    std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << '\n';
    std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << '\n';
    std::cout << " - maxBindGroups: " << limits.maxBindGroups << '\n';
    std::cout << " - maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers
              << '\n';
    std::cout << " - maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << '\n';
    std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: "
              << limits.maxDynamicUniformBuffersPerPipelineLayout << '\n';
    std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: "
              << limits.maxDynamicStorageBuffersPerPipelineLayout << '\n';
    std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage
              << '\n';
    std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << '\n';
    std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage
              << '\n';
    std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage
              << '\n';
    std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage
              << '\n';
    std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << '\n';
    std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << '\n';
    std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment
              << '\n';
    std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment
              << '\n';
    std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << '\n';
    std::cout << " - maxBufferSize: " << limits.maxBufferSize << '\n';
    std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << '\n';
    std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << '\n';
    std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << '\n';
    std::cout << " - maxColorAttachments: " << limits.maxColorAttachments << '\n';
    std::cout << " - maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample
              << '\n';
    std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize
              << '\n';
    std::cout << " - maxComputeInvocationsPerWorkgroup: "
              << limits.maxComputeInvocationsPerWorkgroup << '\n';
    std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << '\n';
    std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << '\n';
    std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << '\n';
    std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension
              << '\n';
    // std::cout << " - maxStorageBuffersInVertexStage: " << limits.maxStorageBuffersInVertexStage
    //           << std::endl;
    // std::cout << " - maxStorageTexturesInVertexStage: " <<
    // limits.maxStorageTexturesInVertexStage
    //           << std::endl;
    // std::cout << " - maxStorageBuffersInFragmentStage: "
    //           << limits.maxStorageBuffersInFragmentStage << std::endl;
    // std::cout << " - maxStorageTexturesInFragmentStage: "
    //           << limits.maxStorageTexturesInFragmentStage << std::endl;
  }
}

void RenderSystems::TestCommandQueueImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Testing WebGPU command queue...");
  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpu_res = world.get_mut<WebGPUContext>();
  // Get the queue
  WGPUQueue queue = wgpuDeviceGetQueue(webgpu_res.device_);
  webgpu_res.queue_ = queue;
  // Create the command encoder
  WGPUCommandEncoderDescriptor encoder_desc = {};
  encoder_desc.label = ToWgpuStringView("My command encoder");

  // Create buffers
  // Create buffer A
  WGPUBufferDescriptor buffer_desc_a = {};
  buffer_desc_a.size = 256;
  // Buffer A is *written* on CPU, and used as *source* of a GPU-side copy
  buffer_desc_a.usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
  buffer_desc_a.label = ToWgpuStringView("Buffer A");
  buffer_desc_a.mappedAtCreation = true;

  WGPUBuffer buffer_a = wgpuDeviceCreateBuffer(webgpu_res.device_, &buffer_desc_a);
  // Create buffer B
  // We build a second buffer, called B
  WGPUBufferDescriptor bufferDescB = {};
  bufferDescB.size = 32;
  // Buffer B is *read* on CPU, and used as *destination* of a GPU-side copy
  bufferDescB.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
  bufferDescB.label = ToWgpuStringView("Buffer B");

  WGPUBuffer bufferB = wgpuDeviceCreateBuffer(webgpu_res.device_, &bufferDescB);
  // Writhe initial data to buffer A
  // Get a pointer to the entire mapped buffer and interpret it as 8-bit unsigned integers
  uint8_t* bufferDataA
      = static_cast<uint8_t*>(wgpuBufferGetMappedRange(buffer_a, 0, buffer_desc_a.size));

  // Write 0, 1, 2, 3, ... in bufferA
  for (size_t i = 0; i < 256; ++i) {
    bufferDataA[i] = static_cast<uint8_t>(i);
  }

  // see also wgpuBufferWriteMappedRange, wgpuQueueWriteBuffer

  wgpuBufferUnmap(buffer_a);
  // Do NOT use bufferDataA beyond this point!

  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(webgpu_res.device_, &encoder_desc);

  // Insert debug markers
  // wgpuCommandEncoderInsertDebugMarker(encoder, ToWgpuStringView("Do one thing"));
  // wgpuCommandEncoderInsertDebugMarker(encoder, ToWgpuStringView("Do another thing"));
  wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer_a,
                                       16,  // sourceOffset
                                       bufferB,
                                       0,  // destinationOffset
                                       bufferDescB.size);

  // generate the command buffer by finishing the command encoder
  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.label = ToWgpuStringView("Command buffer");
  WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
  wgpuCommandEncoderRelease(encoder);  // release encoder after it's finished

  // Finally submit the command queue
  std::cout << "Submitting command..." << std::endl;
  wgpuQueueSubmit(queue, 1, &command);
  wgpuCommandBufferRelease(command);
  std::cout << "Command submitted." << std::endl;

  // wait for completion
  //  Our callback invoked when GPU instructions have been executed
  auto onQueuedWorkDone = [](WGPUQueueWorkDoneStatus status, WGPUStringView /* message */,
                             void* userdata1, void* /* userdata2 */
                          ) {
    // Display a warning when status is not success
    if (status != WGPUQueueWorkDoneStatus_Success) {
      VividLogger::render_error(
          "Warning: wgpuQueueOnSubmittedWorkDone failed, this is suspicious!");
    }

    // Interpret userdata1 as a pointer to a boolean (and turn it into a
    // mutable reference), then turn it to 'true'
    bool& workDone = *reinterpret_cast<bool*>(userdata1);
    workDone = true;
  };

  // Create the boolean that will be passed to the callback as userdata1
  // and initialize it to 'false'
  bool workDone = false;

  // Create the callback info
  WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
  callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
  callbackInfo.callback = onQueuedWorkDone;
  callbackInfo.userdata1 = &workDone;  // pass the address of workDone

  // Add the async operation to the queue
  wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);

  // Hand the execution to the WebGPU instance until onQueuedWorkDone gets invoked
  wgpuInstanceProcessEvents(webgpu_res.instance_);
  while (!workDone) {
    SleepForMilliseconds(200);
    wgpuInstanceProcessEvents(webgpu_res.instance_);
  }

  FetchBufferDataSync(webgpu_res.instance_, bufferB, bufferDescB.size, [&](const void* data) {
    const auto* buffer_data_b = static_cast<const uint8_t*>(data);
    std::cout << "Buffer B: [";
    for (size_t i = 0; i < bufferDescB.size; ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << static_cast<int>(buffer_data_b[i]);
    }
    std::cout << "]" << '\n';
  });

  wgpuBufferUnmap(bufferB);

  // At the end of the program:
  wgpuBufferRelease(buffer_a);
  wgpuBufferRelease(bufferB);

  VividLogger::app_info("All queued instructions have been executed!");

  VividLogger::app_info("WebGPU command queue tested");
}

// ============================================================================
// Core System Implementations (registered and actually used)
// ============================================================================

void RenderSystems::SyncSceneImpl(flecs::entity entity, const MeshComponent& mesh,
                                  const MaterialComponent& material, WebGPUContext& webgpuRes) {
  // Process entities that have the CPU-side data (Mesh, Material)
  // but DO NOT have the GPU-side data (GpuMeshComponent) yet.
  // (filtered by .without<GpuMeshComponent>() in system registration)

  if (mesh.vertices_.empty() || mesh.indices_.empty() || material.shader_path_.empty()) return;

  // 创建和绑定VBO
  // Create vertex buffer
  WGPUBufferDescriptor bufferDesc = {};
  bufferDesc.nextInChain = nullptr;
  bufferDesc.label = ToWgpuStringView("Vertex buffer");
  bufferDesc.size = mesh.vertices_.size() * sizeof(float);
  bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
  WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(webgpuRes.device_, &bufferDesc);

  // Upload geometry data to the buffer
  wgpuQueueWriteBuffer(webgpuRes.queue_, vertexBuffer, 0, mesh.vertices_.data(), bufferDesc.size);

  // 创建IBO
  // Create index buffer (use 32-bit indices to match MeshComponent definition)
  // (we reuse the bufferDesc initialized for the vertexBuffer)
  bufferDesc.size = mesh.indices_.size() * sizeof(uint32_t);

  // only need when using uint16_t, uint32_t is 4 bytes aligned
  // bufferDesc.size = (bufferDesc.size + 3) & ~3;  // round up to the next multiple of 4
  // mesh.m_Indices.resize((mesh.m_Indices.size() + 1)
  //                       & ~1);  // round up to the next multiple of 2
  bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
  WGPUBuffer indexBuffer = wgpuDeviceCreateBuffer(webgpuRes.device_, &bufferDesc);

  wgpuQueueWriteBuffer(webgpuRes.queue_, indexBuffer, 0, mesh.indices_.data(), bufferDesc.size);

  // 创建和绑定VAO
  WGPUVertexBufferLayout vertexBufferLayout = {};
  std::vector<WGPUVertexAttribute> vertexAttribs(2);
  // Describe the position attribute
  vertexAttribs[0].shaderLocation = 0;  // @location(0)
  vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
  vertexAttribs[0].offset = 0;
  // Describe the color attribute
  vertexAttribs[1].shaderLocation = 1;                   // @location(1)
  vertexAttribs[1].format = WGPUVertexFormat_Float32x3;  // different type!
  vertexAttribs[1].offset = 3 * sizeof(float);           // non null offset!

  vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
  vertexBufferLayout.attributes = vertexAttribs.data();

  vertexBufferLayout.arrayStride = 6 * sizeof(float);
  vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

  // shader (WGSL Blinn-Phong equivalent of standalone/res/shaders/BlinnPhong.shader)
  const char* shaderSource = R"(
struct VertexInput {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) fragPos: vec3f,
  @location(1) normal: vec3f,
};

struct BPUniforms {
  model: mat4x4<f32>,
  view: mat4x4<f32>,
  projection: mat4x4<f32>,
  normalMatrix: mat4x4<f32>,
  viewPos: vec4f,
  lightPos: vec4f,
  objectColor: vec4f,
  lightColor: vec4f,
  ambientColor: vec4f,
  specularColor: vec4f,
  params: vec4f, // x: constant, y: linear, z: quadratic, w: shininess
};

@group(0) @binding(0)
var<uniform> u: BPUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  let worldPos = (u.model * vec4f(in.position, 1.0)).xyz;
  out.fragPos = worldPos;
  out.normal = normalize((u.normalMatrix * vec4f(in.normal, 0.0)).xyz);
  out.position = u.projection * u.view * u.model * vec4f(in.position, 1.0);
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let ambient = u.ambientColor.xyz * u.objectColor.xyz;

  let distance = length(u.lightPos.xyz - in.fragPos);
  let attenuation = 1.0 / (u.params.x + u.params.y * distance + u.params.z * distance * distance);
  let attenuatedLight = u.lightColor.xyz * attenuation;

  let n = normalize(in.normal);
  let lightDir = normalize(u.lightPos.xyz - in.fragPos);
  let diff = max(dot(n, lightDir), 0.0);
  let diffuse = diff * attenuatedLight * u.objectColor.xyz;

  let viewDir = normalize(u.viewPos.xyz - in.fragPos);
  let halfwayDir = normalize(lightDir + viewDir);
  let spec = max(pow(max(dot(n, halfwayDir), 0.0), u.params.w), 0.0);
  let specular = spec * attenuatedLight * u.specularColor.xyz;

  return vec4f(ambient + diffuse + specular, 1.0);
}
    )";

  // create the shader module
  WGPUShaderSourceWGSL wgslDesc = {};
  wgslDesc.chain.next = nullptr;
  wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgslDesc.code = ToWgpuStringView(shaderSource);
  WGPUShaderModuleDescriptor shaderDesc = {};
  shaderDesc.nextInChain = &wgslDesc.chain;  // connect the chained extension
  shaderDesc.label = ToWgpuStringView("Shader source");
  WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(webgpuRes.device_, &shaderDesc);

  // When describing the render pipeline:
  WGPURenderPipelineDescriptor pipelineDesc = {};
  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = ToWgpuStringView("vs_main");

  WGPUFragmentState fragmentState = {};
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = ToWgpuStringView("fs_main");
  WGPUColorTargetState colorTarget = {};
  colorTarget.format = webgpuRes.surface_format_;
  WGPUBlendState blendState = {};
  colorTarget.blend = &blendState;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;
  pipelineDesc.fragment = &fragmentState;

  // Primitive state
  WGPUPrimitiveState primitive = {};
  primitive.topology = WGPUPrimitiveTopology_TriangleList;
  primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  primitive.frontFace = WGPUFrontFace_CCW;
  primitive.cullMode = WGPUCullMode_Back;  // cull back faces
  pipelineDesc.primitive = primitive;

  // Multisample state
  WGPUMultisampleState multisample = {};
  multisample.count = 1;
  multisample.mask = 0xFFFFFFFF;
  multisample.alphaToCoverageEnabled = false;
  pipelineDesc.multisample = multisample;

  // Depth-stencil state
  WGPUDepthStencilState depthStencil = {};
  depthStencil.format = webgpuRes.depth_format_;
  depthStencil.depthWriteEnabled = WGPUOptionalBool_True;
  depthStencil.depthCompare = WGPUCompareFunction_Less;
  depthStencil.stencilReadMask = 0xFFFFFFFF;
  depthStencil.stencilWriteMask = 0xFFFFFFFF;
  pipelineDesc.depthStencil = &depthStencil;
  // Define binding layout for a uniform buffer used in VS/FS
  WGPUBindGroupLayoutEntry bindingLayout = {};
  bindingLayout.binding = 0;  // shader @binding(0)
  bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
  bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
  bindingLayout.buffer.hasDynamicOffset = false;
  bindingLayout.buffer.minBindingSize = sizeof(BPUniforms);

  // Create a bind group layout
  WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
  bindGroupLayoutDesc.entryCount = 1;
  bindGroupLayoutDesc.entries = &bindingLayout;
  WGPUBindGroupLayout bindGroupLayout
      = wgpuDeviceCreateBindGroupLayout(webgpuRes.device_, &bindGroupLayoutDesc);

  // Create the pipeline layout
  WGPUPipelineLayoutDescriptor layoutDesc = {};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = (const WGPUBindGroupLayout*)&bindGroupLayout;
  WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(webgpuRes.device_, &layoutDesc);

  // Assign the PipelineLayout to the RenderPipelineDescriptor's layout field
  pipelineDesc.layout = layout;
  WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(webgpuRes.device_, &pipelineDesc);
  wgpuShaderModuleRelease(shaderModule);

  // Now we use emplace, because we know the component doesn't exist yet.
  GpuMeshComponent gpuMeshComponent;
  gpuMeshComponent.vertex_buffer_ = vertexBuffer;
  gpuMeshComponent.index_buffer_ = indexBuffer;
  gpuMeshComponent.index_count_ = (unsigned int)mesh.indices_.size();
  gpuMeshComponent.vertex_buffer_layout_ = vertexBufferLayout;
  gpuMeshComponent.layout_ = layout;
  gpuMeshComponent.bind_group_layout_ = bindGroupLayout;
  gpuMeshComponent.pipeline_ = pipeline;

  // Create per-entity uniform buffer and bind group (persist across frames)
  WGPUBufferDescriptor uniformDesc = {};
  uniformDesc.nextInChain = nullptr;
  uniformDesc.label = ToWgpuStringView("Per-entity uniform buffer");
  uniformDesc.size = sizeof(BPUniforms);
  uniformDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  gpuMeshComponent.uniform_buffer_ = wgpuDeviceCreateBuffer(webgpuRes.device_, &uniformDesc);

  WGPUBindGroupEntry bgEntry = {};
  bgEntry.binding = 0;
  bgEntry.buffer = gpuMeshComponent.uniform_buffer_;
  bgEntry.offset = 0;
  bgEntry.size = sizeof(BPUniforms);

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.nextInChain = nullptr;
  bgDesc.layout = bindGroupLayout;
  bgDesc.entryCount = 1;
  bgDesc.entries = &bgEntry;
  gpuMeshComponent.bind_group_ = wgpuDeviceCreateBindGroup(webgpuRes.device_, &bgDesc);

  entity.set<GpuMeshComponent>(gpuMeshComponent);
}

// ============================================================================
// PreparePhase Systems
// ============================================================================

// PrepareSurfaceSystem: Manage main window surface resources
void RenderSystems::PrepareSurfaceImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    return;
  }

  if (!world.has<vivid::window::WindowContext>()) {
    return;
  }

  auto& webgpuRes = world.get_mut<WebGPUContext>();
  const auto& windowContext = world.get<vivid::window::WindowContext>();

  // Validate WebGPU state - must be initialized before preparing surface
  if (!webgpuRes.device_ || !webgpuRes.initialized_ || !webgpuRes.surface_
      || webgpuRes.surface_format_ == WGPUTextureFormat_Undefined) {
    webgpuRes.target_view_ = nullptr;
    return;
  }

  // Note: targetView from previous frame should have been released in submitMainRenderPassImpl
  // If it still exists here, it means the previous frame was skipped, so release it now
  if (webgpuRes.target_view_) {
    wgpuTextureViewRelease(webgpuRes.target_view_);
    webgpuRes.target_view_ = nullptr;
  }

  // Check current window pixel size and reconfigure if changed or zero
  if (windowContext.pixel_width_ <= 0 || windowContext.pixel_height_ <= 0) {
    // Minimized or not ready; mark as invalid
    webgpuRes.target_view_ = nullptr;
    return;
  }

  // Reconfigure surface if dimensions changed
  if (webgpuRes.configured_width_ != static_cast<uint32_t>(windowContext.pixel_width_)
      || webgpuRes.configured_height_ != static_cast<uint32_t>(windowContext.pixel_height_)) {
    reconfigureSurface(webgpuRes, static_cast<uint32_t>(windowContext.pixel_width_),
                       static_cast<uint32_t>(windowContext.pixel_height_));
    // Skip this frame after reconfiguration
    webgpuRes.target_view_ = nullptr;
    return;
  }

  // Get the next target texture_ view
  wgpuSurfaceGetCurrentTexture(webgpuRes.surface_, &webgpuRes.surface_texture_);

  if (webgpuRes.surface_texture_.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
      && webgpuRes.surface_texture_.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
#if defined(WGPUSurfaceGetCurrentTextureStatus_Success)
      && webgpuRes.surface_texture_.status != WGPUSurfaceGetCurrentTextureStatus_Success
#endif
  ) {
    VividLogger::render_error("Surface texture_ status error: %d",
                              webgpuRes.surface_texture_.status);
    if (webgpuRes.surface_texture_.status == WGPUSurfaceGetCurrentTextureStatus_Outdated
        || webgpuRes.surface_texture_.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
      // Reconfigure on outdated/lost
      reconfigureSurface(webgpuRes, static_cast<uint32_t>(windowContext.pixel_width_),
                         static_cast<uint32_t>(windowContext.pixel_height_));
    }
    // Skip this frame for any non-success status
    webgpuRes.target_view_ = nullptr;
    return;
  }

  // Create texture_ view for surface
  WGPUTextureViewDescriptor viewDescriptor = {};
  viewDescriptor.nextInChain = nullptr;
  viewDescriptor.label = ToWgpuStringView("Surface texture_ view");
  viewDescriptor.format = wgpuTextureGetFormat(webgpuRes.surface_texture_.texture);
  viewDescriptor.dimension = WGPUTextureViewDimension_2D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = WGPUTextureAspect_All;
  // View usage must be compatible with the surface texture_'s usage (RENDER_ATTACHMENT)
  viewDescriptor.usage = WGPUTextureUsage_RenderAttachment;
  webgpuRes.target_view_
      = wgpuTextureCreateView(webgpuRes.surface_texture_.texture, &viewDescriptor);

  if (webgpuRes.target_view_ == nullptr) {
    VividLogger::render_error("Failed to create surface texture_ view");
  }
}

// PrepareViewportResourcesSystem: Manage viewport offscreen texture_ resources
void RenderSystems::PrepareViewportResourcesImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    return;
  }

  const auto& webgpuRes = world.get<WebGPUContext>();

  // Process pending resource releases (called once per frame)
  ProcessPendingReleases();

  // Validate WebGPU state
  if (!webgpuRes.device_ || !webgpuRes.queue_ || !webgpuRes.initialized_
      || webgpuRes.surface_format_ == WGPUTextureFormat_Undefined) {
    return;
  }

  // Query all viewports and ensure their resources are up-to-date
  auto viewportQuery = world.query<ViewportComponent>();

  viewportQuery.each([&](flecs::entity entity, ViewportComponent& viewport) {
    uint32_t width = static_cast<uint32_t>(viewport.width_);
    uint32_t height = static_cast<uint32_t>(viewport.height_);
    if (width == 0 || height == 0) {
      return;
    }

    // Ensure viewport resources (textures) are created and up-to-date
    ensureViewportResources(viewport, webgpuRes, width, height);
  });
}

// ============================================================================
// RenderPhase Systems
// ============================================================================

// BeginMainRenderPassSystem: Create render pass for main window (only when no viewport exists)
void RenderSystems::BeginMainRenderPassImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    return;
  }

  auto& webgpuRes = world.get_mut<WebGPUContext>();

  // Validate WebGPU state
  if (!webgpuRes.device_ || !webgpuRes.queue_ || !webgpuRes.initialized_ || !webgpuRes.surface_
      || webgpuRes.surface_format_ == WGPUTextureFormat_Undefined
      || webgpuRes.configured_width_ == 0 || webgpuRes.configured_height_ == 0) {
    webgpuRes.render_pass_ = nullptr;
    webgpuRes.encoder_ = nullptr;
    return;
  }

  // Check if any viewport exists with ready resources - if yes, skip main window rendering
  auto viewportCheck = world.query<ViewportComponent>();
  bool hasReadyViewport = false;
  viewportCheck.each([&](flecs::entity, const ViewportComponent& viewport) {
    if (viewport.width_ > 0 && viewport.height_ > 0 && viewport.render_texture_view_ != nullptr
        && viewport.depth_view_ != nullptr) {
      hasReadyViewport = true;
    }
  });

  // Always create main window render pass for ImGui rendering, even if viewports exist
  // Scene rendering will be skipped if viewports exist, but ImGui still needs the render pass
  if (!webgpuRes.target_view_ || !webgpuRes.depth_view_) {
    webgpuRes.render_pass_ = nullptr;
    webgpuRes.encoder_ = nullptr;
    return;
  }

  // Create Command Encoder
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = ToWgpuStringView("Main window command encoder");
  webgpuRes.encoder_ = wgpuDeviceCreateCommandEncoder(webgpuRes.device_, &encoderDesc);

  if (webgpuRes.encoder_ == nullptr) {
    VividLogger::app_error("Failed to create main window command encoder");
    return;
  }

  // Create Render Pass
  WGPURenderPassColorAttachment renderPassColorAttachment = {};
  renderPassColorAttachment.view = webgpuRes.target_view_;
  renderPassColorAttachment.resolveTarget = nullptr;
  renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
  renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
  renderPassColorAttachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};
  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

  WGPURenderPassDescriptor renderPassDesc = {};
  renderPassDesc.nextInChain = nullptr;
  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &renderPassColorAttachment;
  WGPURenderPassDepthStencilAttachment depthAttach = {};
  depthAttach.view = webgpuRes.depth_view_;
  depthAttach.depthClearValue = 1.0f;
  depthAttach.depthLoadOp = WGPULoadOp_Clear;
  depthAttach.depthStoreOp = WGPUStoreOp_Store;
  depthAttach.depthReadOnly = false;
  depthAttach.stencilReadOnly = true;
  renderPassDesc.depthStencilAttachment = &depthAttach;
  renderPassDesc.timestampWrites = nullptr;

  webgpuRes.render_pass_ = wgpuCommandEncoderBeginRenderPass(webgpuRes.encoder_, &renderPassDesc);

  if (webgpuRes.render_pass_ == nullptr) {
    VividLogger::app_error("Failed to begin main window render pass");
    wgpuCommandEncoderRelease(webgpuRes.encoder_);
    webgpuRes.encoder_ = nullptr;
  }
}

// BeginViewportRenderPassSystem: Render each viewport independently and atomically
// Each viewport: create encoder -> begin render pass -> render scene -> end pass -> submit
// This prevents uniform buffer corruption by ensuring each viewport completes before the next
// starts
void RenderSystems::BeginViewportRenderPassImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    return;
  }

  const auto& webgpuRes = world.get<WebGPUContext>();

  // Validate WebGPU state
  if (!webgpuRes.device_ || !webgpuRes.queue_ || !webgpuRes.initialized_ || !webgpuRes.surface_
      || webgpuRes.surface_format_ == WGPUTextureFormat_Undefined
      || webgpuRes.configured_width_ == 0 || webgpuRes.configured_height_ == 0) {
    return;
  }

  // Query and create render pass for each viewport
  auto viewportQuery = world.query<CameraComponent, ViewportComponent, TransformComponent>();

  viewportQuery.each([&](flecs::entity entity, const CameraComponent& camera,
                         ViewportComponent& viewport, const TransformComponent& transform) {
    uint32_t width = static_cast<uint32_t>(viewport.width_);
    uint32_t height = static_cast<uint32_t>(viewport.height_);
    if (width == 0 || height == 0) {
      return;
    }

    // Ensure viewport resources are ready
    if (!viewport.render_texture_view_ || !viewport.depth_view_) {
      return;
    }

    // Create independent command encoder for this viewport
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = ToWgpuStringView("Viewport command encoder");

    WGPUCommandEncoder viewportEncoder
        = wgpuDeviceCreateCommandEncoder(webgpuRes.device_, &encoderDesc);

    if (viewportEncoder == nullptr) {
      VividLogger::app_error("Failed to create viewport command encoder");
      return;
    }

    // Create render pass for this viewport
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = viewport.render_texture_view_;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = WGPUColor{0.1, 0.1, 0.1, 1.0};
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.view = viewport.depth_view_;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthAttachment.depthReadOnly = false;
    depthAttachment.stencilReadOnly = true;

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = &depthAttachment;
    renderPassDesc.timestampWrites = nullptr;

    WGPURenderPassEncoder viewportPass
        = wgpuCommandEncoderBeginRenderPass(viewportEncoder, &renderPassDesc);

    if (viewportPass == nullptr) {
      VividLogger::app_error("Failed to begin viewport render pass");
      wgpuCommandEncoderRelease(viewportEncoder);
      return;
    }

    // Query scene context for this viewport (only for light info, not camera)
    SceneRenderContext scene_ctx = querySceneContext(world, width, height);

    // Override with viewport-specific camera (must recalculate view and projection)
    scene_ctx.view_pos_ = transform.position_;

    // Calculate view matrix from this viewport's camera
    if (entity.has<CameraControllerComponent>()) {
      const auto& controller = entity.get<CameraControllerComponent>();
      glm::vec3 target = transform.position_ + controller.front_;
      scene_ctx.view_matrix_ = glm::lookAt(transform.position_, target, controller.up_);
    } else {
      scene_ctx.view_matrix_ = glm::lookAt(
          transform.position_, transform.position_ + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    }

    // Calculate projection matrix based on this viewport's dimensions
    // Each viewport must have its own projection matrix matching its aspect ratio
    if (camera.projection_matrix_ != glm::mat4(1.0f)) {
      scene_ctx.projection_matrix_ = camera.projection_matrix_;
    } else if (height > 0) {
      float aspect = static_cast<float>(width) / static_cast<float>(height);
      scene_ctx.projection_matrix_ = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    // Render and submit this viewport immediately to prevent uniform buffer corruption
    // wgpuQueueWriteBuffer writes uniforms immediately, so we must submit each viewport atomically
    RenderTarget viewport_target = {viewportPass, width, height};
    renderSceneToTarget(world, viewport_target, scene_ctx, webgpuRes);

    // End render pass immediately after rendering
    wgpuRenderPassEncoderEnd(viewportPass);
    wgpuRenderPassEncoderRelease(viewportPass);

    // Finish and submit command encoder immediately
    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    cmd_buffer_desc.nextInChain = nullptr;
    cmd_buffer_desc.label = ToWgpuStringView("Viewport command buffer");
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(viewportEncoder, &cmd_buffer_desc);

    if (command != nullptr) {
      wgpuCommandEncoderRelease(viewportEncoder);

      // Submit immediately - ensures uniform updates are executed before next viewport
      if (webgpuRes.queue_ != nullptr) {
        wgpuQueueSubmit(webgpuRes.queue_, 1, &command);
        wgpuCommandBufferRelease(command);
      } else {
        VividLogger::app_error("WebGPU queue is null, cannot submit viewport command");
        wgpuCommandBufferRelease(command);
      }
    } else {
      VividLogger::app_error("Failed to finish viewport command encoder");
      wgpuCommandEncoderRelease(viewportEncoder);
    }
  });
}

// RenderSceneSystem: Render scene to main window only (viewports handled in
// beginViewportRenderPass)
void RenderSystems::RenderSceneImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    return;
  }

  const auto& webgpu_res = world.get<WebGPUContext>();

  // Check if any viewport exists with ready resources
  auto viewport_check = world.query<ViewportComponent>();
  bool has_ready_viewport = false;
  viewport_check.each([&](flecs::entity, const ViewportComponent& viewport) {
    if (viewport.width_ > 0 && viewport.height_ > 0 && viewport.render_texture_view_ != nullptr
        && viewport.depth_view_ != nullptr) {
      has_ready_viewport = true;
    }
  });

  // Render to main window only if no viewports exist
  // Main window renderPass is always created (for ImGui), but scene rendering is conditional
  if (!has_ready_viewport && webgpu_res.render_pass_ != nullptr) {
    SceneRenderContext const kSceneCtx
        = querySceneContext(world, webgpu_res.configured_width_, webgpu_res.configured_height_);

    RenderTarget const kMainTarget
        = {webgpu_res.render_pass_, webgpu_res.configured_width_, webgpu_res.configured_height_};
    renderSceneToTarget(world, kMainTarget, kSceneCtx, webgpu_res);
  }
}

// EndViewportRenderPassSystem: No-op (viewports are rendered and submitted in
// beginViewportRenderPass)
void RenderSystems::EndViewportRenderPassImpl(const flecs::iter& it) {
  // Viewports are rendered and submitted atomically in beginViewportRenderPassImpl
  // to prevent uniform buffer corruption, so this system is now a no-op.
}

// ============================================================================
// SubmitPhase Systems
// ============================================================================

// SubmitMainRenderPassSystem: Submit main window command buffer and present
void RenderSystems::SubmitMainRenderPassImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }

  auto& webgpu_res = world.get_mut<WebGPUContext>();

  // Skip if no render pass was created this frame (e.g_., viewports exist or window resize)
  if (webgpu_res.render_pass_ == nullptr) {
    return;
  }

  // End render pass
  wgpuRenderPassEncoderEnd(webgpu_res.render_pass_);
  wgpuRenderPassEncoderRelease(webgpu_res.render_pass_);
  webgpu_res.render_pass_ = nullptr;

  // Finish encoding and submit
  if (webgpu_res.encoder_ == nullptr) {
    return;
  }

  WGPUCommandBufferDescriptor cmd_buffer_descriptor = {};
  cmd_buffer_descriptor.nextInChain = nullptr;
  cmd_buffer_descriptor.label = ToWgpuStringView("Command buffer");
  WGPUCommandBuffer command = wgpuCommandEncoderFinish(webgpu_res.encoder_, &cmd_buffer_descriptor);
  wgpuCommandEncoderRelease(webgpu_res.encoder_);
  webgpu_res.encoder_ = nullptr;

  // Submit the command queue
  if (command != nullptr && webgpu_res.queue_ != nullptr) {
    wgpuQueueSubmit(webgpu_res.queue_, 1, &command);
    wgpuCommandBufferRelease(command);
  } else {
    if (command != nullptr) {
      wgpuCommandBufferRelease(command);
    }
    VividLogger::app_error("Failed to submit main window command buffer");
    return;
  }

  // Present the surface onto the window
  if (webgpu_res.target_view_ != nullptr) {
    wgpuTextureViewRelease(webgpu_res.target_view_);
    webgpu_res.target_view_ = nullptr;
  }

#ifndef WEBGPU_BACKEND_WGPU
  // We no longer need the texture_, only its view
  // (NB: with wgpu-native, surface textures must be release after the call to wgpuSurfacePresent)
  if (webgpu_res.surface_texture_.texture != nullptr) {
    wgpuTextureRelease(webgpu_res.surface_texture_.texture);
  }
#endif  // WEBGPU_BACKEND_WGPU

  // In the context of a Web browser, we do not present the surface texture_ ourselves. We rather
  // rely on emscripten_set_main_loop_arg (a.k.a. requestAnimationFrame in JavaScript) to call our
  // MainLoop() function right before presenting.
#ifndef __EMSCRIPTEN__
  if (webgpu_res.surface_ != nullptr) {
    wgpuSurfacePresent(webgpu_res.surface_);
  }
#  if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
  if (webgpu_res.device_ != nullptr) {
    wgpuDeviceTick(webgpu_res.device_);
  }
#  endif
#endif

#ifdef WEBGPU_BACKEND_WGPU
  if (webgpu_res.surface_texture_.texture != nullptr) {
    wgpuTextureRelease(webgpu_res.surface_texture_.texture);
  }
#endif
}

void RenderSystems::CreatePipelineImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Creating WebGPU pipeline...");
  auto& webgpu_res = world.get_mut<WebGPUContext>();
  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }

  const char* shader_source = R"(
      @vertex
      fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
          if (in_vertex_index == 0u) {
              return vec4f(-0.45, 0.5, 0.0, 1.0);
          } else if (in_vertex_index == 1u) {
              return vec4f(0.45, 0.5, 0.0, 1.0);
          } else {
              return vec4f(0.0, -0.5, 0.0, 1.0);
          }
      }

      @fragment
      fn fs_main() -> @location(0) vec4f {
          return vec4f(0.0, 0.4, 0.7, 1.0);
      }
    )";

  // create the shader module
  WGPUShaderSourceWGSL wgsl_desc = {};
  wgsl_desc.chain.next = nullptr;
  wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgsl_desc.code = ToWgpuStringView(shader_source);
  WGPUShaderModuleDescriptor shader_desc = {};
  shader_desc.nextInChain = &wgsl_desc.chain;  // connect the chained extension
  shader_desc.label = ToWgpuStringView("Shader source");
  WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(webgpu_res.device_, &shader_desc);

  // Describe render pipeline
  WGPURenderPipelineDescriptor pipeline_desc = {};
  pipeline_desc.vertex.module = shader_module;
  pipeline_desc.vertex.entryPoint = ToWgpuStringView("vs_main");

  // const char *fragmentShaderSource = R"(
  //   @fragment
  //   fn fs_main() -> @location(0) vec4f {
  //       return vec4f(0.0, 0.4, 0.7, 1.0);
  //   }
  // )";

  WGPUFragmentState fragment_state = {};
  fragment_state.module = shader_module;
  fragment_state.entryPoint = ToWgpuStringView("fs_main");
  pipeline_desc.fragment = &fragment_state;

  WGPUColorTargetState color_target = {};
  color_target.format = webgpu_res.surface_format_;
  color_target.writeMask = WGPUColorWriteMask_All;
  WGPUBlendState blend_state = {};
  blend_state.color.srcFactor = WGPUBlendFactor_One;
  blend_state.color.dstFactor = WGPUBlendFactor_Zero;
  blend_state.color.operation = WGPUBlendOperation_Add;
  blend_state.alpha.srcFactor = WGPUBlendFactor_One;
  blend_state.alpha.dstFactor = WGPUBlendFactor_Zero;
  blend_state.alpha.operation = WGPUBlendOperation_Add;
  color_target.blend = &blend_state;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;

  // Primitive state
  WGPUPrimitiveState primitive = {};
  primitive.topology = WGPUPrimitiveTopology_TriangleList;
  primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  primitive.frontFace = WGPUFrontFace_CCW;
  primitive.cullMode = WGPUCullMode_None;
  pipeline_desc.primitive = primitive;

  // Multisample state
  WGPUMultisampleState multisample = {};
  multisample.count = 1;
  multisample.mask = 0xFFFFFFFF;
  multisample.alphaToCoverageEnabled = false;
  pipeline_desc.multisample = multisample;

  webgpu_res.pipeline_ = wgpuDeviceCreateRenderPipeline(webgpu_res.device_, &pipeline_desc);

  // @compute @workgroup_size(32)
  // fn computeStuff(@builtin(global_invocation_id) threadId: vec3u) {
  //     // [...]
  // }
}

void RenderSystems::ReleaseWebGPUResourcesImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Releasing WebGPU resources...");

  // Release all pending viewport resources immediately on shutdown
  for (auto& pending : pending_releases) {
    if (pending.texture_view_ != nullptr) {
      wgpuTextureViewRelease(pending.texture_view_);
    }
    if (pending.texture_ != nullptr) {
      wgpuTextureRelease(pending.texture_);
    }
  }
  pending_releases.clear();

  // Release viewport resources
  {
    auto viewport_query = world.query<ViewportComponent>();
    viewport_query.each([&](flecs::entity entity, ViewportComponent& viewport) {
      if (viewport.depth_view_) {
        wgpuTextureViewRelease(viewport.depth_view_);
        viewport.depth_view_ = nullptr;
      }
      if (viewport.depth_texture_) {
        wgpuTextureRelease(viewport.depth_texture_);
        viewport.depth_texture_ = nullptr;
      }
      if (viewport.render_texture_view_) {
        wgpuTextureViewRelease(viewport.render_texture_view_);
        viewport.render_texture_view_ = nullptr;
      }
      if (viewport.render_texture_) {
        wgpuTextureRelease(viewport.render_texture_);
        viewport.render_texture_ = nullptr;
      }
      viewport = ViewportComponent{};  // Reset to default state
    });
  }

  // Release all per-entity GPU resources
  {
    auto query = world.query<GpuMeshComponent>();
    query.each([&](flecs::entity entity, GpuMeshComponent& gpu) {
      if (gpu.bind_group_) {
        wgpuBindGroupRelease(gpu.bind_group_);
        gpu.bind_group_ = nullptr;
      }
      if (gpu.uniform_buffer_) {
        wgpuBufferRelease(gpu.uniform_buffer_);
        gpu.uniform_buffer_ = nullptr;
      }
      if (gpu.vertex_buffer_) {
        wgpuBufferRelease(gpu.vertex_buffer_);
        gpu.vertex_buffer_ = nullptr;
      }
      if (gpu.index_buffer_) {
        wgpuBufferRelease(gpu.index_buffer_);
        gpu.index_buffer_ = nullptr;
      }
      if (gpu.pipeline_) {
        wgpuRenderPipelineRelease(gpu.pipeline_);
        gpu.pipeline_ = nullptr;
      }
      if (gpu.layout_) {
        wgpuPipelineLayoutRelease(gpu.layout_);
        gpu.layout_ = nullptr;
      }
      if (gpu.bind_group_layout_) {
        wgpuBindGroupLayoutRelease(gpu.bind_group_layout_);
        gpu.bind_group_layout_ = nullptr;
      }
      // Remove component from entity
      entity.remove<GpuMeshComponent>();
    });
  }

  auto& webgpu_res = world.get_mut<WebGPUContext>();
  if (world.has<WebGPUContext>()) {
    if (webgpu_res.depth_view_ != nullptr) {
      wgpuTextureViewRelease(webgpu_res.depth_view_);
      webgpu_res.depth_view_ = nullptr;
    }
    if (webgpu_res.depth_texture_ != nullptr) {
      wgpuTextureRelease(webgpu_res.depth_texture_);
      webgpu_res.depth_texture_ = nullptr;
    }
    wgpuSurfaceRelease(webgpu_res.surface_);
    // wgpuRenderPipelineRelease(webgpuRes.pipeline_);
    wgpuQueueRelease(webgpu_res.queue_);
    wgpuAdapterRelease(webgpu_res.adapter_);
    wgpuDeviceRelease(webgpu_res.device_);
    wgpuInstanceProcessEvents(webgpu_res.instance_);
    wgpuInstanceRelease(webgpu_res.instance_);
    VividLogger::app_info("WGPU instance, adapter, and device released");
    webgpu_res.queue_ = nullptr;
    webgpu_res.surface_ = nullptr;
    webgpu_res.pipeline_ = nullptr;
    webgpu_res.instance_ = nullptr;
    webgpu_res.adapter_ = nullptr;
    webgpu_res.device_ = nullptr;
    webgpu_res.adapter_request_ended_ = false;
    webgpu_res.device_request_ended_ = false;
  }
}

void RenderSystems::InitWebGPUImpl(const vivid::window::WindowContext& window_context,
                                   WebGPUContext& webgpu_res) {
  VividLogger::app_info("=== InitWebGPU system called ===");
  // VividLogger::app_info("Entity: %s (ID: %llu)", entity.name(), entity.id());
  VividLogger::app_info("Window handle: %p", window_context.window_handle_);
  VividLogger::app_info("WebGPU initialized flag: %d", static_cast<int>(webgpu_res.initialized_));

  // Check if already initialized (due to .each(), this may run on multiple window entities)
  if (webgpu_res.initialized_) {
    VividLogger::app_warn("WebGPU already initialized, skipping...");
    return;  // Already initialized, skip
  }

  VividLogger::app_info("Initializing WebGPU...");

  WGPUTextureFormat const kPreferredFmt
      = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities

  // Google DAWN backend: Adapter and Device acquisition, Surface creation
  wgpu::InstanceDescriptor instance_descriptor = {};
  static constexpr wgpu::InstanceFeatureName requiredInstanceFeatures[] = {
      wgpu::InstanceFeatureName::TimedWaitAny,
  };
  instance_descriptor.requiredFeatureCount = std::size(requiredInstanceFeatures);
  instance_descriptor.requiredFeatures = requiredInstanceFeatures;
  wgpu::Instance instance = wgpu::CreateInstance(&instance_descriptor);

  // We can check whether there is actually an instance created
  if (!instance) {
    VividLogger::app_error("Could not initialize WebGPU!");
    return;
  }

  // Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  VividLogger::app_info("WGPU instance: %p", &instance);

  wgpu::Adapter adapter{getAdapter(instance)};  // RequestWebGPUAdapterSync
  webgpu_res.device_ = getDevice(instance, adapter);

  if (webgpu_res.device_ == nullptr) {
    VividLogger::app_error("Failed to acquire WebGPU device");
    return;
  }

  // Create the surface.
#ifdef __EMSCRIPTEN__
  wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
  canvasDesc.selector = "#canvas";

  wgpu::SurfaceDescriptor surfaceDesc = {};
  surfaceDesc.nextInChain = &canvasDesc;
  wgpu::Surface surface = instance.CreateSurface(&surfaceDesc);
  if (!surface) {
    VividLogger::app_error("Could not create WebGPU surface!");
    return;
  }
  webgpu_res.surface_ = surface.MoveToCHandle();
#endif

  // Set instance BEFORE accessing window (needed for surface creation on non-Emscripten)
  webgpu_res.instance_ = instance.MoveToCHandle();

  // Get window pixel size from the current entity's WindowGpuComponent
  int pixel_width = 0;
  int pixel_height = 0;

  if (window_context.window_handle_ != nullptr) {
#ifndef __EMSCRIPTEN__
    webgpu_res.surface_
        = ImGui_ImplSDL3_CreateWGPUSurface(webgpu_res.instance_, window_context.window_handle_);
#endif

    // TODO(zhaoy): move this to the window systems
    SDL_GetWindowSizeInPixels(window_context.window_handle_, &pixel_width, &pixel_height);
    // VividLogger::app_info("Using window: entity=%llu, handle=%p, size=%dx%d", entity.id(),
    //                       windowContext.window_handle, pixel_width, pixel_height);
  }

  // Guard against zero-sized surfaces (e.g_., minimized window); fall back to a small valid size
  if (pixel_width <= 0 || pixel_height <= 0) {
    VividLogger::app_warn(
        "Window pixel size is %dx%d; using fallback size for surface configuration", pixel_width,
        pixel_height);
    pixel_width = 1;
    pixel_height = 1;
  }
  if (webgpu_res.surface_ == nullptr) {
    VividLogger::app_error("Could not create WebGPU surface!");
    return;
  }

  webgpu_res.instance_ = instance.MoveToCHandle();

  WGPUSurfaceCapabilities surface_capabilities = {};
  wgpuSurfaceGetCapabilities(webgpu_res.surface_, adapter.Get(), &surface_capabilities);

  // preferred_fmt = surface_capabilities.formats[0];
  webgpu_res.surface_format_ = surface_capabilities.formats[0];

  webgpu_res.adapter_ = adapter.MoveToCHandle();
  // webgpuRes.device_ = device.MoveToCHandle();
  if (webgpu_res.adapter_ == nullptr) {
    VividLogger::app_error("WebGPU device handle is null");
    return;
  }

  webgpu_res.surface_configuration_.presentMode = WGPUPresentMode_Fifo;
  webgpu_res.surface_configuration_.alphaMode = WGPUCompositeAlphaMode_Auto;
  webgpu_res.surface_configuration_.usage = WGPUTextureUsage_RenderAttachment;

  webgpu_res.surface_configuration_.width = webgpu_res.configured_width_ = pixel_width;
  webgpu_res.surface_configuration_.height = webgpu_res.configured_height_ = pixel_height;
  webgpu_res.surface_configuration_.device = webgpu_res.device_;
  webgpu_res.surface_configuration_.format = webgpu_res.surface_format_;

  wgpuSurfaceConfigure(webgpu_res.surface_, &webgpu_res.surface_configuration_);
  webgpu_res.queue_ = wgpuDeviceGetQueue(webgpu_res.device_);
  if (webgpu_res.queue_ == nullptr) {
    VividLogger::app_error("Failed to acquire WebGPU device queue");
    return;
  }
  // reconfigureSurface(entity.world(), pixel_width, pixel_height);
  reconfigureSurface(webgpu_res, pixel_width, pixel_height);

  // Mark as initialized to prevent re-initialization
  webgpu_res.initialized_ = true;
  VividLogger::app_info("WebGPU initialized successfully");
}
}  // namespace vivid::render
