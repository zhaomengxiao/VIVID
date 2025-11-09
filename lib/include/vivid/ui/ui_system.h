#pragma once

#include <flecs.h>

#include "ui_component.h"
#include "vivid/app/App.h"
#include "vivid/render/render_systems.h"
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
#define VIVID_LOG_SUCCESS(msg, ...) VividLogger::app_info("✅ " msg, __VA_ARGS__);
#define VIVID_LOG_ERROR(msg) VividLogger::app_error("❌ " msg);

namespace VIVID {
namespace UI {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(const WINDOW::WindowContext& windowContext,
                            const RENDER::WebGPUContext& webgpuRes);
  static void processImGuiEventImpl(VIVID::APP::EventQueues& eventQueues);
  static void showImGuiDemoImpl(const flecs::iter& it);
  static void renderImGuiImpl(RENDER::WebGPUContext& webgpuRes);
  static void shutDownImGuiImpl(const flecs::iter& it);
  // Display viewport windows in ImGui
  static void displayViewportWindowsImpl(const flecs::iter& it);
};

// Constructor - Register module and systems
inline UISystems::UISystems(flecs::world& world) {
  // Display module overview only in Debug mode to reduce verbosity
  VIVID_LOG_MODULE_HEADER("UI SYSTEMS", "🖼️", {
    VIVID_LOG_MODULE_INFO("📦 Module: UISystems");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📋 DEPENDENCIES:");
    VIVID_LOG_MODULE_INFO("├── 📦 UIComponents (imported)");
    VIVID_LOG_MODULE_INFO("├── 📦 WindowContext (from WindowSystems)");
    VIVID_LOG_MODULE_INFO("├── 📦 WebGPUContext (from RenderSystems)");
    VIVID_LOG_MODULE_INFO("└── 📦 RenderUIPhase (from RenderSystems pipeline)");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("🏗️  SYSTEMS REGISTRATION:");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: OnStart");
    VIVID_LOG_MODULE_INFO("├── 🔄 InitImGui");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WindowContext, WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: initImGuiImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: PreUpdate");
    VIVID_LOG_MODULE_INFO("├── 🔄 ProcessImGuiEvent");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: EventQueues");
    VIVID_LOG_MODULE_INFO("│   └── Executes: processImGuiEventImpl()");
    VIVID_LOG_MODULE_INFO("└── 🔄 ShowImGuiDemo");
    VIVID_LOG_MODULE_INFO("    └── Executes: showImGuiDemoImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: RenderUIPhase (from RenderSystems)");
    VIVID_LOG_MODULE_INFO("├── 🔄 RenderImGui");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: renderImGuiImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
    VIVID_LOG_MODULE_INFO("└── 🔄 ShutDownImGui");
    VIVID_LOG_MODULE_INFO("    └── Executes: shutDownImGuiImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("💾 INTEGRATION:");
    VIVID_LOG_MODULE_INFO("└── 🔗 Hooks into RenderSystems RenderUIPhase for ImGui rendering");
  });

  VIVID_LOG_SYSTEM("Registering UISystems...");
  // Register module
  world.module<UISystems>();

  // Import components module
  world.import <UIComponents>();

  // Register systems
  VIVID_LOG_SYSTEM("Registering InitImGui system...");

  world.system<const WINDOW::WindowContext, const RENDER::WebGPUContext>("InitImGui")
      .term_at(0)
      .src<WINDOW::WindowContext>()
      .term_at(1)
      .src<RENDER::WebGPUContext>()
      .kind(flecs::OnStart)
      .each(initImGuiImpl);

  VIVID_LOG_SUCCESS("InitImGui system registered successfully");

  // 2. Process ImGui events - runs every frame in PreUpdate
  world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
      .kind(flecs::PreUpdate)
      .each(processImGuiEventImpl);

  // 3. Show ImGui demo - runs every frame in Update
  world.system("ShowImGuiDemo").kind(flecs::PreUpdate).run(showImGuiDemoImpl);

  // 3.5 Display viewport windows in ImGui - runs in PreUpdate before rendering
  world.system("DisplayViewportWindows").kind(flecs::PreUpdate).run(displayViewportWindowsImpl);

  // 4. Render viewport windows to offscreen textures - between RenderPhase and RenderUIPhase
  VIVID_LOG_SYSTEM("Looking up RenderPhase and RenderUIPhase from RenderSystems...");
  flecs::entity RenderPhase = world.lookup("RenderPhase");
  flecs::entity RenderUIPhase = world.lookup("RenderUIPhase");

  // Try alternative lookup if not found (with module prefix)
  if (RenderPhase.id() == 0) {
    RenderPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderPhase");
  }
  if (RenderUIPhase.id() == 0) {
    RenderUIPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderUIPhase");
  }

  if (RenderPhase.id() == 0 || RenderUIPhase.id() == 0) {
    VIVID_LOG_ERROR(
        "RenderPhase or RenderUIPhase not found! Make sure RenderSystems is imported before "
        "UISystems.");
    return;
  } else {
    VIVID_LOG_SUCCESS("RenderPhase found: %s", RenderPhase.name());
    VIVID_LOG_SUCCESS("RenderUIPhase found: %s", RenderUIPhase.name());
  }

  // Create RenderViewportPhase between RenderPhase and RenderUIPhase
  // The execution order will be: RenderPhase -> RenderViewportPhase -> RenderUIPhase
  // We create RenderViewportPhase to depend on RenderPhase, and ensure RenderUIPhase
  // depends on both RenderPhase (existing) and RenderViewportPhase (new)
  // Flecs will automatically order phases correctly based on dependencies
  flecs::entity RenderViewportPhase
      = world.entity("RenderViewportPhase").add(flecs::Phase).depends_on(RenderPhase);
  // Make RenderUIPhase also depend on RenderViewportPhase to ensure correct ordering
  RenderUIPhase.add(flecs::DependsOn, RenderViewportPhase);

  // 4.5 Render ImGui draw data within the render pass
  world.system<RENDER::WebGPUContext>("RenderImGui").kind(RenderUIPhase).each(renderImGuiImpl);

  // 5. Shutdown ImGui - runs once at shutdown
  world.system("ShutDownImGui").kind<ShutdownPhase>().run(shutDownImGuiImpl);

  VIVID_LOG_SUCCESS("UISystems module registration completed!");
}

}  // namespace UI
}  // namespace VIVID
