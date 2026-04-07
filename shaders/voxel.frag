#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;
in vec4 FragPosLightSpace;

// Mapa de profundidade gerado no primeiro passe de renderização
uniform sampler2D shadowMap;

// Variáveis de iluminação
uniform vec3 sunLightDir;
uniform vec3 sunLightColor;
uniform vec3 skyLightColor;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // Um viés quase zero, apenas para resolver as micro-flutuações do float
    float bias = max(0.0015 * (1.0 - dot(normal, lightDir)), 0.00001);

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 lightDir = -normalize(sunLightDir);

    // 1. Iluminação Ambiente Hemisférica (Céu e Chão)
    // Faces apontando para cima recebem mais luz do céu
    float skyFactor = max(dot(normal, vec3(0.0, 1.0, 0.0)), 0.0);
    // Faces apontando para baixo recebem um leve rebatimento (bounce light) simulado
    float groundFactor = max(dot(normal, vec3(0.0, -1.0, 0.0)), 0.0);

    vec3 ambient = (skyLightColor * 0.4 * skyFactor) + (vec3(0.15) * groundFactor) + vec3(0.2); // + vec3(0.2) é a luz base global

    // 2. Iluminação Difusa (Sol)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * sunLightColor;

    // 3. Cálculo de Sombras
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);

    // Combinação Final: O ambiente sempre afeta, mas o sol e suas sombras são dinâmicos
    vec3 lighting = (ambient + (1.0 - shadow) * diffuse) * Color.rgb;

    // Correção Gamma simples para cores mais vivas (opcional, mas recomendado para PBR/Voxel)
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, Color.a);
}
