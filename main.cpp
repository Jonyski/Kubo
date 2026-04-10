// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Helper classes
#include "./src/camera.h"
#include "./src/shader.h"
#include "./src/voxelvolume.h"

// C++ Libraries
#include <iostream>

// Window dimensions
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// --- Globals for Camera ---
Camera camera(glm::vec3(8.0f, 2.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// --- Input Callbacks ---
void processInput(GLFWwindow *window);                             // Keyboard
void mouse_callback(GLFWwindow *window, double xpos, double ypos); // Mouse aim
void scroll_callback(GLFWwindow *window, double xoffset,
                     double yoffset); // Mouse zoom

void error_callback(int error, const char *description) {
  std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// Build & Run command: cmake .. && cmake --build . && ./main
int main() {
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  // Set to OpenGL 4.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // GLFW stuff
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Loader", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Tell GLFW to use our callbacks
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  // Glad stuff
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Basic settings
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

  // Obligatory VAO
  unsigned int emptyVAO;
  glGenVertexArrays(1, &emptyVAO);

  // Lighting settings
  glm::vec3 sunLightDir = glm::normalize(glm::vec3(0.3f, -0.5f, -0.3f));
  glm::vec3 sunLightColor = glm::vec3(1.0f, 0.95f, 0.8f);
  glm::vec3 skyLightColor = glm::vec3(0.7f, 0.5f, 1.0f);

  // Shaders
  Shader voxelShader("../shaders/voxel.vert", "../shaders/voxel.frag");

  // Example voxel art
  VoxelVolume volume(10, 10, 10);
  // Setting up some voxels
  volume.setVoxel(4, 0, 4, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.setVoxel(4, 1, 4, true, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  volume.setVoxel(5, 0, 4, true, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  volume.setVoxel(3, 0, 4, true, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  volume.setVoxel(4, 0, 3, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.setVoxel(4, 0, 5, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.update();

  // --- Main Render Loop ---
  while (!glfwWindowShouldClose(window)) {
    // Delta time calculation
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Processing Input
    processInput(window);
    volume.update();

    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    voxelShader.use();

    // Invertemos as matrizes da câmera para enviar para o shader
    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    voxelShader.setMat4("invProj", glm::inverse(projection));
    voxelShader.setMat4("invView", glm::inverse(view));
    voxelShader.setVec3("camPos", camera.Position);
    voxelShader.setVec3("sunLightDir", glm::normalize(sunLightDir));
    voxelShader.setVec3("gridSize",
                        glm::vec3(volume.width, volume.height, volume.depth));

    // Liga a textura 3D
    volume.bind(0);
    voxelShader.setInt("voxelGrid", 0);

    glBindVertexArray(emptyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // --- GLFW tasks ---
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

// ----- Callback and IO functions -----

// Reads WASD, space, esc and lshift for camera control and window exit
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    camera.ProcessKeyboard(UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    camera.ProcessKeyboard(DOWN, deltaTime);
}

// Camera direction control through mouse movement
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

// Camera zoom control through mouse scroll
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // yoffset is the amount of scroll. Pass it to the camera.
  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
