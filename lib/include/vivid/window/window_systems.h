#pragma once

#include <flecs.h>
#include <vivid/log/log.h>

#include "window_component.h"

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

namespace VIVID {
namespace WINDOW {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// Window Systems Module - manages window lifecycle, events, and updates
struct WindowSystems {
  // Constructor - Register module and systems
  WindowSystems(flecs::world& world) {
    // Display module overview only in Debug mode to reduce verbosity
    VIVID_LOG_MODULE_HEADER("WINDOW SYSTEMS", "🔧", {
      VIVID_LOG_MODULE_INFO("📦 Module: WindowSystems");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("📋 DEPENDENCIES:");
      VIVID_LOG_MODULE_INFO("└── 📦 WindowComponents (imported)");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("🏗️  SYSTEMS REGISTRATION:");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("📍 PHASE: OnStart");
      VIVID_LOG_MODULE_INFO("├── 🔄 WindowInitialization");
      VIVID_LOG_MODULE_INFO("│   ├── Queries: WindowContext");
      VIVID_LOG_MODULE_INFO("│   └── Executes: windowInitImpl()");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("📍 PHASE: OnUpdate");
      VIVID_LOG_MODULE_INFO("├── 🔄 WindowUpdate");
      VIVID_LOG_MODULE_INFO("│   ├── Queries: WindowContext");
      VIVID_LOG_MODULE_INFO("│   └── Executes: windowUpdateImpl()");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
      VIVID_LOG_MODULE_INFO("└── 🔄 WindowCleanup");
      VIVID_LOG_MODULE_INFO("    ├── Queries: WindowContext");
      VIVID_LOG_MODULE_INFO("    └── Executes: windowCleanupImpl()");
      VIVID_LOG_MODULE_INFO("");
      VIVID_LOG_MODULE_INFO("💾 RESOURCES:");
      VIVID_LOG_MODULE_INFO("└── 🔹 WindowContext (singleton)");
    });

    // Register module
    world.module<WindowSystems>();

    // Import components module
    world.import <WindowComponents>();

    // Simplified logging to reduce verbosity
    VIVID_LOG_SYSTEM("Setting WindowContext singleton...");
    world.set<WindowContext>({});
    VIVID_LOG_SUCCESS("WindowContext singleton created");

    // Verify the singleton was set
    if (world.has<WindowContext>()) {
      VIVID_LOG_SUCCESS("WindowContext singleton verified");
    } else {
      VIVID_LOG_ERROR("WindowContext singleton NOT found!");
    }

    // Register systems
    VIVID_LOG_SYSTEM("Registering WindowInitialization system...");
    world.system<WindowContext>("WindowInitialization").kind(flecs::OnStart).each(windowInitImpl);
    VIVID_LOG_SUCCESS("WindowInitialization system registered");

    VIVID_LOG_SYSTEM("Registering WindowUpdate system...");
    world.system<WindowContext>("WindowUpdate").kind(flecs::OnUpdate).each(windowUpdateImpl);
    VIVID_LOG_SUCCESS("WindowUpdate system registered");

    VIVID_LOG_SYSTEM("Registering WindowCleanup system...");
    world.system<WindowContext>("WindowCleanup").kind<ShutdownPhase>().each(windowCleanupImpl);
    VIVID_LOG_SUCCESS("WindowCleanup system registered");

    VIVID_LOG_SUCCESS("WindowSystems module registration completed!");
  }

private:
  // Static member functions for system implementations
  static void windowInitImpl(flecs::entity e, WindowContext& windowContext);
  // static void windowEventProcessingImpl(flecs::iter& it);
  static void windowUpdateImpl(flecs::entity e, WindowContext& windowContext);
  static void windowCleanupImpl(flecs::entity e, WindowContext& windowContext);
};

}  // namespace WINDOW
}  // namespace VIVID
