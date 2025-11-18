#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct CameraControllerComponent {
  // glm::vec3 group (4 * 12 = 48 bytes, aligned to 16-byte boundary)
  glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

  // float group (8 * 4 = 32 bytes, aligned to 4-byte boundary)
  float MouseSensitivity = 0.1f;
  float MovementSpeed = 5.0f;
  float ZoomSpeed = 2.0f;
  float PanSpeed = 0.01f;  // Pan speed coefficient for middle mouse button dragging
  float MinZoom = 0.1f;
  float MaxZoom = 100.0f;
  float Yaw = -90.0f;
  float Pitch = 0.0f;

  void UpdateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
  }
};