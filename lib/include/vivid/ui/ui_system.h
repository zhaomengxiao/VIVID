#pragma once

#include <flecs.h>

#include <flecs/addons/cpp/mixins/pipeline/decl.hpp>

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
#define VIVID_LOG_SUCCESS(msg, ...) VividLogger::app_info("✅ " msg, ##__VA_ARGS__);
#define VIVID_LOG_ERROR(msg) VividLogger::app_error("❌ " msg);

namespace VIVID::UI {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(const WINDOW::WindowContext& windowContext,
                            const RENDER::WebGPUContext& webgpuRes);
  static void processImGuiEventImpl(VIVID::APP::EventQueues& eventQueues);
  static void newFrameImpl(const flecs::iter& it);
  static void renderUIImpl(RENDER::WebGPUContext& webgpuRes);
  static void endFrameImpl(const flecs::iter& it);
  static void shutDownUIImpl(const flecs::iter& it);
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
    VIVID_LOG_MODULE_INFO("🎯 CUSTOM PIPELINE PHASES:");
    VIVID_LOG_MODULE_INFO("├── 📍 NewFramePhase  ← depends_on(PreUpdate)");
    VIVID_LOG_MODULE_INFO("├── 📍 DrawFramePhase  ← depends_on(NewFramePhase)");
    VIVID_LOG_MODULE_INFO("└── 📍 EndFramePhase  ← depends_on(DrawFramePhase)");
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
    VIVID_LOG_MODULE_INFO("└── 🔄 NewFrame");
    VIVID_LOG_MODULE_INFO("    └── Executes: newFrameImpl()");
    VIVID_LOG_MODULE_INFO("└── 🔄 DisplayViewportWindows");
    VIVID_LOG_MODULE_INFO("    └── Executes: displayViewportWindowsImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: EndFramePhase");
    VIVID_LOG_MODULE_INFO("├── 🔄 EndFrame");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: renderUIImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: RenderUIPhase (from RenderSystems)");
    VIVID_LOG_MODULE_INFO("├── 🔄 RenderImGui");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: renderUIImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
    VIVID_LOG_MODULE_INFO("└── 🔄 ShutDownImGui");
    VIVID_LOG_MODULE_INFO("    └── Executes: shutDownUIImpl()");
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

  // Process ImGui events
  world.system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
      .kind(flecs::PreUpdate)
      .each(processImGuiEventImpl);

  // Start UI frame
  world.system("NewFrame").kind(flecs::PreUpdate).run(newFrameImpl);

  // Display viewport windows
  world.system("DisplayViewportWindows").kind(flecs::OnUpdate).run(displayViewportWindowsImpl);

  // End UI frame
  world.system("EndFrame").kind(flecs::PostUpdate).run(endFrameImpl);

  VIVID_LOG_SYSTEM("Looking up RenderUIPhase from RenderSystems...");

  flecs::entity RenderUIPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderUIPhase");
  if (RenderUIPhase.id() == 0) {
    VIVID_LOG_ERROR(
        "RenderUIPhase not found! Make sure RenderSystems is imported before UISystems.");
    return;
  }

  // Render ImGui draw data within the render pass
  world.system<RENDER::WebGPUContext>("RenderImGui").kind(RenderUIPhase).each(renderUIImpl);

  // Shutdown ImGui - runs once at shutdown
  world.system("ShutDownUI").kind<ShutdownPhase>().run(shutDownUIImpl);

  VIVID_LOG_SUCCESS("UISystems module registration completed!");
}

}  // namespace VIVID::UI
