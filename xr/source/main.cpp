#pragma once

#include <GL/glew.h>

#include "vivid/app/App.h"
#include "vivid/plugins/DefaultPlugin.h"
#include "vivid/rendering/render_plugin.h"

// Platform headers
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <Windows.h>
#  include <unknwn.h>
#  define XR_USE_PLATFORM_WIN32
#  define XR_TUTORIAL_USE_OPENGL

#  if defined(XR_TUTORIAL_USE_D3D11)
#    define XR_USE_GRAPHICS_API_D3D11
#  endif
#  if defined(XR_TUTORIAL_USE_D3D12)
#    define XR_USE_GRAPHICS_API_D3D12
#  endif
#  if defined(XR_TUTORIAL_USE_OPENGL)
#    define XR_USE_GRAPHICS_API_OPENGL
#  endif
#  if defined(XR_TUTORIAL_USE_VULKAN)
#    define XR_USE_GRAPHICS_API_VULKAN
#  endif
#endif  // _WIN32

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
// #include "OpenXRDebugUtils.h"
#include <OpenXRHelper.h>

#ifdef _WIN32
#  undef APIENTRY
#  ifndef GLFW_EXPOSE_NATIVE_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32
#  endif
#  include <GLFW/glfw3native.h>  // for glfwGetWin32Window()
#endif

#include "vivid/rendering/render_system.h"

// #define XR_USE_GRAPHICS_API_OPENGL
// #define XR_USE_PLATFORM_WIN32

// debugUtial
//   XR_DOCS_TAG_BEGIN_OpenXRMessageCallbackFunction
XrBool32 OpenXRMessageCallbackFunction(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                                       XrDebugUtilsMessageTypeFlagsEXT messageType,
                                       const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                       void *pUserData) {
  // Lambda to covert an XrDebugUtilsMessageSeverityFlagsEXT to std::string. Bitwise check to
  // concatenate multiple severities to the output string.
  auto GetMessageSeverityString
      = [](XrDebugUtilsMessageSeverityFlagsEXT messageSeverity) -> std::string {
    bool separator = false;

    std::string msgFlags;
    if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
      msgFlags += "VERBOSE";
      separator = true;
    }
    if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
      if (separator) {
        msgFlags += ",";
      }
      msgFlags += "INFO";
      separator = true;
    }
    if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
      if (separator) {
        msgFlags += ",";
      }
      msgFlags += "WARN";
      separator = true;
    }
    if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
      if (separator) {
        msgFlags += ",";
      }
      msgFlags += "ERROR";
    }
    return msgFlags;
  };
  // Lambda to covert an XrDebugUtilsMessageTypeFlagsEXT to std::string. Bitwise check to
  // concatenate multiple types to the output string.
  auto GetMessageTypeString = [](XrDebugUtilsMessageTypeFlagsEXT messageType) -> std::string {
    bool separator = false;

    std::string msgFlags;
    if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)) {
      msgFlags += "GEN";
      separator = true;
    }
    if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)) {
      if (separator) {
        msgFlags += ",";
      }
      msgFlags += "SPEC";
      separator = true;
    }
    if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) {
      if (separator) {
        msgFlags += ",";
      }
      msgFlags += "PERF";
    }
    return msgFlags;
  };

  // Collect message data.
  std::string functionName = (pCallbackData->functionName) ? pCallbackData->functionName : "";
  std::string messageSeverityStr = GetMessageSeverityString(messageSeverity);
  std::string messageTypeStr = GetMessageTypeString(messageType);
  std::string messageId = (pCallbackData->messageId) ? pCallbackData->messageId : "";
  std::string message = (pCallbackData->message) ? pCallbackData->message : "";

  // String stream final message.
  std::stringstream errorMessage;
  errorMessage << functionName << "(" << messageSeverityStr << " / " << messageTypeStr
               << "): msgNum: " << messageId << " - " << message;

  // Log and debug break.
  std::cerr << errorMessage.str() << std::endl;
  if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
    DEBUG_BREAK;
  }
  return XrBool32();
}
// XR_DOCS_TAG_END_OpenXRMessageCallbackFunction

// XR_DOCS_TAG_BEGIN_Create_DestroyDebugMessenger
XrDebugUtilsMessengerEXT CreateOpenXRDebugUtilsMessenger(XrInstance xrInstance) {
  // Fill out a XrDebugUtilsMessengerCreateInfoEXT structure specifying all severities and types.
  // Set the userCallback to OpenXRMessageCallbackFunction().
  XrDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{
      XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debugUtilsMessengerCI.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                            | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                            | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                            | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugUtilsMessengerCI.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                       | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                       | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                       | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
  debugUtilsMessengerCI.userCallback
      = (PFN_xrDebugUtilsMessengerCallbackEXT)OpenXRMessageCallbackFunction;
  debugUtilsMessengerCI.userData = nullptr;

  // Load xrCreateDebugUtilsMessengerEXT() function pointer as it is not default loaded by the
  // OpenXR loader.
  XrDebugUtilsMessengerEXT debugUtilsMessenger{};
  PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
  OPENXR_CHECK(xrInstance,
               xrGetInstanceProcAddr(xrInstance, "xrCreateDebugUtilsMessengerEXT",
                                     (PFN_xrVoidFunction *)&xrCreateDebugUtilsMessengerEXT),
               "Failed to get InstanceProcAddr.");

  // Finally create and return the XrDebugUtilsMessengerEXT.
  OPENXR_CHECK(
      xrInstance,
      xrCreateDebugUtilsMessengerEXT(xrInstance, &debugUtilsMessengerCI, &debugUtilsMessenger),
      "Failed to create DebugUtilsMessenger.");
  return debugUtilsMessenger;
}

void DestroyOpenXRDebugUtilsMessenger(XrInstance xrInstance,
                                      XrDebugUtilsMessengerEXT debugUtilsMessenger) {
  // Load xrDestroyDebugUtilsMessengerEXT() function pointer as it is not default loaded by the
  // OpenXR loader.
  PFN_xrDestroyDebugUtilsMessengerEXT xrDestroyDebugUtilsMessengerEXT;
  OPENXR_CHECK(xrInstance,
               xrGetInstanceProcAddr(xrInstance, "xrDestroyDebugUtilsMessengerEXT",
                                     (PFN_xrVoidFunction *)&xrDestroyDebugUtilsMessengerEXT),
               "Failed to get InstanceProcAddr.");

  // Destroy the provided XrDebugUtilsMessengerEXT.
  OPENXR_CHECK(xrInstance, xrDestroyDebugUtilsMessengerEXT(debugUtilsMessenger),
               "Failed to destroy DebugUtilsMessenger.");
}
// XR_DOCS_TAG_END_Create_DestroyDebugMessenger

// XR resources
struct ImageCreateInfo {
  uint32_t dimension;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t mipLevels;
  uint32_t arrayLayers;
  uint32_t sampleCount;
  int64_t format;
  bool cubemap;
  bool colorAttachment;
  bool depthAttachment;
  bool sampled;
};
struct ImageViewCreateInfo {
  void *image;
  enum class Type : uint8_t { RTV, DSV, SRV, UAV } type;
  enum class View : uint8_t {
    TYPE_1D,
    TYPE_2D,
    TYPE_3D,
    TYPE_CUBE,
    TYPE_1D_ARRAY,
    TYPE_2D_ARRAY,
    TYPE_CUBE_ARRAY,
  } view;
  int64_t format;
  enum class Aspect : uint8_t { COLOR_BIT = 0x01, DEPTH_BIT = 0x02, STENCIL_BIT = 0x04 } aspect;
  uint32_t baseMipLevel;
  uint32_t levelCount;
  uint32_t baseArrayLayer;
  uint32_t layerCount;
};
struct SwapchainInfo {
  XrSwapchain swapchain = XR_NULL_HANDLE;
  int64_t swapchainFormat = 0;
  std::vector<void *> imageViews;
};

enum class SwapchainType : uint8_t { COLOR, DEPTH };

struct RenderLayerInfo {
  XrTime predictedDisplayTime;
  std::vector<XrCompositionLayerBaseHeader *> layers;
  XrCompositionLayerProjection layerProjection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
};
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
  // std::unique_ptr<GraphicsAPI> graphicsAPI = nullptr;

  XrSession session = XR_NULL_HANDLE;
  XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;

  bool applicationRunning = true;
  bool sessionRunning = false;

  // GraphicsAPI_Resource
  XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
  std::unordered_map<XrSwapchain, std::pair<SwapchainType, std::vector<XrSwapchainImageOpenGLKHR>>>
      swapchainImagesMap{};

  std::unordered_map<GLuint, ImageCreateInfo> images{};
  std::unordered_map<GLuint, ImageViewCreateInfo> imageViews{};

  // Swapchain
  std::vector<XrViewConfigurationType> applicationViewConfigurations
      = {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO};
  std::vector<XrViewConfigurationType> viewConfigurations;
  XrViewConfigurationType viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
  std::vector<XrViewConfigurationView> viewConfigurationViews;

  std::vector<SwapchainInfo> colorSwapchainInfos = {};
  std::vector<SwapchainInfo> depthSwapchainInfos = {};

  // Building Render Loop
  std::vector<XrEnvironmentBlendMode> applicationEnvironmentBlendModes
      = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE};
  std::vector<XrEnvironmentBlendMode> environmentBlendModes = {};
  XrEnvironmentBlendMode environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

  XrSpace localSpace = XR_NULL_HANDLE;

  GLuint setFramebuffer = 0;
  GLuint vertexArray = 0;
  GLuint setIndexBuffer = 0;

  // RenderFrame
  RenderLayerInfo renderLayerInfo = {};
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

void InitializeDevice_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Extension function must be loaded by name
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
  OPENXR_CHECK(xrRes->xrInstance,
               xrGetInstanceProcAddr(
                   xrRes->xrInstance, "xrGetOpenGLGraphicsRequirementsKHR",
                   reinterpret_cast<PFN_xrVoidFunction *>(&pfnGetOpenGLGraphicsRequirementsKHR)),
               "Failed to get xrGetOpenGLGraphicsRequirementsKHR");

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
  OPENXR_CHECK(xrRes->xrInstance,
               pfnGetOpenGLGraphicsRequirementsKHR(xrRes->xrInstance, xrRes->systemID,
                                                   &graphicsRequirements),
               "Failed to get OpenGL Graphics Requirements.");

  GLint major = 0;
  GLint minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);

  const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
  // if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
  //   std::cerr << "Runtime does not support desired Graphics API and/or version" << std::endl;
  //   std::cerr << "Runtime: " << graphicsRequirements.minApiVersionSupported << std::endl;
  //   std::cerr << "Desired: " << desiredApiVersion << std::endl;
  //   return;
  // }
}

// An XrSession encapsulates the state of the application from the perspective of OpenXR. When
// an XrSession is created, it starts in the XrSessionState XR_SESSION_STATE_IDLE. It is up to
// the runtime to provide any updates to the XrSessionState and for the application to query
// them and react to them.
void CreateSession_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};
  // xrRes->graphicsAPI = std::make_unique<GraphicsAPI_OpenGL>(xrRes->xrInstance, xrRes->systemID);
  // GetGraphicsBinding
  auto *windowRes = res.get<WindowResource>();
  if (!windowRes) {
    std::cerr << "Failed to get WindowResource." << std::endl;
    return;
  }

  xrRes->graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
  xrRes->graphicsBinding.hDC = GetDC(glfwGetWin32Window(windowRes->window));
  xrRes->graphicsBinding.hGLRC = wglGetCurrentContext();

  sessionCI.next = &xrRes->graphicsBinding;
  sessionCI.createFlags = 0;
  sessionCI.systemId = xrRes->systemID;

  OPENXR_CHECK(xrRes->xrInstance, xrCreateSession(xrRes->xrInstance, &sessionCI, &xrRes->session),
               "Failed to create Session.");
}

// Grahics API system

void DestroySession_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  OPENXR_CHECK(xrRes->xrInstance, xrDestroySession(xrRes->session), "Failed to destroy Session.");
}

// OpenXR uses an event-based system to describe changes within the XR system. It’s the
// application’s responsibility to poll these events and react to them. The polling of events is
// done by the function xrPollEvent. The application should call this function on every frame of its
// lifetime. Within a single XR frame, the application should continuously call xrPollEvent until
// the internal event queue is ‘drained’; multiple events can occur in an XR frame and the
// application needs to handle and respond to each accordingly.
void PollEvents_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Poll OpenXR for a new event.
  XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
  auto XrPollEvents = [&]() -> bool {
    eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    return xrPollEvent(xrRes->xrInstance, &eventData) == XR_SUCCESS;
  };

  while (XrPollEvents()) {
    switch (eventData.type) {
      // Log the number of lost events from the runtime.
      // The event queue has overflowed and some events were lost.
      case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
        XrEventDataEventsLost *eventsLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);
        std::cout << "OPENXR: Events Lost: " << eventsLost->lostEventCount << std::endl;
        break;
      }
      // Log that an instance loss is pending and shutdown the application.
      // The application is about to lose the instance.
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
        XrEventDataInstanceLossPending *instanceLossPending
            = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
        std::cout << "OPENXR: Instance Loss Pending at: " << instanceLossPending->lossTime
                  << std::endl;
        xrRes->sessionRunning = false;
        xrRes->applicationRunning = false;
        break;
      }
      // Log that the interaction profile has changed.
      // The active input form factor for one or more top-level user paths has changed.
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
        XrEventDataInteractionProfileChanged *interactionProfileChanged
            = reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);
        std::cout << "OPENXR: Interaction Profile changed for Session: "
                  << interactionProfileChanged->session << std::endl;
        if (interactionProfileChanged->session != xrRes->session) {
          std::cout << "XrEventDataInteractionProfileChanged for unknown Session" << std::endl;
          break;
        }
        break;
      }
      // Log that there's a reference space change pending.
      // The runtime will begin operating with updated space bounds.
      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
        XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending
            = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);
        std::cout << "OPENXR: Reference Space Change pending for Session: "
                  << referenceSpaceChangePending->session << std::endl;
        if (referenceSpaceChangePending->session != xrRes->session) {
          std::cout << "XrEventDataReferenceSpaceChangePending for unknown Session" << std::endl;
          break;
        }
        break;
      }
      // Session State changes:
      // The application has changed its lifecycle state.
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
        XrEventDataSessionStateChanged *sessionStateChanged
            = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);
        if (sessionStateChanged->session != xrRes->session) {
          std::cout << "XrEventDataSessionStateChanged for unknown Session" << std::endl;
          break;
        }

        if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
          // SessionState is ready. Begin the XrSession using the XrViewConfigurationType.
          XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
          sessionBeginInfo.primaryViewConfigurationType = xrRes->viewConfiguration;
          OPENXR_CHECK(xrRes->xrInstance, xrBeginSession(xrRes->session, &sessionBeginInfo),
                       "Failed to begin Session.");
          xrRes->sessionRunning = true;
        }
        if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
          // SessionState is stopping. End the XrSession.
          OPENXR_CHECK(xrRes->xrInstance, xrEndSession(xrRes->session), "Failed to end Session.");
          xrRes->sessionRunning = false;
        }
        if (sessionStateChanged->state == XR_SESSION_STATE_EXITING) {
          // SessionState is exiting. Exit the application.
          xrRes->sessionRunning = false;
          xrRes->applicationRunning = false;
        }
        if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING) {
          // SessionState is loss pending. Exit the application.
          // It's possible to try a reestablish an XrInstance and XrSession, but we will simply exit
          // here.
          xrRes->sessionRunning = false;
          xrRes->applicationRunning = false;
        }
        // Store state for reference across the application.
        xrRes->sessionState = sessionStateChanged->state;
        break;
      }
      default: {
        break;
      }
    }
  }
}

void GetViewConfigurationViews_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }

  // Gets the View Configuration Types. The first call gets the count of the array that will be
  // returned. The next call fills out the array.
  uint32_t viewConfigurationCount = 0;
  OPENXR_CHECK(xrRes->xrInstance,
               xrEnumerateViewConfigurations(xrRes->xrInstance, xrRes->systemID, 0,
                                             &viewConfigurationCount, nullptr),
               "Failed to enumerate View Configurations.");
  xrRes->viewConfigurations.resize(viewConfigurationCount);
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateViewConfigurations(xrRes->xrInstance, xrRes->systemID, viewConfigurationCount,
                                    &viewConfigurationCount, xrRes->viewConfigurations.data()),
      "Failed to enumerate View Configurations.");

  // Pick the first application supported View Configuration Type con supported by the hardware.
  for (const XrViewConfigurationType &viewConfiguration : xrRes->applicationViewConfigurations) {
    if (std::find(xrRes->viewConfigurations.begin(), xrRes->viewConfigurations.end(),
                  viewConfiguration)
        != xrRes->viewConfigurations.end()) {
      xrRes->viewConfiguration = viewConfiguration;
      break;
    }
  }
  if (xrRes->viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM) {
    std::cerr << "Failed to find a view configuration type. Defaulting to "
                 "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO."
              << std::endl;
    xrRes->viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  }

  // Gets the View Configuration Views. The first call gets the count of the array that will be
  // returned. The next call fills out the array.
  uint32_t viewConfigurationViewCount = 0;
  OPENXR_CHECK(xrRes->xrInstance,
               xrEnumerateViewConfigurationViews(xrRes->xrInstance, xrRes->systemID,
                                                 xrRes->viewConfiguration, 0,
                                                 &viewConfigurationViewCount, nullptr),
               "Failed to enumerate ViewConfiguration Views.");
  xrRes->viewConfigurationViews.resize(viewConfigurationViewCount,
                                       {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateViewConfigurationViews(
          xrRes->xrInstance, xrRes->systemID, xrRes->viewConfiguration, viewConfigurationViewCount,
          &viewConfigurationViewCount, xrRes->viewConfigurationViews.data()),
      "Failed to enumerate ViewConfiguration Views.");
}

// TODO: move out
// XR_DOCS_TAG_BEGIN_GraphicsAPI_OpenGL_AllocateSwapchainImageData

XrSwapchainImageBaseHeader *AllocateSwapchainImageData(Resources &res, XrSwapchain swapchain,
                                                       SwapchainType type, uint32_t count) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return nullptr;
  }
  xrRes->swapchainImagesMap[swapchain].first = type;
  xrRes->swapchainImagesMap[swapchain].second.resize(count, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
  return reinterpret_cast<XrSwapchainImageBaseHeader *>(
      xrRes->swapchainImagesMap[swapchain].second.data());
};
// XR_DOCS_TAG_END_GraphicsAPI_OpenGL_AllocateSwapchainImageData
void *CreateImageView(const ImageViewCreateInfo &imageViewCI, XrResource *xrRes) {
  GLuint framebuffer = 0;
  glGenFramebuffers(1, &framebuffer);

  GLenum attachment = imageViewCI.aspect == ImageViewCreateInfo::Aspect::COLOR_BIT
                          ? GL_COLOR_ATTACHMENT0
                          : GL_DEPTH_ATTACHMENT;

  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY) {
    glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, attachment,
                                     (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel,
                                     imageViewCI.baseArrayLayer, imageViewCI.layerCount);
  } else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D) {
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
                           (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
  } else {
    DEBUG_BREAK;
    std::cout << "ERROR: OPENGL: Unknown ImageView View type." << std::endl;
  }

  GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  if (result != GL_FRAMEBUFFER_COMPLETE) {
    DEBUG_BREAK;
    std::cout << "ERROR: OPENGL: Framebuffer is not complete." << std::endl;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  xrRes->imageViews[framebuffer] = imageViewCI;
  return (void *)(uint64_t)framebuffer;
}

void DestroyImageView(void *imageView, XrResource *xrRes) {
  GLuint framebuffer = (GLuint)(uint64_t)imageView;
  xrRes->imageViews.erase(framebuffer);
  glDeleteFramebuffers(1, &framebuffer);
  imageView = nullptr;
}

void CreateSwapchains_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Get the supported swapchain formats as an array of int64_t and ordered by runtime preference.
  uint32_t formatCount = 0;
  OPENXR_CHECK(xrRes->xrInstance,
               xrEnumerateSwapchainFormats(xrRes->session, 0, &formatCount, nullptr),
               "Failed to enumerate Swapchain Formats");
  std::vector<int64_t> formats(formatCount);
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateSwapchainFormats(xrRes->session, formatCount, &formatCount, formats.data()),
      "Failed to enumerate Swapchain Formats");
  // if (m_graphicsAPI->SelectDepthSwapchainFormat(formats) == 0) {
  //   std::cerr << "Failed to find depth format for Swapchain." << std::endl;
  //   DEBUG_BREAK;
  // }

  // SelectDepthSwapchainFormat
  const std::vector<int64_t> &supportSwapchainFormats{GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT32,
                                                      GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16};

  const std::vector<int64_t>::const_iterator &swapchainFormatIt
      = std::find_first_of(formats.begin(), formats.end(), std::begin(supportSwapchainFormats),
                           std::end(supportSwapchainFormats));
  if (swapchainFormatIt == formats.end()) {
    std::cerr << "ERROR: Unable to find supported Depth Swapchain Format" << std::endl;
    DEBUG_BREAK;
  }

  // Resize the SwapchainInfo to match the number of view in the View Configuration.
  xrRes->colorSwapchainInfos.resize(xrRes->viewConfigurationViews.size());
  xrRes->depthSwapchainInfos.resize(xrRes->viewConfigurationViews.size());

  for (size_t i = 0; i < xrRes->viewConfigurationViews.size(); i++) {
    SwapchainInfo &colorSwapchainInfo = xrRes->colorSwapchainInfos[i];
    SwapchainInfo &depthSwapchainInfo = xrRes->depthSwapchainInfos[i];

    // Fill out an XrSwapchainCreateInfo structure and create an XrSwapchain.
    // Color.
    XrSwapchainCreateInfo swapchainCI{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainCI.createFlags = 0;
    swapchainCI.usageFlags
        = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    const std::vector<int64_t> &supportedColorSwapchainFormats{
        GL_RGB10_A2,
        GL_RGBA16F,
        // The two below should only be used as a fallback, as they are linear color formats without
        // enough bits for color depth, thus leading to banding.
        GL_RGBA8,
        GL_RGBA8_SNORM,
    };
    const std::vector<int64_t>::const_iterator &swapchainFormatIt_color = std::find_first_of(
        formats.begin(), formats.end(), std::begin(supportedColorSwapchainFormats),
        std::end(supportedColorSwapchainFormats));
    if (swapchainFormatIt_color == formats.end()) {
      std::cerr << "ERROR: Unable to find supported Color Swapchain Format" << std::endl;
      DEBUG_BREAK;
    }
    swapchainCI.format = *swapchainFormatIt_color;
    swapchainCI.sampleCount
        = xrRes->viewConfigurationViews[i]
              .recommendedSwapchainSampleCount;  // Use the recommended values from the
                                                 // XrViewConfigurationView.
    swapchainCI.width = xrRes->viewConfigurationViews[i].recommendedImageRectWidth;
    swapchainCI.height = xrRes->viewConfigurationViews[i].recommendedImageRectHeight;
    swapchainCI.faceCount = 1;
    swapchainCI.arraySize = 1;
    swapchainCI.mipCount = 1;
    OPENXR_CHECK(xrRes->xrInstance,
                 xrCreateSwapchain(xrRes->session, &swapchainCI, &colorSwapchainInfo.swapchain),
                 "Failed to create Color Swapchain");
    colorSwapchainInfo.swapchainFormat
        = swapchainCI.format;  // Save the swapchain format for later use.

    // Depth.
    swapchainCI.createFlags = 0;
    swapchainCI.usageFlags
        = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    const std::vector<int64_t> &supportedDepthSwapchainFormats{
        GL_DEPTH_COMPONENT32F,
        GL_DEPTH_COMPONENT32,
        GL_DEPTH_COMPONENT24,
        GL_DEPTH_COMPONENT16,
    };
    const std::vector<int64_t>::const_iterator &swapchainFormatIt_depth = std::find_first_of(
        formats.begin(), formats.end(), std::begin(supportedDepthSwapchainFormats),
        std::end(supportedDepthSwapchainFormats));
    if (swapchainFormatIt_depth == formats.end()) {
      std::cerr << "ERROR: Unable to find supported Depth Swapchain Format" << std::endl;
      DEBUG_BREAK;
    }
    swapchainCI.format = *swapchainFormatIt_depth;
    swapchainCI.sampleCount
        = xrRes->viewConfigurationViews[i]
              .recommendedSwapchainSampleCount;  // Use the recommended values from the
                                                 // XrViewConfigurationView.
    swapchainCI.width = xrRes->viewConfigurationViews[i].recommendedImageRectWidth;
    swapchainCI.height = xrRes->viewConfigurationViews[i].recommendedImageRectHeight;
    swapchainCI.faceCount = 1;
    swapchainCI.arraySize = 1;
    swapchainCI.mipCount = 1;
    OPENXR_CHECK(xrRes->xrInstance,
                 xrCreateSwapchain(xrRes->session, &swapchainCI, &depthSwapchainInfo.swapchain),
                 "Failed to create Depth Swapchain");
    depthSwapchainInfo.swapchainFormat
        = swapchainCI.format;  // Save the swapchain format for later use.

    // Get the number of images in the color/depth swapchain and allocate Swapchain image data via
    // GraphicsAPI to store the returned array.
    uint32_t colorSwapchainImageCount = 0;
    OPENXR_CHECK(xrRes->xrInstance,
                 xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0,
                                            &colorSwapchainImageCount, nullptr),
                 "Failed to enumerate Color Swapchain Images.");
    XrSwapchainImageBaseHeader *colorSwapchainImages = AllocateSwapchainImageData(
        res, colorSwapchainInfo.swapchain, SwapchainType::COLOR, colorSwapchainImageCount);
    OPENXR_CHECK(xrRes->xrInstance,
                 xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount,
                                            &colorSwapchainImageCount, colorSwapchainImages),
                 "Failed to enumerate Color Swapchain Images.");

    uint32_t depthSwapchainImageCount = 0;
    OPENXR_CHECK(xrRes->xrInstance,
                 xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0,
                                            &depthSwapchainImageCount, nullptr),
                 "Failed to enumerate Depth Swapchain Images.");

    XrSwapchainImageBaseHeader *depthSwapchainImages = AllocateSwapchainImageData(
        res, depthSwapchainInfo.swapchain, SwapchainType::DEPTH, depthSwapchainImageCount);
    OPENXR_CHECK(xrRes->xrInstance,
                 xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount,
                                            &depthSwapchainImageCount, depthSwapchainImages),
                 "Failed to enumerate Depth Swapchain Images.");

    // Per image in the swapchains, fill out a GraphicsAPI::ImageViewCreateInfo structure and create
    // a color/depth image view.
    for (uint32_t j = 0; j < colorSwapchainImageCount; j++) {
      ImageViewCreateInfo imageViewCI;
      // m_graphicsAPI->GetSwapchainImageData
      imageViewCI.image = (void *)(uint64_t)xrRes->swapchainImagesMap[colorSwapchainInfo.swapchain]
                              .second[j]
                              .image;
      imageViewCI.type = ImageViewCreateInfo::Type::RTV;
      imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
      imageViewCI.format = colorSwapchainInfo.swapchainFormat;
      imageViewCI.aspect = ImageViewCreateInfo::Aspect::COLOR_BIT;
      imageViewCI.baseMipLevel = 0;
      imageViewCI.levelCount = 1;
      imageViewCI.baseArrayLayer = 0;
      imageViewCI.layerCount = 1;
      // m_graphicsAPI->CreateImageView

      colorSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI, xrRes));
    }
    for (uint32_t j = 0; j < depthSwapchainImageCount; j++) {
      ImageViewCreateInfo imageViewCI;
      // GetSwapchainImageData
      imageViewCI.image = (void *)(uint64_t)xrRes->swapchainImagesMap[depthSwapchainInfo.swapchain]
                              .second[j]
                              .image;
      imageViewCI.type = ImageViewCreateInfo::Type::DSV;
      imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
      imageViewCI.format = depthSwapchainInfo.swapchainFormat;
      imageViewCI.aspect = ImageViewCreateInfo::Aspect::DEPTH_BIT;
      imageViewCI.baseMipLevel = 0;
      imageViewCI.levelCount = 1;
      imageViewCI.baseArrayLayer = 0;
      imageViewCI.layerCount = 1;
      depthSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI, xrRes));
    }
  }
}

void DestroySwapchains_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Per view in the view configuration:
  for (size_t i = 0; i < xrRes->viewConfigurationViews.size(); i++) {
    SwapchainInfo &colorSwapchainInfo = xrRes->colorSwapchainInfos[i];
    SwapchainInfo &depthSwapchainInfo = xrRes->depthSwapchainInfos[i];

    // Destroy the color and depth image views from GraphicsAPI.
    for (void *&imageView : colorSwapchainInfo.imageViews) {
      // m_graphicsAPI->DestroyImageView(imageView);
      DestroyImageView(imageView, xrRes);
    }
    for (void *&imageView : depthSwapchainInfo.imageViews) {
      // m_graphicsAPI->DestroyImageView(imageView);
      DestroyImageView(imageView, xrRes);
    }

    // Free the Swapchain Image Data.
    // m_graphicsAPI->FreeSwapchainImageData(colorSwapchainInfo.swapchain);
    xrRes->swapchainImagesMap[colorSwapchainInfo.swapchain].second.clear();
    xrRes->swapchainImagesMap.erase(colorSwapchainInfo.swapchain);
    // m_graphicsAPI->FreeSwapchainImageData(depthSwapchainInfo.swapchain);
    xrRes->swapchainImagesMap[depthSwapchainInfo.swapchain].second.clear();
    xrRes->swapchainImagesMap.erase(depthSwapchainInfo.swapchain);

    // Destroy the swapchains.
    OPENXR_CHECK(xrRes->xrInstance, xrDestroySwapchain(colorSwapchainInfo.swapchain),
                 "Failed to destroy Color Swapchain");
    OPENXR_CHECK(xrRes->xrInstance, xrDestroySwapchain(depthSwapchainInfo.swapchain),
                 "Failed to destroy Depth Swapchain");
  }
}

// Building Render Loop

void GetEnvironmentBlendModes_system(Resources &res, entt::registry &world) {
  // Retrieves the available blend modes. The first call gets the count of the array that will be
  // returned. The next call fills out the array.
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  uint32_t environmentBlendModeCount = 0;
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateEnvironmentBlendModes(xrRes->xrInstance, xrRes->systemID, xrRes->viewConfiguration,
                                       0, &environmentBlendModeCount, nullptr),
      "Failed to enumerate EnvironmentBlend Modes.");
  xrRes->environmentBlendModes.resize(environmentBlendModeCount);
  OPENXR_CHECK(
      xrRes->xrInstance,
      xrEnumerateEnvironmentBlendModes(xrRes->xrInstance, xrRes->systemID, xrRes->viewConfiguration,
                                       environmentBlendModeCount, &environmentBlendModeCount,
                                       xrRes->environmentBlendModes.data()),
      "Failed to enumerate EnvironmentBlend Modes.");

  // Pick the first application supported blend mode supported by the hardware.
  for (const XrEnvironmentBlendMode &environmentBlendMode :
       xrRes->applicationEnvironmentBlendModes) {
    if (std::find(xrRes->environmentBlendModes.begin(), xrRes->environmentBlendModes.end(),
                  environmentBlendMode)
        != xrRes->environmentBlendModes.end()) {
      xrRes->environmentBlendMode = environmentBlendMode;
      break;
    }
  }
  if (xrRes->environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM) {
    std::cerr
        << "Failed to find a compatible blend mode. Defaulting to XR_ENVIRONMENT_BLEND_MODE_OPAQUE."
        << std::endl;
    xrRes->environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  }
}
void CreateReferenceSpace_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Fill out an XrReferenceSpaceCreateInfo structure and create a reference XrSpace, specifying a
  // Local space with an identity pose as the origin.
  XrReferenceSpaceCreateInfo referenceSpaceCI{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  referenceSpaceCI.poseInReferenceSpace = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
  OPENXR_CHECK(xrRes->xrInstance,
               xrCreateReferenceSpace(xrRes->session, &referenceSpaceCI, &xrRes->localSpace),
               "Failed to create ReferenceSpace.");
}
void DestroyReferenceSpace_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  // Destroy the reference XrSpace.
  OPENXR_CHECK(xrRes->xrInstance, xrDestroySpace(xrRes->localSpace), "Failed to destroy Space.");
}

bool RenderLayer(Resources &res, entt::registry &world) {
  // XrResource *xrRes, RenderLayerInfo &renderLayerInfo
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return false;
  }

  // RenderLayerInfo renderlarInfo;
  // xrRes->renderLayerInfo.predictedDisplayTime = 0;
  // Locate the views from the view configuration within the (reference) space at the display time.
  std::vector<XrView> views(xrRes->viewConfigurationViews.size(), {XR_TYPE_VIEW});

  XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position
                                              // and/or orientation is valid and/or tracked.
  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = xrRes->viewConfiguration;
  viewLocateInfo.displayTime = xrRes->renderLayerInfo.predictedDisplayTime;
  viewLocateInfo.space = xrRes->localSpace;
  uint32_t viewCount = 0;
  XrResult result = xrLocateViews(xrRes->session, &viewLocateInfo, &viewState,
                                  static_cast<uint32_t>(views.size()), &viewCount, views.data());
  if (result != XR_SUCCESS) {
    std::cerr << "Failed to locate Views." << std::endl;
    return false;
  }

  // Resize the layer projection views to match the view count. The layer projection views are used
  // in the layer projection.
  xrRes->renderLayerInfo.layerProjectionViews.resize(viewCount,
                                                     {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

  // Per view in the view configuration:
  for (uint32_t i = 0; i < viewCount; i++) {
    SwapchainInfo &colorSwapchainInfo = xrRes->colorSwapchainInfos[i];
    SwapchainInfo &depthSwapchainInfo = xrRes->depthSwapchainInfos[i];

    // Acquire and wait for an image from the swapchains.
    // Get the image index of an image in the swapchains.
    // The timeout is infinite.
    uint32_t colorImageIndex = 0;
    uint32_t depthImageIndex = 0;
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    OPENXR_CHECK(
        xrRes->xrInstance,
        xrAcquireSwapchainImage(colorSwapchainInfo.swapchain, &acquireInfo, &colorImageIndex),
        "Failed to acquire Image from the Color Swapchian");
    OPENXR_CHECK(
        xrRes->xrInstance,
        xrAcquireSwapchainImage(depthSwapchainInfo.swapchain, &acquireInfo, &depthImageIndex),
        "Failed to acquire Image from the Depth Swapchian");

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    OPENXR_CHECK(xrRes->xrInstance, xrWaitSwapchainImage(colorSwapchainInfo.swapchain, &waitInfo),
                 "Failed to wait for Image from the Color Swapchain");
    OPENXR_CHECK(xrRes->xrInstance, xrWaitSwapchainImage(depthSwapchainInfo.swapchain, &waitInfo),
                 "Failed to wait for Image from the Depth Swapchain");

    // Get the width and height and construct the viewport and scissors.
    const uint32_t &width = xrRes->viewConfigurationViews[i].recommendedImageRectWidth;
    const uint32_t &height = xrRes->viewConfigurationViews[i].recommendedImageRectHeight;
    // GraphicsAPI::Viewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
    // GraphicsAPI::Rect2D scissor = {{(int32_t)0, (int32_t)0}, {width, height}};
    float nearZ = 0.05f;
    float farZ = 100.0f;

    // Fill out the XrCompositionLayerProjectionView structure specifying the pose and fov from the
    // view. This also associates the swapchain image with this layer projection view.
    xrRes->renderLayerInfo.layerProjectionViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    xrRes->renderLayerInfo.layerProjectionViews[i].pose = views[i].pose;
    xrRes->renderLayerInfo.layerProjectionViews[i].fov = views[i].fov;
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.swapchain
        = colorSwapchainInfo.swapchain;
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.x = 0;
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.y = 0;
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.width
        = static_cast<int32_t>(width);
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.height
        = static_cast<int32_t>(height);
    xrRes->renderLayerInfo.layerProjectionViews[i].subImage.imageArrayIndex
        = 0;  // Useful for multiview rendering.

    // Rendering code to clear the color and depth image views.
    // m_graphicsAPI->BeginRendering();
    // auto BeginRendering = [](XrResource *xrRes) {
    //   glGenVertexArrays(1, &xrRes->vertexArray);
    //   glBindVertexArray(xrRes->vertexArray);

    // glGenFramebuffers(1, &xrRes->setFramebuffer);
    // glBindFramebuffer(GL_FRAMEBUFFER, xrRes->setFramebuffer);
    // };
    // BeginRendering(xrRes);

    auto frameBuffer = res.get<VIVID::FrameBuffer>();
    frameBuffer->m_Image = colorSwapchainInfo.imageViews[colorImageIndex];
    frameBuffer->m_Width = width;
    frameBuffer->m_Height = height;
    // frameBuffer->Invalidate();

    VIVID::ClearColor_system(res, world);
    auto ClearColor = [](XrResource *xrRes, void *imageView, float r, float g, float b, float a) {
      glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)(uint64_t)imageView);
      glClearColor(r, g, b, a);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };
    // if (xrRes->environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
    //   // VR mode use a background color.
    //   // m_graphicsAPI->ClearColor(colorSwapchainInfo.imageViews[colorImageIndex], 0.17f, 0.17f,
    //   // 0.17f,
    //   //                           1.00f);

    //   ClearColor(xrRes, colorSwapchainInfo.imageViews[colorImageIndex], 0.0f, 0.8f,
    //   0.17f, 1.00f);
    // } else {
    //   // In AR mode make the background color black.
    //   // m_graphicsAPI->ClearColor(colorSwapchainInfo.imageViews[colorImageIndex], 0.00f, 0.00f,
    //   // 0.00f,
    //   //                           1.00f);
    //   ClearColor(xrRes, colorSwapchainInfo.imageViews[colorImageIndex], 0.00f, 0.00f,
    //   0.00f, 1.00f);
    // }
    // m_graphicsAPI->ClearDepth(depthSwapchainInfo.imageViews[depthImageIndex], 1.0f);
    auto ClearDepth = [](XrResource *xrRes, void *imageView, float d) {
      glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)(uint64_t)imageView);
      glClearDepth(d);
      glClear(GL_DEPTH_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };
    ClearDepth(xrRes, depthSwapchainInfo.imageViews[depthImageIndex], 1.0f);
    // m_graphicsAPI->EndRendering();
    // auto EndRendering = [](XrResource *xrRes) {
    //   glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //   glDeleteFramebuffers(1, &xrRes->setFramebuffer);
    //   xrRes->setFramebuffer = 0;

    //   glBindVertexArray(0);
    //   glDeleteVertexArrays(1, &xrRes->vertexArray);
    //   xrRes->vertexArray = 0;
    // };
    // EndRendering(xrRes);

    // Give the swapchain image back to OpenXR, allowing the compositor to use the image.
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    OPENXR_CHECK(xrRes->xrInstance,
                 xrReleaseSwapchainImage(colorSwapchainInfo.swapchain, &releaseInfo),
                 "Failed to release Image back to the Color Swapchain");
    OPENXR_CHECK(xrRes->xrInstance,
                 xrReleaseSwapchainImage(depthSwapchainInfo.swapchain, &releaseInfo),
                 "Failed to release Image back to the Depth Swapchain");
  }

  // Fill out the XrCompositionLayerProjection structure for usage with xrEndFrame().
  xrRes->renderLayerInfo.layerProjection.layerFlags
      = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
        | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
  xrRes->renderLayerInfo.layerProjection.space = xrRes->localSpace;
  xrRes->renderLayerInfo.layerProjection.viewCount
      = static_cast<uint32_t>(xrRes->renderLayerInfo.layerProjectionViews.size());
  xrRes->renderLayerInfo.layerProjection.views = xrRes->renderLayerInfo.layerProjectionViews.data();

  return true;
}

// For each frame, we sequence through the three primary functions: xrWaitFrame, xrBeginFrame and
// xrEndFrame. These functions wrap around our rendering code and communicate to the OpenXR runtime
// that we are rendering and that we need to synchronize with the XR compositor.
void RenderFrame_system(Resources &res, entt::registry &world) {
  // Get the XrFrameState for timing and rendering info.
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }

  if (xrRes->sessionRunning == false) {
    return;
  }

  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  OPENXR_CHECK(xrRes->xrInstance, xrWaitFrame(xrRes->session, &frameWaitInfo, &frameState),
               "Failed to wait for XR Frame.");

  // Tell the OpenXR compositor that the application is beginning the frame.
  XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
  OPENXR_CHECK(xrRes->xrInstance, xrBeginFrame(xrRes->session, &frameBeginInfo),
               "Failed to begin the XR Frame.");

  // Variables for rendering and layer composition.
  bool rendered = false;

  // RenderLayerInfo renderLayerInfo;
  xrRes->renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

  // Check that the session is active and that we should render.
  bool sessionActive = (xrRes->sessionState == XR_SESSION_STATE_SYNCHRONIZED
                        || xrRes->sessionState == XR_SESSION_STATE_VISIBLE
                        || xrRes->sessionState == XR_SESSION_STATE_FOCUSED);
  if (sessionActive && frameState.shouldRender) {
    // Render the stereo image and associate one of swapchain images with the
    // XrCompositionLayerProjection structure.
    // rendered = RenderLayer(xrRes, renderLayerInfo);
    rendered = RenderLayer(res, world);
    xrRes->renderLayerInfo.layers.clear();
    if (rendered) {
      xrRes->renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(
          &xrRes->renderLayerInfo.layerProjection));
    }
  }

  // Tell OpenXR that we are finished with this frame; specifying its display time, environment
  // blending and layers.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = frameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = xrRes->environmentBlendMode;
  frameEndInfo.layerCount = static_cast<uint32_t>(xrRes->renderLayerInfo.layers.size());
  frameEndInfo.layers = xrRes->renderLayerInfo.layers.data();
  OPENXR_CHECK(xrRes->xrInstance, xrEndFrame(xrRes->session, &frameEndInfo),
               "Failed to end the XR Frame.");
}

int main() {
  // auto &app = App::new_app().add_plugin<DefaultPlugin>().add_plugin<RenderPlugin>();
  auto &app = App::new_app().add_plugin<DefaultPlugin>();
  app.add_system(ScheduleLabel::Startup, CreateInstance_system);
  app.add_system(ScheduleLabel::Startup, CreateDebugMessenger_system);
  app.add_system(ScheduleLabel::Startup, GetInstanceProperties_system);
  app.add_system(ScheduleLabel::Startup, GetSystemId_system);

  app.add_system(ScheduleLabel::Startup, GetViewConfigurationViews_system);
  app.add_system(ScheduleLabel::Startup, GetEnvironmentBlendModes_system);

  app.add_system(ScheduleLabel::Startup, InitializeDevice_system);
  app.add_system(ScheduleLabel::Startup, CreateSession_system);
  app.add_system(ScheduleLabel::Startup, CreateReferenceSpace_system);
  app.add_system(ScheduleLabel::Startup, CreateSwapchains_system);
  app.add_system(ScheduleLabel::Startup, VIVID::Init_system);

  app.add_system(ScheduleLabel::Update, PollEvents_system);
  app.add_system(ScheduleLabel::Update, RenderFrame_system);

  app.add_system(ScheduleLabel::Shutdown, DestroyInstance_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyDebugMessenger_system);
  app.add_system(ScheduleLabel::Shutdown, DestroySession_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyReferenceSpace_system);
  app.add_system(ScheduleLabel::Shutdown, DestroySwapchains_system);

  app.run();

  return 0;
}