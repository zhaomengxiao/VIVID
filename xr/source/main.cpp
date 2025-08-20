#include "OpenXRDebugUtils.h"
#include "OpenXRHelper.h"
#include "openxr/openxr.h"
#include "vivid/app/App.h"
#include "vivid/plugins/DefaultPlugin.h"
#include "vivid/rendering/render_plugin.h"

// XR resources
struct XrResource {
  XrInstance xrInstance = {};
  std::vector<const char *> activeAPILayers = {};
  std::vector<const char *> activeInstanceExtensions = {};
  std::vector<std::string> apiLayers = {};
  std::vector<std::string> instanceExtensions = {};

  XrDebugUtilsMessengerEXT debugUtilsMessenger = {};

  XrFormFactor formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XrSystemId systemID = {};
  XrSystemProperties systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};
  // GraphicsAPI_Type apiType = UNKNOWN;
  bool applicationRunning = true;
  bool sessionRunning = false;
};

// XR system

// The XrInstance is the foundational object that we need to create first.
// It encompasses the application setup state, OpenXR API version and any layers and extensions.
void CreateInstance_system(Resources &res, entt::registry &world) {
  XrApplicationInfo appInfo;
  strncpy(appInfo.applicationName, "XrPlugin", XR_MAX_APPLICATION_NAME_SIZE);
  appInfo.apiVersion = XR_CURRENT_API_VERSION;
  appInfo.applicationVersion = XR_MAKE_VERSION(1, 0, 0);
  strncpy(appInfo.engineName, "OpenXR Engine", XR_MAX_ENGINE_NAME_SIZE);
  appInfo.engineVersion = XR_MAKE_VERSION(1, 0, 0);

  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    xrRes = &res.insert<XrResource>();
  }

  // The XR_EXT_debug_utils is an extension that checks the validity of calls made to OpenXR and can
  // use a callback function to handle any raised errors.
  xrRes->instanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // Set which graphics API we are using.OpenXR supports D3D11, D3D12, Vulkan, OpenGL and OpenGL ES
  // Ensure m_apiType is already defined when we call this line.
  // xrRes->instanceExtensions.push_back(GetGraphicsAPIInstanceExtensionString(m_apiType));
  xrRes->instanceExtensions.push_back("XR_KHR_opengl_enable");

  // Get all the API Layers from the OpenXR runtime.Not all API layers and extensions are available
  // to use, check which ones the runtime can provide
  uint32_t apiLayerCount = 0;
  std::vector<XrApiLayerProperties> apiLayerProperties;
  OPENXR_CHECK(xrRes->xrInstance, xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr),
               "Failed to enumerate ApiLayerProperties.");
  apiLayerProperties.resize(apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES});
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data()),
      "Failed to enumerate ApiLayerProperties.");

  // Check the requested API layers against the ones from the OpenXR. If found add it to the Active
  // API Layers.
  for (auto &requestLayer : xrRes->apiLayers) {
    for (auto &layerProperty : apiLayerProperties) {
      // strcmp returns 0 if the strings match.
      if (strcmp(requestLayer.c_str(), layerProperty.layerName) != 0) {
        continue;
      } else {
        xrRes->activeAPILayers.push_back(requestLayer.c_str());
        break;
      }
    }
  }

  // Get all the Instance Extensions from the OpenXR instance.
  uint32_t extensionCount = 0;
  std::vector<XrExtensionProperties> extensionProperties;
  OPENXR_CHECK(xrRes->xrInstance,
               xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
               "Failed to enumerate InstanceExtensionProperties.");
  extensionProperties.resize(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
  OPENXR_CHECK(xrRes->xrInstance,
               xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount,
                                                      extensionProperties.data()),
               "Failed to enumerate InstanceExtensionProperties.");

  // Check the requested Instance Extensions against the ones from the OpenXR runtime.
  // If an extension is found add it to Active Instance Extensions.
  // Log error if the Instance Extension is not found.
  for (auto &requestedInstanceExtension : xrRes->instanceExtensions) {
    bool found = false;
    for (auto &extensionProperty : extensionProperties) {
      // strcmp returns 0 if the strings match.
      if (strcmp(requestedInstanceExtension.c_str(), extensionProperty.extensionName) != 0) {
        continue;
      } else {
        xrRes->activeInstanceExtensions.push_back(requestedInstanceExtension.c_str());
        found = true;
        break;
      }
    }
    if (!found) {
      std::cerr << "Failed to find OpenXR instance extension: " << requestedInstanceExtension
                << std::endl;
    }
  }

  XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
  instanceCI.createFlags = 0;
  instanceCI.applicationInfo = appInfo;
  instanceCI.enabledApiLayerCount = static_cast<uint32_t>(xrRes->activeAPILayers.size());
  instanceCI.enabledApiLayerNames = xrRes->activeAPILayers.data();
  instanceCI.enabledExtensionCount = static_cast<uint32_t>(xrRes->activeInstanceExtensions.size());
  instanceCI.enabledExtensionNames = xrRes->activeInstanceExtensions.data();
  OPENXR_CHECK(xrRes->xrInstance, xrCreateInstance(&instanceCI, &xrRes->xrInstance),
               "Failed to create Instance.");
}

void DestroyInstance_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  OPENXR_CHECK(xrRes->xrInstance, xrDestroyInstance(xrRes->xrInstance),
               "Failed to destroy Instance.");
}

// R_EXT_debug_utils is an instance extension for OpenXR, which allows the application to get more
// information on errors, warnings and messages raised by the runtime. You can specify which message
// severities and types are checked. If a debug message is raised, it is passed to the callback
// function, which can optionally use the user data pointer provided in the
// XrDebugUtilsMessengerCreateInfoEXT structure.
void CreateDebugMessenger_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Check that "XR_EXT_debug_utils" is in the active Instance Extensions before creating an
  // XrDebugUtilsMessengerEXT.
  if (IsStringInVector(xrRes->activeInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    xrRes->debugUtilsMessenger
        = CreateOpenXRDebugUtilsMessenger(xrRes->xrInstance);  // From OpenXRDebugUtils.h.
  }
}

void DestroyDebugMessenger_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Check that "XR_EXT_debug_utils" is in the active Instance Extensions before destroying the
  // XrDebugUtilsMessengerEXT.
  if (xrRes->debugUtilsMessenger != XR_NULL_HANDLE) {
    DestroyOpenXRDebugUtilsMessenger(xrRes->xrInstance,
                                     xrRes->debugUtilsMessenger);  // From OpenXRDebugUtils.h.
  }
}

void GetInstanceProperties_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  OPENXR_CHECK(xrRes->xrInstance, xrGetInstanceProperties(xrRes->xrInstance, &instanceProperties),
               "Failed to get InstanceProperties.");

  std::cout << "OpenXR Runtime: " << instanceProperties.runtimeName << " - "
            << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
}

// According to System in the OpenXR spec, OpenXR separates the concept of physical systems of XR
// devices from the logical objects that applications interact with directly. A system represents a
// collection of related devices in the runtime, often made up of several individual hardware
// components working together to enable XR experiences. So, an XrSystemId could represent a VR
// headset and a pair of controllers, or perhaps a mobile device with video pass-through for AR.
void GetSystemId_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Get the XrSystemId from the instance and the supplied XrFormFactor.
  XrSystemGetInfo systemGI{XR_TYPE_SYSTEM_GET_INFO};
  systemGI.formFactor = xrRes->formFactor;
  OPENXR_CHECK(xrRes->xrInstance, xrGetSystem(xrRes->xrInstance, &systemGI, &xrRes->systemID),
               "Failed to get SystemID.");

  // Get the System's properties for some general information about the hardware and the vendor.
  OPENXR_CHECK(xrRes->xrInstance,
               xrGetSystemProperties(xrRes->xrInstance, xrRes->systemID, &xrRes->systemProperties),
               "Failed to get SystemProperties.");
}

int main() {
  auto &app = App::new_app().add_plugin<DefaultPlugin>().add_plugin<RenderPlugin>();
  app.add_system(ScheduleLabel::Startup, CreateInstance_system);
  app.add_system(ScheduleLabel::Startup, CreateDebugMessenger_system);
  app.add_system(ScheduleLabel::Startup, GetInstanceProperties_system);
  app.add_system(ScheduleLabel::Startup, GetSystemId_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyInstance_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyDebugMessenger_system);
  app.run();

  return 0;
}