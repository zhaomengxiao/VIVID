#pragma once
#include <entt/entity/registry.hpp>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "vivid/app/App.h"

namespace VIVID {

  struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
  };
  class FrameBuffer {
  public:
    FrameBuffer(uint32_t width, uint32_t height);

    ~FrameBuffer();

    void Invalidate();

    void Bind();

    static void Unbind();

    void Resize(uint32_t width, uint32_t height);

    uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }

  private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0, m_DepthAttachment = 0;
    uint32_t m_Width, m_Height;
  };

  // 主更新函数，每帧调用
  void Sync_system(entt::registry &world);
  void Update_system(Resources &res, entt::registry &world);

  void Init_system(Resources &res, entt::registry &world);
  void Shutdown_system(Resources &res, entt::registry &world);

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
}  // namespace VIVID