#pragma once

#include "vivid/app/App.h"
#include "vivid/app/Plugin.h"

namespace VIVID::Render {
  /// @brief 创建WebGPU实例
  /// @param res
  /// @param world
  void CreateWebGPUInstance(Resources &res, entt::registry &world);

  /// @brief 同步请求WebGPU适配器
  /// @note We will no longer need to use the instance once we have selected our adapter, so we will
  /// call ReleaseWebGPUInstance right after the adapter request instead of at the very end.
  /// The underlying instance object will keep on living until the adapter gets released but we do
  /// not need to manage this.
  /// @param res
  /// @param world
  void RequestWebGPUAdapterSync(Resources &res, entt::registry &world);

  void RequestWebGPUDeviceSync(Resources &res, entt::registry &world);

  void InspectWebGPUAdapter(Resources &res, entt::registry &world);

  void InspectWebGPUDevice(Resources &res, entt::registry &world);

  void TestCommandQueue(Resources &res, entt::registry &world);

  // This must be done at the end of the initialization:
  void ConfigureSurface(Resources &res, entt::registry &world);

  void Draw(Resources &res, entt::registry &world);

  void CreatePipeline(Resources &res, entt::registry &world);

  /// @brief 释放WebGPU资源
  /// @param res
  /// @param world
  void ReleaseWebGPUResources(Resources &res, entt::registry &world);
}  // namespace VIVID::Render