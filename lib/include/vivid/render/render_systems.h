#pragma once

#include <glm/glm.hpp>
#include <string>

#include "vivid/app/App.h"
#include "vivid/app/Plugin.h"
struct ShaderProgramSource {
  std::string VertexSource;
  std::string FragmentSource;
};
// helper functions
ShaderProgramSource ParseShader(const std::string &filepath);
unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader);
unsigned int CompileShader(unsigned int type, const std::string &source);
int GetUniformLocation(unsigned int program, const std::string &name);
void SetUniform1i(unsigned int program, const std::string &name, int value);
void SetUniform1f(unsigned int program, const std::string &name, float value);
void SetUniform3f(unsigned int program, const std::string &name, float v0, float v1, float v2);
void SetUniform4f(unsigned int program, const std::string &name, float v0, float v1, float v2,
                  float v3);
void SetUniformMat4f(unsigned int program, const std::string &name, const glm::mat4 &matrix);

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

  void SyncScene(Resources &res, entt::registry &world);

  void Draw(Resources &res, entt::registry &world);

  void CreatePipeline(Resources &res, entt::registry &world);

  /// @brief 释放WebGPU资源
  /// @param res
  /// @param world
  void ReleaseWebGPUResources(Resources &res, entt::registry &world);
}  // namespace VIVID::Render