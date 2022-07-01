#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform int kernelSize;
uniform float radius;

uniform samplerCube skybox;

uniform mat4 projection;
uniform mat4 iview; // view to world-space

void main() {
    vec2 noiseScale = textureSize(gNormal,0) / textureSize(texNoise,0);
    // 从buffer中提取数据
    vec3 fragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // TBN: 从切线空间到观察空间
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // 对每个采样点计算SSDO直接光照
    vec3 directLight = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < kernelSize; ++i) {
        // 采样点位置
        vec3 samplePos = TBN * samples[i]; // 从切线空间到观察空间
        samplePos = fragPos + samplePos * radius;

        // 将采样点投影到屏幕空间
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // 从观察空间到裁剪空间
        offset.xyz /= offset.w; // 透视除法
        offset.xyz = offset.xyz * 0.5 + 0.5; // 变换到0.0 - 1.0

        // 获取采样点深度
        float sampleDepth = texture(gPositionDepth, offset.xy).z;
        float depth2 = texture(gPositionDepth, offset.xy).a; // has it been drawn to ?

        // 范围检查 & 累加
		if (sampleDepth < samplePos.z || depth2 == 1) {
			vec4 skyboxDirection = iview * vec4(samplePos - fragPos, 0.0);
			vec3 skyboxColor = texture(skybox, skyboxDirection.xyz).xyz;
			directLight += skyboxColor * dot(normal, normalize(samplePos - fragPos));
		}
    }

    FragColor = directLight / kernelSize;
}
