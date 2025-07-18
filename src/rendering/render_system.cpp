#include "render_system.h"
#include "render_component.h"
#include <glm/gtc/type_ptr.hpp>

#include "opengl/GLErrorHandler.h"
#include "../input/camera_controller.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace VIVID
{
    // helper functions
    ShaderProgramSource RendererSystem::ParseShader(const std::string &filepath)
    {
        std::ifstream stream(filepath);

        enum class ShaderType
        {
            NONE = -1,
            VERTEX = 0,
            FRAGMENT = 1
        };

        std::string line;
        std::stringstream ss[2];
        ShaderType type = ShaderType::NONE;

        while (getline(stream, line))
        {
            if (line.find("#shader") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    type = ShaderType::VERTEX;
                else if (line.find("fragment") != std::string::npos)
                    type = ShaderType::FRAGMENT;
            }
            else
            {
                if (type != ShaderType::NONE)
                    ss[(int)type] << line << '\n';
            }
        }

        return {ss[0].str(), ss[1].str()};
    }

    unsigned int RendererSystem::CreateShader(const std::string &vertexShader, const std::string &fragmentShader)
    {
        GLCall(unsigned int program = glCreateProgram());
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

        GLCall(glAttachShader(program, vs));
        GLCall(glAttachShader(program, fs));
        GLCall(glLinkProgram(program));

        // 检查链接错误
        int result;
        glGetProgramiv(program, GL_LINK_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            char *message = (char *)alloca(length * sizeof(char));
            glGetProgramInfoLog(program, length, &length, message);
            std::cerr << "Failed to link shader program:" << std::endl;
            std::cerr << message << std::endl;
        }

        GLCall(glDeleteShader(vs));
        GLCall(glDeleteShader(fs));

        return program;
    }

    unsigned int RendererSystem::CompileShader(unsigned int type, const std::string &source)
    {
        GLCall(unsigned int id = glCreateShader(type));
        const char *src = source.c_str();
        GLCall(glShaderSource(id, 1, &src, nullptr));
        GLCall(glCompileShader(id));

        // 检查编译错误
        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char *message = (char *)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, message);
            std::cerr << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader:" << std::endl;
            std::cerr << message << std::endl;
            GLCall(glDeleteShader(id));
            return 0;
        }

        return id;
    }

    int RendererSystem::GetUniformLocation(unsigned int program, const std::string &name)
    {
        // if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        //     return m_UniformLocationCache[name];

        GLCall(int location = glGetUniformLocation(program, name.c_str()));
        if (location == -1)
            std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;

        // m_UniformLocationCache[name] = location;
        return location;
    }

    void RendererSystem::SetUniform1i(unsigned int program, const std::string &name, int value)
    {
        GLCall(glUniform1i(GetUniformLocation(program, name), value));
    }

    void RendererSystem::SetUniform1f(unsigned int program, const std::string &name, float value)
    {
        GLCall(glUniform1f(GetUniformLocation(program, name), value));
    }

    void RendererSystem::SetUniform3f(unsigned int program, const std::string &name, float v0, float v1, float v2)
    {
        GLCall(glUniform3f(GetUniformLocation(program, name), v0, v1, v2));
    }

    void RendererSystem::SetUniform4f(unsigned int program, const std::string &name, float v0, float v1, float v2, float v3)
    {
        GLCall(glUniform4f(GetUniformLocation(program, name), v0, v1, v2, v3));
    }

    void RendererSystem::SetUniformMat4f(unsigned int program, const std::string &name, const glm::mat4 &matrix)
    {
        GLCall(glUniformMatrix4fv(GetUniformLocation(program, name), 1, GL_FALSE, &matrix[0][0]));
    }

    // --- Framebuffer Class Implementation ---
    class VIVID::RendererSystem::Framebuffer
    {
    public:
        Framebuffer(uint32_t width, uint32_t height)
            : m_Width(width), m_Height(height)
        {
            Invalidate();
        }

        ~Framebuffer()
        {
            GLCall(glDeleteFramebuffers(1, &m_RendererID));
            GLCall(glDeleteTextures(1, &m_ColorAttachment));
            GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
        }

        void Invalidate()
        {
            if (m_RendererID)
            {
                GLCall(glDeleteFramebuffers(1, &m_RendererID));
                GLCall(glDeleteTextures(1, &m_ColorAttachment));
                GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
            }

            GLCall(glGenFramebuffers(1, &m_RendererID));
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));

            GLCall(glGenTextures(1, &m_ColorAttachment));
            GLCall(glBindTexture(GL_TEXTURE_2D, m_ColorAttachment));
            GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0));

            GLCall(glGenRenderbuffers(1, &m_DepthAttachment));
            GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment));
            GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height));
            GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthAttachment));

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Framebuffer is not complete!" << std::endl;

            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        }

        void Bind()
        {
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));
            GLCall(glViewport(0, 0, m_Width, m_Height));
        }

        void Unbind()
        {
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        }

        void Resize(uint32_t width, uint32_t height)
        {
            if (m_Width != width || m_Height != height)
            {
                m_Width = width;
                m_Height = height;
                Invalidate();
            }
        }

        uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0, m_DepthAttachment = 0;
        uint32_t m_Width, m_Height;
    };

    // --- RendererSystem Implementation ---
    std::unique_ptr<VIVID::RendererSystem::Framebuffer> VIVID::RendererSystem::s_Framebuffer;

    void VIVID::RendererSystem::Init()
    {
        // Default size, will be resized by viewport component.
        s_Framebuffer = std::make_unique<Framebuffer>(1280, 720);
    }

    void VIVID::RendererSystem::Shutdown()
    {
        s_Framebuffer.reset();
    }

    void RendererSystem::Sync(entt::registry &registry)
    {
        // We only want to process entities that have the CPU-side data (Mesh, Material)
        // but DO NOT have the GPU-side data (GpuMeshComponent) yet.
        // Using entt::exclude prevents us from re-processing entities and leaking resources.
        auto view = registry.view<MeshComponent, MaterialComponent>(entt::exclude<GpuMeshComponent>);
        view.each(
            [&](auto entity, auto &mesh, auto &material)
            {
                if (mesh.m_Vertices.empty() || mesh.m_Indices.empty() || material.ShaderPath.empty())
                    return;

                // 创建和绑定VBO
                unsigned int vboID;
                GLCall(glGenBuffers(1, &vboID));
                GLCall(glBindBuffer(GL_ARRAY_BUFFER, vboID));
                GLCall(glBufferData(GL_ARRAY_BUFFER, mesh.m_Vertices.size() * sizeof(float), mesh.m_Vertices.data(), GL_STATIC_DRAW));

                // 创建IBO
                unsigned int iboID;
                GLCall(glGenBuffers(1, &iboID));
                GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID));
                GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_Indices.size() * sizeof(unsigned int), mesh.m_Indices.data(), GL_STATIC_DRAW));

                // 创建和绑定VAO
                unsigned int vaoID;
                GLCall(glGenVertexArrays(1, &vaoID));
                GLCall(glBindVertexArray(vaoID));
                GLCall(glBindBuffer(GL_ARRAY_BUFFER, vboID));

                // 设置顶点属性指针
                GLCall(glEnableVertexAttribArray(0));
                GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0)); // position 0
                GLCall(glEnableVertexAttribArray(1));
                GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)))); // normal 1

                // shader
                unsigned int shaderProgramID;
                ShaderProgramSource source = ParseShader(material.ShaderPath);
                shaderProgramID = CreateShader(source.VertexSource, source.FragmentSource);

                // Now we use emplace, because we know the component doesn't exist yet.
                registry.emplace<GpuMeshComponent>(entity,
                                                   GpuMeshComponent{vaoID,
                                                                    vboID,
                                                                    iboID,
                                                                    (unsigned int)mesh.m_Indices.size()});

                // GpuMaterialComponent 同理
                registry.emplace<GpuMaterialComponent>(entity, shaderProgramID);
            });
    }

    void RendererSystem::Update(entt::registry &registry)
    {
        entt::entity mainCameraEntity = entt::null;
        TransformComponent *mainCameraTransform = nullptr;
        CameraComponent *mainCameraComponent = nullptr;
        ViewportComponent *viewportComponent = nullptr;

        auto cameraView = registry.view<TransformComponent, CameraComponent, ViewportComponent>();
        if (cameraView.size_hint() > 0)
        {
            mainCameraEntity = cameraView.front();
            mainCameraTransform = &cameraView.get<TransformComponent>(mainCameraEntity);
            mainCameraComponent = &cameraView.get<CameraComponent>(mainCameraEntity);
            viewportComponent = &cameraView.get<ViewportComponent>(mainCameraEntity);
        }

        if (mainCameraEntity == entt::null)
            return; // No camera with a viewport, can't render.

        // Resize framebuffer if viewport size changed
        if (viewportComponent->Width > 0 && viewportComponent->Height > 0)
        {
            s_Framebuffer->Resize(static_cast<uint32_t>(viewportComponent->Width), static_cast<uint32_t>(viewportComponent->Height));
            mainCameraComponent->ProjectionMatrix = glm::perspective(glm::radians(45.0f), viewportComponent->Width / viewportComponent->Height, 0.1f, 100.0f);
        }

        // Bind framebuffer and render
        s_Framebuffer->Bind();

        GLCall(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        glm::mat4 viewMatrix;
        glm::vec3 viewPos = mainCameraTransform->Position;
        
        // Check if camera has a controller
        auto cameraControllerView = registry.view<CameraControllerComponent>();
        bool hasController = false;
        for (auto entity : cameraControllerView)
        {
            if (entity == mainCameraEntity)
            {
                hasController = true;
                break;
            }
        }
        
        if (hasController)
        {
            // Use camera controller for proper view matrix
            auto& cameraController = registry.get<CameraControllerComponent>(mainCameraEntity);
            glm::vec3 target = mainCameraTransform->Position + cameraController.Front;
            viewMatrix = glm::lookAt(mainCameraTransform->Position, target, cameraController.Up);
        }
        else
        {
            // Fallback to original behavior
            viewMatrix = glm::lookAt(mainCameraTransform->Position, mainCameraTransform->Position + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
        }
        
        glm::mat4 projectionMatrix = mainCameraComponent->ProjectionMatrix;

        // Find light
        auto lightView = registry.view<TransformComponent, LightComponent>();
        auto lightEntity = lightView.front();
        auto &lightTransform = lightView.get<TransformComponent>(lightEntity);
        auto &lightComponent = lightView.get<LightComponent>(lightEntity);

        // Render all entities with GPU data
        auto renderableView = registry.view<TransformComponent, GpuMeshComponent, GpuMaterialComponent, MaterialComponent>();
        renderableView.each(
            [&](auto entity, auto &transform, auto &gpuMesh, auto &gpuMaterial, auto &material)
            {
                GLCall(glUseProgram(gpuMaterial.ShaderProgram_ID));
                GLCall(glBindVertexArray(gpuMesh.VAO_ID));
                GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.IBO_ID));

                glm::mat4 modelMatrix = transform.GetTransform();
                glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelMatrix));

                // --- 设置所有Uniforms ---

                // a) 场景级Uniforms (摄像机)
                SetUniformMat4f(gpuMaterial.ShaderProgram_ID, "u_View", viewMatrix);
                SetUniformMat4f(gpuMaterial.ShaderProgram_ID, "u_Projection", projectionMatrix);
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_ViewPos", viewPos.x, viewPos.y, viewPos.z);

                // b) 场景级Uniforms (光源)
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_LightPos", lightTransform.Position.x, lightTransform.Position.y, lightTransform.Position.z);
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_LightColor", lightComponent.LightColor.r, lightComponent.LightColor.g, lightComponent.LightColor.b);
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_AmbientColor", lightComponent.AmbientColor.r, lightComponent.AmbientColor.g, lightComponent.AmbientColor.b);
                SetUniform1f(gpuMaterial.ShaderProgram_ID, "u_Constant", lightComponent.Constant);
                SetUniform1f(gpuMaterial.ShaderProgram_ID, "u_Linear", lightComponent.Linear);
                SetUniform1f(gpuMaterial.ShaderProgram_ID, "u_Quadratic", lightComponent.Quadratic);

                // c) 物体级Uniforms (变换和材质)
                SetUniformMat4f(gpuMaterial.ShaderProgram_ID, "u_Model", modelMatrix);
                SetUniformMat4f(gpuMaterial.ShaderProgram_ID, "u_NormalMatrix", normalMatrix);
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_ObjectColor", material.ObjectColor.r, material.ObjectColor.g, material.ObjectColor.b);
                SetUniform3f(gpuMaterial.ShaderProgram_ID, "u_SpecularColor", material.SpecularColor.r, material.SpecularColor.g, material.SpecularColor.b);
                SetUniform1f(gpuMaterial.ShaderProgram_ID, "u_Shininess", material.Shininess);

                // 绘制
                GLCall(glDrawElements(GL_TRIANGLES, gpuMesh.IndexCount, GL_UNSIGNED_INT, nullptr));
            });

        s_Framebuffer->Unbind();

        // Update viewport component with the rendered texture
        viewportComponent->TextureID = s_Framebuffer->GetColorAttachmentRendererID();
    }
}