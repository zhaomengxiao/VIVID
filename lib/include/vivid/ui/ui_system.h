#pragma once

#include "vivid/app/App.h"

// 如何在SDL3窗口中显示imgui: 1.添加imgui头文件

#include <entt/entt.hpp>

namespace VIVID::UI {

  void initImGui(Resources& res, entt::registry& world);

  void ProcessImGuiEvent(Resources& res, entt::registry& world);

  void ShowImGuiDemo(Resources& res, entt::registry& world);

  void ShutDownImGui(Resources& res, entt::registry& world);
}  // namespace VIVID::UI
