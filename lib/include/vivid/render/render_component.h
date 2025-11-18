#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace VIVID::RENDER {

//
// 基础组件
//

// 变换组件，存储物体的位置、旋转、缩放
struct TransformComponent {
  glm::vec3 Position{0.0f, 0.0f, 0.0f};
  glm::vec3 Rotation{0.0f, 0.0f, 0.0f};  // 欧拉角
  glm::vec3 Scale{1.0f, 1.0f, 1.0f};

  // 辅助函数，用于计算模型矩阵
  glm::mat4 GetTransform() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);
    transform = glm::rotate(transform, Rotation.x, {1, 0, 0});
    transform = glm::rotate(transform, Rotation.y, {0, 1, 0});
    transform = glm::rotate(transform, Rotation.z, {0, 0, 1});
    transform = glm::scale(transform, Scale);
    return transform;
  }
};

// 标签组件，用于给实体一个可读的名称
struct TagComponent {
  std::string Tag;
};

//
// 渲染专属组件
//

// 网格组件，持有渲染所需的顶点数据
struct MeshComponent {
  std::vector<float> m_Vertices;  // Combined positions and normals
  std::vector<unsigned int> m_Indices;
  size_t m_IndexCount;
};

// 材质组件，定义物体的外观和着色器
struct MaterialComponent {
  std::string ShaderPath = "res/shaders/BlinnPhong.shader";  // 默认着色器
  glm::vec3 ObjectColor{0.8f, 0.8f, 0.8f};                   // 默认颜色为灰色
  glm::vec3 SpecularColor{0.5f, 0.5f, 0.5f};
  float Shininess = 32.0f;
};

// 光源组件
struct LightComponent {
  glm::vec3 LightColor{1.0f, 1.0f, 1.0f};
  glm::vec3 AmbientColor{0.2f, 0.2f, 0.2f};
  // 衰减系数
  float Constant = 1.0f;
  float Linear = 0.09f;
  float Quadratic = 0.032f;
};

// 相机组件
struct CameraComponent {
  glm::mat4 ProjectionMatrix{1.0f};
  bool IsPrimary = true;  // 标记为主相机
                          // 视图矩阵由相机位置（TransformComponent）计算而来
};

struct ViewportComponent {
  float Width = 1280.0f;
  float Height = 720.0f;
  uintptr_t TextureID = 0;

  bool IsFocused = false;
  bool IsHovered = false;

  // Offscreen rendering resources for ImGui viewport windows
  WGPUTexture renderTexture = nullptr;          // Offscreen render target texture
  WGPUTextureView renderTextureView = nullptr;  // Texture view for ImGui
  WGPUTexture depthTexture = nullptr;           // Depth texture for offscreen rendering
  WGPUTextureView depthView = nullptr;          // Depth texture view
  uint32_t configuredWidth = 0;                 // Track configured texture width
  uint32_t configuredHeight = 0;                // Track configured texture height
  bool initialized = false;                     // Flag to ensure one-time initialization
};

template <typename Elem, typename Vector = std::vector<Elem>>
flecs::opaque<Vector, Elem> std_vector_support(flecs::world& world) {
  return flecs::opaque<Vector, Elem>()
      .as_type(world.vector<Elem>())

      // Forward elements of std::vector value to serializer
      .serialize([](const flecs::serializer* s, const Vector* data) {
        for (const auto& el : *data) {
          s->value(el);
        }
        return 0;
      })

      // Return vector count
      .count([](const Vector* data) { return data->size(); })

      // Resize contents of vector
      .resize([](Vector* data, size_t size) { data->resize(size); })

      // Ensure element exists, return pointer
      .ensure_element([](Vector* data, size_t elem) {
        if (data->size() <= elem) {
          data->resize(elem + 1);
        }

        return &data->data()[elem];
      });
}

// Render Components Module
struct RenderComponents {
  RenderComponents(flecs::world& world) {
    // Register module
    world.module<RenderComponents>();

    // register type
    world.component<glm::vec3>().member<float>("x").member<float>("y").member<float>("z");
    world.component<std::string>()
        .opaque(flecs::String)  // Opaque type that maps to string
        .serialize([](const flecs::serializer* s, const std::string* data) {
          const char* str = data->c_str();
          return s->value(flecs::String, &str);  // Forward to serializer
        })
        .assign_string([](std::string* data, const char* value) {
          *data = value;  // Assign new value to std::string
        });

    // Register reflection for std::vector<int>
    world.component<std::vector<int>>().opaque(std_vector_support<int>);

    // Register reflection for std::vector<std::string>
    world.component<std::vector<std::string>>().opaque(std_vector_support<std::string>);

    // Register components
    world.component<TransformComponent>();
    world.component<TagComponent>();
    world.component<MeshComponent>();
    world.component<MaterialComponent>()
        .member<std::string>("ShaderPath")
        .member<glm::vec3>("ObjectColor")
        .member<glm::vec3>("SpecularColor")
        .member<float>("Shininess");

    world.component<LightComponent>()
        .opaque(world.component()
                    .member<float>("LightColor_x")
                    .member<float>("LightColor_y")
                    .member<float>("LightColor_z")
                    .member<float>("AmbientColor_x")
                    .member<float>("AmbientColor_y")
                    .member<float>("AmbientColor_z")
                    .member<float>("Constant")
                    .member<float>("Linear")
                    .member<float>("Quadratic"))
        .serialize([](const flecs::serializer* s, const LightComponent* data) {
          // 序列化真实成员x/y/z（直接读取glm::vec3的原生成员）
          s->member("LightColor_x");
          s->value(data->LightColor.x);
          s->member("LightColor_y");
          s->value(data->LightColor.y);
          s->member("LightColor_z");
          s->value(data->LightColor.z);
          s->member("AmbientColor_x");
          s->value(data->AmbientColor.x);
          s->member("AmbientColor_y");
          s->value(data->AmbientColor.y);
          s->member("AmbientColor_z");
          s->value(data->AmbientColor.z);
          s->member("Constant");
          s->value(data->Constant);
          s->member("Linear");
          s->value(data->Linear);
          return 0;  // 序列化成功返回0
        })
        .ensure_member([](LightComponent* dst, const char* member_name) -> void* {
          // 反序列化仅处理真实成员x/y/z，虚拟成员length不支持赋值
          if (strcmp(member_name, "LightColor_x") == 0) {
            return &(dst->LightColor.x);
          } else if (strcmp(member_name, "LightColor_y") == 0) {
            return &(dst->LightColor.y);
          } else if (strcmp(member_name, "LightColor_z") == 0) {
            return &(dst->LightColor.z);
          } else if (strcmp(member_name, "AmbientColor_x") == 0) {
            return &(dst->AmbientColor.x);
          } else if (strcmp(member_name, "AmbientColor_y") == 0) {
            return &(dst->AmbientColor.y);
          } else if (strcmp(member_name, "AmbientColor_z") == 0) {
            return &(dst->AmbientColor.z);
          } else if (strcmp(member_name, "Constant") == 0) {
            return &(dst->Constant);
          } else if (strcmp(member_name, "Linear") == 0) {
            return &(dst->Linear);
          } else if (strcmp(member_name, "Quadratic") == 0) {
            return &(dst->Quadratic);
          } else {
            return nullptr;
          }
        });

    world.component<CameraComponent>();
    world.component<ViewportComponent>();
  }
};

}  // namespace VIVID::RENDER