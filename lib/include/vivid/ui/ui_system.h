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
#define VIVID_LOG_SUCCESS(msg) VividLogger::app_info("✅ " msg);
#define VIVID_LOG_ERROR(msg) VividLogger::app_error("❌ " msg);

namespace VIVID {
namespace UI {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(WINDOW::WindowContext& windowContext, RENDER::WebGPUContext& webgpuRes);
  static void processImGuiEventImpl(flecs::entity e, VIVID::APP::EventQueues& eventQueues);
  static void showImGuiDemoImpl(flecs::iter& it);
  static void renderImGuiImpl(flecs::entity e, RENDER::WebGPUContext& webgpuRes);
  static void shutDownImGuiImpl(flecs::iter& it);
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

  // Import render module //TODO: Test duplicate import
  // world.import <RENDER::RenderComponents>();
  // world.import <RENDER::RenderSystems>();

  // Register systems
  VIVID_LOG_SYSTEM("Registering InitImGui system...");

  // Debug: Check if singletons exist (only in debug builds)
#ifndef NDEBUG
  if (world.has<WINDOW::WindowContext>()) {
    VividLogger::app_info("✅ WindowContext singleton exists for InitImGui");
  } else {
    VividLogger::app_error("❌ WindowContext singleton NOT found for InitImGui!");
  }

  if (world.has<RENDER::WebGPUContext>()) {
    VividLogger::app_info("✅ WebGPUContext singleton exists for InitImGui");
  } else {
    VividLogger::app_error("❌ WebGPUContext singleton NOT found for InitImGui!");
  }
#endif

  world.system<WINDOW::WindowContext, RENDER::WebGPUContext>("InitImGui")
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

  // 4. Render ImGui - integrated with RenderSystems pipeline
  VIVID_LOG_SYSTEM("Looking up RenderUIPhase from RenderSystems...");
  flecs::entity RenderUIPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderUIPhase");
  if (RenderUIPhase.id() == 0) {
    VIVID_LOG_ERROR(
        "RenderUIPhase not found! Make sure RenderSystems is imported before UISystems.");
    return;
  } else {
    VIVID_LOG_SUCCESS("RenderUIPhase found: %s", RenderUIPhase.name());
  }
  // 3.5 Render ImGui draw data within the render pass
  world.system<RENDER::WebGPUContext>("RenderImGui").kind(RenderUIPhase).each(renderImGuiImpl);

  // 5. Shutdown ImGui - runs once at shutdown
  world.system("ShutDownImGui").kind<ShutdownPhase>().run(shutDownImGuiImpl);

  VIVID_LOG_SUCCESS("UISystems module registration completed!");
}

}  // namespace UI
}  // namespace VIVID
