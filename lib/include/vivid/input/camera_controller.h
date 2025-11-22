#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct CameraControllerComponent {
  // glm::vec3 group (4 * 12 = 48 bytes, aligned to 16-byte boundary)
  glm::vec3 front_ = glm::vec3(0.0F, 0.0F, -1.0F);
  glm::vec3 up_ = glm::vec3(0.0F, 1.0F, 0.0F);
  glm::vec3 right_ = glm::vec3(1.0F, 0.0F, 0.0F);
  glm::vec3 world_up_ = glm::vec3(0.0F, 1.0F, 0.0F);

  // float group (8 * 4 = 32 bytes, aligned to 4-byte boundary)
  float mouse_sensitivity_ = 0.1F;
  float movement_speed_ = 5.0F;
  float zoom_speed_ = 2.0F;
  float pan_speed_ = 0.01F;  // Pan speed coefficient for middle mouse button dragging
  float min_zoom_ = 0.1F;
  float max_zoom_ = 100.0F;
  float yaw_ = -90.0F;
  float pitch_ = 0.0F;

  void UpdateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front);
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_ = glm::normalize(glm::cross(right_, front_));
  }
};