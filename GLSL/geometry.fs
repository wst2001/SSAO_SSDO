#version 330 core
layout (location = 0) out vec4 gPositionDepth;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

const float NEAR = 0.1;
const float FAR = 50.0;
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
} 

void main()
{    
    // 存储第一个G缓冲纹理中的片段位置向量
    gPositionDepth.xyz = FragPos;
    gPositionDepth.w = LinearizeDepth(gl_FragCoord.z);
    // 同样存储对每个逐片段法线到G缓冲中
    gNormal = normalize(Normal);
    // 漫反射对每个逐片段颜色
    gAlbedo.rgb = vec3(0.95);
    // 存储镜面强度到gAlbedoSpec的alpha分量
    gAlbedo.a = 0;
}