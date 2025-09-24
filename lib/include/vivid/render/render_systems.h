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

  // Startup Stage Systems (one time initialization)
  void CreateWebGPUInstance(Resources &res, entt::registry &world);

  void RequestWebGPUAdapterSync(Resources &res, entt::registry &world);

  void RequestWebGPUDeviceSync(Resources &res, entt::registry &world);

  void InspectWebGPUAdapter(Resources &res, entt::registry &world);

  void InspectWebGPUDevice(Resources &res, entt::registry &world);

  void TestCommandQueue(Resources &res, entt::registry &world);

  void ConfigureSurface(Resources &res, entt::registry &world);

  // Resouces Sync Stage Systems (increment)
  void SyncScene(Resources &res, entt::registry &world);

  void Draw(Resources &res, entt::registry &world);

  void CreatePipeline(Resources &res, entt::registry &world);

  void ReleaseWebGPUResources(Resources &res, entt::registry &world);
}  // namespace VIVID::Render