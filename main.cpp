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
#include "./src/model.h"
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
  glClearColor(0.01f, 0.0f, 0.05f, 1.0f);

  // Lighting settings
  glm::vec3 sunLightDir = glm::normalize(glm::vec3(0.3f, -0.5f, -0.3f));
  glm::vec3 sunLightColor = glm::vec3(1.0f, 0.95f, 0.8f);
  glm::vec3 skyLightColor = glm::vec3(0.7f, 0.5f, 1.0f);

  // Shaders
  Shader voxelShader("../shaders/voxel.vert", "../shaders/voxel.frag");
  Shader depthShader("../shaders/depth.vert", "../shaders/depth.frag");

  // Shadow Mapping's FBO
  const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
  unsigned int depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);

  // Creates a 2D texture to store depth info
  unsigned int depthMap;
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // Put the texture in an FBO
  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Example voxel art
  VoxelVolume volume(16, 16, 16);
  // Setting up some voxels
  volume.setVoxel(8, 0, 8, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.setVoxel(8, 1, 8, true, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
  volume.setVoxel(9, 0, 8, true, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  volume.setVoxel(7, 0, 8, true, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  volume.setVoxel(8, 0, 7, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.setVoxel(8, 0, 9, true, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  volume.update();

  // --- Main Render Loop ---
  while (!glfwWindowShouldClose(window)) {
    // Delta time calculation
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Processing Input
    processInput(window); // Handle keyboard movement
    volume.update();

    // SHADOW MAPPING ----------------------------------------------------------
    glm::mat4 lightProjection, lightView, lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 50.0f;

    lightProjection =
        glm::ortho(-12.0f, 12.0f, -12.0f, 12.0f, near_plane, far_plane);

    glm::vec3 lightPos = -sunLightDir * 30.0f;
    lightView = glm::lookAt(lightPos, glm::vec3(8.0f, 0.0f, 8.0f),
                            glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;

    glm::mat4 model = glm::mat4(1.0f);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 4.0f);

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    depthShader.setMat4("model", model);
    volume.draw(depthShader);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // -------------------------------------------------------------------------

    // RENDERING THE VOXELS ----------------------------------------------------
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    voxelShader.use();

    // Matrices
    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

    voxelShader.setMat4("projection", projection);
    voxelShader.setMat4("view", view);
    voxelShader.setMat4("model", model);
    voxelShader.setMat3("normalMatrix", normalMatrix);

    // Lighting properties
    voxelShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    voxelShader.setVec3("sunLightDir", sunLightDir);
    voxelShader.setVec3("sunLightColor", sunLightColor);
    voxelShader.setVec3("skyLightColor", skyLightColor);

    // Binding the depth map generated before
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    volume.draw(voxelShader);

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
