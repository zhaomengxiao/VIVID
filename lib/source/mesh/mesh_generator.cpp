#include "vivid/mesh/mesh_generator.h"

#include <cmath>
#include <vector>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace vivid::mesh {

render::MeshComponent MeshGenerator::CreateCube(float width, float height, float depth) {
  float const kHalfW = width * 0.5F;
  float const kHalfH = height * 0.5F;
  float const kHalfD = depth * 0.5F;

  std::vector<float> const kVertices = {
      // positions          // normals
      // Back face (z = -d)
      -kHalfW,
      -kHalfH,
      -kHalfD,
      0.0F,
      0.0F,
      -1.0F,  // 0: BL
      kHalfW,
      -kHalfH,
      -kHalfD,
      0.0F,
      0.0F,
      -1.0F,  // 1: BR
      kHalfW,
      kHalfH,
      -kHalfD,
      0.0F,
      0.0F,
      -1.0F,  // 2: TR
      -kHalfW,
      kHalfH,
      -kHalfD,
      0.0F,
      0.0F,
      -1.0F,  // 3: TL

      // Front face (z = +d)
      -kHalfW,
      -kHalfH,
      kHalfD,
      0.0F,
      0.0F,
      1.0F,  // 4: BL
      kHalfW,
      -kHalfH,
      kHalfD,
      0.0F,
      0.0F,
      1.0F,  // 5: BR
      kHalfW,
      kHalfH,
      kHalfD,
      0.0F,
      0.0F,
      1.0F,  // 6: TR
      -kHalfW,
      kHalfH,
      kHalfD,
      0.0F,
      0.0F,
      1.0F,  // 7: TL

      // Left face (x = -w)
      -kHalfW,
      -kHalfH,
      -kHalfD,
      -1.0F,
      0.0F,
      0.0F,  // 8: BL
      -kHalfW,
      -kHalfH,
      kHalfD,
      -1.0F,
      0.0F,
      0.0F,  // 9: BR
      -kHalfW,
      kHalfH,
      kHalfD,
      -1.0F,
      0.0F,
      0.0F,  // 10: TR
      -kHalfW,
      kHalfH,
      -kHalfD,
      -1.0F,
      0.0F,
      0.0F,  // 11: TL

      // Right face (x = +w)
      kHalfW,
      -kHalfH,
      kHalfD,
      1.0F,
      0.0F,
      0.0F,  // 12: BL
      kHalfW,
      -kHalfH,
      -kHalfD,
      1.0F,
      0.0F,
      0.0F,  // 13: BR
      kHalfW,
      kHalfH,
      -kHalfD,
      1.0F,
      0.0F,
      0.0F,  // 14: TR
      kHalfW,
      kHalfH,
      kHalfD,
      1.0F,
      0.0F,
      0.0F,  // 15: TL

      // Bottom face (y = -h)
      -kHalfW,
      -kHalfH,
      -kHalfD,
      0.0F,
      -1.0F,
      0.0F,  // 16: BL
      kHalfW,
      -kHalfH,
      -kHalfD,
      0.0F,
      -1.0F,
      0.0F,  // 17: BR
      kHalfW,
      -kHalfH,
      kHalfD,
      0.0F,
      -1.0F,
      0.0F,  // 18: TR
      -kHalfW,
      -kHalfH,
      kHalfD,
      0.0F,
      -1.0F,
      0.0F,  // 19: TL

      // Top face (y = +h)
      -kHalfW,
      kHalfH,
      kHalfD,
      0.0F,
      1.0F,
      0.0F,  // 20: BL
      kHalfW,
      kHalfH,
      kHalfD,
      0.0F,
      1.0F,
      0.0F,  // 21: BR
      kHalfW,
      kHalfH,
      -kHalfD,
      0.0F,
      1.0F,
      0.0F,  // 22: TR
      -kHalfW,
      kHalfH,
      -kHalfD,
      0.0F,
      1.0F,
      0.0F,  // 23: TL
  };

  std::vector<unsigned int> const kIndices = {// Back
                                              0, 2, 1, 2, 0, 3,
                                              // Front
                                              4, 5, 6, 6, 7, 4,
                                              // Left
                                              8, 9, 10, 10, 11, 8,
                                              // Right
                                              12, 13, 14, 14, 15, 12,
                                              // Bottom
                                              16, 17, 18, 18, 19, 16,
                                              // Top
                                              20, 21, 22, 22, 23, 20};

  return {kVertices, kIndices, kIndices.size()};
}

render::MeshComponent MeshGenerator::CreatePlane(float width, float depth) {
  auto const kHalfW = width * 0.5F;
  auto const kHalfD = depth * 0.5F;

  std::vector<float> const kVertices = {
      // positions          // normals
      -kHalfW, 0.0F, -kHalfD, 0.0F, 1.0F, 0.0F,  // 0: TL
      kHalfW,  0.0F, -kHalfD, 0.0F, 1.0F, 0.0F,  // 1: TR
      kHalfW,  0.0F, kHalfD,  0.0F, 1.0F, 0.0F,  // 2: BR
      -kHalfW, 0.0F, kHalfD,  0.0F, 1.0F, 0.0F   // 3: BL
  };

  // CCW: 0->3->2, 2->1->0
  std::vector<unsigned int> const kIndices = {0, 3, 2, 2, 1, 0};

  return {kVertices, kIndices, kIndices.size()};
}

render::MeshComponent MeshGenerator::CreateSphere(float radius, int rings, int sectors) {
  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  auto const kR = 1.0F / static_cast<float>(rings - 1);
  auto const kS = 1.0F / static_cast<float>(sectors - 1);

  for (int r = 0; r < rings; r++) {
    for (int s = 0; s < sectors; s++) {
      auto const kY = static_cast<float>(sin((-M_PI / 2) + (M_PI * r * kR)));
      auto const kX = static_cast<float>(cos(2 * M_PI * s * kS) * sin(M_PI * r * kR));
      auto const kZ = static_cast<float>(sin(2 * M_PI * s * kS) * sin(M_PI * r * kR));

      // Position
      vertices.push_back(kX * radius);
      vertices.push_back(kY * radius);
      vertices.push_back(kZ * radius);

      // Normal (normalized position for sphere at origin)
      vertices.push_back(kX);
      vertices.push_back(kY);
      vertices.push_back(kZ);
    }
  }

  for (int r = 0; r < rings - 1; r++) {
    for (int s = 0; s < sectors - 1; s++) {
      int const kCurRow = r * sectors;
      int const kNextRow = (r + 1) * sectors;

      indices.push_back(kCurRow + s);
      indices.push_back(kNextRow + s);
      indices.push_back(kNextRow + (s + 1));

      indices.push_back(kCurRow + s);
      indices.push_back(kNextRow + (s + 1));
      indices.push_back(kCurRow + (s + 1));
    }
  }

  return {vertices, indices, indices.size()};
}

}  // namespace vivid::mesh
