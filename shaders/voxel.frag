#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 camPos;
uniform vec3 sunLightDir;

uniform sampler3D voxelGrid;
uniform vec3 gridSize;
uniform int hasCursor;
uniform vec3 cursorTarget;
uniform vec2 resolution;

struct HitResult {
    bool hit;
    vec3 pos;
    bool isFloor;
    vec3 normal;
    vec4 color;
};

// Tests the intersection of the ray with the bounding volume
vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

// Main ray-marching algorithm
HitResult RayMarchDDA(vec3 ro, vec3 rd, vec3 boxMin, vec3 boxMax) {
    HitResult result;
    result.hit = false;
    result.isFloor = false;
    result.normal = vec3(0.0);
    result.color = vec4(0.0);

    // Early return if it doesn't intersect the volume
    vec2 tBox = intersectAABB(ro, rd, boxMin, boxMax);
    if (tBox.x > tBox.y || tBox.y < 0.0) return result;

    float t = max(0.0, tBox.x + 0.0001);
    vec3 p = ro + rd * t;

    ivec3 mapPos = ivec3(floor(p));
    mapPos = clamp(mapPos, ivec3(0), ivec3(gridSize) - 1);

    vec3 deltaDist = abs(1.0 / rd);
    ivec3 rayStep = ivec3(sign(rd));
    vec3 sideDist = (sign(rd) * (vec3(mapPos) - p) + (sign(rd) * 0.5) + 0.5) * deltaDist;

    ivec3 mask = ivec3(0);

    for (int i = 0; i < 64; i++) {
        vec3 texCoord = (vec3(mapPos) + 0.5) / gridSize;
        vec4 voxel = texture(voxelGrid, texCoord);

        if (voxel.a > 0.0) {
            result.hit = true;
            result.color = voxel;

            if (mask == ivec3(0)) {
                // Se bateu no passo 0, a normal é a própria face da Bounding Box (AABB)
                result.pos = p;
                vec3 center = (boxMin + boxMax) * 0.5;
                vec3 extents = (boxMax - boxMin) * 0.5;
                vec3 localPos = p - center;
                vec3 d = abs(localPos) / extents;
                if (d.x >= d.y && d.x >= d.z) result.normal = vec3(sign(localPos.x), 0, 0);
                else if (d.y >= d.x && d.y >= d.z) result.normal = vec3(0, sign(localPos.y), 0);
                else result.normal = vec3(0, 0, sign(localPos.z));
            } else {
                // Se bateu num passo normal, recuamos para encontrar o ponto exato da face
                float hitT = dot(vec3(mask), sideDist - deltaDist);
                result.pos = p + rd * hitT;
                result.normal = -vec3(mask) * sign(rd);
            }
            break;
        }

        mask = ivec3(lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy)));
        sideDist += vec3(mask) * deltaDist;
        mapPos += mask * rayStep;

        if (any(lessThan(mapPos, ivec3(0))) || any(greaterThanEqual(mapPos, ivec3(gridSize)))) break;
    }

    return result;
}

float interleavedGradientNoise(vec2 coord) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(coord, magic.xy)));
}

void main() {
    vec4 skyColor = vec4(0.5, 0.7, 1.0, 1.0);
    vec2 ndc = TexCoords * 2.0 - 1.0;
    vec4 target = invProj * vec4(ndc.x, ndc.y, 1.0, 1.0);
    vec3 rayDir = normalize((invView * vec4(normalize(target.xyz / target.w), 0.0)).xyz);

    vec3 boxMin = vec3(0.0);
    vec3 boxMax = gridSize;

    // Dispara o raio da câmera (Visão)
    HitResult cameraHit = RayMarchDDA(camPos, rayDir, boxMin, boxMax);

    // Grade no chão
    float floorY = -0.001;
    float tFloor = (floorY - camPos.y) / rayDir.y;

    // Se o raio aponta para baixo (tFloor > 0), ele vai bater no chão em algum lugar
    if (tFloor > 0.0) {
        vec3 floorP = camPos + rayDir * tFloor;

        if (floorP.x >= 0.0 && floorP.x <= gridSize.x && floorP.z >= 0.0 && floorP.z <= gridSize.z) {
            // Distância até o voxel (se não bateu em nada, joga ao infinito)
            float distVoxel = cameraHit.hit ? length(cameraHit.pos - camPos) : 9999999.0;

            // Se o raio bateu no chão ANTES de bater num cubo, o chão é o que deve ser desenhado
            if (tFloor < distVoxel) {
                cameraHit.hit = true;
                cameraHit.isFloor = true;
                cameraHit.pos = floorP;
                cameraHit.normal = vec3(0.0, 1.0, 0.0);

                // Matemática para desenhar as linhas do Grid
                vec2 gridUV = fract(floorP.xz);
                float edgeDist = min(min(gridUV.x, 1.0 - gridUV.x), min(gridUV.y, 1.0 - gridUV.y));
                float lineVal = step(edgeDist * 3, 0.04);

                // Cores do blueprint/chão
                vec4 baseColor = vec4(0.1, 0.1, 0.2, 1.0);
                vec4 lineColor = vec4(0.4, 0.4, 0.6, 1.0);

                // Mistura as cores baseada nas linhas
                vec4 finalColor = mix(baseColor, lineColor, lineVal);

                cameraHit.color = vec4(finalColor);
            }
        }
    }

    vec3 lightDir = normalize(-sunLightDir);
    float diff = max(dot(cameraHit.normal, lightDir), 0.0);

    // --- SOFT SHADOWS ---
    float shadow = 0.0;
    int shadowSamples = 12;
    float shadowSpread = 0.04; // Diminua ligeiramente o spread para concentrar a penumbra

    vec3 up = abs(lightDir.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(lightDir, up));
    vec3 bitangent = cross(lightDir, tangent);

    vec3 shadowOrigin = cameraHit.pos + cameraHit.normal * 0.01;

    // Gera um ângulo de rotação pseudo-aleatório perfeito para o pixel atual
    float noise = interleavedGradientNoise(gl_FragCoord.xy);
    float randomAngle = noise * 6.2831853; // noise * 2 * PI
    float cosAngle = cos(randomAngle);
    float sinAngle = sin(randomAngle);

    // Matriz de rotação 2D
    mat2 rotation = mat2(cosAngle, -sinAngle, sinAngle, cosAngle);

    for (int i = 0; i < shadowSamples; i++) {
        // Amostragem de Disco de Vogel (Distribuição em espiral uniforme)
        float r = sqrt(float(i) + 0.5) / sqrt(float(shadowSamples));
        float theta = float(i) * 2.3999632; // Ângulo de Ouro em radianos

        vec2 diskPos = vec2(cos(theta), sin(theta)) * r;

        // Rotaciona a amostra baseada no pixel atual para esconder o padrão (Dithering)
        diskPos = rotation * diskPos;

        // Aplica a amostra aos vetores tangentes para espalhar o raio da luz
        vec3 jitteredLightDir = normalize(lightDir + (tangent * diskPos.x + bitangent * diskPos.y) * shadowSpread);

        // Dispara o raio de sombra modificado
        HitResult shadowHit = RayMarchDDA(shadowOrigin, jitteredLightDir, boxMin, boxMax);
        if (shadowHit.hit) {
            shadow += 1.0;
        }
    }
    shadow /= float(shadowSamples);

    // --- ILUMINAÇÃO FINAL ---
    vec3 ambient = vec3(0.64, 0.5, 0.75) + max(dot(cameraHit.normal, vec3(0, 1, 0)), 0.0) * 0.3;
    vec3 diffuseColor = diff * vec3(1.0, 0.95, 0.8) * (1.0 - shadow);
    float floorAO = mix(0.1, 1.0, smoothstep(0.0, 1.2, cameraHit.pos.y / 2));

    vec4 lighting = vec4((ambient + diffuseColor) * cameraHit.color.rgb, 1.0);
    if (!cameraHit.isFloor) {
        float ao = mix(0.0, 0.1, smoothstep(0.0, 1.25, cameraHit.pos.y));
        lighting -= vec4(0.1 - ao, 0.1 - ao, 0.05 - 0.5 * ao, 0.0);
    }

    vec4 finalColor = mix(skyColor, lighting, cameraHit.color.a);

    // VOXEL TRANSPARENTE
    if (hasCursor == 1) {
        vec3 boxMin = cursorTarget;
        vec3 boxMax = cursorTarget + vec3(1.0);

        vec2 tCursor = intersectAABB(camPos, rayDir, boxMin, boxMax);

        // Se o raio atravessa o "cubo alvo"
        if (tCursor.x <= tCursor.y && tCursor.x > 0.0) {
            float distToHit = cameraHit.hit ? length(cameraHit.pos - camPos) : 999999.0;

            // Desenha apenas se o cursor flutuante estiver "à frente" dos blocos sólidos
            if (tCursor.x < distToHit) {
                vec3 p = camPos + rayDir * tCursor.x;
                vec3 localP = p - boxMin;

                // Deteta as quinas do bloco (usando o limite 0.5 a partir do centro)
                vec3 d = abs(localP - 0.5);
                vec3 e = step(0.485, d);
                // O bloco de aresta acontece se duas faces estiverem muito próximas do extremo
                float edgeFactor = clamp(e.x * e.y + e.y * e.z + e.z * e.x, 0.0, 1.0);

                vec4 ghostColor = mix(vec4(0.9, 0.75, 1.0, 0.2), vec4(0.8, 0.75, 1.0, 1.0), edgeFactor);

                // Aplica a mesclagem sobre os gráficos existentes
                finalColor = mix(finalColor, ghostColor, ghostColor.a);
            }
        }
    }

    // CURSOR
    vec2 screenCenter = resolution * 0.5;

    // gl_FragCoord.xy contém a posição [X, Y] em pixels do fragmento atual
    float distToCenter = length(gl_FragCoord.xy - screenCenter);

    // Desenha um ponto branco sólido com 2 pixels de raio
    if (distToCenter <= 3.0) {
        finalColor = vec4(1.0);
    }

    FragColor = finalColor;
}
