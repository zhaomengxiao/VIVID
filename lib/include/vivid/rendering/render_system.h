#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace VIVID
{
    struct ShaderProgramSource
    {
        std::string VertexSource;
        std::string FragmentSource;
    };

    class RendererSystem
    {
    private:
        class Framebuffer;
        static std::unique_ptr<Framebuffer> s_Framebuffer;

    public:
        // 同步组件到GPU
        static void Sync(entt::registry &registry);
        // 主更新函数，每帧调用
        static void Update(entt::registry &registry);
        static void Init();
        static void Shutdown();

    private:
        // shader helper functions
        static ShaderProgramSource ParseShader(const std::string &filepath);
        static unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader);
        static unsigned int CompileShader(unsigned int type, const std::string &source);
        static int GetUniformLocation(unsigned int program, const std::string &name);
        static void SetUniform1i(unsigned int program, const std::string &name, int value);
        static void SetUniform1f(unsigned int program, const std::string &name, float value);
        static void SetUniform3f(unsigned int program, const std::string &name, float v0, float v1, float v2);
        static void SetUniform4f(unsigned int program, const std::string &name, float v0, float v1, float v2, float v3);
        static void SetUniformMat4f(unsigned int program, const std::string &name, const glm::mat4 &matrix);
    };
}