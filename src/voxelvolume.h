#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class VoxelVolume {
public:
  int width, height, depth;
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

  // Verifica se um ponto no grid tem um voxel sólido
  bool isSolid(int x, int y, int z) const {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
      return false;
    int idx = (z * width * height + y * width + x) * 4;
    return grid[idx + 3] > 0; // Alpha > 0 significa que existe um bloco
  }

  // O algoritmo DDA implementado para o CPU
  bool raycast(glm::vec3 origin, glm::vec3 dir, float maxDist,
               glm::ivec3 &outTarget, glm::ivec3 &outHit, bool &hitBlock) {
    glm::ivec3 mapPos = glm::ivec3(glm::floor(origin));
    glm::vec3 deltaDist = glm::abs(1.0f / dir);
    glm::ivec3 rayStep(dir.x < 0 ? -1 : 1, dir.y < 0 ? -1 : 1,
                       dir.z < 0 ? -1 : 1);

    glm::vec3 sideDist(
        (dir.x < 0 ? (origin.x - mapPos.x) : (mapPos.x + 1.0f - origin.x)) *
            deltaDist.x,
        (dir.y < 0 ? (origin.y - mapPos.y) : (mapPos.y + 1.0f - origin.y)) *
            deltaDist.y,
        (dir.z < 0 ? (origin.z - mapPos.z) : (mapPos.z + 1.0f - origin.z)) *
            deltaDist.z);

    glm::ivec3 lastPos = mapPos;
    float dist = 0.0f;

    for (int i = 0; i < 100 && dist < maxDist; i++) {
      // Se o raio bater em um voxel
      if (isSolid(mapPos.x, mapPos.y, mapPos.z)) {
        outTarget = lastPos; // Espaço vazio para colocar blocos
        outHit = mapPos;     // Bloco sólido para deletar blocos
        hitBlock = true;     // Confirma que atingimos um voxel

        // Só desenha o cursor se o espaço vazio estiver DENTRO dos limites do
        // volume
        return (outTarget.x >= 0 && outTarget.x < width && outTarget.y >= 0 &&
                outTarget.y < height && outTarget.z >= 0 &&
                outTarget.z < depth);
      }

      // Se o raio bater no "chão"
      if (mapPos.y < 0) {
        outTarget = lastPos;
        outTarget.y = 0;  // Força a colocação na base (Y=0)
        hitBlock = false; // Bateu no chão, não num bloco

        // Só desenha o cursor se estiver DENTRO da área demarcada do chão (ex:
        // 16x16)
        return (outTarget.x >= 0 && outTarget.x < width && outTarget.z >= 0 &&
                outTarget.z < depth);
      }

      lastPos = mapPos;

      if (sideDist.x < sideDist.y) {
        if (sideDist.x < sideDist.z) {
          sideDist.x += deltaDist.x;
          mapPos.x += rayStep.x;
          dist = sideDist.x;
        } else {
          sideDist.z += deltaDist.z;
          mapPos.z += rayStep.z;
          dist = sideDist.z;
        }
      } else {
        if (sideDist.y < sideDist.z) {
          sideDist.y += deltaDist.y;
          mapPos.y += rayStep.y;
          dist = sideDist.y;
        } else {
          sideDist.z += deltaDist.z;
          mapPos.z += rayStep.z;
          dist = sideDist.z;
        }
      }
    }
    return false;
  }

private:
  bool isDirty = true;
};
