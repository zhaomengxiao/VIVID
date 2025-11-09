#pragma once
#include <flecs.h>
#include <vivid/log/log.h>

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

namespace editor {
namespace inspector {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct InspectorSystems {
  InspectorSystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void drawSceneHierarchy(const flecs::iter& it);
};

// Constructor - Register module and systems
inline InspectorSystems::InspectorSystems(flecs::world& world) {
  world.module<InspectorSystems>();

  auto testEntity1 = world.entity("InspectorTest1");
  auto testEntity2 = world.entity("InspectorTest2");
  VividLogger::app_info("Created test entities in InspectorSystems constructor");

  flecs::entity RenderUIPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderUIPhase");
  if (RenderUIPhase.id() == 0) {
    VIVID_LOG_ERROR(
        "RenderUIPhase not found! Make sure RenderSystems is imported before UISystems.");
    return;
  } else {
    VIVID_LOG_SUCCESS("RenderUIPhase found: %s", RenderUIPhase.name());
  }

  world.system("DrawSceneHierarchy").kind(flecs::PreUpdate).run(drawSceneHierarchy);
}

}  // namespace inspector
}  // namespace editor