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

#include <GLFW/glfw3.h>

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
};

struct GraphicsAPI_Resource {
  XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
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

// An XrSession encapsulates the state of the application from the perspective of OpenXR. When an
// XrSession is created, it starts in the XrSessionState XR_SESSION_STATE_IDLE. It is up to the
// runtime to provide any updates to the XrSessionState and for the application to query them and
// react to them.
void CreateSession_system(Resources &res, entt::registry &world) {
  XrResource *xrRes = res.get<XrResource>();
  if (!xrRes) {
    std::cerr << "Failed to get XrResource." << std::endl;
    return;
  }
  XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};
  // xrRes->graphicsAPI = std::make_unique<GraphicsAPI_OpenGL>(xrRes->xrInstance, xrRes->systemID);
  // GetGraphicsBinding
  GraphicsAPI_Resource *graphicsAPIRes = res.get<GraphicsAPI_Resource>();
  if (!graphicsAPIRes) {
    graphicsAPIRes = &res.insert<GraphicsAPI_Resource>();
    std::cout << "Created GraphicsAPI_Resource." << std::endl;
  }
  auto *windowRes = res.get<WindowResource>();
  if (!windowRes) {
    std::cerr << "Failed to get WindowResource." << std::endl;
    return;
  }
  graphicsAPIRes->graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
  graphicsAPIRes->graphicsBinding.hDC = GetDC(glfwGetWin32Window(windowRes->window));
  graphicsAPIRes->graphicsBinding.hGLRC = wglGetCurrentContext();

  sessionCI.next = &graphicsAPIRes->graphicsBinding;
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
          sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
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

int main() {
  auto &app = App::new_app().add_plugin<DefaultPlugin>().add_plugin<RenderPlugin>();
  app.add_system(ScheduleLabel::Startup, CreateInstance_system);
  app.add_system(ScheduleLabel::Startup, CreateDebugMessenger_system);
  app.add_system(ScheduleLabel::Startup, GetInstanceProperties_system);
  app.add_system(ScheduleLabel::Startup, GetSystemId_system);
  app.add_system(ScheduleLabel::Startup, CreateSession_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyInstance_system);
  app.add_system(ScheduleLabel::Shutdown, DestroyDebugMessenger_system);
  app.add_system(ScheduleLabel::Shutdown, DestroySession_system);
  app.add_system(ScheduleLabel::Update, PollEvents_system);
  app.run();

  return 0;
}