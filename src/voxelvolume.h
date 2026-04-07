#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "shader.h"

// Estrutura que o seu model.vert espera
struct VoxelVertex {
  glm::vec3 Position; // location = 0
  glm::vec4 Color;    // location = 1
  glm::vec3 Normal;   // location = 2
};

struct Voxel {
  bool isActive = false;
  glm::vec4 color = glm::vec4(1.0f);
};

class VoxelVolume {
public:
  int width, height, depth;
  std::vector<Voxel> grid;

  VoxelVolume(int w, int h, int d) : width(w), height(h), depth(d) {
    grid.resize(width * height * depth);
    setupMesh();
  }

  // Acesso 1D seguro para a matriz 3D
  int getIndex(int x, int y, int z) const {
    return x + y * width + z * width * height;
  }

  bool isVoxelSolid(int x, int y, int z) const {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
      return false; // Trata os limites externos como ar/vazio
    return grid[getIndex(x, y, z)].isActive;
  }

  void setVoxel(int x, int y, int z, bool active,
                glm::vec4 color = glm::vec4(1.0f)) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
      return;

    int index = getIndex(x, y, z);
    grid[index].isActive = active;
    grid[index].color = color;

    // Marca a malha para ser recriada (idealmente, você faria isso no final do
    // frame e não a cada cubo individualmente se o usuário pintar vários de uma
    // vez)
    isDirty = true;
  }

  void update() {
    if (isDirty) {
      buildMesh();
      isDirty = false;
    }
  }

  void draw(Shader &shader) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
  }

private:
  unsigned int VAO, VBO;
  std::vector<VoxelVertex> vertices;
  bool isDirty = false;

  void setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // A configuração dos ponteiros de atributos será feita dentro do
    // buildMesh() já que os dados mudarão
  }

  void addFace(glm::vec3 pos, glm::vec4 color, int faceDirection) {
    // Direções: 0=Frente, 1=Trás, 2=Esquerda, 3=Direita, 4=Cima, 5=Baixo
    if (faceDirection == 0) { // Frente (Z+)
      glm::vec3 n(0.0f, 0.0f, 1.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 1), color, n});
    } else if (faceDirection == 1) { // Trás (Z-)
      glm::vec3 n(0.0f, 0.0f, -1.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 0), color, n});
    } else if (faceDirection == 2) { // Esquerda (X-)
      glm::vec3 n(-1.0f, 0.0f, 0.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 0), color, n});
    } else if (faceDirection == 3) { // Direita (X+)
      glm::vec3 n(1.0f, 0.0f, 0.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 1), color, n});
    } else if (faceDirection == 4) { // Cima (Y+)
      glm::vec3 n(0.0f, 1.0f, 0.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 1, 1), color, n});
    } else if (faceDirection == 5) { // Baixo (Y-)
      glm::vec3 n(0.0f, -1.0f, 0.0f);
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 0), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(1, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 1), color, n});
      vertices.push_back(VoxelVertex{pos + glm::vec3(0, 0, 0), color, n});
    }
  }

  void buildMesh() {
    vertices.clear();

    for (int z = 0; z < depth; ++z) {
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          if (isVoxelSolid(x, y, z)) {
            glm::vec3 pos(x, y, z);
            glm::vec4 color = grid[getIndex(x, y, z)].color;

            // Face Culling: Só adiciona a face se o vizinho estiver vazio
            if (!isVoxelSolid(x, y, z + 1))
              addFace(pos, color, 0); // Frente
            if (!isVoxelSolid(x, y, z - 1))
              addFace(pos, color, 1); // Trás
            if (!isVoxelSolid(x - 1, y, z))
              addFace(pos, color, 2); // Esquerda
            if (!isVoxelSolid(x + 1, y, z))
              addFace(pos, color, 3); // Direita
            if (!isVoxelSolid(x, y + 1, z))
              addFace(pos, color, 4); // Cima
            if (!isVoxelSolid(x, y - 1, z))
              addFace(pos, color, 5); // Baixo
          }
        }
      }
    }

    // Envia para a GPU usando GL_DYNAMIC_DRAW, vital para edição em tempo real
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VoxelVertex),
                 vertices.data(), GL_DYNAMIC_DRAW);

    // Position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
                          (void *)0);
    // Color (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
                          (void *)offsetof(VoxelVertex, Color));
    // Normal (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
                          (void *)offsetof(VoxelVertex, Normal));

    glBindVertexArray(0);
  }
};
