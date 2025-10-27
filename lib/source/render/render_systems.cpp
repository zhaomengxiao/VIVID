#include "vivid/render/render_systems.h"

#include <SDL3/SDL.h>
// #define __EMSCRIPTEN__

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

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

// Utility functions
std::string_view toStdStringView(WGPUStringView wgpuStringView) {
  return wgpuStringView.data == nullptr ? std::string_view()
         : wgpuStringView.length == WGPU_STRLEN
             ? std::string_view(wgpuStringView.data)
             : std::string_view(wgpuStringView.data, wgpuStringView.length);
}

WGPUStringView toWgpuStringView(std::string_view stdStringView) {
  return {stdStringView.data(), stdStringView.size()};
}
WGPUStringView toWgpuStringView(const char* cString) { return {cString, WGPU_STRLEN}; }

void sleepForMilliseconds(unsigned int milliseconds) {
#ifdef __EMSCRIPTEN__
  emscripten_sleep(milliseconds);
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

/**
 * Fetch data from a GPU buffer back to the CPU.
 * This function blocks until the data is available on CPU, then calls the
 * `processBufferData` callback, and finally unmap the buffer.
 */
void fetchBufferDataSync(WGPUInstance instance, WGPUBuffer buffer, size_t bufferSize,
                         std::function<void(const void*)> processBufferData) {
  // Read the data back from buffer B
  // Context passed to `onBufferBMapped` through theuserdata pointer:
  struct OnBufferBMappedContext {
    bool operationEnded = false;       // Turned true as soon as the callback is invoked
    bool mappingIsSuccessful = false;  // Turned true only if mapping succeeded
  };

  // This function has the type WGPUBufferMapCallback as defined in webgpu.h
  auto onBufferMapped = [](WGPUMapAsyncStatus status, struct WGPUStringView message,
                           void* userdata1, void* /* userdata2 */
                        ) {
    OnBufferBMappedContext& context = *reinterpret_cast<OnBufferBMappedContext*>(userdata1);
    context.operationEnded = true;
    if (status == WGPUMapAsyncStatus_Success) {
      context.mappingIsSuccessful = true;
    } else {
      std::cout << "Could not map buffer B! Status: " << status
                << ", message: " << toStdStringView(message) << std::endl;
    }
  };

  // We create an instance of the context shared with `onBufferBMapped`
  OnBufferBMappedContext context;

  // And we build the callback info:
  WGPUBufferMapCallbackInfo bufferMapCallbackInfo = {};
  bufferMapCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
  bufferMapCallbackInfo.callback = onBufferMapped;
  bufferMapCallbackInfo.userdata1 = &context;

  // And finally we launch the asynchronous operation
  wgpuBufferMapAsync(buffer, WGPUMapMode_Read,
                     0,  // offset
                     bufferSize, bufferMapCallbackInfo);

  // Process events until the map operation ended
  wgpuInstanceProcessEvents(instance);
  while (!context.operationEnded) {
    sleepForMilliseconds(200);
    wgpuInstanceProcessEvents(instance);
  }

  if (context.mappingIsSuccessful) {
    const void* bufferData = wgpuBufferGetConstMappedRange(buffer, 0, bufferSize);
    processBufferData(bufferData);
  }
}

// Components

namespace VIVID::RENDER {

// Internal structs
// Uniforms for Blinn-Phong shading. Layout is 16-byte aligned for WGSL std140-like rules.
struct BPUniforms {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 normalMatrix;              // store as mat4 for alignment; use upper-left 3x3 in shader
  std::array<float, 4> viewPos;        // xyz + pad
  std::array<float, 4> lightPos;       // xyz + pad
  std::array<float, 4> objectColor;    // rgb + pad
  std::array<float, 4> lightColor;     // rgb + pad
  std::array<float, 4> ambientColor;   // rgb + pad
  std::array<float, 4> specularColor;  // rgb + pad
  std::array<float, 4> params;         // constant, linear, quadratic, shininess
};

// ============================================================================
// Helper Functions
// ============================================================================

static void reconfigureSurface(WebGPUContext& webgpuRes, uint32_t width, uint32_t height) {
  if (!webgpuRes.surface || !webgpuRes.device) {
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
  config.format = webgpuRes.surfaceFormat;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.device = webgpuRes.device;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;
  wgpuSurfaceConfigure(webgpuRes.surface, &config);

  webgpuRes.configuredWidth = width;
  webgpuRes.configuredHeight = height;

  // (Re)create depth resources matching the surface size
  if (webgpuRes.depthView) {
    wgpuTextureViewRelease(webgpuRes.depthView);
    webgpuRes.depthView = nullptr;
  }
  if (webgpuRes.depthTexture) {
    wgpuTextureRelease(webgpuRes.depthTexture);
    webgpuRes.depthTexture = nullptr;
  }

  WGPUTextureDescriptor depthDesc = {};
  depthDesc.nextInChain = nullptr;
  depthDesc.label = toWgpuStringView("Depth texture");
  depthDesc.usage = WGPUTextureUsage_RenderAttachment;
  depthDesc.dimension = WGPUTextureDimension_2D;
  depthDesc.size.width = width;
  depthDesc.size.height = height;
  depthDesc.size.depthOrArrayLayers = 1;
  depthDesc.format = webgpuRes.depthFormat;
  depthDesc.mipLevelCount = 1;
  depthDesc.sampleCount = 1;
  webgpuRes.depthTexture = wgpuDeviceCreateTexture(webgpuRes.device, &depthDesc);

  WGPUTextureViewDescriptor depthViewDesc = {};
  depthViewDesc.nextInChain = nullptr;
  depthViewDesc.label = toWgpuStringView("Depth texture view");
  depthViewDesc.format = webgpuRes.depthFormat;
  depthViewDesc.dimension = WGPUTextureViewDimension_2D;
  depthViewDesc.baseMipLevel = 0;
  depthViewDesc.mipLevelCount = 1;
  depthViewDesc.baseArrayLayer = 0;
  depthViewDesc.arrayLayerCount = 1;
  depthViewDesc.aspect = WGPUTextureAspect_All;
  webgpuRes.depthView = wgpuTextureCreateView(webgpuRes.depthTexture, &depthViewDesc);
}

static WGPUAdapter getAdapter(wgpu::Instance& instance) {
  wgpu::Adapter acquiredAdapter;
  wgpu::RequestAdapterOptions adapterOptions;

  auto onRequestAdapter
      = [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
          if (status != wgpu::RequestAdapterStatus::Success) {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
          }
          acquiredAdapter = std::move(adapter);  // FIXME-WGPU: no need to use std::move?
        };

  // Synchronously (wait until) acquire Adapter
  wgpu::Future waitAdapterFunc{
      instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter)};
  wgpu::WaitStatus waitStatusAdapter = instance.WaitAny(waitAdapterFunc, UINT64_MAX);
  VIVID_ASSERT(acquiredAdapter != nullptr && waitStatusAdapter == wgpu::WaitStatus::Success
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
  wgpu::Future waitDeviceFunc{
      adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice)};
  wgpu::WaitStatus waitStatusDevice = instance.WaitAny(waitDeviceFunc, UINT64_MAX);
  IM_ASSERT(acquiredDevice != nullptr && waitStatusDevice == wgpu::WaitStatus::Success
            && "Error on Device request");
  return acquiredDevice.MoveToCHandle();
}

// ============================================================================
// System Implementations (not registered, converted to Flecs format)
// ============================================================================

void RenderSystems::createWebGPUInstanceImpl(const flecs::iter& it) {
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
  webgpu_res.instance = instance;

  // Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  VividLogger::app_info("WGPU instance: %p", instance);
}

void RenderSystems::requestWebGPUAdapterSyncImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Requesting WebGPU adapter...");

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get_mut<WebGPUContext>();

  // A simple structure holding the local information shared with the
  // onAdapterRequestEnded callback.
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

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
                                                        void* userdata1, void* userdata2) {
    UserData& userData = *reinterpret_cast<UserData*>(userdata1);
    if (status == WGPURequestAdapterStatus_Success) {
      userData.adapter = adapter;
    } else {
      VividLogger::app_error("Could not get WebGPU adapter: %s", toStdStringView(message).data());
    }
    userData.requestEnded = true;
  };

  WGPURequestAdapterCallbackInfo callbackInfo = {
      nullptr, WGPUCallbackMode_AllowProcessEvents, onAdapterRequestEnded, (void*)&userData,
      nullptr,
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(webgpuRes.instance /* equivalent of navigator.gpu */, &adapterOpts,
                             callbackInfo);

  // We wait until userData.requestEnded gets true

  // Hand the execution to the WebGPU instance so that it can check for
  // pending async operations, in which case it invokes our callbacks.
  // NB: We test once before the loop not to wait for 200ms in case it is
  // already ready
  wgpuInstanceProcessEvents(webgpuRes.instance);

  while (!userData.requestEnded) {
    // Waiting for 200 ms to avoid asking too often to process events
    sleepForMilliseconds(200);

    wgpuInstanceProcessEvents(webgpuRes.instance);
  }

  VIVID_ASSERT(userData.requestEnded);

  auto& webgpuRes_mut = world.get_mut<WebGPUContext>();
  webgpuRes_mut.adapter = userData.adapter;
  webgpuRes_mut.adapterRequestEnded = userData.requestEnded;

  VividLogger::app_info("Got WebGPU adapter");
}

void RenderSystems::inspectWebGPUAdapterImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Inspecting WebGPU adapter...");

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get<WebGPUContext>();

#ifndef __EMSCRIPTEN__
  WGPULimits supportedLimits = {};
  supportedLimits.nextInChain = nullptr;

#  ifdef WEBGPU_BACKEND_DAWN
  bool success = wgpuAdapterGetLimits(webgpuRes.adapter, &supportedLimits) == WGPUStatus_Success;
#  else
  bool success = wgpuAdapterGetLimits(webgpuRes.adapter, &supportedLimits);
#  endif

  if (success) {
    VividLogger::app_info("Adapter limits:");
    VividLogger::app_info(" - maxTextureDimension1D: %u", supportedLimits.maxTextureDimension1D);
    VividLogger::app_info(" - maxTextureDimension2D: %u", supportedLimits.maxTextureDimension2D);
    VividLogger::app_info(" - maxTextureDimension3D: %u", supportedLimits.maxTextureDimension3D);
    VividLogger::app_info(" - maxTextureArrayLayers: %u", supportedLimits.maxTextureArrayLayers);
  }
#endif  // NOT __EMSCRIPTEN__

  // Features

  WGPUSupportedFeatures supportedFeatures = {};

  // Call the function a first time with a null return address, just to get
  // the entry count.
  wgpuAdapterGetFeatures(webgpuRes.adapter, &supportedFeatures);

  std::cout << "Adapter features:" << std::endl;
  std::cout << std::hex;  // Write integers as hexadecimal to ease comparison with webgpu.h literals
  for (size_t i = 0; i < supportedFeatures.featureCount; ++i) {
    std::cout << " - 0x" << supportedFeatures.features[i] << std::endl;
  }
  std::cout << std::dec;  // Restore decimal numbers

  // Free the memory that had potentially been allocated by wgpuAdapterGetFeatures()
  wgpuSupportedFeaturesFreeMembers(supportedFeatures);
  // One shall no longer use features beyond this line.

  // Properties
  WGPUAdapterInfo properties;
  properties.nextInChain = nullptr;
  wgpuAdapterGetInfo(webgpuRes.adapter, &properties);
  VividLogger::app_info("Adapter properties:");
  VividLogger::app_info(" - vendorID: %u", properties.vendorID);
  VividLogger::app_info(" - vendorName: %s", toStdStringView(properties.vendor).data());
  VividLogger::app_info(" - architecture: %s", toStdStringView(properties.architecture).data());
  VividLogger::app_info(" - deviceID: %u", properties.deviceID);
  VividLogger::app_info(" - name: %s", toStdStringView(properties.device).data());
  VividLogger::app_info(" - driverDescription: %s", toStdStringView(properties.description).data());
  VividLogger::app_info(" - adapterType: 0x%X", properties.adapterType);
  VividLogger::app_info(" - backendType: 0x%X", properties.backendType);
  wgpuAdapterInfoFreeMembers(properties);
}

void RenderSystems::requestWebGPUDeviceSyncImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Requesting WebGPU device...");
  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  // Any name works here, that's your call
  deviceDesc.label = toWgpuStringView("My Device");
  deviceDesc.requiredFeatureCount = 0;
  deviceDesc.requiredFeatures = nullptr;
  deviceDesc.requiredLimits = nullptr;
  deviceDesc.defaultQueue.label = toWgpuStringView("The Default Queue");

  auto onDeviceLost = [](WGPUDevice const* device, WGPUDeviceLostReason reason,
                         struct WGPUStringView message, void* /* userdata1 */, void* /* userdata2 */
                      ) {
    // All we do is display a message when the device is lost
    std::cout << "Device " << device << " was lost: reason " << reason << " ("
              << toStdStringView(message) << ")" << std::endl;
  };

  deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
  deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

  auto onDeviceError
      = [](WGPUDevice const* device, WGPUErrorType type, struct WGPUStringView message,
           void* /* userdata1 */, void* /* userdata2 */
        ) {
          std::cout << "Uncaptured error in device " << device << ": type " << type << " ("
                    << toStdStringView(message) << ")" << std::endl;
        };

  deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get_mut<WebGPUContext>();

  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  // The callback
  auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                 WGPUStringView message, void* userdata1, void* /* userdata2 */
                              ) {
    UserData& userData = *reinterpret_cast<UserData*>(userdata1);
    if (status == WGPURequestDeviceStatus_Success) {
      userData.device = device;
    } else {
      std::cerr << "Error while requesting device: " << toStdStringView(message) << std::endl;
    }
    userData.requestEnded = true;
  };

  // Build the callback info
  WGPURequestDeviceCallbackInfo callbackInfo = {/* nextInChain = */ nullptr,
                                                /* mode = */ WGPUCallbackMode_AllowProcessEvents,
                                                /* callback = */ onDeviceRequestEnded,
                                                /* userdata1 = */ &userData,
                                                /* userdata2 = */ nullptr};

  // Call to the WebGPU request adapter procedure
  wgpuAdapterRequestDevice(webgpuRes.adapter, &deviceDesc, callbackInfo);

  // Hand the execution to the WebGPU instance until the request ended
  wgpuInstanceProcessEvents(webgpuRes.instance);
  while (!userData.requestEnded) {
    sleepForMilliseconds(200);
    wgpuInstanceProcessEvents(webgpuRes.instance);
  }

  VIVID_ASSERT(userData.requestEnded);

  auto& webgpuRes_mut = world.get_mut<WebGPUContext>();
  webgpuRes_mut.device = userData.device;
  webgpuRes_mut.deviceRequestEnded = userData.requestEnded;
  // Set default queue for later write/submit operations
  webgpuRes_mut.queue = wgpuDeviceGetQueue(webgpuRes_mut.device);

  VividLogger::app_info("Got WebGPU device");
}

void RenderSystems::inspectWebGPUDeviceImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Inspecting WebGPU device...");

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get<WebGPUContext>();

  WGPUSupportedFeatures features = {};
  wgpuDeviceGetFeatures(webgpuRes.device, &features);
  std::cout << "Device features:" << std::endl;
  std::cout << std::hex;
  for (size_t i = 0; i < features.featureCount; ++i) {
    std::cout << " - 0x" << features.features[i] << std::endl;
  }
  std::cout << std::dec;
  wgpuSupportedFeaturesFreeMembers(features);

  WGPULimits limits = {};
  bool success = wgpuDeviceGetLimits(webgpuRes.device, &limits) == WGPUStatus_Success;

  if (success) {
    std::cout << "Device limits:" << std::endl;
    std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
    std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
    std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
    std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    std::cout << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
    std::cout << " - maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers
              << std::endl;
    std::cout << " - maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << std::endl;
    std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: "
              << limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
    std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: "
              << limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
    std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage
              << std::endl;
    std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
    std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage
              << std::endl;
    std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage
              << std::endl;
    std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage
              << std::endl;
    std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize
              << std::endl;
    std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize
              << std::endl;
    std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment
              << std::endl;
    std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment
              << std::endl;
    std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
    std::cout << " - maxBufferSize: " << limits.maxBufferSize << std::endl;
    std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
    std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride
              << std::endl;
    std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables
              << std::endl;
    std::cout << " - maxColorAttachments: " << limits.maxColorAttachments << std::endl;
    std::cout << " - maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample
              << std::endl;
    std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize
              << std::endl;
    std::cout << " - maxComputeInvocationsPerWorkgroup: "
              << limits.maxComputeInvocationsPerWorkgroup << std::endl;
    std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
    std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
    std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
    std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension
              << std::endl;
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

void RenderSystems::testCommandQueueImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Testing WebGPU command queue...");
  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get_mut<WebGPUContext>();
  // Get the queue
  WGPUQueue queue = wgpuDeviceGetQueue(webgpuRes.device);
  webgpuRes.queue = queue;
  // Create the command encoder
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.label = toWgpuStringView("My command encoder");

  // Create buffers
  // Create buffer A
  WGPUBufferDescriptor bufferDescA = {};
  bufferDescA.size = 256;
  // Buffer A is *written* on CPU, and used as *source* of a GPU-side copy
  bufferDescA.usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
  bufferDescA.label = toWgpuStringView("Buffer A");
  bufferDescA.mappedAtCreation = true;

  WGPUBuffer bufferA = wgpuDeviceCreateBuffer(webgpuRes.device, &bufferDescA);
  // Create buffer B
  // We build a second buffer, called B
  WGPUBufferDescriptor bufferDescB = {};
  bufferDescB.size = 32;
  // Buffer B is *read* on CPU, and used as *destination* of a GPU-side copy
  bufferDescB.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
  bufferDescB.label = toWgpuStringView("Buffer B");

  WGPUBuffer bufferB = wgpuDeviceCreateBuffer(webgpuRes.device, &bufferDescB);
  // Writhe initial data to buffer A
  // Get a pointer to the entire mapped buffer and interpret it as 8-bit unsigned integers
  uint8_t* bufferDataA
      = static_cast<uint8_t*>(wgpuBufferGetMappedRange(bufferA, 0, bufferDescA.size));

  // Write 0, 1, 2, 3, ... in bufferA
  for (size_t i = 0; i < 256; ++i) {
    bufferDataA[i] = static_cast<uint8_t>(i);
  }

  // see also wgpuBufferWriteMappedRange, wgpuQueueWriteBuffer

  wgpuBufferUnmap(bufferA);
  // Do NOT use bufferDataA beyond this point!

  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(webgpuRes.device, &encoderDesc);

  // Insert debug markers
  // wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do one thing"));
  // wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do another thing"));
  wgpuCommandEncoderCopyBufferToBuffer(encoder, bufferA,
                                       16,  // sourceOffset
                                       bufferB,
                                       0,  // destinationOffset
                                       bufferDescB.size);

  // generate the command buffer by finishing the command encoder
  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
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
  wgpuInstanceProcessEvents(webgpuRes.instance);
  while (!workDone) {
    sleepForMilliseconds(200);
    wgpuInstanceProcessEvents(webgpuRes.instance);
  }

  fetchBufferDataSync(webgpuRes.instance, bufferB, bufferDescB.size, [&](const void* data) {
    const uint8_t* bufferDataB = static_cast<const uint8_t*>(data);
    std::cout << "Buffer B: [";
    for (size_t i = 0; i < bufferDescB.size; ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << static_cast<int>(bufferDataB[i]);
    }
    std::cout << "]" << std::endl;
  });

  wgpuBufferUnmap(bufferB);

  // At the end of the program:
  wgpuBufferRelease(bufferA);
  wgpuBufferRelease(bufferB);

  VividLogger::app_info("All queued instructions have been executed!");

  VividLogger::app_info("WebGPU command queue tested");
}

// ============================================================================
// Core System Implementations (registered and actually used)
// ============================================================================

void RenderSystems::syncSceneImpl(flecs::entity entity, const MeshComponent& mesh,
                                  const MaterialComponent& material, WebGPUContext& webgpuRes) {
  // Process entities that have the CPU-side data (Mesh, Material)
  // but DO NOT have the GPU-side data (GpuMeshComponent) yet.
  // (filtered by .without<GpuMeshComponent>() in system registration)

  if (mesh.m_Vertices.empty() || mesh.m_Indices.empty() || material.ShaderPath.empty()) return;

  // 创建和绑定VBO
  // Create vertex buffer
  WGPUBufferDescriptor bufferDesc = {};
  bufferDesc.nextInChain = nullptr;
  bufferDesc.label = toWgpuStringView("Vertex buffer");
  bufferDesc.size = mesh.m_Vertices.size() * sizeof(float);
  bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
  WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(webgpuRes.device, &bufferDesc);

  // Upload geometry data to the buffer
  wgpuQueueWriteBuffer(webgpuRes.queue, vertexBuffer, 0, mesh.m_Vertices.data(), bufferDesc.size);

  // 创建IBO
  // Create index buffer (use 32-bit indices to match MeshComponent definition)
  // (we reuse the bufferDesc initialized for the vertexBuffer)
  bufferDesc.size = mesh.m_Indices.size() * sizeof(uint32_t);

  // only need when using uint16_t, uint32_t is 4 bytes aligned
  // bufferDesc.size = (bufferDesc.size + 3) & ~3;  // round up to the next multiple of 4
  // mesh.m_Indices.resize((mesh.m_Indices.size() + 1)
  //                       & ~1);  // round up to the next multiple of 2
  bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
  WGPUBuffer indexBuffer = wgpuDeviceCreateBuffer(webgpuRes.device, &bufferDesc);

  wgpuQueueWriteBuffer(webgpuRes.queue, indexBuffer, 0, mesh.m_Indices.data(), bufferDesc.size);

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
  wgslDesc.code = toWgpuStringView(shaderSource);
  WGPUShaderModuleDescriptor shaderDesc = {};
  shaderDesc.nextInChain = &wgslDesc.chain;  // connect the chained extension
  shaderDesc.label = toWgpuStringView("Shader source");
  WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(webgpuRes.device, &shaderDesc);

  // When describing the render pipeline:
  WGPURenderPipelineDescriptor pipelineDesc = {};
  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");

  WGPUFragmentState fragmentState = {};
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = toWgpuStringView("fs_main");
  WGPUColorTargetState colorTarget = {};
  colorTarget.format = webgpuRes.surfaceFormat;
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
  depthStencil.format = webgpuRes.depthFormat;
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
      = wgpuDeviceCreateBindGroupLayout(webgpuRes.device, &bindGroupLayoutDesc);

  // Create the pipeline layout
  WGPUPipelineLayoutDescriptor layoutDesc = {};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = (const WGPUBindGroupLayout*)&bindGroupLayout;
  WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(webgpuRes.device, &layoutDesc);

  // Assign the PipelineLayout to the RenderPipelineDescriptor's layout field
  pipelineDesc.layout = layout;
  WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(webgpuRes.device, &pipelineDesc);
  wgpuShaderModuleRelease(shaderModule);

  // Now we use emplace, because we know the component doesn't exist yet.
  GpuMeshComponent gpuMeshComponent;
  gpuMeshComponent.vertexBuffer = vertexBuffer;
  gpuMeshComponent.indexBuffer = indexBuffer;
  gpuMeshComponent.indexCount = (unsigned int)mesh.m_Indices.size();
  gpuMeshComponent.vertexBufferLayout = vertexBufferLayout;
  gpuMeshComponent.layout = layout;
  gpuMeshComponent.bindGroupLayout = bindGroupLayout;
  gpuMeshComponent.pipeline = pipeline;

  // Create per-entity uniform buffer and bind group (persist across frames)
  WGPUBufferDescriptor uniformDesc = {};
  uniformDesc.nextInChain = nullptr;
  uniformDesc.label = toWgpuStringView("Per-entity uniform buffer");
  uniformDesc.size = sizeof(BPUniforms);
  uniformDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  gpuMeshComponent.uniformBuffer = wgpuDeviceCreateBuffer(webgpuRes.device, &uniformDesc);

  WGPUBindGroupEntry bgEntry = {};
  bgEntry.binding = 0;
  bgEntry.buffer = gpuMeshComponent.uniformBuffer;
  bgEntry.offset = 0;
  bgEntry.size = sizeof(BPUniforms);

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.nextInChain = nullptr;
  bgDesc.layout = bindGroupLayout;
  bgDesc.entryCount = 1;
  bgDesc.entries = &bgEntry;
  gpuMeshComponent.bindGroup = wgpuDeviceCreateBindGroup(webgpuRes.device, &bgDesc);

  entity.set<GpuMeshComponent>(gpuMeshComponent);
}

void RenderSystems::renderMeshImpl(const flecs::iter& it) {
  auto world = it.world();

  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }

  if (!world.has<VIVID::WINDOW::WindowContext>()) {
    VividLogger::app_error("Could not get Window context!");
    return;
  }

  auto& webgpuRes = world.get_mut<WebGPUContext>();
  auto& windowContext = world.get<VIVID::WINDOW::WindowContext>();
  // Check current window pixel size and reconfigure if changed or zero

  if (windowContext.pixel_width <= 0 || windowContext.pixel_height <= 0) {
    // Minimized or not ready; skip this frame
    webgpuRes.renderPass = nullptr;  // Mark render pass as invalid to signal frame skip
    return;
  }

  // TODO: refactor this
  if (webgpuRes.configuredWidth != static_cast<uint32_t>(windowContext.pixel_width)
      || webgpuRes.configuredHeight != static_cast<uint32_t>(windowContext.pixel_height)) {
    reconfigureSurface(webgpuRes, static_cast<uint32_t>(windowContext.pixel_width),
                       static_cast<uint32_t>(windowContext.pixel_height));
    // Skip this frame after reconfiguration
    webgpuRes.renderPass
        = nullptr;  // Mark render pass as invalid to prevent UI from rendering to stale pass
    return;
  }

  // [...] Get the next target texture view
  wgpuSurfaceGetCurrentTexture(webgpuRes.surface, &webgpuRes.surfaceTexture);

  if (webgpuRes.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
      && webgpuRes.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
#if defined(WGPUSurfaceGetCurrentTextureStatus_Success)
      && webgpuRes.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success
#endif
  ) {
    VividLogger::render_error("Surface texture status error: %d", webgpuRes.surfaceTexture.status);
    if (webgpuRes.surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated
        || webgpuRes.surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
      // Reconfigure on outdated/lost
      reconfigureSurface(webgpuRes, static_cast<uint32_t>(windowContext.pixel_width),
                         static_cast<uint32_t>(windowContext.pixel_height));
    }
    // Skip this frame for any non-success status
    webgpuRes.renderPass = nullptr;  // Mark render pass as invalid
    return;
  }

  WGPUTextureViewDescriptor viewDescriptor = {};
  viewDescriptor.nextInChain = nullptr;
  viewDescriptor.label = toWgpuStringView("Surface texture view");
  viewDescriptor.format = wgpuTextureGetFormat(webgpuRes.surfaceTexture.texture);
  viewDescriptor.dimension = WGPUTextureViewDimension_2D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = WGPUTextureAspect_All;
  // View usage must be compatible with the surface texture's usage (RENDER_ATTACHMENT)
  viewDescriptor.usage = WGPUTextureUsage_RenderAttachment;
  webgpuRes.targetView = wgpuTextureCreateView(webgpuRes.surfaceTexture.texture, &viewDescriptor);

  // [...] Draw things
  // [...] Create Command Encoder
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = toWgpuStringView("begin render pass encoder");
  webgpuRes.encoder = wgpuDeviceCreateCommandEncoder(webgpuRes.device, &encoderDesc);

  // [...] Encode Render Pass
  // Describe the attachment
  WGPURenderPassColorAttachment renderPassColorAttachment = {};
  renderPassColorAttachment.view = webgpuRes.targetView;
  renderPassColorAttachment.resolveTarget = nullptr;
  renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
  renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
  renderPassColorAttachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};
  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

  // Describe the render pass
  WGPURenderPassDescriptor renderPassDesc = {};
  renderPassDesc.nextInChain = nullptr;
  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &renderPassColorAttachment;
  WGPURenderPassDepthStencilAttachment depthAttach = {};
  depthAttach.view = webgpuRes.depthView;
  depthAttach.depthClearValue = 1.0f;
  depthAttach.depthLoadOp = WGPULoadOp_Clear;
  depthAttach.depthStoreOp = WGPUStoreOp_Store;
  depthAttach.depthReadOnly = false;
  depthAttach.stencilReadOnly = true;
  renderPassDesc.depthStencilAttachment = &depthAttach;
  renderPassDesc.timestampWrites
      = nullptr;  // When measuring the performance of a render pass, it is not possible to use
                  // CPU-side timing functions, since the commands are not executed synchronously.
                  // Instead, the render pass can receive a set of timestamp queries.

  webgpuRes.renderPass = wgpuCommandEncoderBeginRenderPass(webgpuRes.encoder, &renderPassDesc);

  // Use Render Pass
  // Build camera matrices and positions
  glm::mat4 viewMatrix(1.0f);
  glm::mat4 projectionMatrix(1.0f);
  glm::vec3 viewPos(0.0f);

  // Find the first camera entity
  flecs::entity mainCameraEntity;
  {
    auto cameraQuery = world.query<TransformComponent, CameraComponent>();
    cameraQuery.each(
        [&](flecs::entity entity, TransformComponent& transform, CameraComponent& camera) {
          if (!mainCameraEntity.is_valid()) {
            mainCameraEntity = entity;
          }
        });
  }

  if (mainCameraEntity.is_valid()) {
    const auto& mainCameraTransform = mainCameraEntity.get<TransformComponent>();
    const auto& mainCameraComponent = mainCameraEntity.get<CameraComponent>();

    viewPos = mainCameraTransform.Position;
    if (mainCameraEntity.has<CameraControllerComponent>()) {
      const auto& controller = mainCameraEntity.get<CameraControllerComponent>();
      glm::vec3 target = mainCameraTransform.Position + controller.Front;
      viewMatrix = glm::lookAt(mainCameraTransform.Position, target, controller.Up);
      // VividLogger::app_info("Camera controller component found, using view matrix");
    } else {
      viewMatrix
          = glm::lookAt(mainCameraTransform.Position,
                        mainCameraTransform.Position + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
      // VividLogger::app_info("No camera controller component found, using default view matrix");
    }
    // VividLogger::app_info("Using view matrix");
    projectionMatrix = mainCameraComponent.ProjectionMatrix;
    if (projectionMatrix == glm::mat4(1.0f) && webgpuRes.configuredHeight > 0) {
      float aspect = static_cast<float>(webgpuRes.configuredWidth)
                     / static_cast<float>(webgpuRes.configuredHeight);
      projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    } else {
      // VividLogger::app_info("Using default projection matrix");
    }
  } else {
    VividLogger::app_info("No camera found");
  }

  // Query first light
  glm::vec3 lightPos(5.0f, 5.0f, 5.0f);
  glm::vec3 lightColor(1.0f);
  glm::vec3 ambientColor(0.2f);
  float constant = 1.0f, linear = 0.09f, quadratic = 0.032f;

  flecs::entity lightEntity;
  {
    auto lightQuery = world.query<TransformComponent, LightComponent>();
    lightQuery.each([&](flecs::entity entity, TransformComponent& lightTransform,
                        LightComponent& lightComponent) {
      if (!lightEntity.is_valid()) {
        lightEntity = entity;
      }
    });
  }

  if (lightEntity.is_valid()) {
    const auto& lightTransform = lightEntity.get<TransformComponent>();
    const auto& lightComponent = lightEntity.get<LightComponent>();

    lightPos = lightTransform.Position;
    lightColor = lightComponent.LightColor;
    ambientColor = lightComponent.AmbientColor;
    constant = lightComponent.Constant;
    linear = lightComponent.Linear;
    quadratic = lightComponent.Quadratic;
  }

  // Iterate over all GPU meshes and draw
  auto drawQuery = world.query<GpuMeshComponent, TransformComponent, MaterialComponent>();
  drawQuery.each([&](flecs::entity entity, const GpuMeshComponent& gpu,
                     const TransformComponent& transform, const MaterialComponent& material) {
    if (gpu.pipeline == nullptr || gpu.vertexBuffer == nullptr || gpu.indexBuffer == nullptr
        || gpu.indexCount == 0) {
      return;
    }

    // Prepare per-entity uniforms
    BPUniforms uniforms = {};
    const glm::mat4 model = transform.GetTransform();
    const glm::mat4 normalMat = glm::transpose(glm::inverse(model));
    uniforms.model = model;
    uniforms.view = viewMatrix;
    uniforms.projection = projectionMatrix;
    uniforms.normalMatrix = normalMat;
    uniforms.viewPos = {viewPos.x, viewPos.y, viewPos.z, 0.0f};
    uniforms.lightPos = {lightPos.x, lightPos.y, lightPos.z, 0.0f};
    uniforms.objectColor
        = {material.ObjectColor.r, material.ObjectColor.g, material.ObjectColor.b, 0.0f};
    uniforms.lightColor = {lightColor.r, lightColor.g, lightColor.b, 0.0f};
    uniforms.ambientColor = {ambientColor.r, ambientColor.g, ambientColor.b, 0.0f};
    uniforms.specularColor
        = {material.SpecularColor.r, material.SpecularColor.g, material.SpecularColor.b, 0.0f};
    uniforms.params = {constant, linear, quadratic, material.Shininess};

    // Update per-entity uniform buffer content
    if (gpu.uniformBuffer != nullptr) {
      wgpuQueueWriteBuffer(webgpuRes.queue, gpu.uniformBuffer, 0, &uniforms, sizeof(uniforms));
    }

    // Bind pipeline and buffers, then draw
    wgpuRenderPassEncoderSetPipeline(webgpuRes.renderPass, gpu.pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(webgpuRes.renderPass, 0, gpu.vertexBuffer, 0,
                                         WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetIndexBuffer(webgpuRes.renderPass, gpu.indexBuffer,
                                        WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetBindGroup(webgpuRes.renderPass, 0, gpu.bindGroup, 0, nullptr);
    wgpuRenderPassEncoderDrawIndexed(webgpuRes.renderPass, gpu.indexCount, 1, 0, 0, 0);
  });
}

void RenderSystems::submitImpl(const flecs::iter& it) {
  auto world = it.world();
  auto& webgpuRes = world.get<WebGPUContext>();
  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }

  // Skip if no render pass was created this frame (e.g., window resize)
  if (webgpuRes.renderPass == nullptr) {
    return;
  }

  wgpuRenderPassEncoderEnd(webgpuRes.renderPass);
  wgpuRenderPassEncoderRelease(webgpuRes.renderPass);

  // [...] Finish encoding and submit
  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.nextInChain = nullptr;
  cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
  WGPUCommandBuffer command = wgpuCommandEncoderFinish(webgpuRes.encoder, &cmdBufferDescriptor);
  wgpuCommandEncoderRelease(webgpuRes.encoder);  // release encoder after it's finished

  // Finally submit the command queue
  // std::cout << "Submitting command..." << std::endl;
  wgpuQueueSubmit(webgpuRes.queue, 1, &command);
  wgpuCommandBufferRelease(command);
  // std::cout << "Command submitted." << std::endl;

  // [...] Present the surface onto the window
  wgpuTextureViewRelease(webgpuRes.targetView);
#ifndef WEBGPU_BACKEND_WGPU
  // We no longer need the texture, only its view
  // (NB: with wgpu-native, surface textures must be release after the call to wgpuSurfacePresent)
  wgpuTextureRelease(webgpuRes.surfaceTexture.texture);
#endif  // WEBGPU_BACKEND_WGPU

  // In the context of a Web browser, we do not present the surface texture ourselves. We rather
  // rely on emscripten_set_main_loop_arg (a.k.a. requestAnimationFrame in JavaScript) to call our
  // MainLoop() function right before presenting.
#ifndef __EMSCRIPTEN__
  wgpuSurfacePresent(webgpuRes.surface);
#  if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(webgpuRes.device);
#  endif
#endif

#ifdef WEBGPU_BACKEND_WGPU
  wgpuTextureRelease(surfaceTexture.texture);
#endif
}

void RenderSystems::createPipelineImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Creating WebGPU pipeline...");
  auto& webgpuRes = world.get_mut<WebGPUContext>();
  if (!world.has<WebGPUContext>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }

  const char* shaderSource = R"(
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
  WGPUShaderSourceWGSL wgslDesc = {};
  wgslDesc.chain.next = nullptr;
  wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgslDesc.code = toWgpuStringView(shaderSource);
  WGPUShaderModuleDescriptor shaderDesc = {};
  shaderDesc.nextInChain = &wgslDesc.chain;  // connect the chained extension
  shaderDesc.label = toWgpuStringView("Shader source");
  WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(webgpuRes.device, &shaderDesc);

  // Describe render pipeline
  WGPURenderPipelineDescriptor pipelineDesc = {};
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");

  // const char *fragmentShaderSource = R"(
  //   @fragment
  //   fn fs_main() -> @location(0) vec4f {
  //       return vec4f(0.0, 0.4, 0.7, 1.0);
  //   }
  // )";

  WGPUFragmentState fragmentState = {};
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = toWgpuStringView("fs_main");
  pipelineDesc.fragment = &fragmentState;

  WGPUColorTargetState colorTarget = {};
  colorTarget.format = webgpuRes.surfaceFormat;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  WGPUBlendState blendState = {};
  blendState.color.srcFactor = WGPUBlendFactor_One;
  blendState.color.dstFactor = WGPUBlendFactor_Zero;
  blendState.color.operation = WGPUBlendOperation_Add;
  blendState.alpha.srcFactor = WGPUBlendFactor_One;
  blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
  blendState.alpha.operation = WGPUBlendOperation_Add;
  colorTarget.blend = &blendState;
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;

  // Primitive state
  WGPUPrimitiveState primitive = {};
  primitive.topology = WGPUPrimitiveTopology_TriangleList;
  primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  primitive.frontFace = WGPUFrontFace_CCW;
  primitive.cullMode = WGPUCullMode_None;
  pipelineDesc.primitive = primitive;

  // Multisample state
  WGPUMultisampleState multisample = {};
  multisample.count = 1;
  multisample.mask = 0xFFFFFFFF;
  multisample.alphaToCoverageEnabled = false;
  pipelineDesc.multisample = multisample;

  webgpuRes.pipeline = wgpuDeviceCreateRenderPipeline(webgpuRes.device, &pipelineDesc);

  // @compute @workgroup_size(32)
  // fn computeStuff(@builtin(global_invocation_id) threadId: vec3u) {
  //     // [...]
  // }
}

void RenderSystems::releaseWebGPUResourcesImpl(const flecs::iter& it) {
  auto world = it.world();
  VividLogger::app_info("Releasing WebGPU instance...");

  // Release all per-entity GPU resources first
  {
    auto query = world.query<GpuMeshComponent>();
    query.each([&](flecs::entity entity, GpuMeshComponent& gpu) {
      if (gpu.bindGroup) {
        wgpuBindGroupRelease(gpu.bindGroup);
        gpu.bindGroup = nullptr;
      }
      if (gpu.uniformBuffer) {
        wgpuBufferRelease(gpu.uniformBuffer);
        gpu.uniformBuffer = nullptr;
      }
      if (gpu.vertexBuffer) {
        wgpuBufferRelease(gpu.vertexBuffer);
        gpu.vertexBuffer = nullptr;
      }
      if (gpu.indexBuffer) {
        wgpuBufferRelease(gpu.indexBuffer);
        gpu.indexBuffer = nullptr;
      }
      if (gpu.pipeline) {
        wgpuRenderPipelineRelease(gpu.pipeline);
        gpu.pipeline = nullptr;
      }
      if (gpu.layout) {
        wgpuPipelineLayoutRelease(gpu.layout);
        gpu.layout = nullptr;
      }
      if (gpu.bindGroupLayout) {
        wgpuBindGroupLayoutRelease(gpu.bindGroupLayout);
        gpu.bindGroupLayout = nullptr;
      }
      // Remove component from entity
      entity.remove<GpuMeshComponent>();
    });
  }

  auto& webgpuRes = world.get_mut<WebGPUContext>();
  if (world.has<WebGPUContext>()) {
    if (webgpuRes.depthView) {
      wgpuTextureViewRelease(webgpuRes.depthView);
      webgpuRes.depthView = nullptr;
    }
    if (webgpuRes.depthTexture) {
      wgpuTextureRelease(webgpuRes.depthTexture);
      webgpuRes.depthTexture = nullptr;
    }
    wgpuSurfaceRelease(webgpuRes.surface);
    // wgpuRenderPipelineRelease(webgpuRes.pipeline);
    wgpuQueueRelease(webgpuRes.queue);
    wgpuAdapterRelease(webgpuRes.adapter);
    wgpuDeviceRelease(webgpuRes.device);
    wgpuInstanceProcessEvents(webgpuRes.instance);
    wgpuInstanceRelease(webgpuRes.instance);
    VividLogger::app_info("WGPU instance, adapter, and device released");
    webgpuRes.queue = nullptr;
    webgpuRes.surface = nullptr;
    webgpuRes.pipeline = nullptr;
    webgpuRes.instance = nullptr;
    webgpuRes.adapter = nullptr;
    webgpuRes.device = nullptr;
    webgpuRes.adapterRequestEnded = false;
    webgpuRes.deviceRequestEnded = false;
  }
}

void RenderSystems::initWebGPUImpl(const VIVID::WINDOW::WindowContext& windowContext,
                                   WebGPUContext& webgpuRes) {
  VividLogger::app_info("=== InitWebGPU system called ===");
  // VividLogger::app_info("Entity: %s (ID: %llu)", entity.name(), entity.id());
  VividLogger::app_info("Window handle: %p", windowContext.window_handle);
  VividLogger::app_info("WebGPU initialized flag: %d", webgpuRes.initialized);

  // Check if already initialized (due to .each(), this may run on multiple window entities)
  if (webgpuRes.initialized) {
    VividLogger::app_warn("WebGPU already initialized, skipping...");
    return;  // Already initialized, skip
  }

  VividLogger::app_info("Initializing WebGPU...");

  WGPUTextureFormat preferred_fmt
      = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities

  // Google DAWN backend: Adapter and Device acquisition, Surface creation
  wgpu::InstanceDescriptor instanceDescriptor = {};
  static constexpr wgpu::InstanceFeatureName requiredInstanceFeatures[] = {
      wgpu::InstanceFeatureName::TimedWaitAny,
  };
  instanceDescriptor.requiredFeatureCount = std::size(requiredInstanceFeatures);
  instanceDescriptor.requiredFeatures = requiredInstanceFeatures;
  wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);

  // We can check whether there is actually an instance created
  if (!instance) {
    VividLogger::app_error("Could not initialize WebGPU!");
    return;
  }

  // Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  VividLogger::app_info("WGPU instance: %p", &instance);

  wgpu::Adapter adapter{getAdapter(instance)};  // RequestWebGPUAdapterSync
  webgpuRes.device = getDevice(instance, adapter);

  if (!webgpuRes.device) {
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
  webgpuRes.surface = surface.MoveToCHandle();
#endif

  // Set instance BEFORE accessing window (needed for surface creation on non-Emscripten)
  webgpuRes.instance = instance.MoveToCHandle();

  // Get window pixel size from the current entity's WindowGpuComponent
  int pixel_width = 0;
  int pixel_height = 0;

  if (windowContext.window_handle) {
#ifndef __EMSCRIPTEN__
    webgpuRes.surface
        = ImGui_ImplSDL3_CreateWGPUSurface(webgpuRes.instance, windowContext.window_handle);
#endif

    // TODO: move this to the window systems
    SDL_GetWindowSizeInPixels(windowContext.window_handle, &pixel_width, &pixel_height);
    // VividLogger::app_info("Using window: entity=%llu, handle=%p, size=%dx%d", entity.id(),
    //                       windowContext.window_handle, pixel_width, pixel_height);
  }

  // Guard against zero-sized surfaces (e.g., minimized window); fall back to a small valid size
  if (pixel_width <= 0 || pixel_height <= 0) {
    VividLogger::app_warn(
        "Window pixel size is %dx%d; using fallback size for surface configuration", pixel_width,
        pixel_height);
    pixel_width = 1;
    pixel_height = 1;
  }
  if (!webgpuRes.surface) {
    VividLogger::app_error("Could not create WebGPU surface!");
    return;
  }

  // Moving Dawn objects into WGPU handles
  // wgpu_instance = instance.MoveToCHandle();
  // wgpu_surface = surface.MoveToCHandle();

  webgpuRes.instance = instance.MoveToCHandle();

  WGPUSurfaceCapabilities surface_capabilities = {};
  wgpuSurfaceGetCapabilities(webgpuRes.surface, adapter.Get(), &surface_capabilities);

  // preferred_fmt = surface_capabilities.formats[0];
  webgpuRes.surfaceFormat = surface_capabilities.formats[0];

  webgpuRes.adapter = adapter.MoveToCHandle();
  // webgpuRes.device = device.MoveToCHandle();
  if (!webgpuRes.adapter) {
    VividLogger::app_error("WebGPU device handle is null");
    return;
  }

  webgpuRes.surfaceConfiguration.presentMode = WGPUPresentMode_Fifo;
  webgpuRes.surfaceConfiguration.alphaMode = WGPUCompositeAlphaMode_Auto;
  webgpuRes.surfaceConfiguration.usage = WGPUTextureUsage_RenderAttachment;

  webgpuRes.surfaceConfiguration.width = webgpuRes.configuredWidth = pixel_width;
  webgpuRes.surfaceConfiguration.height = webgpuRes.configuredHeight = pixel_height;
  webgpuRes.surfaceConfiguration.device = webgpuRes.device;
  webgpuRes.surfaceConfiguration.format = webgpuRes.surfaceFormat;

  wgpuSurfaceConfigure(webgpuRes.surface, &webgpuRes.surfaceConfiguration);
  webgpuRes.queue = wgpuDeviceGetQueue(webgpuRes.device);
  if (!webgpuRes.queue) {
    VividLogger::app_error("Failed to acquire WebGPU device queue");
    return;
  }
  // reconfigureSurface(entity.world(), pixel_width, pixel_height);
  reconfigureSurface(webgpuRes, pixel_width, pixel_height);

  // Mark as initialized to prevent re-initialization
  webgpuRes.initialized = true;
  VividLogger::app_info("WebGPU initialized successfully");
}

// ============================================================================
// RenderSystems Constructor
// ============================================================================

// ============================================================================
// New Modular System Implementations
// ============================================================================

// void RenderSystems::surfaceManagementImpl(flecs::entity e,
//                                           VIVID::WINDOW::WindowGpuComponent& gpu_comp,
//                                           WebGPUContext& webgpuRes, RenderContext& renderCtx) {
//   // Check current window pixel size and reconfigure if changed or zero
//   int pixel_width = 0;
//   int pixel_height = 0;

//   if (gpu_comp.window_handle) {
//     SDL_GetWindowSizeInPixels(gpu_comp.window_handle, &pixel_width, &pixel_height);
//   }

//   if (pixel_width <= 0 || pixel_height <= 0) {
//     // Minimized or not ready; skip this frame
//     ImGui::EndFrame();
//     return;
//   }

//   // Check if surface needs reconfiguration
//   if (webgpuRes.configuredWidth != static_cast<uint32_t>(pixel_width)
//       || webgpuRes.configuredHeight != static_cast<uint32_t>(pixel_height)) {
//     reconfigureSurface(e.world(), static_cast<uint32_t>(pixel_width),
//                        static_cast<uint32_t>(pixel_height));
//   }

//   // Get the next target texture view
//   wgpuSurfaceGetCurrentTexture(webgpuRes.surface, &renderCtx.surfaceTexture);
//   if (renderCtx.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
//       && renderCtx.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
// #if defined(WGPUSurfaceGetCurrentTextureStatus_Success)
//       && renderCtx.surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success
// #endif
//   ) {
//     if (renderCtx.surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated
//         || renderCtx.surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
//       // Reconfigure on outdated/lost
//       reconfigureSurface(e.world(), static_cast<uint32_t>(pixel_width),
//                          static_cast<uint32_t>(pixel_height));
//     }
//     // Skip this frame for any non-success status
//     ImGui::EndFrame();
//     return;
//   }

//   WGPUTextureViewDescriptor viewDescriptor = {};
//   viewDescriptor.nextInChain = nullptr;
//   viewDescriptor.label = toWgpuStringView("Surface texture view");
//   viewDescriptor.format = wgpuTextureGetFormat(renderCtx.surfaceTexture.texture);
//   viewDescriptor.dimension = WGPUTextureViewDimension_2D;
//   viewDescriptor.baseMipLevel = 0;
//   viewDescriptor.mipLevelCount = 1;
//   viewDescriptor.baseArrayLayer = 0;
//   viewDescriptor.arrayLayerCount = 1;
//   viewDescriptor.aspect = WGPUTextureAspect_All;
//   viewDescriptor.usage = WGPUTextureUsage_RenderAttachment;
//   WGPUTextureView targetView
//       = wgpuTextureCreateView(renderCtx.surfaceTexture.texture, &viewDescriptor);

//   // Update render context
//   renderCtx.surfaceWidth = pixel_width;
//   renderCtx.surfaceHeight = pixel_height;
//   renderCtx.targetView = targetView;
//   renderCtx.surfaceReady = true;
// }

// void RenderSystems::sceneCollectionImpl(flecs::entity e, RenderContext& renderCtx) {
//   auto world = e.world();

//   // Find the first camera entity
//   flecs::entity mainCameraEntity;
//   {
//     auto cameraQuery = world.query<TransformComponent, CameraComponent>();
//     cameraQuery.each(
//         [&](flecs::entity entity, TransformComponent& transform, CameraComponent& camera) {
//           if (!mainCameraEntity.is_valid()) {
//             mainCameraEntity = entity;
//           }
//         });
//   }

//   if (mainCameraEntity.is_valid()) {
//     const auto& mainCameraTransform = mainCameraEntity.get<TransformComponent>();
//     const auto& mainCameraComponent = mainCameraEntity.get<CameraComponent>();

//     renderCtx.viewPos = mainCameraTransform.Position;
//     if (mainCameraEntity.has<CameraControllerComponent>()) {
//       const auto& controller = mainCameraEntity.get<CameraControllerComponent>();
//       glm::vec3 target = mainCameraTransform.Position + controller.Front;
//       renderCtx.viewMatrix = glm::lookAt(mainCameraTransform.Position, target, controller.Up);
//     } else {
//       renderCtx.viewMatrix
//           = glm::lookAt(mainCameraTransform.Position,
//                         mainCameraTransform.Position + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
//     }

//     renderCtx.projectionMatrix = mainCameraComponent.ProjectionMatrix;
//     if (renderCtx.projectionMatrix == glm::mat4(1.0f) && renderCtx.surfaceHeight > 0) {
//       float aspect = static_cast<float>(renderCtx.surfaceWidth)
//                      / static_cast<float>(renderCtx.surfaceHeight);
//       renderCtx.projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
//     }
//   }

//   // Query first light
//   flecs::entity lightEntity;
//   {
//     auto lightQuery = world.query<TransformComponent, LightComponent>();
//     lightQuery.each([&](flecs::entity entity, TransformComponent& lightTransform,
//                         LightComponent& lightComponent) {
//       if (!lightEntity.is_valid()) {
//         lightEntity = entity;
//       }
//     });
//   }

//   if (lightEntity.is_valid()) {
//     const auto& lightTransform = lightEntity.get<TransformComponent>();
//     const auto& lightComponent = lightEntity.get<LightComponent>();

//     renderCtx.lightPos = lightTransform.Position;
//     renderCtx.lightColor = lightComponent.LightColor;
//     renderCtx.ambientColor = lightComponent.AmbientColor;
//     renderCtx.constant = lightComponent.Constant;
//     renderCtx.linear = lightComponent.Linear;
//     renderCtx.quadratic = lightComponent.Quadratic;
//   }

//   renderCtx.sceneReady = true;
// }

// void RenderSystems::meshRenderImpl(flecs::entity e, GpuMeshComponent& gpuMesh,
//                                    TransformComponent& transform, MaterialComponent& material,
//                                    RenderContext& renderCtx) {
//   if (!renderCtx.surfaceReady || !renderCtx.sceneReady) {
//     ImGui::EndFrame();  // Skip if surface or scene not ready
//     return;
//   }

//   if (gpuMesh.pipeline == nullptr || gpuMesh.vertexBuffer == nullptr
//       || gpuMesh.indexBuffer == nullptr || gpuMesh.indexCount == 0) {
//     ImGui::EndFrame();
//     return;
//   }

//   if (!renderCtx.renderPass) {
//     // Create command encoder and render pass if not exists
//     auto world = e.world();
//     auto& webgpuRes = world.get<WebGPUContext>();

//     WGPUCommandEncoderDescriptor encoderDesc = {};
//     encoderDesc.nextInChain = nullptr;
//     encoderDesc.label = toWgpuStringView("Mesh render pass encoder");
//     renderCtx.encoder = wgpuDeviceCreateCommandEncoder(webgpuRes.device, &encoderDesc);

//     WGPURenderPassColorAttachment renderPassColorAttachment = {};
//     renderPassColorAttachment.view = renderCtx.targetView;
//     renderPassColorAttachment.resolveTarget = nullptr;
//     renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
//     renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
//     renderPassColorAttachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};
//     renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

//     WGPURenderPassDescriptor renderPassDesc = {};
//     renderPassDesc.nextInChain = nullptr;
//     renderPassDesc.colorAttachmentCount = 1;
//     renderPassDesc.colorAttachments = &renderPassColorAttachment;
//     WGPURenderPassDepthStencilAttachment depthAttach = {};
//     depthAttach.view = webgpuRes.depthView;
//     depthAttach.depthClearValue = 1.0f;
//     depthAttach.depthLoadOp = WGPULoadOp_Clear;
//     depthAttach.depthStoreOp = WGPUStoreOp_Store;
//     depthAttach.depthReadOnly = false;
//     depthAttach.stencilReadOnly = true;
//     renderPassDesc.depthStencilAttachment = &depthAttach;

//     renderCtx.renderPass = wgpuCommandEncoderBeginRenderPass(renderCtx.encoder, &renderPassDesc);
//   }

//   // Prepare per-entity uniforms
//   BPUniforms uniforms = {};
//   const glm::mat4 model = transform.GetTransform();
//   const glm::mat4 normalMat = glm::transpose(glm::inverse(model));
//   uniforms.model = model;
//   uniforms.view = renderCtx.viewMatrix;
//   uniforms.projection = renderCtx.projectionMatrix;
//   uniforms.normalMatrix = normalMat;
//   uniforms.viewPos = {renderCtx.viewPos.x, renderCtx.viewPos.y, renderCtx.viewPos.z, 0.0f};
//   uniforms.lightPos = {renderCtx.lightPos.x, renderCtx.lightPos.y, renderCtx.lightPos.z, 0.0f};
//   uniforms.objectColor
//       = {material.ObjectColor.r, material.ObjectColor.g, material.ObjectColor.b, 0.0f};
//   uniforms.lightColor
//       = {renderCtx.lightColor.r, renderCtx.lightColor.g, renderCtx.lightColor.b, 0.0f};
//   uniforms.ambientColor
//       = {renderCtx.ambientColor.r, renderCtx.ambientColor.g, renderCtx.ambientColor.b, 0.0f};
//   uniforms.specularColor
//       = {material.SpecularColor.r, material.SpecularColor.g, material.SpecularColor.b, 0.0f};
//   uniforms.params = {renderCtx.constant, renderCtx.linear, renderCtx.quadratic,
//   material.Shininess};

//   // Update per-entity uniform buffer content
//   auto world = e.world();
//   auto& webgpuRes = world.get<WebGPUContext>();
//   if (gpuMesh.uniformBuffer != nullptr) {
//     wgpuQueueWriteBuffer(webgpuRes.queue, gpuMesh.uniformBuffer, 0, &uniforms, sizeof(uniforms));
//   }

//   // Bind pipeline and buffers, then draw
//   wgpuRenderPassEncoderSetPipeline(renderCtx.renderPass, gpuMesh.pipeline);
//   wgpuRenderPassEncoderSetVertexBuffer(renderCtx.renderPass, 0, gpuMesh.vertexBuffer, 0,
//                                        WGPU_WHOLE_SIZE);
//   wgpuRenderPassEncoderSetIndexBuffer(renderCtx.renderPass, gpuMesh.indexBuffer,
//                                       WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
//   wgpuRenderPassEncoderSetBindGroup(renderCtx.renderPass, 0, gpuMesh.bindGroup, 0, nullptr);
//   wgpuRenderPassEncoderDrawIndexed(renderCtx.renderPass, gpuMesh.indexCount, 1, 0, 0, 0);

//   renderCtx.meshesReady = true;
// }

// void RenderSystems::uiRenderImpl(flecs::entity e, RenderContext& renderCtx) {
//   if (!renderCtx.surfaceReady || !renderCtx.meshesReady) {
//     ImGui::EndFrame();  // Skip if surface or meshes not ready
//     return;
//   }

//   // Render ImGui draw data within the same render pass
//   ImGui::Render();
//   ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderCtx.renderPass);

//   renderCtx.uiReady = true;
// }

// void RenderSystems::commandSubmissionImpl(flecs::entity e, WebGPUContext& webgpuRes,
//                                           RenderContext& renderCtx) {
//   if (!renderCtx.surfaceReady || !renderCtx.sceneReady || !renderCtx.meshesReady
//       || !renderCtx.uiReady) {
//     ImGui::EndFrame();  // Skip if not all stages are ready
//     return;
//   }

//   if (!renderCtx.renderPass) {
//     ImGui::EndFrame();  // Skip if no render pass was created
//     return;
//   }

//   // End render pass
//   wgpuRenderPassEncoderEnd(renderCtx.renderPass);
//   wgpuRenderPassEncoderRelease(renderCtx.renderPass);
//   renderCtx.renderPass = nullptr;

//   // Finish encoding and submit
//   WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
//   cmdBufferDescriptor.nextInChain = nullptr;
//   cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
//   WGPUCommandBuffer command = wgpuCommandEncoderFinish(renderCtx.encoder, &cmdBufferDescriptor);
//   wgpuCommandEncoderRelease(renderCtx.encoder);
//   renderCtx.encoder = nullptr;

//   // Submit command queue
//   wgpuQueueSubmit(webgpuRes.queue, 1, &command);
//   wgpuCommandBufferRelease(command);

//   // Present the surface onto the window
//   wgpuTextureViewRelease(renderCtx.targetView);
//   renderCtx.targetView = nullptr;

// #ifndef WEBGPU_BACKEND_WGPU
//   // We no longer need the texture, only its view
//   // (NB: with wgpu-native, surface textures must be release after the call to
//   wgpuSurfacePresent) wgpuTextureRelease(renderCtx.surfaceTexture.texture);
// #endif  // WEBGPU_BACKEND_WGPU

//   // In the context of a Web browser, we do not present the surface texture ourselves. We rather
//   // rely on emscripten_set_main_loop_arg (a.k.a. requestAnimationFrame in JavaScript) to call
//   our
//   // MainLoop() function right before presenting.
// #ifndef __EMSCRIPTEN__
//   wgpuSurfacePresent(webgpuRes.surface);
// #  if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
//   wgpuDeviceTick(webgpuRes.device);
// #  endif
// #endif

// #ifdef WEBGPU_BACKEND_WGPU
//   wgpuTextureRelease(renderCtx.surfaceTexture.texture);
// #endif

//   // Reset render context for next frame
//   renderCtx.surfaceReady = false;
//   renderCtx.sceneReady = false;
//   renderCtx.meshesReady = false;
//   renderCtx.uiReady = false;
// }

}  // namespace VIVID::RENDER
