#include "vivid/render/render_systems.h"

#include <sstream>
#include <thread>

#include "vivid/log/log.h"
#include "webgpu/webgpu.h"

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
WGPUStringView toWgpuStringView(const char *cString) { return {cString, WGPU_STRLEN}; }

struct WebGPUResources {
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  bool adapterRequestEnded = false;
};

namespace VIVID::Render {
  void CreateWebGPUInstance(Resources &res, entt::registry &world) {
    // We create a descriptor

    // The descriptor is a kind of way to pack many function arguments together, because some
    // descriptors really have a lot of fields. It can also be used to write utility functions that
    // take care of populating the arguments, to ease the program’s architecture.
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

// We create the instance using this descriptor
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(nullptr);
#else   //  WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(&desc);
#endif  //  WEBGPU_BACKEND_EMSCRIPTEN

    // We can check whether there is actually an instance created
    if (!instance) {
      VividLogger::app_error("Could not initialize WebGPU!");
      return;
    }

    auto &webgpu_res = res.insert<WebGPUResources>();
    webgpu_res.instance = instance;

    // Display the object (WGPUInstance is a simple pointer, it may be
    // copied around without worrying about its size).
    VividLogger::app_info("WGPU instance: %p", instance);
  }

  void sleepForMilliseconds(unsigned int milliseconds) {
#ifdef __EMSCRIPTEN__
    emscripten_sleep(milliseconds);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
  }
  void RequestWebGPUAdapterSync(Resources &res, entt::registry &world) {
    VividLogger::app_debug("Requesting WebGPU adapter...");

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;

    auto webgpuRes = res.get<WebGPUResources>();
    if (!webgpuRes) {
      VividLogger::app_error("Could not get WebGPU resources!");
      return;
    }
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
    WGPURequestAdapterCallback onAdapterRequestEnded
        = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
             void *userdata1, void *userdata2) {
            UserData &userData = *reinterpret_cast<UserData *>(userdata1);
            if (status == WGPURequestAdapterStatus_Success) {
              userData.adapter = adapter;
            } else {
              VividLogger::app_error("Could not get WebGPU adapter: %s",
                                     std::string(toStdStringView(message)));
            }
            userData.requestEnded = true;
          };

    WGPURequestAdapterCallbackInfo callbackInfo = {
        nullptr, WGPUCallbackMode_AllowProcessEvents, onAdapterRequestEnded, (void *)&userData,
        nullptr,
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(webgpuRes->instance /* equivalent of navigator.gpu */, &adapterOpts,
                               callbackInfo);

    // We wait until userData.requestEnded gets true

    // Hand the execution to the WebGPU instance so that it can check for
    // pending async operations, in which case it invokes our callbacks.
    // NB: We test once before the loop not to wait for 200ms in case it is
    // already ready
    wgpuInstanceProcessEvents(webgpuRes->instance);

    while (!userData.requestEnded) {
      // Waiting for 200 ms to avoid asking too often to process events
      sleepForMilliseconds(200);

      wgpuInstanceProcessEvents(webgpuRes->instance);
    }

    VIVID_ASSERT(userData.requestEnded);

    webgpuRes->adapter = userData.adapter;
    webgpuRes->adapterRequestEnded = userData.requestEnded;

    VividLogger::app_debug("Got WebGPU adapter");

    ReleaseWebGPUInstance(res, world);
  }

  void ReleaseWebGPUInstance(Resources &res, entt::registry &world) {
    VividLogger::app_debug("Releasing WebGPU instance...");

    auto webgpuRes = res.get<WebGPUResources>();
    if (webgpuRes) {
      wgpuInstanceRelease(webgpuRes->instance);
      VividLogger::app_debug("WGPU instance released");
      webgpuRes->instance = nullptr;
    }
  }

  void InspectWebGPUAdapter(Resources &res, entt::registry &world) {
    VividLogger::app_debug("Inspecting WebGPU adapter...");

    auto webgpuRes = res.get<WebGPUResources>();
    if (!webgpuRes) {
      VividLogger::app_error("Could not get WebGPU resources!");
      return;
    }

#ifndef __EMSCRIPTEN__
    WGPULimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

#  ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuAdapterGetLimits(webgpuRes->adapter, &supportedLimits) == WGPUStatus_Success;
#  else
    bool success = wgpuAdapterGetLimits(webgpuRes->adapter, &supportedLimits);
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
    wgpuAdapterGetFeatures(webgpuRes->adapter, &supportedFeatures);

    std::cout << "Adapter features:" << std::endl;
    std::cout
        << std::hex;  // Write integers as hexadecimal to ease comparison with webgpu.h literals
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
    wgpuAdapterGetInfo(webgpuRes->adapter, &properties);
    VividLogger::app_info("Adapter properties:");
    VividLogger::app_info(" - vendorID: %u", properties.vendorID);
    VividLogger::app_info(" - vendorName: %s", toStdStringView(properties.vendor).data());
    VividLogger::app_info(" - architecture: %s", toStdStringView(properties.architecture).data());
    VividLogger::app_info(" - deviceID: %u", properties.deviceID);
    VividLogger::app_info(" - name: %s", toStdStringView(properties.device).data());
    VividLogger::app_info(" - driverDescription: %s",
                          toStdStringView(properties.description).data());
    VividLogger::app_info(" - adapterType: 0x%X", properties.adapterType);
    VividLogger::app_info(" - backendType: 0x%X", properties.backendType);
    wgpuAdapterInfoFreeMembers(properties);
  }

}  // namespace VIVID::Render