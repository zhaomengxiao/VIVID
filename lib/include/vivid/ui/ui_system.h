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

namespace vivid::ui {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  explicit UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(const vivid::window::WindowContext& window_context,
                            const vivid::render::WebGPUContext& webgpu_res);
  static void processImGuiEventImpl(vivid::app::EventQueues& event_queues);
  static void newFrameImpl([[maybe_unused]] const flecs::iter& it);
  static void renderUIImpl(vivid::render::WebGPUContext& webgpu_res);
  static void endFrameImpl([[maybe_unused]] const flecs::iter& it);
  static void shutDownUIImpl([[maybe_unused]] const flecs::iter& it);
  // Display viewport windows in ImGui
  static void displayViewportWindowsImpl([[maybe_unused]] const flecs::iter& it);
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
    VIVID_LOG_MODULE_INFO("└── 📦 renderUIPhase (from RenderSystems pipeline)");
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
    VIVID_LOG_MODULE_INFO("📍 PHASE: renderUIPhase (from RenderSystems)");
    VIVID_LOG_MODULE_INFO("├── 🔄 RenderImGui");
    VIVID_LOG_MODULE_INFO("│   ├── Requires: WebGPUContext");
    VIVID_LOG_MODULE_INFO("│   └── Executes: renderUIImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("📍 PHASE: Shutdown");
    VIVID_LOG_MODULE_INFO("└── 🔄 ShutDownImGui");
    VIVID_LOG_MODULE_INFO("    └── Executes: shutDownUIImpl()");
    VIVID_LOG_MODULE_INFO("");
    VIVID_LOG_MODULE_INFO("💾 INTEGRATION:");
    VIVID_LOG_MODULE_INFO("└── 🔗 Hooks into RenderSystems renderUIPhase for ImGui rendering");
  });

  VIVID_LOG_SYSTEM("Registering UISystems...");
  // Register module
  world.module<UISystems>();

  // Import components module
  world.import <UIComponents>();

  // Register systems
  VIVID_LOG_SYSTEM("Registering InitImGui system...");

  world.system<const vivid::window::WindowContext, const vivid::render::WebGPUContext>("InitImGui")
      .term_at(0)
      .src<vivid::window::WindowContext>()
      .term_at(1)
      .src<vivid::render::WebGPUContext>()
      .kind(flecs::OnStart)
      .each(initImGuiImpl);

  VIVID_LOG_SUCCESS("InitImGui system registered successfully");

  // Process ImGui events
  world.system<vivid::app::EventQueues>("ProcessImGuiEvent")
      .kind(flecs::PreUpdate)
      .each(processImGuiEventImpl);

  // Start UI frame
  world.system("NewFrame").kind(flecs::PreUpdate).run(newFrameImpl);

  // Display viewport windows
  world.system("DisplayViewportWindows").kind(flecs::OnUpdate).run(displayViewportWindowsImpl);

  // End UI frame
  world.system("EndFrame").kind(flecs::PostUpdate).run(endFrameImpl);

  VIVID_LOG_SYSTEM("Looking up renderUIPhase from RenderSystems...");

  const flecs::entity kRenderUiPhase = world.lookup("vivid::render::RenderSystems::RenderUIPhase");
  if (kRenderUiPhase.id() == 0) {
    VIVID_LOG_ERROR(
        "renderUIPhase not found! Make sure RenderSystems is imported before UISystems.");
    return;
  }

  // Render ImGui draw data within the render pass
  world.system<vivid::render::WebGPUContext>("RenderImGui").kind(kRenderUiPhase).each(renderUIImpl);

  // Shutdown ImGui - runs once at shutdown
  world.system("ShutDownUI").kind<ShutdownPhase>().run(shutDownUIImpl);

  VIVID_LOG_SUCCESS("UISystems module registration completed!");
}

}  // namespace vivid::ui
