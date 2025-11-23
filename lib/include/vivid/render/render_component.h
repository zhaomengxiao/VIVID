#pragma once

#include <flecs.h>
#include <webgpu/webgpu.h>

#include <flecs/addons/cpp/mixins/units/decl.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace vivid::render {

// typedef glm::vec3 Color3f;
typedef glm::vec3 Vector3f;

struct Color3f {
  float r;
  float g;
  float b;

  // Default constructor with default values
  Color3f() : r(1.0f), g(1.0f), b(1.0f) {}

  // Constructor with three float parameters
  Color3f(float r_val, float g_val, float b_val) : r(r_val), g(g_val), b(b_val) {}
};

//
// 基础组件
//

// 变换组件，存储物体的位置、旋转、缩放
struct TransformComponent {
  glm::vec3 position_{0.0F, 0.0F, 0.0F};
  glm::vec3 rotation_{0.0F, 0.0F, 0.0F};  // 欧拉角
  glm::vec3 scale_{1.0F, 1.0F, 1.0F};

  // 辅助函数，用于计算模型矩阵
  glm::mat4 GetTransform() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position_);
    transform = glm::rotate(transform, rotation_.x, {1, 0, 0});
    transform = glm::rotate(transform, rotation_.y, {0, 1, 0});
    transform = glm::rotate(transform, rotation_.z, {0, 0, 1});
    transform = glm::scale(transform, scale_);
    return transform;
  }
};

// 标签组件，用于给实体一个可读的名称
struct TagComponent {
  std::string tag_;
};

//
// 渲染专属组件
//

// 网格组件，持有渲染所需的顶点数据
struct MeshComponent {
  std::vector<float> vertices_;  // Combined positions and normals
  std::vector<unsigned int> indices_;
  size_t index_count_;
};

// 材质组件，定义物体的外观和着色器
struct MaterialComponent {
  std::string shader_path_ = "res/shaders/BlinnPhong.shader";  // 默认着色器
  Color3f object_color_{0.8f, 0.8f, 0.8f};                     // 默认颜色为灰色
  Color3f specular_color_{0.5f, 0.5f, 0.5f};
  float shininess_ = 32.0f;
};

// 光源组件
struct LightComponent {
  Color3f light_color_{1.0f, 1.0f, 1.0f};
  Color3f ambient_color_{0.2f, 0.2f, 0.2f};
  // 衰减系数
  float constant_ = 1.0f;
  float linear_ = 0.09f;
  float quadratic_ = 0.032f;
};

// 相机组件
struct CameraComponent {
  glm::mat4 projection_matrix_{1.0f};
  bool is_primary_ = true;  // 标记为主相机
                            // 视图矩阵由相机位置（TransformComponent）计算而来
};

struct ViewportComponent {
  float width_ = 1280.0f;
  float height_ = 720.0f;
  uintptr_t texture_id_ = 0;

  bool is_focused_ = false;
  bool is_hovered_ = false;

  float content_start_pos_x_ = 0.0f;
  float content_start_pos_y_ = 0.0f;

  // Offscreen rendering resources for ImGui viewport windows
  WGPUTexture render_texture_ = nullptr;           // Offscreen render target texture
  WGPUTextureView render_texture_view_ = nullptr;  // Texture view for ImGui
  WGPUTexture depth_texture_ = nullptr;            // Depth texture for offscreen rendering
  WGPUTextureView depth_view_ = nullptr;           // Depth texture view
  uint32_t configured_width_ = 0;                  // Track configured texture width
  uint32_t configured_height_ = 0;                 // Track configured texture height
  bool initialized_ = false;                       // Flag to ensure one-time initialization
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

    world.import <flecs::units>();

    // register type
    // world.component<glm::vec3>()
    //     .member<float, flecs::units::color::Rgb>("x")
    //     .range(0.0, 1.0f)
    //     .member<float, flecs::units::color::Rgb>("y")
    //     .range(0.0, 1.0f)
    //     .member<float, flecs::units::color::Rgb>("z")
    //     .range(0.0, 1.0f);

    world.component<Color3f>()
        .member<float, flecs::units::color::Rgb>("r")
        .range(0.0f, 1.0f)
        .member<float, flecs::units::color::Rgb>("g")
        .range(0.0f, 1.0f)
        .member<float, flecs::units::color::Rgb>("b")
        .range(0.0f, 1.0f);

    // world.component<Vector3f>()
    //     .member<float>("x")
    //     .range(-1.0f, 1.0f)
    //     .member<float>("y")
    //     .range(-1.0f, 1.0f)
    //     .member<float>("z")
    //     .range(-1.0f, 1.0f);

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
        .member<std::string>("shader_path_")
        .member<Color3f>("object_color_")
        .member<Color3f>("specular_color_")
        .member<float>("shininess_")
        .range(0.0, 100.0);

    world.component<LightComponent>()
        .member<Color3f>("light_color_")
        .member<Color3f>("ambient_color_")
        .member<float>("constant_")
        .range(0.0, 1.0)
        .member<float>("linear_")
        .range(0.0, 1.0)
        .member<float>("quadratic_")
        .range(0.0, 1.0);

    world.component<CameraComponent>();
    world.component<ViewportComponent>();
  }
};

}  // namespace vivid::render