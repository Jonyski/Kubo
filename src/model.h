#pragma once

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h" // Assumes stb_image.h is in the same directory

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// GLAD
#include <glad/glad.h>

// Std
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Forward declare Shader
#include "shader.h"

// --- Helper Functions/Structs ---

unsigned int TextureFromFile(const char *path, const std::string &directory);

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
};

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

// --- Mesh Class ---
// A model is made of one or more meshes. This class handles one mesh.
class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  unsigned int VAO;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    setupMesh();
  }

  void Draw(Shader &shader) {
    unsigned int diffuseNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      std::string name = textures[i].type;
      if (name == "texture_diffuse")
        name += std::to_string(diffuseNr++);

      glUniform1i(glGetUniformLocation(shader.ID, name.c_str()), i);
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
  }

private:
  unsigned int VBO, EBO;

  void setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // Vertex Texture Coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));
    // Vertex Normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));
    glBindVertexArray(0);
  }
};

// --- Model Class ---
// This class loads the full model file and contains all the mesh objects.
class Model {
public:
  std::vector<Texture> textures_loaded;
  std::vector<Mesh> meshes;
  std::string directory;

  Model(std::string const &path) { loadModel(path); }

  void Draw(Shader &shader) {
    for (unsigned int i = 0; i < meshes.size(); i++)
      meshes[i].Draw(shader);
  }

private:
  void loadModel(std::string const &path) {
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
                                    aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
      std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
      return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
  }

  void processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene);
    }
  }

  Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      glm::vec3 vector;

      // Positions
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.Position = vector;

      // Normals
      if (mesh->HasNormals()) {
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
      }

      // Texture Coordinates
      if (mesh->mTextureCoords[0]) {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.TexCoords = vec;
      } else
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);

      vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture> diffuseMaps = loadMaterialTextures(
        material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    return Mesh(vertices, indices, textures);
  }

  std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                            std::string typeName) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
      aiString str;
      mat->GetTexture(type, i, &str);

      bool skip = false;
      for (unsigned int j = 0; j < textures_loaded.size(); j++) {
        if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
          textures.push_back(textures_loaded[j]);
          skip = true;
          break;
        }
      }
      if (!skip) {
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), this->directory);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
        textures_loaded.push_back(texture);
      }
    }
    return textures;
  }
};

// Helper function to load a texture from a file
unsigned int TextureFromFile(const char *path, const std::string &directory) {
  std::string filename = std::string(path);
  filename = directory + '/' + filename;

  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  unsigned char *data =
      stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << filename << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}
