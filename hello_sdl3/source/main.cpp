// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <utility>

// SDL3 主入口定义（必须在SDL3App.h之前）
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "vivid/app/SDL3App.h"
#include "vivid/plugins/DefaultPlugin.h"

struct MyResource {
  int value;
  MyResource(int v) : value(v) {}
};

// Simple startup system
void hello_startup_system(Resources&, entt::registry& world) {
  std::cout << "Hello from SDL3 startup system!" << std::endl;

  // Create a simple entity
  auto entity = world.create();
  // Add components as needed...
}

// Simple update system
void hello_update_system(Resources& res, entt::registry& world) {
  static int frame_count = 0;
  frame_count++;

  if (frame_count % 60 == 0) {  // Print every 60 frames
    std::cout << "SDL3 app running... Frame: " << frame_count << std::endl;
    std::cout << "MyResource value: " << res.get<MyResource>()->value << std::endl;
  }

  // Exit after 300 frames (about 5 seconds at 60 FPS)
  // if (frame_count > 300) {
  //   // Access the app instance to exit (this would need to be implemented)
  //   std::cout << "Exiting SDL3 application..." << std::endl;
  // }
}

// Method 1: Using macro with chain calls (most concise)
// VIVID_SDL3_MAIN(.add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))

// 其他链式调用方法示例：

// Method 2: Direct function with chain calls
SDL3AppBuilder create_app_instance() {
  return std::move(create_sdl3_app()
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

// Method 4: With resources
// VIVID_SDL3_MAIN(.insert_resource<MyResource>(100)
//                     .add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))
