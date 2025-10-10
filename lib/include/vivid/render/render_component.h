#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <vector>

//
// 基础组件
//

// 变换组件，存储物体的位置、旋转、缩放
struct TransformComponent
{
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation{0.0f, 0.0f, 0.0f}; // 欧拉角
    glm::vec3 Scale{1.0f, 1.0f, 1.0f};

    // 辅助函数，用于计算模型矩阵
    glm::mat4 GetTransform() const
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);
        transform = glm::rotate(transform, Rotation.x, {1, 0, 0});
        transform = glm::rotate(transform, Rotation.y, {0, 1, 0});
        transform = glm::rotate(transform, Rotation.z, {0, 0, 1});
        transform = glm::scale(transform, Scale);
        return transform;
    }
};

// 标签组件，用于给实体一个可读的名称
struct TagComponent
{
    std::string Tag;
};

//
// 渲染专属组件
//

// 网格组件，持有渲染所需的顶点数据
struct MeshComponent
{
    std::vector<float> m_Vertices; // Combined positions and normals
    std::vector<unsigned int> m_Indices;
    size_t m_IndexCount;
};

// GPU资源组件 - 只存储OpenGL ID
struct GpuMeshComponent
{
    unsigned int VAO_ID = 0;
    unsigned int VBO_ID = 0;
    unsigned int IBO_ID = 0;
    unsigned int IndexCount = 0; // 必须存储，因为绘制时需要
};

struct GpuMaterialComponent
{
    unsigned int ShaderProgram_ID = 0;

    // 如果有纹理，也只存储ID
    // unsigned int DiffuseTexture_ID = 0;
    // unsigned int SpecularTexture_ID = 0;
};

// 材质组件，定义物体的外观和着色器
struct MaterialComponent
{
    std::string ShaderPath = "res/shaders/BlinnPhong.shader"; // 默认着色器
    glm::vec3 ObjectColor{0.8f, 0.8f, 0.8f};                  // 默认颜色为灰色
    glm::vec3 SpecularColor{0.5f, 0.5f, 0.5f};
    float Shininess = 32.0f;
};

// 光源组件
struct LightComponent
{
    glm::vec3 LightColor{1.0f, 1.0f, 1.0f};
    glm::vec3 AmbientColor{0.2f, 0.2f, 0.2f};
    // 衰减系数
    float Constant = 1.0f;
    float Linear = 0.09f;
    float Quadratic = 0.032f;
};

// 相机组件
struct CameraComponent
{
    glm::mat4 ProjectionMatrix{1.0f};
    bool IsPrimary = true; // 标记为主相机
    // 视图矩阵由相机位置（TransformComponent）计算而来
};

struct ViewportComponent
{
    float Width = 1280.0f;
    float Height = 720.0f;
    uint32_t TextureID = 0;

    bool IsFocused = false;
    bool IsHovered = false;
};