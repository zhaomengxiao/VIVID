#include "vivid/ui/ui_system.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <vivid/render/render_systems.h>
#include <vivid/window/window_systems.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#  if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#    include <emscripten/html5_webgpu.h>
#  endif

#  include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include <webgpu/webgpu.h>
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
#  include <webgpu/webgpu_cpp.h>
#endif
#include <vivid/log/log.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#endif
#include <vivid/app/SDL3App.h>

bool ImGui_ImplWGPU_CheckSurfaceTextureOptimalStatus_Helper(
    WGPUSurfaceGetCurrentTextureStatus status) {
  switch (status) {
#if defined(__EMSCRIPTEN__) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    case WGPUSurfaceGetCurrentTextureStatus_Success:
      return true;
#else
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
      return true;
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
#endif
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
      // if the status is NOT Optimal it's necessary try to reconfigure the surface
      return false;
      // Unrecoverable errors
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    case WGPUSurfaceGetCurrentTextureStatus_Error:
#else  // IMGUI_IMPL_WEBGPU_BACKEND_WGPU
    case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
    case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
#endif
    case WGPUSurfaceGetCurrentTextureStatus_Force32:
      // Fatal error
      fprintf(stderr, "Unrecoverable Error Check Surface Texture status=%#.8x\n", status);
      abort();

    default:  // should never be reached
      fprintf(stderr, "Unexpected Error Check Surface Texture status=%#.8x\n", status);
      abort();
  }
}

namespace VIVID::UI {
  // 如何在SDL3窗口中显示imgui: 2.初始化
  void initImGui(Resources& res, entt::registry& world) {
    auto webgpuRes = res.get<WebGPUResources>();
    if (!webgpuRes) {
      VividLogger::app_error("Could not get WebGPU resources!");
      return;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // IF using Docking Branch

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    // style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a
    // solution for dynamic style scaling, changing this requires resetting Style + calling this
    // again) style.FontScaleDpi = main_scale;        // Set initial font scale. (using
    // io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation
    // purpose) Setup Platform/Renderer backends
    auto view = world.view<VIVID::Window::WindowGpuComponent>();

    view.each([&](auto entity, auto& gpu_comp) {
      ImGui_ImplSDL3_InitForOther(gpu_comp.window_handle);
      ImGui_ImplWGPU_InitInfo init_info;
      init_info.Device = webgpuRes->device;
      init_info.NumFramesInFlight = 3;
      init_info.RenderTargetFormat = webgpuRes->surfaceFormat;
      init_info.DepthStencilFormat = webgpuRes->depthFormat;
      ImGui_ImplWGPU_Init(&init_info);
      return;  // TODO: 目前只处理第一个窗口
    });

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple
    // fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the
    // font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those
    // errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher
    // quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but
    // want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to
    // write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the
    // "fonts/" folder. See Makefile.emscripten for details.
    // style.FontSizeBase = 20.0f;
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    // IM_ASSERT(font != nullptr);
  }

  // 如何在SDL3窗口中显示imgui: 3.处理事件

  void ProcessImGuiEvent(Resources& res, entt::registry& world) {
    auto eventQueues = res.get<EventQueues>();

    if (eventQueues) {
      ImGui_ImplSDL3_ProcessEvent(&eventQueues->raw_sdl_events.front());
      eventQueues->raw_sdl_events.pop();
    }
  }

// 如何在SDL3窗口中显示imgui: 4.显示imgui Demo
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
  // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
  // settings from your own storage.
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
//   while (!canCloseWindow)
#endif
  void ShowImGuiDemo(Resources& res, entt::registry& world) {
    // Build ImGui frame only; actual rendering happens in Render::Draw
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named
    // window.

    // Our state

    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

    ImGui::Text(
        "This is some useful text.");  // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window",
                    &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

    if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true
                                  // when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();

    if (show_demo_window) {
      ImGui::ShowDemoWindow();
    }

    // 3. Show another simple window.
    if (show_another_window) {
      ImGui::Begin(
          "Another Window",
          &show_another_window);  // Pass a pointer to our bool variable (the window will have a
                                  // closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")) show_another_window = false;
      ImGui::End();
    }

    // Do not call ImGui::Render() here; it will be invoked in Render::Draw
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  void ShutDownImGui(Resources& res, entt::registry& world) {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::DestroyContext();
  }
}  // namespace VIVID::UI
