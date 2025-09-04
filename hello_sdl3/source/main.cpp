// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <utility>

#include "vivid/app/SDL3App.h"
#include "vivid/log/log.h"
#include "vivid/plugins/DefaultPlugin.h"

struct MyResource {
  int value;
  MyResource(int v) : value(v) {}
};

// Simple startup system
void hello_startup_system(Resources&, entt::registry& world) {
  VividLogger::app_info("Hello from SDL3 startup system!");

  // Create a simple entity
  auto entity = world.create();
  // Add components as needed...
}

// Simple update system
void hello_update_system(Resources& res, entt::registry& world) {
  static int frame_count = 0;
  frame_count++;

  if (frame_count % 60 == 0) {  // Print every 60 frames
    VividLogger::app_info("SDL3 app running... Frame: %d", frame_count);
    VividLogger::app_debug("MyResource value: %d", res.get<MyResource>()->value);
  }

  // Exit after 300 frames (about 5 seconds at 60 FPS)
  // if (frame_count > 300) {
  //   // Access the app instance to exit (this would need to be implemented)
  //   VividLogger::app_info("Exiting SDL3 application...");
  // }
}

// Method 1: Using macro with chain calls (most concise)
// VIVID_SDL3_MAIN(.add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))

// 其他链式调用方法示例：

// Method 2: Direct function with chain calls and metadata (新的简化API)
SDL3AppBuilder create_app_instance() {
  return std::move(
      create_sdl3_app()
          // 设置应用基本信息（推荐方法）
          .set_app_info("VIVID Hello SDL3", "1.0.0", "com.vivid.hello_sdl3")
          // 使用枚举设置其他元数据
          .set_metadata(SDL3MetadataProperty::Creator, "VIVID Engine Team")
          .set_metadata(SDL3MetadataProperty::Copyright, "Copyright (c) 2024 VIVID Engine")
          .set_metadata(SDL3MetadataProperty::Url, "https://github.com/vivid-engine/vivid")
          .set_metadata(SDL3MetadataProperty::Type, SDL3AppType::Application)
          // 自定义属性
          .set_custom_metadata("custom_property", "custom_value")
          // 配置日志系统 - 设置为Debug级别以显示详细日志
          .set_default_log_level(VividLogLevel::Debug)
          .set_log_level(VividLogCategory::Application, VividLogLevel::Debug)
          // 应用配置
          .insert_resource<MyResource>(100)
          .add_plugin<DefaultPlugin>()
          .add_startup_system(hello_startup_system)
          .add_system(ScheduleLabel::Update, hello_update_system));
}

// Method 3: Step-by-step building
// SDL3AppBuilder create_app_instance() {
//   auto builder = create_sdl3_app();
//   builder.add_plugin<DefaultPlugin>();
//   builder.add_startup_system(hello_startup_system);
//   builder.add_system(ScheduleLabel::Update, hello_update_system);
//   builder.insert_resource<MyResource>(100);
//   return std::move(builder);
// }

// Method 4: Using VIVID_SDL3_MAIN macro with simplified metadata API (游戏示例)
// VIVID_SDL3_MAIN(.set_app_info("My Awesome Game", "2.0.0", "com.mygame.app")
//                     .set_metadata(SDL3MetadataProperty::Type, SDL3AppType::Game)
//                     .set_default_log_level(VividLogLevel::Info)
//                     .set_log_level(VividLogCategory::Application, VividLogLevel::Debug)
//                     .insert_resource<MyResource>(100)
//                     .add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))

// Method 5: Step-by-step with simplified metadata API
// SDL3AppBuilder create_app_instance() {
//   auto builder = create_sdl3_app();
//
//   // Set metadata using simplified API
//   builder.set_app_info("My SDL3 Application", "1.0.0-beta", "com.company.myapp");
//   builder.set_metadata(SDL3MetadataProperty::Creator, "My Company");
//   builder.set_metadata(SDL3MetadataProperty::Type, "application");
//   builder.set_custom_metadata("build_config", "debug");
//
//   // Configure logging
//   builder.set_default_log_level(VividLogLevel::Info);
//   builder.set_log_level(VividLogCategory::Application, VividLogLevel::Debug);
//
//   // Configure app
//   builder.add_plugin<DefaultPlugin>();
//   builder.add_startup_system(hello_startup_system);
//   builder.add_system(ScheduleLabel::Update, hello_update_system);
//   builder.insert_resource<MyResource>(100);
//
//   return std::move(builder);
// }
