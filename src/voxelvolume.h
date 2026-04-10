#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class VoxelVolume {
public:
  int width, height, depth;
  // O formato RGBA8: 3 bytes para cor, 1 byte para dizer se está ativo (Alpha >
  // 0)
  std::vector<unsigned char> grid;
  unsigned int textureID;

  VoxelVolume(int w, int h, int d) : width(w), height(h), depth(d) {
    grid.resize(w * h * d * 4, 0); // Inicializa o volume vazio (0)

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);
    // Em Voxel Art, não queremos interpolação suave (borrada) entre os blocos
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Aloca a memória na GPU
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, grid.data());
  }

  void setVoxel(int x, int y, int z, bool active,
                glm::vec4 color = glm::vec4(1.0f)) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
      return;

    int idx = (z * width * height + y * width + x) * 4;
    grid[idx + 0] = active ? (unsigned char)(color.r * 255.0f) : 0;
    grid[idx + 1] = active ? (unsigned char)(color.g * 255.0f) : 0;
    grid[idx + 2] = active ? (unsigned char)(color.b * 255.0f) : 0;
    grid[idx + 3] = active ? 255 : 0; // Alpha 255 = sólido

    isDirty = true;
  }

  void update() {
    if (isDirty) {
      glBindTexture(GL_TEXTURE_3D, textureID);
      // Atualiza a textura na GPU com os dados novos
      glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RGBA,
                      GL_UNSIGNED_BYTE, grid.data());
      isDirty = false;
    }
  }

  void bind(unsigned int slot = 0) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, textureID);
  }

private:
  bool isDirty = true;
};
