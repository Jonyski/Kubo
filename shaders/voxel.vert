#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 1) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

// Matriz que representa a "câmera" da sua luz direcional
uniform mat4 lightSpaceMatrix;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalize(normalMatrix * aNormal);
    Color = aColor;

    // Calcula diretamente com o FragPos limpo
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
