#pragma once

#include "vivid/render/render_component.h"

namespace vivid::mesh {

class MeshGenerator {
public:
  /**
   * @brief Create a cube mesh with specified dimensions.
   *
   * @param width Width of the cube (X-axis).
   * @param height Height of the cube (Y-axis).
   * @param depth Depth of the cube (Z-axis).
   * @return render::MeshComponent The generated mesh component.
   */
  static render::MeshComponent CreateCube(float width, float height, float depth);

  /**
   * @brief Create a plane mesh on the XZ plane.
   *
   * @param width Width of the plane (X-axis).
   * @param depth Depth of the plane (Z-axis).
   * @return render::MeshComponent The generated mesh component.
   */
  static render::MeshComponent CreatePlane(float width, float depth);

  /**
   * @brief Create a sphere mesh.
   *
   * @param radius Radius of the sphere.
   * @param rings Number of horizontal rings (latitude).
   * @param sectors Number of vertical sectors (longitude).
   * @return render::MeshComponent The generated mesh component.
   */
  static render::MeshComponent CreateSphere(float radius, int rings, int sectors);
};

}  // namespace vivid::mesh
